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

#include "DeviceSensor.h"

DeviceSensor::DeviceSensor(const uint8_t &address, const uint8_t &type, const uint8_t &globalIndex, const uint8_t &typeIndex, const uint32_t &sensorId, std::shared_ptr<double> value) {
	this->address = address;
	this->type = type;
	this->globalIndex = globalIndex;
	this->typeIndex = typeIndex;
	this->sensorId = sensorId;
	this->value = value;
}

const uint8_t& DeviceSensor::getAddress() const {
	return this->address;
}

void DeviceSensor::setAddress(const uint8_t &address) {
	this->address = address;
}

const uint8_t& DeviceSensor::getType() const {
	return this->type;
}

void DeviceSensor::setType(const uint8_t &type) {
	this->type = type;
}

const uint8_t& DeviceSensor::getGlobalIndex() const {
	return this->globalIndex;
}

void DeviceSensor::setGlobalIndex(const uint8_t &globalIndex) {
	this->globalIndex = globalIndex;
}

const uint8_t& DeviceSensor::getTypeIndex() const {
	return this->typeIndex;
}

void DeviceSensor::setTypeIndex(const uint8_t &typeIndex) {
	this->typeIndex = typeIndex;
}

const uint32_t& DeviceSensor::getSensorId() const {
	return this->sensorId;
}

void DeviceSensor::setSensorId(const uint32_t &sensorId) {
	this->sensorId = sensorId;
}

std::shared_ptr<double> DeviceSensor::getValue() const {
	return this->value;
}

void DeviceSensor::setValue(std::shared_ptr<double> value) {
	this->value = value;
}

std::shared_ptr<std::string> DeviceSensor::getUpdated() const {
	return this->updated;
}

void DeviceSensor::setUpdated(std::shared_ptr<std::string> updated) {
	this->updated = updated;
}

std::shared_ptr<std::string> DeviceSensor::getMetadata() const {
	return this->metadata;
}

void DeviceSensor::setMetadata(std::shared_ptr<std::string> metadata) {
	this->metadata = metadata;
}
