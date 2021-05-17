#include "NetworkInterface.h"

NetworkInterface::NetworkInterface(const std::string &ip, const std::string &mac, const std::time_t &expiration):
    ip(ip), mac(mac), expiration(expiration) {
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

bool NetworkInterface::isExpired() const {
	return expiration <= time(nullptr);
}

void NetworkInterface::setExpiration(const std::time_t expiration) {
	this->expiration = expiration;
}
