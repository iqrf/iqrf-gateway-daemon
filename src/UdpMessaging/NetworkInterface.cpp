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
