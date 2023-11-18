// simple_stream example, this example code shows how to create and publish data to two simple monitors

#include <SmartSyncEvent.h>
#include <SkyStreamConsole.h>
#include <cstdlib>
#include <ctime>

SkyStreamConsole monitor_core("Core Monitor");
SkyStreamConsole monitor_wifi("WiFi Monitor");
SSCHandler ssc_handler;

void setup() {
	ssc_handler.begin();
	ssc_handler.add(monitor_core);
	ssc_handler.add(monitor_wifi);
	ssc_handler.start();

	// Seed the random number generator
	srand(time(NULL));
}

void loop() {
	if (SYNC_EVENT(100)) {
		char randomCharCore = 'C' + rand() % 26; // This will generate a random uppercase letter
		monitor_core.printf("%lu Core Monitor Data: %c\n", millis(), randomCharCore);

		char randomCharWifi = 'W' + rand() % 26; // This will generate a random uppercase letter
		monitor_wifi.printf("%lu WiFi Monitor Data: %c\n", millis(), randomCharWifi);
	}
}