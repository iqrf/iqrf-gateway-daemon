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

#include "Device.h"

Device::Device(const uint8_t &address, bool discovered, const uint32_t &mid, const uint8_t &vrn, const uint8_t &zone, std::shared_ptr<uint8_t> parent) {
	this->address = address;
	this->discovered = discovered;
	this->mid = mid;
	this->vrn = vrn;
	this->zone = zone;
	this->parent = parent;
	this->enumerated = false;
	this->productId = 0;
}

const uint32_t& Device::getId() const {
	return this->id;
}

void Device::setId(const uint32_t &id) {
	this->id = id;
}

const uint8_t& Device::getAddress() const {
	return this->address;
}

void Device::setAddress(const uint8_t &address) {
	this->address = address;
}

bool Device::isDiscovered() const{
	return this->discovered;
}

void Device::setDiscovered(bool discovered) {
	this->discovered = discovered;
}

const uint32_t& Device::getMid() const {
	return this->mid;
}

void Device::setMid(const uint32_t &mid) {
	this->mid = mid;
}

const uint8_t& Device::getVrn() const {
	return this->vrn;
}

void Device::setVrn(const uint8_t &vrn) {
	this->vrn = vrn;
}

const uint8_t& Device::getZone() const {
	return this->zone;
}

void Device::setZone(const uint8_t &zone) {
	this->zone = zone;
}

std::shared_ptr<uint8_t> Device::getParent() const {
	return this->parent;
}

void Device::setParent(std::shared_ptr<uint8_t> parent) {
	this->parent = std::move(parent);
}

bool Device::isEnumerated() const {
	return this->enumerated;
}

void Device::setEnumerated(bool enumerated) {
	this->enumerated = enumerated;
}

const uint32_t& Device::getProductId() const {
	return this->productId;
}

void Device::setProductId(const uint32_t &productId) {
	this->productId = productId;
}

std::shared_ptr<std::string> Device::getMetadata() const {
	return this->metadata;
}

void Device::setMetadata(std::shared_ptr<std::string> metadata) {
	this->metadata = std::move(metadata);
}

bool Device::isValid() {
	return this->mid > 0;
}
