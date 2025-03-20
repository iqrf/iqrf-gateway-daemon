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

#include "BinaryOutput.h"

const uint32_t& BinaryOutput::getId() const {
	return this->id;
}

void BinaryOutput::setId(const uint32_t &id) {
	this->id = id;
}

const uint32_t& BinaryOutput::getDeviceId() const {
	return this->deviceId;
}

void BinaryOutput::setDeviceId(const uint32_t &deviceId) {
	this->deviceId = deviceId;
}

const uint8_t& BinaryOutput::getCount() const {
	return this->count;
}

void BinaryOutput::setCount(const uint8_t &count) {
	this->count = count;
}
