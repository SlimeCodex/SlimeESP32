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

#include <SkyStreamConsole.h>

// Callback Connection per server
class SSCCallbacks : public BLEServerCallbacks {
	void onConnect(BLEServer* pServer) override {
		Serial.println(pServer->getPeerMTU(pServer->getConnId()));
	}
	void onDisconnect(BLEServer* pServer) override {
		pServer->startAdvertising(); // Restart advertising on disconnect
	}
};

// Callback RX per console
class RxCharacteristicCallbacks : public BLECharacteristicCallbacks {
	SkyStreamConsole* console;

public:
	RxCharacteristicCallbacks(SkyStreamConsole* console) : console(console) {
	}
	void onWrite(BLECharacteristic* pCharacteristic, esp_ble_gatts_cb_param_t* param) override {
		if (console) {
			console->setNewDataAvailable(true);
		}
	}
};

// Constructor for handler
SkyStreamHandler::SkyStreamHandler() {
}

// Begin: Initialize BLE
void SkyStreamHandler::begin() {
	BLEDevice::init("SkyStreamConsole");
}

// Add OTA console to global map
void SkyStreamHandler::ota(SkyStreamConsole& console) {
	consoles.push_back(console);
}

// Add console to global map
void SkyStreamHandler::add(SkyStreamConsole& console) {
	consoles.push_back(console);
}

// Start: Start BLE server and advertising
void SkyStreamHandler::start() {
	pServer = BLEDevice::createServer();
	pServer->setCallbacks(new SSCCallbacks());

	for (auto& console : consoles) {
		console.get().start(pServer);
	}
	BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
	pAdvertising->start();
}

// Constructor for consoles
SkyStreamConsole::SkyStreamConsole(const std::string& monitorName) : pServer(nullptr), serviceID(-1), monitorName(monitorName) {
}
int SkyStreamConsole::serviceCount = 0;

// Start: Create server and service
void SkyStreamConsole::start(BLEServer* existingServer) {
	if (existingServer != nullptr) {
		pServer = existingServer;
	}
	serviceID = createService();
}

// Create service: Create console with TX, TXS, RX and Name Characteristics
int SkyStreamConsole::createService() {

	// Service 0 is reserved for OTA
	if (serviceCount == 0)
		_ota_console = true;
	else
		_ota_console = false;

	// Create service
	char serviceUUID[37];
	snprintf(serviceUUID, sizeof(serviceUUID), "4fafc201-1fb5-459e-%04x-c5c9c3319%03x", serviceCount, serviceCount);
	pService = pServer->createService(BLEUUID(serviceUUID), 15, 0);

	// TX
	char txCharUUID[37];
	snprintf(txCharUUID, sizeof(txCharUUID), "4fafc201-1fb5-459e-%04x-c5c9c3319a%02x", serviceCount, serviceCount);
	BLECharacteristic* txCharacteristic = pService->createCharacteristic(txCharUUID, BLECharacteristic::PROPERTY_NOTIFY);
	txCharacteristic->addDescriptor(new BLE2902());

	// TX (single)
	char txsCharUUID[37];
	snprintf(txsCharUUID, sizeof(txsCharUUID), "4fafc201-1fb5-459e-%04x-c5c9c3319b%02x", serviceCount, serviceCount);
	BLECharacteristic* txsCharacteristic = pService->createCharacteristic(txsCharUUID, BLECharacteristic::PROPERTY_NOTIFY);
	txsCharacteristic->addDescriptor(new BLE2902());

	// RX
	char rxCharUUID[37];
	snprintf(rxCharUUID, sizeof(rxCharUUID), "4fafc201-1fb5-459e-%04x-c5c9c3319c%02x", serviceCount, serviceCount);
	BLECharacteristic* rxCharacteristic = pService->createCharacteristic(rxCharUUID, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
	rxCharacteristic->setCallbacks(new RxCharacteristicCallbacks(this));

	// Name
	char nameCharUUID[37];
	snprintf(nameCharUUID, sizeof(nameCharUUID), "4fafc201-1fb5-459e-%04x-c5c9c3319d%02x", serviceCount, serviceCount);
	BLECharacteristic* monitorNameCharacteristic = pService->createCharacteristic(nameCharUUID, BLECharacteristic::PROPERTY_READ);
	monitorNameCharacteristic->setValue(monitorName);

	// Start service
	pService->start();
	pServer->getAdvertising()->addServiceUUID(serviceUUID);

	// Add service to map
	services[serviceCount] = ServiceCharacteristics{txCharacteristic, txsCharacteristic, rxCharacteristic};
	return serviceCount++;
}

// Printf TX: Multiline TX with format
void SkyStreamConsole::printf(const char* format, ...) {
	if (serviceID == -1) {
		return;
	}
	char buffer[512];
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);

	auto servicePair = services.find(serviceID);
	if (servicePair != services.end()) {
		BLECharacteristic* txCharacteristic = servicePair->second.txCharacteristic;
		if (txCharacteristic) {
			txCharacteristic->setValue((uint8_t*)buffer, strlen(buffer));
			txCharacteristic->notify();
		}
	}
	va_end(args);
}

