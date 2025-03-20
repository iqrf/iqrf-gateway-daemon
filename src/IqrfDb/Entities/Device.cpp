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
