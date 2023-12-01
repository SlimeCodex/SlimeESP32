/*
 * This file is part of ArcticTerminal Library.
 * Copyright (C) 2023 Alejandro Nicolini
 *
 * ArcticTerminal is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ArcticTerminal is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ArcticTerminal. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef ___ARCTIC_TERMINAL_H___
#define ___ARCTIC_TERMINAL_H___

#include <NimBLEDevice.h>
#include <MD5Builder.h>
#include <Update.h>

#include <cstdarg>
#include <sstream>
#include <atomic>
#include <string>
#include <vector>
#include <map>

class ArcticTerminal;

class ArcticTerminalHandler {
public:
	ArcticTerminalHandler();
	void begin();
	void ota(ArcticTerminal& console); // Register OTA console
	void add(ArcticTerminal& console); // Register data console
	void start();

private:
	NimBLEServer* pServer;
	std::vector<std::reference_wrapper<ArcticTerminal>> consoles;
};

class ArcticTerminal {
public:
	ArcticTerminal(const std::string& monitorName);
	void start(NimBLEServer* existingServer);
	int createService();
	void printf(const char* format, ...);
	void singlef(const char* format, ...);
	bool available();
	bool download();
	bool update();
	void setNewDataAvailable(bool available);
	std::string read(char delimiter = '\n');
	std::vector<uint8_t> raw();

private:
	friend class ArcticTerminalHandler;
	
	struct ServiceCharacteristics {
		NimBLECharacteristic* txCharacteristic;
		NimBLECharacteristic* txsCharacteristic;
		NimBLECharacteristic* rxCharacteristic;
	};

	std::string monitorName;
	static int serviceCount;
	int serviceID;

	NimBLEServer* pServer;
	NimBLEService* pService;

	std::atomic<bool> newDataAvailable{false};
	std::map<int, ServiceCharacteristics> services;


	// OTA variables
	bool _ota_console = false;
	bool _ota_started = false;
	bool _md5_started = false;
	unsigned long _ota_timeout = 0;
	unsigned int _ack_counter = 0;
	MD5Builder _ota_md5;

	// OTA functions
	void ota_digest_chunk();
	void ota_handle_error();
	void ota_check_done();
	void ota_send_ack(const char* ack);
	void ota_clear();
};

#endif