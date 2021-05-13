#include "NetworkInterface.h"

NetworkInterface::NetworkInterface(std::string ip, std::string mac, std::time_t expiration) {
	this->ip = ip;
	this->mac = mac;
	this->expiration = expiration;
}

const std::string NetworkInterface::getIp() const {
	return ip;
}

void NetworkInterface::setIp(const std::string ip) {
	this->ip = ip;
}

const std::string NetworkInterface::getMac() const {
	return mac;
}

void NetworkInterface::setMac(const std::string mac) {
	this->mac = mac;
}

bool NetworkInterface::isExpired() const {
	return expiration <= time(nullptr);
}

void NetworkInterface::setExpiration(const std::time_t expiration) {
	this->expiration = expiration;
}
