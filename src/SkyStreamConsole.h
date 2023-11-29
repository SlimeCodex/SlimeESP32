/*
 * This file is part of SkyStreamConsole Library.
 * Copyright (C) 2023 Alejandro Nicolini
 *
 * SkyStreamConsole is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SkyStreamConsole is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SkyStreamConsole. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef ___SKY_STREAM_CONSOLE_H___
#define ___SKY_STREAM_CONSOLE_H___

#include <Arduino.h>

#include <NimBLEDevice.h>

#include <cstdarg>
#include <sstream>
#include <atomic>
#include <string>
#include <vector>
#include <map>

#include <Update.h>
#include <MD5Builder.h>

class SkyStreamConsole;

class SkyStreamHandler {
public:
	SkyStreamHandler();
	void begin();
	void ota(SkyStreamConsole& console); // Register OTA console
	void add(SkyStreamConsole& console); // Register data console
	void start();

private:
	NimBLEServer* pServer;
	std::vector<std::reference_wrapper<SkyStreamConsole>> consoles;
};

class SkyStreamConsole {
public:
	SkyStreamConsole(const std::string& monitorName);
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
	friend class SkyStreamHandler;
	
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