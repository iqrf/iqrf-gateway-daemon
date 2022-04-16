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

#include "Product.h"

Product::Product(const uint16_t &hwpid, const uint16_t &hwpidVersion, const uint16_t &osBuild, const std::string &osVersion, const uint16_t &dpaVersion) {
	this->hwpid = hwpid;
	this->hwpidVersion = hwpidVersion;
	this->osBuild = osBuild;
	this->osVersion = osVersion;
	this->dpaVersion = dpaVersion;
	this->standardEnumerated = false;
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

std::shared_ptr<std::string> Product::getNotes() const {
	return this->notes;
}

void Product::setNotes(std::shared_ptr<std::string> notes) {
	this->notes = std::move(notes);
}

std::shared_ptr<std::string> Product::getCustomDriver() const {
	return this->customDriver;
}

void Product::setCustomDriver(std::shared_ptr<std::string> customDriver) {
	this->customDriver = std::move(customDriver);
}

const uint32_t& Product::getPackageId() const {
	return this->packageId;
}

void Product::setPackageId(const uint32_t &packageId) {
	this->packageId = packageId;
}

bool Product::isStandardEnumerated() const {
	return this->standardEnumerated;
}

void Product::setStandardEnumerated(bool standardEnumerated) {
	this->standardEnumerated = standardEnumerated;
}

bool Product::isValid() { 
	return (this->osBuild > 0) && (this->dpaVersion > 0);
}
