// simple_stream: this example shows how to use the ArcticTerminal class to create multiple consoles and OTA update

#include <Arduino.h>
#include <SlimyTask.h>
#include <SmartSyncEvent.h>
#include <ArcticTerminal.h>

// Initialize consoles
ArcticTerminalHandler console_handler;
ArcticTerminal console_ota("OTA");
ArcticTerminal console_core("Core Console");
ArcticTerminal console_wifi("WiFi Console");
ArcticTerminal console_hello("Hello Console");

// Task functions
void task_core(void* pvParameter);
void task_ota(void* pvParameter);
void task_wifi(void* pvParameter);
void task_hello(void* pvParameter);

// Task handlers
SlimyTask esp_task_core(5*1024, task_core, "core", 1, 1);
SlimyTask esp_task_ota(5*1024, task_ota, "ota", 1, 1);
SlimyTask esp_task_wifi(5*1024, task_wifi, "wifi", 1, 1);
SlimyTask esp_task_hello(5*1024, task_hello, "hello", 1, 1);

void setup() {
	Serial.begin(115200);

	// Generate consoles
	console_handler.debug(true);
	
	console_handler.begin();
	console_handler.profile(ARCTIC_PROFILE_MAX_SPEED);
	console_handler.ota(console_ota);
	console_handler.add(console_core);
	console_handler.add(console_wifi);
	console_handler.add(console_hello);
	console_handler.start();

	// Start tasks
	esp_task_core.start();
	esp_task_ota.start();
	esp_task_wifi.start();
	esp_task_hello.start();
}

void task_core(void* pvParameter) {
	while(1) {
		// Check for incoming commands
		if (console_core.available()) {
			std::string com = console_core.read();

			if (com == "get_mac") {
				console_core.printf("%lu > AA:BB:CC:DD:EE:FF\n", millis());
			}
			if (com == "reset") {
				console_core.printf("%lu > Restarting MCU\n", millis());
				ESP.restart();
			}

			// Simple progress bar
			if (com == "load") {
				std::string progress_bar;
				for (int i = 0; i <= 100; i++) {
					progress_bar.clear();
					progress_bar.append(i/2, '|');
					progress_bar.append(50 - i/2, ' ');
					console_core.singlef("%lu > Loading data |%s| %d%%\n", millis(), progress_bar.c_str(), i);
					delay(20);
				}
			}
		}
	}
}

void task_ota(void* pvParameter) {
	while(1) {
		// Check for OTA update, should this block the loop?
		if (console_ota.available()) {
			if (console_ota.download()) {
				delay(500);
				ESP.restart();
			}
		}
	}
}

void task_wifi(void* pvParameter) {
	while(1) {
		// Check for incoming commands
		if (console_wifi.available()) {
			std::string com = console_wifi.read();
			
			if (com == "connect") {
				console_wifi.printf("%lu > Connecting to WiFi...\n", millis());
			}
			if (com == "disconnect") {
				console_wifi.printf("%lu > Disconnected\n", millis());
			}
		}
	}
}

void task_hello(void* pvParameter) {
	while(1) {
		// Simple hello world every 500ms
		if (SYNC_EVENT(500)) {
			console_hello.printf("%lu > Hello World !!\n", millis());
		}

		if (console_hello.available()) {
			std::string com = console_hello.read();
			
			if (com == "hello") {
				console_hello.singlef("%lu > hello !!\n", millis());
			}
		}
	}
}

void loop() {vTaskDelete(NULL);}