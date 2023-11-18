// simple_stream: this example code shows how to create and publish data to two simple monitors

#include <SmartSyncEvent.h>
#include <SkyStreamConsole.h>

// Deps for the random generator
#include <cstdlib>
#include <ctime>

// Initialize consoles and handler
SkyStreamConsole monitor_core("Core Monitor");
SkyStreamConsole monitor_wifi("WiFi Monitor");
SSCHandler ssc_handler;

// 10 Digits random number generator for the example
unsigned long long generateRandom10DigitNumber() {
    unsigned long long number = rand() % 9 + 1;
    for (int i = 1; i < 10; ++i) {
        number = number * 10 + (rand() % 10);
    }
    return number;
}

void setup() {
	// Generate consoles
	ssc_handler.begin();
	ssc_handler.add(monitor_core);
	ssc_handler.add(monitor_wifi);
	ssc_handler.start();
}

void loop() {
	// Print to consoles time and random data
	if (SYNC_EVENT(100)) {
		monitor_core.printf("%lu Core Monitor Data: %llu\n", millis(), generateRandom10DigitNumber());
		monitor_wifi.printf("%lu WiFi Monitor Data: %llu\n", millis(), generateRandom10DigitNumber());
	}
}