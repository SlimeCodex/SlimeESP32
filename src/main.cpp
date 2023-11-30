#include <Arduino.h>
#include <SmartSyncEvent.h>
#include <SkyStreamConsole.h>

// Initialize consoles
SkyStreamHandler ssc_handler;
SkyStreamConsole console_ota("OTA");
SkyStreamConsole console_core("Core Console");
SkyStreamConsole console_wifi("WiFi Console");
SkyStreamConsole console_hello("Hello Console");

void setup() {
	Serial.begin(115200);

	// Generate consoles
	ssc_handler.begin();
	ssc_handler.ota(console_ota);
	ssc_handler.add(console_core);
	ssc_handler.add(console_wifi);
	ssc_handler.add(console_hello);
	ssc_handler.start();
}

void loop() {

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

	// Check for incoming commands
	if (console_core.available()) {
		std::string com = console_core.read();

		if (com == "get_mac") {
			console_core.printf("%lu > AA:BB:CC:DD:EE:FF\n", millis());
		}
		if (com == "reset") {
			console_core.printf("%lu > Restarting MCU\n", millis());
			delay(500);
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

	// Check for OTA update
	if (console_ota.available()) {
		if (console_ota.download()) {
		}
	}
}