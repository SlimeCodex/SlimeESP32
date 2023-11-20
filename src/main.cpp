// simple_stream: this example show how to send and receive data from multiple consoles

#include <Arduino.h>
#include <SmartSyncEvent.h>
#include <SkyStreamConsole.h>

// Initialize consoles and handler
SkyStreamConsole console_core("Core Console");
SkyStreamConsole console_wifi("WiFi Console");
SkyStreamConsole console_hello("Hello Console");
SSCHandler ssc_handler;

void setup() {
	// Generate consoles
	ssc_handler.begin();
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

	// Check for incoming commands
	if (console_core.available()) {
		std::string com = console_core.read();

		if (com == "get_mac") {
			console_core.printf("%lu > AA:BB:CC:DD:EE:FF\n", millis());
		}
		if (com == "reset") {
			console_core.printf("%lu > Restarting MCU\n", millis());
		}

		// Simple progress bar
		if (com == "load") {
			std::string progress_bar;
			for (int i = 0; i <= 100; i++) {
				progress_bar.clear();
				progress_bar.append(i/2, 'l');
				progress_bar.append(50 - i/2, ' ');
				console_core.singlef("%lu > Loading data [%s] %d%%\n", millis(), progress_bar.c_str(), i);
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
}