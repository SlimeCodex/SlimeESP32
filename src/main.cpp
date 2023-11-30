// simple_stream: this example show how to send and receive data from multiple consoles

#include <Arduino.h>
#include <SmartSyncEvent.h>
#include <SkyStreamConsole.h>

// Initialize consoles
SkyStreamHandler ssc_handler;
SkyStreamConsole console_ota("OTA");

void setup() {
	Serial.begin(115200);

	// Generate consoles
	ssc_handler.begin();
	ssc_handler.ota(console_ota);
	ssc_handler.start();
}

void loop() {
	// Check for OTA update
	if (console_ota.available()) {
		if (console_ota.download()) {
			if (console_ota.update()) {
			}
		}
	}
}