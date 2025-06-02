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
#include "UdpChannel.h"
#include "Trace.h"
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define SOCKET_ERROR -1

UdpChannel::UdpChannel(unsigned short remotePort, unsigned short localPort, unsigned int expiration, unsigned dataBuffSize) {
	TRC_FUNCTION_ENTER(PAR(remotePort) << PAR(localPort) << PAR(dataBuffSize));
	m_isListening = false;
	m_runListenThread = true;
	m_remotePort = remotePort;
	m_localPort = localPort;
	m_expirationPeriod = expiration;
	m_dataBuffSize = dataBuffSize;

	m_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (m_sockfd == -1) {
		THROW_EXC_TRC_WAR(UdpChannelException, "socket failed: " << errno);
	}

	int ret;
	int opt = 1;
	// Enable packet info
	ret = setsockopt(m_sockfd, IPPROTO_IP, IP_PKTINFO, &opt, sizeof(opt));;
	if (ret == SOCKET_ERROR) {
		close(m_sockfd);
		THROW_EXC_TRC_WAR(UdpChannelException, "setsockopt failed: " << errno);
	}

	// Initialize listener
	memset(&m_listener, 0, sizeof(m_listener));
	m_listener.sin_family = AF_INET;
	m_listener.sin_port = htons(m_localPort);
	m_listener.sin_addr.s_addr = htonl(INADDR_ANY);

	// Initialize sender
	memset(&m_sender, 0, sizeof(m_sender));
	m_sender.sin_family = AF_INET;
	m_sender.sin_port = htons(m_remotePort);

	m_dataBuff = shape_new unsigned char[m_dataBuffSize];
	memset(m_dataBuff, 0, m_dataBuffSize);

	m_controlBuff = shape_new unsigned char[m_controlBuffSize];
	memset(m_controlBuff, 0, m_controlBuffSize);

	m_recIov[0].iov_base = m_dataBuff;
	m_recIov[0].iov_len = m_dataBuffSize;
	m_recHeader.msg_name = &m_addr;
	m_recHeader.msg_namelen = sizeof(m_addr);
	m_recHeader.msg_iov = m_recIov;
	m_recHeader.msg_iovlen = sizeof(m_recIov) / sizeof(*m_recIov);
	m_recHeader.msg_control = m_controlBuff;
	m_recHeader.msg_controllen = m_controlBuffSize;

	// Bind socket to all interfaces
	ret = bind(m_sockfd, (struct sockaddr *)&m_listener, sizeof(m_listener));
	if (ret == SOCKET_ERROR) {
		close(m_sockfd);
		THROW_EXC_TRC_WAR(UdpChannelException, "bind failed: " << errno);
	}

	m_listenThread = std::thread(&UdpChannel::listen, this);
	TRC_FUNCTION_LEAVE("");
}

UdpChannel::~UdpChannel() {
	shutdown(m_sockfd, SHUT_RD);
	close(m_sockfd);

	TRC_DEBUG("joining udp listening thread");
	if (m_listenThread.joinable())
		m_listenThread.join();
	TRC_DEBUG("listening thread joined");

	delete[] m_controlBuff;
	delete[] m_dataBuff;
}

void UdpChannel::sendTo(const std::basic_string<unsigned char>& message) {
	int sentBytes = sendto(m_sockfd, (const char *)message.data(), static_cast<unsigned int>(message.size()), 0, (struct sockaddr *)&m_sender, sizeof(m_sender));
	if (sentBytes == SOCKET_ERROR) {
		THROW_EXC_TRC_WAR(UdpChannelException, "Failed to send message, sendto(): [" << errno << "] " << strerror(errno));
	}
}

