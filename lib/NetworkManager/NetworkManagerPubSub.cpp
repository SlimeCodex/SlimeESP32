#include "NetworkManagerPubSub.h"

// ========================== Constructor/Destructor ==========================

NetworkManager::NetworkManager(const NetworkConfig& cfg) : config(cfg) {
	// Create appropriate WiFi client based on SSL setting
	if (config.mqtt.useSSL) {
		wifiClientSecure = new WiFiClientSecure();
		wifiClientSecure->setInsecure(); // Skip certificate validation for now
		wifiClient = nullptr;
	} else {
		wifiClient = new WiFiClient();
		wifiClientSecure = nullptr;
	}
	
	// Create PubSubClient with appropriate underlying client
	if (config.mqtt.useSSL) {
		mqttClient = new PubSubClient(*wifiClientSecure);
	} else {
		mqttClient = new PubSubClient(*wifiClient);
	}
	
	// Set buffer size for larger messages (default is 256)
	// Must account for MQTT header overhead (topic + ~20 bytes)
	mqttClient->setBufferSize(768);  // Increase to handle larger payloads + MQTT overhead
}

NetworkManager::~NetworkManager() {
	stop();
	if (mqttClient) delete mqttClient;
	if (wifiClient) delete wifiClient;
	if (wifiClientSecure) delete wifiClientSecure;
}

// ========================== Public Methods ==========================

bool NetworkManager::begin() {
	if (initialized) return true;
	
	logInfo("[NetworkManager] Initializing...");
	
	// Setup MQTT server and port
	mqttClient->setServer(config.mqtt.server, config.mqtt.port);
	
	// Set callback using lambda wrapper
	mqttClient->setCallback([this](char* topic, byte* payload, unsigned int length) {
		this->handleMQTTMessage(topic, payload, length);
	});
	
	// Connect WiFi
	if (!connectWiFi()) {
		logError("[NetworkManager] WiFi connection failed");
		return false;
	}
	
	// Connect MQTT
	if (!connectMQTT()) {
		logError("[NetworkManager] MQTT connection failed");
		return false;
	}
	
	initialized = true;
	updateMetrics();
	
	if (connectionCallback) {
		connectionCallback(true);
	}
	
	return true;
}

void NetworkManager::stop() {
	if (!initialized) return;
	
	disconnectMQTT();
	disconnectWiFi();
	initialized = false;
	
	if (connectionCallback) {
		connectionCallback(false);
	}
}

void NetworkManager::loop() {
	if (!initialized) return;
	
	// Always call MQTT loop first to process incoming messages
	if (mqttClient) {
		mqttClient->loop();
	}
	
	// Check MQTT connection status
	if (!mqttClient->connected()) {
		metrics.mqttConnected = false;
		// Only try to reconnect periodically
		unsigned long now = millis();
		if (now - lastReconnectAttempt > config.mqtt.reconnectDelayMs) {
			lastReconnectAttempt = now;
			logInfo("[MQTT] Connection lost, attempting reconnect...");
			if (connectMQTT()) {
				if (connectionCallback) {
					connectionCallback(true);
				}
			}
		}
	} else {
		metrics.mqttConnected = true;
	}
	
	// Check WiFi status
	if (WiFi.status() != WL_CONNECTED) {
		metrics.wifiConnected = false;
		logError("[WiFi] Connection lost!");
		if (!connectWiFi()) {
			return;
		}
	}
	
	updateMetrics();
}

// ========================== WiFi Methods ==========================

bool NetworkManager::connectWiFi() {
	if (WiFi.status() == WL_CONNECTED) {
		metrics.wifiConnected = true;
		return true;
	}
	
	logInfo("[WiFi] Connecting to %s", config.wifi.ssid);
	WiFi.begin(config.wifi.ssid, config.wifi.password);
	
	unsigned long startTime = millis();
	while (WiFi.status() != WL_CONNECTED && 
		   millis() - startTime < config.wifi.connectTimeoutMs) {
		delay(500);
		Serial.print(".");
	}
	Serial.println();
	
	if (WiFi.status() == WL_CONNECTED) {
		metrics.wifiConnected = true;
		metrics.rssi = WiFi.RSSI();
		logInfo("[WiFi] Connected! IP: %s, RSSI: %d dBm", 
				WiFi.localIP().toString().c_str(), metrics.rssi);
		return true;
	}
	
	metrics.wifiConnected = false;
	logError("[WiFi] Failed to connect after %d ms", config.wifi.connectTimeoutMs);
	return false;
}

void NetworkManager::disconnectWiFi() {
	if (WiFi.status() == WL_CONNECTED) {
		WiFi.disconnect();
		metrics.wifiConnected = false;
		logInfo("[WiFi] Disconnected");
	}
}

bool NetworkManager::isWiFiConnected() const {
	return WiFi.status() == WL_CONNECTED;
}

// ========================== MQTT Methods ==========================

bool NetworkManager::connectMQTT() {
	if (mqttClient->connected()) {
		metrics.mqttConnected = true;
		return true;
	}
	
	logInfo("[MQTT] Connecting to %s:%d", config.mqtt.server, config.mqtt.port);
	
	// Connect with username and password
	bool connected = false;
	if (config.mqtt.username && config.mqtt.password) {
		connected = mqttClient->connect(config.mqtt.clientId, config.mqtt.username, config.mqtt.password);
	} else {
		connected = mqttClient->connect(config.mqtt.clientId);
	}
	
	if (connected) {
		metrics.mqttConnected = true;
		metrics.connectAttempts++;
		logInfo("[MQTT] Connected!");
		return true;
	}
	
	metrics.mqttConnected = false;
	metrics.connectAttempts++;
	logError("[MQTT] Connection failed, rc=%d", mqttClient->state());
	return false;
}

