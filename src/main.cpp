#include <Arduino.h>
#include <NetworkManagerPubSub.h>

// ========================== Configuration ==========================
namespace Config {
    const char* WIFI_SSID = "SlimeNetwork2G";
    const char* WIFI_PASSWORD = "bpusixPGMVjeP2QKGsLi";
    
    const char* MQTT_SERVER = "zreds.com";
    const uint16_t MQTT_PORT = 8883;  // SSL port for secure MQTT
    const char* MQTT_USER = "user_67f71f86-1958-43f8-8017-84405ce784ac";
    const char* MQTT_PASS = "6dz5hNvsbyJGlBTU0Q5icA";
    
    const char* SUBSCRIBE_TOPIC = "test/topic";
    const char* PUBLISH_TOPIC = "mcu/uplink";
    
    const uint32_t PUBLISH_INTERVAL_MS = 10000;
}

// ========================== Global Variables ==========================
NetworkManager* network = nullptr;
unsigned long lastPublishTime = 0;

// Simulated sensor base values with drift
struct SensorState {
    float temperature = 22.5;
    float humidity = 55.0;
    float pressure = 1013.25;
    double latitude = 37.7749;
    double longitude = -122.4194;
    float altitude = 52.0;
    float batteryPercent = 85.0;
    float batteryVoltage = 3.85;
    bool charging = false;
    float lightLevel = 450.0;
    float co2Level = 420.0;
    float vocLevel = 150.0;
    float pm25 = 12.5;
    float pm10 = 18.0;
    float noiseLevel = 45.0;
    int motionCount = 0;
    float windSpeed = 5.5;
    float windDirection = 180.0;
    float rainfall = 0.0;
    float soilMoisture = 65.0;
    float uvIndex = 3.5;
    int signalQuality = 75;
    int errorCount = 0;
    unsigned long uptimeSeconds = 0;
} sensorState;

// ========================== Helper Functions ==========================
float addNoise(float value, float maxVariation) {
    return value + (random(-1000, 1001) / 1000.0) * maxVariation;
}

float clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

void updateSensorValues() {
    // Temperature: slow drift with daily variation
    float tempDrift = sin(millis() / 3600000.0) * 2.0;  // Daily cycle
    sensorState.temperature = clamp(22.5 + tempDrift + addNoise(0, 0.5), 15.0, 35.0);
    
    // Humidity: inverse correlation with temperature
    sensorState.humidity = clamp(55.0 - tempDrift * 2 + addNoise(0, 2.0), 20.0, 90.0);
    
    // Pressure: slow changes
    sensorState.pressure += addNoise(0, 0.5);
    sensorState.pressure = clamp(sensorState.pressure, 980.0, 1040.0);
    
    // GPS: small jitter to simulate movement
    sensorState.latitude += addNoise(0, 0.00001);
    sensorState.longitude += addNoise(0, 0.00001);
    
    // Altitude: correlates with pressure
    sensorState.altitude = 52.0 + (1013.25 - sensorState.pressure) * 8.0 + addNoise(0, 0.5);
    
    // Battery: slow discharge or charge
    if (sensorState.charging) {
        sensorState.batteryPercent = min(100.0f, sensorState.batteryPercent + 0.5f);
        sensorState.batteryVoltage = 3.0f + (sensorState.batteryPercent / 100.0f) * 1.2f;
        if (sensorState.batteryPercent >= 99.0) {
            sensorState.charging = false;
        }
    } else {
        sensorState.batteryPercent = max(10.0f, sensorState.batteryPercent - 0.1f);
        sensorState.batteryVoltage = 3.0f + (sensorState.batteryPercent / 100.0f) * 1.2f;
        if (sensorState.batteryPercent <= 15.0 && random(100) < 5) {
            sensorState.charging = true;
        }
    }
    
    // Light level: day/night cycle
    float lightCycle = sin((millis() / 3600000.0) * PI / 12.0);
    sensorState.lightLevel = clamp(450.0 + lightCycle * 400.0 + addNoise(0, 50.0), 10.0, 1000.0);
    
    // Air quality sensors
    sensorState.co2Level = clamp(420.0 + addNoise(0, 30.0), 350.0, 600.0);
    sensorState.vocLevel = clamp(150.0 + addNoise(0, 20.0), 50.0, 300.0);
    sensorState.pm25 = clamp(12.5 + addNoise(0, 3.0), 0.0, 50.0);
    sensorState.pm10 = clamp(18.0 + addNoise(0, 5.0), 0.0, 75.0);
    
    // Environmental
    sensorState.noiseLevel = clamp(45.0 + addNoise(0, 10.0), 30.0, 85.0);
    sensorState.windSpeed = clamp(5.5 + addNoise(0, 2.0), 0.0, 30.0);
    sensorState.windDirection = fmod(sensorState.windDirection + addNoise(0, 10.0) + 360.0, 360.0);
    sensorState.soilMoisture = clamp(sensorState.soilMoisture + addNoise(0, 2.0), 10.0, 95.0);
    sensorState.uvIndex = clamp(max(0.0f, lightCycle * 6.0f + addNoise(0, 0.5f)), 0.0, 11.0);
    
    // Rain: occasional events
    if (random(100) < 5) {
        sensorState.rainfall = random(0, 100) / 10.0;
    } else {
        sensorState.rainfall = max(0.0f, sensorState.rainfall - 0.5f);
    }
    
    // Motion: random events
    if (random(100) < 10) {
        sensorState.motionCount++;
    }
    
    // Signal quality: varies with conditions
    sensorState.signalQuality = clamp(75 + addNoise(0, 10.0), 0, 100);
    
    // Error count: occasional errors
    if (random(1000) < 5) {
        sensorState.errorCount++;
    }
    
    // Uptime
    sensorState.uptimeSeconds = millis() / 1000;
}

