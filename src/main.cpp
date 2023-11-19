// simple_stream: this example show how to send and receive data from multiple consoles

#include <WiFi.h>
#include <SmartSyncEvent.h>
#include <SkyStreamConsole.h>

// Initialize consoles and handler
SkyStreamConsole console_core("Core Console");
SkyStreamConsole console_wifi("WiFi Console");
SSCHandler ssc_handler;

void setup() {
	Serial.begin(115200);
	Serial.println("Boot!");

	// Generate consoles
	ssc_handler.begin();
	ssc_handler.add(console_core);
	ssc_handler.add(console_wifi);
	ssc_handler.start();
}

void loop() {

	// Simple periodic hello world
	if (SYNC_EVENT(500)) {
		console_core.printf("%lu > Core: Hello World !!\n", millis());
		console_wifi.printf("%lu > Wifi: Hello World !!\n", millis());
	}

	// Check for incoming commands
	if (console_core.available()) {
		std::string com = console_core.read();

		if (com == "reset") {
			console_core.printf("%lu > Restarting MCU\n", millis());
		}
		if (com == "get_mac") {
			console_core.printf("%lu > FF:FF:FF:FF:FF:FF\n", millis());
		}
	}

	// Check for incoming commands
	if (console_wifi.available()) {
		std::string com = console_wifi.read();
		
		if (com == "connect") {
			console_wifi.printf("%lu > Connecting to WiFi...\n", millis());
		}
	}
}