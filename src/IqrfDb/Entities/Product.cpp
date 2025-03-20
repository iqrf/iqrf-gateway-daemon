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

#include "Product.h"

Product::Product(const uint16_t &hwpid, const uint16_t &hwpidVersion, const uint16_t &osBuild, const std::string &osVersion, const uint16_t &dpaVersion) {
	this->hwpid = hwpid;
	this->hwpidVersion = hwpidVersion;
	this->osBuild = osBuild;
	this->osVersion = osVersion;
	this->dpaVersion = dpaVersion;
}

const uint32_t& Product::getId() const {
	return this->id;
}

void Product::setId(const uint32_t &id) {
	this->id = id;
}

const uint16_t& Product::getHwpid() const {
	return this->hwpid;
}

void Product::setHwpid(const uint16_t &hwpid) {
	this->hwpid = hwpid;
}

const uint16_t& Product::getHwpidVersion() const {
	return this->hwpidVersion;
}

void Product::setHwpidVersion(const uint16_t &hwpidVersion) {
	this->hwpidVersion = hwpidVersion;
}

const uint16_t& Product::getOsBuild() const {
	return this->osBuild;
}

void Product::setOsBuild(const uint16_t &osBuild) {
	this->osBuild = osBuild;
}

const std::string& Product::getOsVersion() const {
	return this->osVersion;
}

void Product::setOsVersion(const std::string &osVersion) {
	this->osVersion = osVersion;
}

const uint16_t& Product::getDpaVersion() const {
	return this->dpaVersion;
}

void Product::setDpaVersion(const uint16_t &dpaVersion) {
	this->dpaVersion = dpaVersion;
}

std::shared_ptr<std::string> Product::getHandlerUrl() const {
	return this->handlerUrl;
}

void Product::setHandlerUrl(std::shared_ptr<std::string> handlerUrl) {
	this->handlerUrl = std::move(handlerUrl);
}

std::shared_ptr<std::string> Product::getHandlerHash() const {
	return this->handlerHash;
}

void Product::setHandlerHash(std::shared_ptr<std::string> handlerHash) {
	this->handlerHash = std::move(handlerHash);
}

std::shared_ptr<std::string> Product::getCustomDriver() const {
	return this->customDriver;
}

void Product::setCustomDriver(std::shared_ptr<std::string> customDriver) {
	this->customDriver = std::move(customDriver);
}

std::shared_ptr<uint32_t> Product::getPackageId() const {
	return this->packageId;
}

void Product::setPackageId(std::shared_ptr<uint32_t> packageId) {
	this->packageId = std::move(packageId);
}

std::shared_ptr<std::string> Product::getName() const {
	return this->name;
}

void Product::setName(std::shared_ptr<std::string> name) {
	this->name = std::move(name);
}

bool Product::isValid() {
	return (this->osBuild > 0) && (this->dpaVersion > 0);
}