void UdpChannel::listen() {
	TRC_FUNCTION_ENTER("thread starts");

	try {
		m_isListening = true;
		while (m_runListenThread) {
			int recBytes = recvmsg(m_sockfd, &m_recHeader, 0);
			if (recBytes == SOCKET_ERROR) {
				THROW_EXC_TRC_WAR(UdpChannelException, "Failed to receive message, recvmsg(): [" << errno << "] " << strerror(errno));
			}
			identifyReceivingInterface();

			if (m_receivingIp == "0.0.0.0") {
				continue;
			}

			std::string senderIp(inet_ntoa(m_addr.sin_addr));
			TRC_DEBUG("Received UDP datagram at IP " << m_receivingIp << ", MAC " << m_receivingMac << " from IP " << senderIp);
			if (recBytes > 0) {
				if (m_messageHandler) {
					std::basic_string<unsigned char> message((unsigned char *)m_recHeader.msg_iov[0].iov_base, recBytes);
					if (m_messageHandler(message) == 0) {
						m_sender.sin_addr = m_addr.sin_addr;
					}
				} else {
					TRC_WARNING("No message handler registered.");
				}
			}
		}
	} catch (const UdpChannelException& e) {
		CATCH_EXC_TRC_WAR(UdpChannelException, e, "listening thread finished");
		m_runListenThread = false;
	}
	m_isListening = false;
	TRC_FUNCTION_LEAVE("thread stopped");
}

void UdpChannel::identifyReceivingInterface() {
	m_receivingIp = "0.0.0.0";
	m_receivingMac = "00-00-00-00-00-00";
	m_receivingMetric = INT32_MAX;
	int idx = -1;
	for (m_cmsg = CMSG_FIRSTHDR(&m_recHeader); m_cmsg != NULL; m_cmsg = CMSG_NXTHDR(&m_recHeader, m_cmsg)) {
		if (m_cmsg->cmsg_level == IPPROTO_IP && m_cmsg->cmsg_type == IP_PKTINFO) {
			struct in_pktinfo *pi = (struct in_pktinfo *)CMSG_DATA(m_cmsg);
			idx = pi->ipi_ifindex;
			break;
		}
	}

	TRC_DEBUG("Index of receiving interface: " << idx);

	if (idx == -1) {
		return;
	}

	if (m_interfaces.count(idx)) {
		NetworkInterface iface = m_interfaces.find(idx)->second;
		TRC_DEBUG("Interface found in map.");
		if (!iface.isExpired()) {
			m_receivingMetric = iface.getMetric();
			if (isPriorityInterface(idx)) {
				m_receivingIp = iface.getIp();
				m_receivingMac = iface.getMac();
			}
			return;
		}
		TRC_DEBUG("Interface record in map expired.");
	}

	findInterfaceByIndex(idx);
}

bool UdpChannel::isPriorityInterface(const uint32_t &idx) {
	for (auto const& device: m_interfaces) {
		NetworkInterface storedIface = device.second;
		if (device.first == idx) {
			continue;
		}
		if (storedIface.isExpired()) {
			continue;
		}
		if (storedIface.getIp() == "0.0.0.0") {
			continue;
		}
		if (storedIface.hasLowerMetric(m_receivingMetric)) {
			return false;
		}
	}
	return true;
}

