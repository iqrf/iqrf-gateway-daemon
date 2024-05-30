/**
 * Copyright 2015-2021 IQRF Tech s.r.o.
 * Copyright 2019-2021 MICRORISC s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Driver.h"

Driver::Driver(const std::string &name, const int16_t &peripheralNumber, const double &version, const uint8_t &versionFlags, const std::string &driver, const std::string &driverHash) {
	this->name = name;
	this->peripheralNumber = peripheralNumber;
	this->version = version;
	this->versionFlags = versionFlags;
	this->driver = driver;
	this->driverHash = driverHash;
}

const uint32_t& Driver::getId() const {
	return this->id;
}

void Driver::setId(const uint32_t &id) {
	this->id = id;
}

const std::string& Driver::getName() const {
	return this->name;
}

void Driver::setName(const std::string &name) {
	this->name = name;
}

const int16_t& Driver::getPeripheralNumber() const {
	return this->peripheralNumber;
}

void Driver::setPeripheralNumber(const int16_t &peripheralNumber) {
	this->peripheralNumber = peripheralNumber;
}

const double& Driver::getVersion() const {
	return this->version;
}

void Driver::setVersion(const double &version) {
	this->version = version;
}

const uint8_t& Driver::getVersionFlags() const {
	return this->versionFlags;
}

void Driver::setVersionFlags(const uint8_t& versionFlags) {
	this->versionFlags = versionFlags;
}

const std::string& Driver::getDriver() const {
	return this->driver;
}

void Driver::setDriver(const std::string &driver) {
	this->driver = driver;
}

const std::string& Driver::getDriverHash() const {
	return this->driverHash;
}

void Driver::setDriverHash(const std::string &driverHash) {
	this->driverHash = driverHash;
}
