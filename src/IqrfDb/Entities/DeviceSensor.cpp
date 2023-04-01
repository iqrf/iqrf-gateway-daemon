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