// Singlef TX: Singleline TX with format
void SkyStreamConsole::singlef(const char* format, ...) {
	if (serviceID == -1) {
		return;
	}
	char buffer[512];
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);

	auto servicePair = services.find(serviceID);
	if (servicePair != services.end()) {
		BLECharacteristic* txsCharacteristic = servicePair->second.txsCharacteristic;
		if (txsCharacteristic) {
			txsCharacteristic->setValue((uint8_t*)buffer, strlen(buffer));
			txsCharacteristic->notify();
		}
	}

	va_end(args);
}

// Updates new data flag
void SkyStreamConsole::setNewDataAvailable(bool available) {
	newDataAvailable = available;
}

// Available RX: Check if new data is available
bool SkyStreamConsole::available() {
	bool available = newDataAvailable.load();
	newDataAvailable = false;

	// Check if OTA update is ready
	if (_ota_console && _ota_started) {
		ota_check_done();
	}

	return available;
}

// Read RX: Read RX data until delimiter
std::string SkyStreamConsole::read(char delimiter) {
	auto servicePair = services.find(serviceID);
	if (servicePair != services.end()) {
		BLECharacteristic* rxCharacteristic = servicePair->second.rxCharacteristic;
		if (rxCharacteristic) {
			std::string value = rxCharacteristic->getValue();
			std::stringstream valueStream(value);
			std::string line;
			std::getline(valueStream, line, delimiter);
			return line;
		}
	}
	return "";
}

// Read RX: Read raw RX data as vector
std::vector<uint8_t> SkyStreamConsole::raw() {
	auto servicePair = services.find(serviceID);
	if (servicePair != services.end()) {
		BLECharacteristic* rxCharacteristic = servicePair->second.rxCharacteristic;
		if (rxCharacteristic) {
			std::string value = rxCharacteristic->getValue();
			std::vector<uint8_t> bytes(value.begin(), value.end());
			return bytes;
		}
	}
	return std::vector<uint8_t>();
}

// Perform OTA update
bool SkyStreamConsole::update() {
	if (!_ota_console) return false;

	if (!_ota_started) {
		std::string ota_command = read();
		size_t ota_file_size = std::stoul(ota_command);
		_ota_timeout = millis();

		if (!Update.begin(ota_file_size)) {
			ota_send_ack("ERROR");
			ota_clear();
			return false;
		}
		ota_send_ack("READY");
		_ota_started = true;
	}
	else if (millis() - _ota_timeout > 5000) {
		Update.abort();
		ota_handle_error();
		ota_send_ack("ERROR");
		ota_clear();
		return false;
	}
	else {
		ota_digest_chunk();
	}
	return false;
}

// Check if OTA update is done
void SkyStreamConsole::ota_check_done() {
	if (Update.isFinished()) {
		if (Update.end()) {
			ota_send_ack("DONE");
			if (_md5_started) {
				_ota_md5.calculate();
				Serial.println(_ota_md5.toString());
			}
		}
		else {
			ota_send_ack("ERROR");
		}
		ota_clear();
	}
}

// Digest OTA chunk
void SkyStreamConsole::ota_digest_chunk() {
	std::vector<uint8_t> ota_bytes = raw();
	_ota_timeout = millis(); // Reset timeout
	if (!_md5_started) {
		_ota_md5.begin();
		_md5_started = true;
	}
	_ota_md5.add(ota_bytes.data(), ota_bytes.size());
	size_t written = Update.write(ota_bytes.data(), ota_bytes.size());
	if (written != ota_bytes.size()) {
		Serial.printf("Only %d/%d bytes written!\n", written, ota_bytes.size());
	}
	ota_send_ack("ACK");
}

// Handle OTA errors
void SkyStreamConsole::ota_handle_error() {
	int err = Update.getError();
	switch (err) {
		case UPDATE_ERROR_SPACE:
			Serial.println("Not enough space for update");
			break;
		case UPDATE_ERROR_SIZE:
			Serial.println("Bad size given");
			break;
		case UPDATE_ERROR_MD5:
			Serial.println("Update file is corrupted");
			break;
		case UPDATE_ERROR_MAGIC_BYTE:
			Serial.println("Magic byte is wrong, not 0xE9");
			break;
		default:
			Serial.printf("Unknown error code: %d\n", err);
			break;
	}
}

// Send ACK response, can be READY, ACK, ERROR or DONE
void SkyStreamConsole::ota_send_ack(const char* ack) {
	singlef("%s[%d]", ack, _ack_counter);
	_ack_counter++;
}

// Clear OTA variables
void SkyStreamConsole::ota_clear() {
	_ota_started = false;
	_md5_started = false;
	_ota_timeout = 0;
	_ack_counter = 0;
}