String getDeviceMAC() {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(macStr);
}

// ========================== Message Handler ==========================
void handleIncomingMessage(const char* topic, JsonDocument& payload, NetworkMetrics& metrics) {
    Serial.printf("\n[APP] Message received on topic: %s\n", topic);
    
    // Check if it's a JSON response
    if (payload.containsKey("raw")) {
        Serial.printf("[APP] Raw message: %s\n", payload["raw"].as<const char*>());
    } else {
        Serial.print("[APP] JSON message: ");
        serializeJsonPretty(payload, Serial);
        Serial.println();
    }
    
    // Handle specific commands if present
    if (payload.containsKey("command")) {
        const char* command = payload["command"];
        Serial.printf("[APP] Received command: %s\n", command);
        
        if (strcmp(command, "status") == 0) {
            // Send back status
            JsonDocument response;
            response["type"] = "status_response";
            response["device_id"] = getDeviceMAC();
            response["heap"] = ESP.getFreeHeap();
            response["uptime"] = millis() / 1000;
            response["rssi"] = metrics.rssi;
            response["messages_sent"] = metrics.messagesSent;
            response["messages_received"] = metrics.messagesReceived;
            
            network->publishJSON(Config::PUBLISH_TOPIC, response);
        } else if (strcmp(command, "reset_errors") == 0) {
            sensorState.errorCount = 0;
            Serial.println("[APP] Error count reset");
        } else if (strcmp(command, "start_charging") == 0) {
            sensorState.charging = true;
            Serial.println("[APP] Charging started");
        } else if (strcmp(command, "stop_charging") == 0) {
            sensorState.charging = false;
            Serial.println("[APP] Charging stopped");
        }
    }
}

// ========================== Connection Handler ==========================
void handleConnectionChange(bool connected) {
    if (connected) {
        Serial.println("\n[APP] ✓ Network connected successfully!");
        
        // Small delay to ensure connection is stable
        delay(100);
        
        // Skip subscription - just publish
        
        // Send initial message
        JsonDocument doc;
        doc["type"] = "connection";
        doc["device_id"] = getDeviceMAC();
        doc["firmware_version"] = "1.2.3";
        doc["hardware_version"] = "ESP32-D0WD-V3";
        doc["status"] = "online";
        doc["timestamp"] = millis();
        doc["boot_reason"] = "power_on";
        
        network->publishJSON(Config::PUBLISH_TOPIC, doc);
    } else {
        Serial.println("\n[APP] ✗ Network disconnected!");
    }
}

// ========================== Error Handler ==========================
void handleError(const char* error) {
    Serial.printf("[APP] Error: %s\n", error);
    sensorState.errorCount++;
}

