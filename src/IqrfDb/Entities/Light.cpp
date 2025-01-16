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

#include "Light.h"

const uint32_t& Light::getId() const {
	return this->id;
}

void Light::setId(const uint32_t &id) {
	this->id = id;
}

const uint32_t& Light::getDeviceId() const {
	return this->deviceId;
}

void Light::setDeviceId(const uint32_t &deviceId) {
	this->deviceId = deviceId;
}

const uint8_t& Light::getCount() const {
	return this->count;
}

void Light::setCount(const uint8_t &count) {
	this->count = count;
}
