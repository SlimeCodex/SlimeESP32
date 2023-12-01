#include <Arduino.h>
#include <SmartSyncEvent.h>
#include <ArcticTerminal.h>
#include <vector>
#include <map>
#include <string>

#define MAX_CONSOLES 20

ArcticTerminal console_ota("OTA");
ArcticTerminalHandler console_handler;

std::vector<ArcticTerminal*> consoles;
std::map<ArcticTerminal*, std::string> consoleNames;

void addConsoles(int count) {
	count = (count > MAX_CONSOLES) ? MAX_CONSOLES : count;
	for (int i = 0; i < count; i++) {
		char consoleName[21];
		sprintf(consoleName, "Dynamic Console %d", i + 1);
		ArcticTerminal* newConsole = new ArcticTerminal(consoleName);
		consoles.push_back(newConsole);
		consoleNames[newConsole] = consoleName;
	}
}

void setup() {
	Serial.begin(115200);
	console_handler.begin();
	console_handler.ota(console_ota);

	addConsoles(20);

	for (auto& console : consoles) {
		console_handler.add(*console);
	}
	console_handler.start();
}

void loop() {
	for (auto& console : consoles) {
		if (console->available()) {
			std::string com = console->read();
			std::string consoleName = consoleNames[console];
			if (com == "info") {
				console->singlef("%lu > This is %s\n", millis(), consoleName.c_str());
			}
		}
	}
	if (SYNC_EVENT(1000)) {
		for (auto& console : consoles) {
			std::string consoleName = consoleNames[console];
			console->printf("%lu > This is %s\n", millis(), consoleName.c_str());
		}
	}
}

void cleanup() {
	for (auto& console : consoles) {
		delete console;
	}
	consoles.clear();
}