// ========================== Telemetry Publishing ==========================
void publishTelemetry() {
    // Check connection first
    if (!network->isMQTTConnected()) {
        Serial.println("[APP] MQTT not connected, skipping telemetry");
        sensorState.errorCount++;
        return;
    }
    
    // Update sensor values with realistic variations
    updateSensorValues();
    
    NetworkMetrics metrics = network->getMetrics();
    unsigned long timestamp = millis();
    String deviceId = getDeviceMAC();
    
    // Send as single comprehensive packet instead of splitting
    Serial.println("\n[APP] Publishing telemetry...");
    
    JsonDocument doc;
    doc["type"] = "telemetry";
    doc["device_id"] = deviceId;
    doc["timestamp"] = timestamp;
    
    // Environmental sensors - use integers to avoid encoding issues
    doc["temp"] = (int)(sensorState.temperature);
    doc["humidity"] = (int)(sensorState.humidity);
    doc["pressure"] = (int)(sensorState.pressure);
    
    // Power
    doc["battery"] = (int)(sensorState.batteryPercent);
    doc["voltage"] = (int)(sensorState.batteryVoltage * 100);  // Store as millivolts
    doc["charging"] = sensorState.charging;
    
    // Air quality
    doc["co2"] = (int)(sensorState.co2Level);
    doc["voc"] = (int)(sensorState.vocLevel);
    doc["pm25"] = (int)(sensorState.pm25 * 10);  // Store as tenths
    doc["pm10"] = (int)(sensorState.pm10 * 10);
    
    // Environmental extended
    doc["light"] = (int)(sensorState.lightLevel);
    doc["noise"] = (int)(sensorState.noiseLevel);
    doc["motion"] = sensorState.motionCount;
    
    // Weather
    doc["wind_speed"] = (int)(sensorState.windSpeed * 10);  // Store as tenths
    doc["wind_dir"] = (int)(sensorState.windDirection);
    doc["rain"] = (int)(sensorState.rainfall * 10);
    doc["uv"] = (int)(sensorState.uvIndex * 10);
    
    // Location
    doc["lat"] = (int)(sensorState.latitude * 10000);  // Store as integer
    doc["lon"] = (int)(sensorState.longitude * 10000);
    doc["alt"] = (int)(sensorState.altitude);
    
    // System
    doc["rssi"] = metrics.rssi;
    doc["heap"] = ESP.getFreeHeap();
    doc["uptime"] = (int)sensorState.uptimeSeconds;
    doc["errors"] = sensorState.errorCount;
    
    // Calculate size before sending
    size_t jsonSize = measureJson(doc);
    Serial.printf("[APP] JSON size: %d bytes\n", jsonSize);
    
    // Publish without calling loop() here to avoid connection issues
    if (network->publishJSON(Config::PUBLISH_TOPIC, doc)) {
        Serial.println("[APP] Telemetry sent successfully");
    } else {
        Serial.println("[APP] Failed to send telemetry");
        sensorState.errorCount++;
    }
}

// ========================== Setup ==========================
void setup() {
    Serial.begin(115200);
    delay(100);
    
    // Initialize random seed
    randomSeed(analogRead(0) + millis());
    
    Serial.println("\n================================");
    Serial.println("    SlimeESP32 MQTT Client");
    Serial.println("    Sensor Testing Mode");
    Serial.println("================================\n");
    
    Serial.printf("Device MAC: %s\n", getDeviceMAC().c_str());
    Serial.printf("Firmware: 1.2.3\n");
    Serial.printf("Hardware: ESP32-D0WD-V3\n\n");
    
    // Create network manager using builder pattern
    network = NetworkBuilder()
        .withWiFi(Config::WIFI_SSID, Config::WIFI_PASSWORD)
        .withMQTT(Config::MQTT_SERVER, Config::MQTT_PORT)
        .withMQTTAuth(Config::MQTT_USER, Config::MQTT_PASS)
        .withClientId(("ESP32_" + getDeviceMAC()).c_str())
        .withSSL(true)  // Using SSL port 8883
        .withTimeouts(30000, 5000)
        .build();
    
    // Set up callbacks
    network->onMessage(handleIncomingMessage);
    network->onConnection(handleConnectionChange);
    network->onError(handleError);
    
    // Initialize network
    if (!network->begin()) {
        Serial.println("[APP] Failed to initialize network. Restarting in 10 seconds...");
        delay(10000);
        ESP.restart();
    }
    
    Serial.println("[APP] Setup complete! Starting sensor simulation...\n");
}

// ========================== Main Loop ==========================
void loop() {
    // Handle network operations - MUST be called frequently to maintain connection
    network->loop();
    
    unsigned long now = millis();
    
    // Publish telemetry periodically
    if (now - lastPublishTime >= Config::PUBLISH_INTERVAL_MS) {
        lastPublishTime = now;
        
        if (network->isMQTTConnected()) {
            // Update sensor values before publishing
            updateSensorValues();
            sensorState.uptimeSeconds = millis() / 1000;
            
            publishTelemetry();
            
            // Call loop again after publish to maintain connection
            network->loop();
        }
    }
    
    // Tiny delay for stability, but keep it minimal
    delay(1);
    
    // Print brief status every minute
    static unsigned long lastStatusPrint = 0;
    if (now - lastStatusPrint >= 60000) {
        lastStatusPrint = now;
        Serial.println("\n=== Quick Status ===");
        Serial.printf("Temp: %.1f°C | Humidity: %.0f%% | Battery: %.0f%% %s\n", 
                      sensorState.temperature, sensorState.humidity, 
                      sensorState.batteryPercent, sensorState.charging ? "⚡" : "🔋");
        Serial.printf("Errors: %d | Motion: %d | Messages: %d sent\n",
                      sensorState.errorCount, sensorState.motionCount, 
                      network->getMetrics().messagesSent);
        Serial.println("==================\n");
    }
    
    // Small delay to prevent watchdog issues
    delay(10);
}