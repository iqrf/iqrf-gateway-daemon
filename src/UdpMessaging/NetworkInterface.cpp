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
