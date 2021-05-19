/**
 * Copyright 2016-2017 MICRORISC s.r.o.
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

#ifndef SHAPE_PLATFORM_WINDOWS
#define GetLastError() errno
#define WSAGetLastError() errno
#define SOCKET_ERROR -1
int closesocket(int __fd) { return close(__fd); }
typedef int opttype;
#else
#define SHUT_RD SD_RECEIVE
typedef char opttype;
#endif

UdpChannel::UdpChannel(unsigned short remotePort, unsigned short localPort, unsigned dataBuffSize) {
	TRC_FUNCTION_ENTER(PAR(remotePort) << PAR(localPort) << PAR(dataBuffSize));
	m_isListening = false;
	m_runListenThread = true;
	m_remotePort = remotePort;
	m_localPort = localPort;
	m_dataBuffSize = dataBuffSize;

#ifdef SHAPE_PLATFORM_WINDOWS
	// Initialize Winsock
	WSADATA wsaData = { 0 };
	int iResult = 0;

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		THROW_EXC_TRC_WAR(UdpChannelException, "WSAStartup failed: " << GetLastError());
	}
#endif

	m_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (m_sockfd == -1) {
		THROW_EXC_TRC_WAR(UdpChannelException, "socket failed: " << GetLastError());
	}

	int ret;
	opttype opt = 1;
	// Enable packet info
	ret = setsockopt(m_sockfd, IPPROTO_IP, IP_PKTINFO, &opt, sizeof(opt));;
	if (ret == SOCKET_ERROR) {
		closesocket(m_sockfd);
		THROW_EXC_TRC_WAR(UdpChannelException, "setsockopt failed: " << GetLastError());
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

	m_dataBuff = shape_new char[m_dataBuffSize];
	memset(m_dataBuff, 0, m_dataBuffSize);

#ifndef SHAPE_PLATFORM_WINDOWS
	m_controlBuff = shape_new char[m_controlBuffSize];
	memset(m_controlBuff, 0, m_controlBuffSize);

	m_recIov[0].iov_base = m_dataBuff;
	m_recIov[0].iov_len = m_dataBuffSize;
	m_recHeader.msg_name = &m_addr;
	m_recHeader.msg_namelen = sizeof(m_addr);
	m_recHeader.msg_iov = m_recIov;
	m_recHeader.msg_iovlen = sizeof(m_recIov) / sizeof(*m_recIov);
	m_recHeader.msg_control = m_controlBuff;
	m_recHeader.msg_controllen = m_controlBuffSize;
#else
	m_receivingIp = "0.0.0.0";
	m_receivingMac = "00-00-00-00-00-00";
#endif

	// Bind socket to all interfaces
	ret = bind(m_sockfd, (struct sockaddr *)&m_listener, sizeof(m_listener));
	if (ret == SOCKET_ERROR) {
		closesocket(m_sockfd);
		THROW_EXC_TRC_WAR(UdpChannelException, "bind failed: " << GetLastError());
	}

	m_listenThread = std::thread(&UdpChannel::listen, this);
	TRC_FUNCTION_LEAVE("");
}

UdpChannel::~UdpChannel() {
	shutdown(m_sockfd, SHUT_RD);
	closesocket(m_sockfd);

	TRC_DEBUG("joining udp listening thread");
	if (m_listenThread.joinable())
		m_listenThread.join();
	TRC_DEBUG("listening thread joined");

#ifdef SHAPE_PLATFORM_WINDOWS
	WSACleanup();
#endif

	delete[] m_dataBuff;
}

void UdpChannel::sendTo(const std::basic_string<unsigned char>& message) {
	int sentBytes = sendto(m_sockfd, (const char *)message.data(), static_cast<unsigned int>(message.size()), 0, (struct sockaddr *)&m_sender, sizeof(m_sender));
	if (sentBytes == SOCKET_ERROR) {
		THROW_EXC_TRC_WAR(UdpChannelException, "Failed to send message, sendto(): [" << GetLastError() << "] " << strerror(GetLastError()));
	}
}

void UdpChannel::listen() {
	TRC_FUNCTION_ENTER("thread starts");

#ifdef SHAPE_PLATFORM_WINDOWS
	socklen_t listenerLen = sizeof(m_listener);
#endif

	try {
		m_isListening = true;
		while (m_runListenThread) {
#ifdef SHAPE_PLATFORM_WINDOWS
			int recBytes = recvfrom(m_sockfd, (char *)m_dataBuff, m_dataBuffSize, 0, (struct sockaddr *)&m_listener, &listenerLen);		
#else
			int recBytes = recvmsg(m_sockfd, &m_recHeader, 0);
#endif
			if (recBytes == SOCKET_ERROR) {
				THROW_EXC_TRC_WAR(UdpChannelException, "Failed to receive message, recvmsg(): [" << GetLastError() << "] " << strerror(GetLastError()));
			}
			identifyReceivingInterface();

			if (m_receivingIp == "0.0.0.0") {
				continue;
			}

			TRC_DEBUG("Received UDP datagram at IP " << m_receivingIp << ", MAC " << m_receivingMac);
			if (recBytes > 0) {
				if (m_messageHandler) {
#ifdef SHAPE_PLATFORM_WINDOWS
					std::basic_string<unsigned char> message(m_dataBuff, recBytes);
					if (m_messageHandler(message) == 0) {
						m_sender.sin_addr.s_addr = m_listener.sin_addr.s_addr;
					}
#else
					std::basic_string<unsigned char> message((unsigned char *)m_recHeader.msg_iov[0].iov_base, recBytes);
					if (m_messageHandler(message) == 0) {
						m_sender.sin_addr = m_addr.sin_addr;
					}
#endif
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

#ifndef SHAPE_PLATFORM_WINDOWS
void UdpChannel::identifyReceivingInterface() {
	m_receivingIp = "0.0.0.0";
	m_receivingMac = "00-00-00-00-00-00";
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
		if (!iface.isExpired()) {
			m_receivingIp = iface.getIp();
			m_receivingMac = iface.getMac();
			return;
		}
	}

	findInterfaceByIndex(idx);
}

void UdpChannel::findInterfaceByIndex(const int &idx) {
	struct ifreq ifrs[32];
	struct ifconf ifc;
	int res;

	memset(&ifc, 0, sizeof(ifconf));
	ifc.ifc_req = ifrs;
	ifc.ifc_len = sizeof(ifrs);

	SOCKET sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd == SOCKET_ERROR) {
		TRC_WARNING("Failed to create socket for interface info: [" << GetLastError() << "]: " << strerror(GetLastError()));
		return;
	}

	res = ioctl(sockfd, SIOCGIFCONF, (char *)&ifc);
	if (res == SOCKET_ERROR) {
		TRC_WARNING("Interface configuration ioctl failed: [" << GetLastError() << "]: " << strerror(GetLastError()));
		closesocket(sockfd);
		return;
	}

	for (unsigned int i = 0; i < ifc.ifc_len/sizeof(struct ifreq); ++i) {
		res = ioctl(sockfd, SIOCGIFINDEX, &ifrs[i]);
		if (res == SOCKET_ERROR) {
			TRC_WARNING("Interface index ioctl failed: [" << GetLastError() << "]: " << strerror(GetLastError()));
			closesocket(sockfd);
			break;
		}

		if (ifrs[i].ifr_ifindex != idx) {
			continue;
		}

		res = ioctl(sockfd, SIOCGIFADDR, &ifrs[i]);
		if (res == SOCKET_ERROR) {
			TRC_WARNING("Interface IP ioctl failed: [" << GetLastError() << "]: " << strerror(GetLastError()));
			closesocket(sockfd);
			break;
		}

		char ipBuffer[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &((struct sockaddr_in *)&ifrs[i].ifr_addr)->sin_addr, ipBuffer, sizeof(ipBuffer));
		
		uint8_t macBuffer[6];
		res = ioctl(sockfd, SIOCGIFHWADDR, &ifrs[i]);
		if (res == SOCKET_ERROR) {
			TRC_WARNING("Interface MAC ioctl failed: [" << GetLastError() << "]: " << strerror(GetLastError()));
			closesocket(sockfd);
			break;
		}
		closesocket(sockfd);
		memcpy(macBuffer, ifrs[i].ifr_hwaddr.sa_data, sizeof(macBuffer));
		
		std::string ip(ipBuffer);
		std::string mac(convertToMacString(macBuffer));
		std::time_t expiration = time(nullptr) + m_expirationPeriod;
		char datetime[32];
		std::strftime(datetime, sizeof(datetime), "%c", std::localtime(&expiration));
		
		if (m_interfaces.count(idx)) {
			TRC_DEBUG("Updating interface at index " << idx << " - IP: " << ip << " MAC: " << mac << ", expires at " << datetime);
			NetworkInterface iface = m_interfaces.find(idx)->second;
			if (iface.getIp() != ip) {
				iface.setIp(ip);
			}
			if (iface.getMac() != mac) {
				iface.setMac(mac);
			}
			iface.setExpiration(expiration);
		} else {
			TRC_DEBUG("Storing interface at index " << idx << " - IP: " << ip << " MAC: " << mac << ", expires at " << datetime);
			m_interfaces.insert(std::make_pair(idx, NetworkInterface(ip, mac, expiration)));
		}
		m_receivingIp = ip;
		m_receivingMac = mac;
		break;
	}
}

std::string UdpChannel::convertToMacString(const uint8_t *macBytes) {
	char buffer[18];
	std::sprintf(buffer, "%02X-%02X-%02X-%02X-%02X-%02X", macBytes[0], macBytes[1], macBytes[2], macBytes[3], macBytes[4], macBytes[5]);
	return std::string(buffer);
}
#endif

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
