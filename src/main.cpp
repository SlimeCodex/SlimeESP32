// simple_stream: this example shows how to use the ArcticTerminal class to create multiple consoles and OTA update

#include <Arduino.h>
#include <ArcticClient.h>
#include <SmartSyncEvent.h>
#include <SlimyTask.h>
#include <WiFi.h>

// Initialize consoles
ArcticClient arctic_client;
ArcticTerminal console_core("Core Console");
ArcticTerminal console_wifi("WiFi Console");

// Task functions
void task_core(void* pvParameter);
void task_ota(void* pvParameter);
void task_wifi(void* pvParameter);

// Task handlers
SlimyTask esp_task_core(8 * 1024, task_core, "core", 1, 1);
SlimyTask esp_task_ota(8 * 1024, task_ota, "ota", 1, 1);
SlimyTask esp_task_wifi(8 * 1024, task_wifi, "wifi", 1, 1);

void setup() {
	Serial.begin(115200);

	// Generate consoles
	arctic_client.begin();
	arctic_client.add(console_core);
	arctic_client.add(console_wifi);
	arctic_client.start();

	// Start tasks
	esp_task_core.start();
	esp_task_wifi.start();
	esp_task_ota.start();
}

void task_core(void* pvParameter) {
	while (1) {
		// Check for incoming commands
		if (console_core.available()) {
			ArcticCommand com(console_core.read());

			if (com.base() == "get_mac") {
				console_core.printf("%lu > AA:BB:CC:DD:EE:FF\n", millis());
			}
			if (com.base() == "reset") {
				console_core.printf("%lu > Restarting MCU\n", millis());
				delay(500);
				ESP.restart();
			}
			if (com.base() == "version") {
				console_core.printf("%lu > Version 1.0.0\n", millis());
			}
			// Simple progress bar
			if (com.base() == "load") {
				std::string progress_bar;
				for (int i = 0; i <= 100; i++) {
					progress_bar.clear();
					progress_bar.append(i / 2, '|');
					progress_bar.append(50 - i / 2, ' ');
					console_core.singlef("%lu > Loading data |%s| %d%%\n", millis(), progress_bar.c_str(), i);
					delay(20);
				}
			}
		}
	}
}

void task_wifi(void* pvParameter) {
	while (1) {
		// Check for incoming commands
		if (console_wifi.available()) {
			ArcticCommand com = console_wifi.read();

			if (com.base() == "connect") {
				if (com.check("-u") && com.check("-p")) {
					std::string user = com.arg("-u");
					std::string pass = com.arg("-p");

					console_wifi.printf("%lu > Connecting to [%s] with password [%s] ", millis(), user.c_str(), pass.c_str());
					WiFi.begin(user.c_str(), pass.c_str());

					while (WiFi.status() != WL_CONNECTED) {
						console_wifi.printf(".");
						delay(500);
					}
					console_wifi.printf("\n");

					console_wifi.printf("%lu > Connected with IP %s\n", millis(), WiFi.localIP().toString().c_str());
				}
			}
			if (com.base() == "disconnect") {
				console_wifi.printf("%lu > Disconnected\n", millis());
				WiFi.disconnect();
			}
		}
	}
}

void task_ota(void* pvParameter) {
	while (1) {
		// Check for OTA update, should this block the loop?
		if (arctic_client.ota.available()) {
			if (arctic_client.ota.download()) {
				delay(500);
				ESP.restart();
			}
		}
	}
}

void loop() {
	vTaskDelete(NULL);
}