void NetworkManager::disconnectMQTT() {
	if (mqttClient->connected()) {
		mqttClient->disconnect();
		metrics.mqttConnected = false;
		metrics.disconnectCount++;
		logInfo("[MQTT] Disconnected");
	}
}

bool NetworkManager::isMQTTConnected() const {
	return mqttClient->connected();
}

bool NetworkManager::subscribe(const char* topic) {
	if (!mqttClient->connected()) {
		logError("[MQTT] Not connected, cannot subscribe");
		return false;
	}
	
	logInfo("[MQTT] Subscribing to: %s", topic);
	bool result = mqttClient->subscribe(topic);
	
	if (result) {
		logInfo("[MQTT] Successfully subscribed to: %s", topic);
	} else {
		logError("[MQTT] Failed to subscribe to: %s", topic);
	}
	
	return result;
}

bool NetworkManager::unsubscribe(const char* topic) {
	if (!mqttClient->connected()) {
		logError("[MQTT] Not connected, cannot unsubscribe");
		return false;
	}
	
	if (mqttClient->unsubscribe(topic)) {
		logInfo("[MQTT] Unsubscribed from: %s", topic);
		return true;
	}
	
	logError("[MQTT] Failed to unsubscribe from: %s", topic);
	return false;
}

bool NetworkManager::publish(const char* topic, const char* payload) {
	if (!mqttClient->connected()) {
		logError("[MQTT] Not connected, cannot publish");
		return false;
	}
	
	requestStartTime = millis();
	
	// Publish with retained = false
	if (mqttClient->publish(topic, payload)) {
		metrics.messagesSent++;
		metrics.lastMessageTime = millis();
		logInfo("[MQTT] Published to %s", topic);
		return true;
	}
	
	logError("[MQTT] Failed to publish to %s", topic);
	return false;
}

bool NetworkManager::ensureConnected() {
	if (mqttClient->connected()) {
		return true;
	}
	
	logInfo("[MQTT] Connection lost, reconnecting...");
	return connectMQTT();
}

// ========================== Callbacks ==========================

void NetworkManager::onMessage(MessageCallback callback) {
	messageCallback = callback;
}

void NetworkManager::onConnection(ConnectionCallback callback) {
	connectionCallback = callback;
}

void NetworkManager::onError(ErrorCallback callback) {
	errorCallback = callback;
}

// ========================== Getters ==========================

NetworkMetrics NetworkManager::getMetrics() const {
	return metrics;
}

// ========================== Private Methods ==========================

void NetworkManager::handleMQTTMessage(char* topic, byte* payload, unsigned int length) {
	metrics.messagesReceived++;
	
	unsigned long responseTime = 0;
	if (requestStartTime > 0) {
		responseTime = millis() - requestStartTime;
		metrics.avgResponseTimeMs = (metrics.avgResponseTimeMs + responseTime) / 2;
		requestStartTime = 0;
	}
	
	logInfo("[MQTT] Message on '%s' (latency: %dms)", topic, responseTime);
	
	if (messageCallback) {
		JsonDocument doc;
		DeserializationError error = deserializeJson(doc, payload, length);
		
		if (error) {
			// If not JSON, create a simple payload
			doc.clear();
			String data;
			data.reserve(length + 1);
			for (unsigned int i = 0; i < length; i++) {
				data += (char)payload[i];
			}
			doc["raw"] = data;
		}
		
		messageCallback(topic, doc, metrics);
	}
}

void NetworkManager::updateMetrics() {
	static unsigned long lastUpdate = 0;
	unsigned long now = millis();
	
	if (now - lastUpdate >= 1000) {
		lastUpdate = now;
		metrics.uptime = now / 1000;
		
		if (WiFi.status() == WL_CONNECTED) {
			metrics.rssi = WiFi.RSSI();
		}
		
		metrics.heap = ESP.getFreeHeap();
	}
}

void NetworkManager::logInfo(const char* format, ...) {
	char buffer[256];
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);
	Serial.println(buffer);
}

void NetworkManager::logError(const char* format, ...) {
	char buffer[256];
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);
	Serial.println(buffer);
	
	metrics.errorCount++;
	
	if (errorCallback) {
		errorCallback(buffer);
	}
}

// ========================== Builder Implementation ==========================

NetworkBuilder& NetworkBuilder::withWiFi(const char* ssid, const char* password) {
	config.wifi.ssid = ssid;
	config.wifi.password = password;
	return *this;
}

NetworkBuilder& NetworkBuilder::withMQTT(const char* server, uint16_t port) {
	config.mqtt.server = server;
	config.mqtt.port = port;
	return *this;
}

NetworkBuilder& NetworkBuilder::withMQTTAuth(const char* username, const char* password) {
	config.mqtt.username = username;
	config.mqtt.password = password;
	return *this;
}

NetworkBuilder& NetworkBuilder::withClientId(const char* clientId) {
	config.mqtt.clientId = clientId;
	return *this;
}

NetworkBuilder& NetworkBuilder::withSSL(bool useSSL) {
	config.mqtt.useSSL = useSSL;
	return *this;
}

NetworkBuilder& NetworkBuilder::withTimeouts(uint32_t wifiTimeout, uint32_t mqttReconnectDelay) {
	config.wifi.connectTimeoutMs = wifiTimeout;
	config.mqtt.reconnectDelayMs = mqttReconnectDelay;
	return *this;
}

NetworkManager* NetworkBuilder::build() {
	return new NetworkManager(config);
}