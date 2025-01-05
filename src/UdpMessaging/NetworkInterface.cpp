/**
 * Copyright 2015-2025 IQRF Tech s.r.o.
 * Copyright 2019-2025 MICRORISC s.r.o.
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
#include "NetworkInterface.h"

NetworkInterface::NetworkInterface(const std::string &ip, const std::string &mac, const int32_t &metric, const std::time_t &expiration):
    ip(ip), mac(mac), metric(metric), expiration(expiration) {
}

const std::string NetworkInterface::getIp() const {
	return ip;
}

void NetworkInterface::setIp(const std::string &ip) {
	this->ip = ip;
}

const std::string NetworkInterface::getMac() const {
	return mac;
}

void NetworkInterface::setMac(const std::string &mac) {
	this->mac = mac;
}

const int32_t& NetworkInterface::getMetric() const {
	return metric;
}

bool NetworkInterface::hasLowerMetric(const int32_t &metric) const {
	return this->metric < metric;
}

void NetworkInterface::setMetric(const int32_t &metric) {
	this->metric = metric;
}

const std::time_t& NetworkInterface::getExpiration() const {
	return expiration;
}

bool NetworkInterface::isExpired() const {
	return expiration <= time(nullptr);
}

void NetworkInterface::setExpiration(const std::time_t &expiration) {
	this->expiration = expiration;
}
