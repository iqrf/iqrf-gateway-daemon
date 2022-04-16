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

#include "Sensor.h"

Sensor::Sensor(const uint8_t &type, const std::string &name, const std::string &shortname, const std::string &unit,
	const uint8_t &decimals, bool frc2Bit, bool frc1Byte, bool frc2Byte, bool frc4Byte) {
	this->type = type;
	this->name = name;
	this->shortname = shortname;
	this->unit = unit;
	this->decimals = decimals;
	this->frc2Bit = frc2Bit;
	this->frc1Byte = frc1Byte;
	this->frc2Byte = frc2Byte;
	this->frc4Byte = frc4Byte;
}

const uint32_t& Sensor::getId() const {
	return this->id;
}

void Sensor::setId(const uint32_t &id) {
	this->id = id;
}

const uint8_t& Sensor::getType() const {
	return this->type;
}

void Sensor::setType(const uint8_t &type) {
	this->type = type;
}

const std::string& Sensor::getName() const {
	return this->name;
}

void Sensor::setName(const std::string &name) {
	this->name = name;
}

const std::string& Sensor::getShortname() const {
	return this->shortname;
}

void Sensor::setShortname(const std::string &shortname) {
	this->shortname = shortname;
}

const std::string& Sensor::getUnit() const {
	return this->unit;
}

void Sensor::setUnit(const std::string &unit) {
	this->unit = unit;
}

const uint8_t& Sensor::getDecimals() const {
	return this->decimals;
}

void Sensor::setDecimals(const uint8_t &decimals) {
	this->decimals = decimals;
}

bool Sensor::hasFrc2Bit() const {
	return this->frc2Bit;
}

void Sensor::setFrc2Bit(bool frc2Bit) {
	this->frc2Bit = frc2Bit;
}

bool Sensor::hasFrc1Byte() const {
	return this->frc1Byte;
}

void Sensor::setFrc1Byte(bool frc1Byte) {
	this->frc1Byte = frc1Byte;
}

bool Sensor::hasFrc2Byte() const {
	return this->frc2Byte;
}

void Sensor::setFrc2Byte(bool frc2Byte) {
	this->frc2Byte = frc2Byte;
}

bool Sensor::hasFrc4Byte() const {
	return this->frc4Byte;
}

void Sensor::setFrc4Byte(bool frc4Byte) {
	this->frc4Byte = frc4Byte;
}

