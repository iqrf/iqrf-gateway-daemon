/**
 * IQRF Gateway Daemon
 * Copyright (C) 2015-2025 IQRF Tech s.r.o., 2019-2025 MICRORISC s.r.o.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
