#ifndef NETWORK_MANAGER_PUBSUB_H
#define NETWORK_MANAGER_PUBSUB_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <functional>

// ========================== Configuration Structure ==========================
struct NetworkConfig {
    // WiFi Configuration
    struct {
        const char* ssid;
        const char* password;
        uint32_t connectTimeoutMs = 30000;
    } wifi;
    
    // MQTT Configuration
    struct {
        const char* server;
        uint16_t port = 1883;
        const char* username;
        const char* password;
        const char* clientId = "ESP32Client";
        uint32_t reconnectDelayMs = 5000;
        bool useSSL = false;
    } mqtt;
};

// ========================== Metrics Structure ==========================
struct NetworkMetrics {
    // Connection status
    bool wifiConnected = false;
    bool mqttConnected = false;
    
    // Performance metrics
    int rssi = 0;
    uint32_t messagesSent = 0;
    uint32_t messagesReceived = 0;
    uint32_t connectAttempts = 0;
    uint32_t disconnectCount = 0;
    uint32_t errorCount = 0;
    
    // Timing
    unsigned long lastMessageTime = 0;
    unsigned long avgResponseTimeMs = 0;
    
    // System
    uint32_t heap = 0;
    uint32_t uptime = 0;
};

// ========================== Callback Types ==========================
using MessageCallback = std::function<void(const char* topic, JsonDocument& payload, NetworkMetrics& metrics)>;
using ConnectionCallback = std::function<void(bool connected)>;
using ErrorCallback = std::function<void(const char* error)>;

// ========================== Network Manager Class ==========================
class NetworkManager {
public:
    // Constructor/Destructor
    NetworkManager(const NetworkConfig& config);
    ~NetworkManager();
    
    // Lifecycle
    bool begin();
    void stop();
    void loop();
    
    // WiFi operations
    bool connectWiFi();
    void disconnectWiFi();
    bool isWiFiConnected() const;
    
    // MQTT operations
    bool connectMQTT();
    void disconnectMQTT();
    bool isMQTTConnected() const;
    bool subscribe(const char* topic);
    bool unsubscribe(const char* topic);
    bool publish(const char* topic, const char* payload);
    
    // JSON publishing helper
    bool publishJSON(const char* topic, JsonDocument& doc) {
        char buffer[768];  // Match PubSubClient buffer size
        memset(buffer, 0, sizeof(buffer));
        size_t len = serializeJson(doc, buffer, sizeof(buffer) - 1);
        
        if (len >= sizeof(buffer) - 1) {
            Serial.printf("[MQTT] JSON payload too large: %d bytes (max %d)\n", len, sizeof(buffer) - 1);
            return false;
        }
        
        buffer[len] = '\0';
        return publish(topic, buffer);
    }
    
    // Callbacks
    void onMessage(MessageCallback callback);
    void onConnection(ConnectionCallback callback);
    void onError(ErrorCallback callback);
    
    // Getters
    NetworkMetrics getMetrics() const;
    
private:
    // Configuration
    NetworkConfig config;
    
    // Network clients
    WiFiClient* wifiClient = nullptr;
    WiFiClientSecure* wifiClientSecure = nullptr;
    PubSubClient* mqttClient = nullptr;
    
    // Metrics
    NetworkMetrics metrics;
    
    // Callbacks
    MessageCallback messageCallback = nullptr;
    ConnectionCallback connectionCallback = nullptr;
    ErrorCallback errorCallback = nullptr;
    
    // State
    bool initialized = false;
    unsigned long requestStartTime = 0;
    unsigned long lastReconnectAttempt = 0;
    
    // Private methods
    void handleMQTTMessage(char* topic, byte* payload, unsigned int length);
    void updateMetrics();
    bool ensureConnected();
    
    // Logging
    void logInfo(const char* format, ...);
    void logError(const char* format, ...);
};

// ========================== Builder Pattern ==========================
class NetworkBuilder {
public:
    NetworkBuilder& withWiFi(const char* ssid, const char* password);
    NetworkBuilder& withMQTT(const char* server, uint16_t port);
    NetworkBuilder& withMQTTAuth(const char* username, const char* password);
    NetworkBuilder& withClientId(const char* clientId);
    NetworkBuilder& withSSL(bool useSSL);
    NetworkBuilder& withTimeouts(uint32_t wifiTimeout, uint32_t mqttReconnectDelay);
    NetworkManager* build();
    
private:
    NetworkConfig config;
};

#endif // NETWORK_MANAGER_PUBSUB_H