void UdpChannel::findInterfaceByIndex(const int &idx) {
	struct ifreq ifrs[32];
	struct ifconf ifc;
	int res;

	memset(&ifc, 0, sizeof(ifconf));
	ifc.ifc_req = ifrs;
	ifc.ifc_len = sizeof(ifrs);

	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd == SOCKET_ERROR) {
		TRC_WARNING("Failed to create socket for interface info: [" << errno << "]: " << strerror(errno));
		return;
	}

	res = ioctl(sockfd, SIOCGIFCONF, (char *)&ifc);
	if (res == SOCKET_ERROR) {
		TRC_WARNING("Interface configuration ioctl failed: [" << errno << "]: " << strerror(errno));
		close(sockfd);
		return;
	}

	for (unsigned int i = 0; i < ifc.ifc_len/sizeof(struct ifreq); ++i) {
		res = ioctl(sockfd, SIOCGIFINDEX, &ifrs[i]);
		if (res == SOCKET_ERROR) {
			TRC_WARNING("Interface index ioctl failed: [" << errno << "]: " << strerror(errno));
			break;
		}

		if (ifrs[i].ifr_ifindex != idx) {
			continue;
		}

		res = ioctl(sockfd, SIOCGIFADDR, &ifrs[i]);
		if (res == SOCKET_ERROR) {
			TRC_WARNING("Interface IP ioctl failed: [" << errno << "]: " << strerror(errno));
			break;
		}
		char ipBuffer[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &((struct sockaddr_in *)&ifrs[i].ifr_addr)->sin_addr, ipBuffer, sizeof(ipBuffer));

		std::string ip(ipBuffer);
		int metric = getInterfaceMetric(ip);

		uint8_t macBuffer[6];
		res = ioctl(sockfd, SIOCGIFHWADDR, &ifrs[i]);
		if (res == SOCKET_ERROR) {
			TRC_WARNING("Interface MAC ioctl failed: [" << errno << "]: " << strerror(errno));
			break;
		}
		memcpy(macBuffer, ifrs[i].ifr_hwaddr.sa_data, sizeof(macBuffer));

		std::string mac(convertToMacString(macBuffer));
		std::time_t expiration = time(nullptr) + m_expirationPeriod;
		char datetime[32];
		std::strftime(datetime, sizeof(datetime), "%c", std::localtime(&expiration));

		std::ostringstream ss;
		ss << "Interface [" << idx << "] - IP: " << ip << " MAC: " << mac << " metric: " << metric << ", expires at " << datetime;

		auto itr = m_interfaces.find(idx);
		if (itr != m_interfaces.end()) {
			ss << " updated";
			TRC_DEBUG(ss.str());
			itr->second.setIp(ip);
			itr->second.setMac(mac);
			itr->second.setMetric(metric);
			itr->second.setExpiration(expiration);
		} else {
			ss << " stored";
			TRC_DEBUG(ss.str());
			m_interfaces.insert(std::make_pair(idx, NetworkInterface(ip, mac, metric, expiration)));
		}

		m_receivingMetric = metric;
		if (isPriorityInterface(idx)) {
			m_receivingIp = ip;
			m_receivingMac = mac;
		}
		break;
	}
	close(sockfd);
}

int UdpChannel::getInterfaceMetric(const std::string &ip) {
	int metric = INT32_MAX;
	std::string command("ip route | grep '" + ip + " metric'"), output;
	char buffer[256];

	FILE *exec = popen(command.c_str(), "r");
	if (!exec) {
		TRC_WARNING("IP route exec failed.");
		return metric;
	}

	while (fgets(buffer, 256, exec) != NULL) {
		output += buffer;
	}
	pclose(exec);

	if (output.empty()) {
		return metric;
	}

	std::vector<std::string> routeTokens = split(output, " ");
	for (size_t i = 0; i < routeTokens.size(); ++i) {
		if (routeTokens[i] == "metric" && (i + 1) < routeTokens.size()) {
			try {
				metric = std::stoi(routeTokens[i + 1]);
			} catch (const std::exception &e) {
				TRC_WARNING("IP route metric conversion failed: " << e.what());
			}
			break;
		}
	}

	return metric;
}

std::vector<std::string> UdpChannel::split(const std::string &string, const std::string &delimiter) {
	std::string token;
	std::vector<std::string> tokens;
	size_t needle, start = 0, len = delimiter.length();
	while ((needle = string.find(delimiter, start)) != std::string::npos) {
		token = string.substr(start, needle - start);
		tokens.push_back(token);
		start = needle + len;
	}
	tokens.push_back(string.substr(start));
	return tokens;
}


std::string UdpChannel::convertToMacString(const uint8_t *macBytes) {
	char buffer[18];
	std::sprintf(buffer, "%02X-%02X-%02X-%02X-%02X-%02X", macBytes[0], macBytes[1], macBytes[2], macBytes[3], macBytes[4], macBytes[5]);
	return std::string(buffer);
}

void UdpChannel::registerReceiveFromHandler(ReceiveFromFunc messageHandler) {
	m_messageHandler = messageHandler;
}

void UdpChannel::unregisterReceiveFromHandler() {
	m_messageHandler = ReceiveFromFunc();
}

IChannel::State UdpChannel::getState() {
	return State::Ready;
}

const std::string& UdpChannel::getListeningIpAddress() const {
	return m_receivingIp;
}

const std::string& UdpChannel::getListeningMacAddress() const {
	return m_receivingMac;
}

unsigned short UdpChannel::getListeningIpPort() const {
	return m_localPort;
}

bool UdpChannel::isListening() const {
	return m_isListening;
}
