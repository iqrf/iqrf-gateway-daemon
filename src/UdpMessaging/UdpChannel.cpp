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
#include <stdlib.h>     //srand, rand
#include <time.h>       //time
#include <string.h>

#ifndef SHAPE_PLATFORM_WINDOWS
#define GetLastError() errno
#define WSAGetLastError() errno
#define SOCKET_ERROR -1
int closesocket(int filedes) { return close(filedes); }
typedef int opttype;
#else
#define SHUT_RD SD_RECEIVE
typedef char opttype;
#endif

UdpChannel::UdpChannel(unsigned short remotePort, unsigned short localPort, unsigned bufsize)
	:m_runListenThread(true),
	m_remotePort(remotePort),
	m_localPort(localPort),
	m_dataBuffSize(bufsize) {
	TRC_FUNCTION_ENTER(PAR(remotePort) << PAR(localPort) << PAR(bufsize));
	m_isListening = false;

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

	m_controlBuff = shape_new char[m_controlBuffSize];
	m_dataBuff = shape_new char[m_dataBuffSize];
	memset(m_controlBuff, 0, m_controlBuffSize);
	memset(m_dataBuff, 0, m_dataBuffSize);
	
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
		THROW_EXC_TRC_WAR(UdpChannelException, "Failed to send message, sendto(): [" << errno << "] " << strerror(errno));
	}
}

void UdpChannel::listen() {
	TRC_FUNCTION_ENTER("thread starts");

	int recBytes;

	try {
		m_isListening = true;
		while (m_runListenThread) {
			recBytes = recvmsg(m_sockfd, &m_recHeader, 0);
			if (recBytes == SOCKET_ERROR) {
				THROW_EXC_TRC_WAR(UdpChannelException, "Failed to receive message, recvmsg(): [" << errno << "] " << strerror(errno));
			}
			identifyReceivingInterface();
			//m_receivingIp = parseReceivingIpAddress();
			/*if (m_receivingIp == "255.255.255.255") {
				char senderIp[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, &((struct sockaddr_in*)m_recHeader.msg_name)->sin_addr, senderIp, sizeof(struct sockaddr_in));
				deduceReceivingInterface(htonl(((struct sockaddr_in*)m_recHeader.msg_name)->sin_addr.s_addr));
			} else {
				m_receivingMac = matchReceivingMacAddress(m_receivingIp);
			}*/
			TRC_DEBUG("Received UDP datagram at IP " << m_receivingIp << ", MAC " << m_receivingMac);
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
	std::string address = "0.0.0.0", mac = "00-00-00-00-00-00";
	int idx = -1;
	for (m_cmsg = CMSG_FIRSTHDR(&m_recHeader); m_cmsg != NULL; m_cmsg = CMSG_NXTHDR(&m_recHeader, m_cmsg)) {
		if (m_cmsg->cmsg_level == IPPROTO_IP && m_cmsg->cmsg_type == IP_PKTINFO) {
			struct in_pktinfo *pi = (struct in_pktinfo *)CMSG_DATA(m_cmsg);
			idx = pi->ipi_ifindex;
			break;
		}
	}

	if (idx != -1) {
		struct ifreq ifrs[32];
		struct ifconf ifc;
		int res;

		memset(&ifc, 0, sizeof(ifconf));
		ifc.ifc_req = ifrs;
		ifc.ifc_len = sizeof(ifrs);

		res = ioctl(m_sockfd, SIOCGIFCONF, (char *)&ifc);
		if (res == SOCKET_ERROR) {
			TRC_WARNING("Interface configuration ioctl failed: [" << errno << "]: " << strerror(errno));
			m_receivingIp = address;
			m_receivingMac = mac;
			return;
		}

		for (unsigned int i = 0; i < ifc.ifc_len/sizeof(struct ifreq); ++i) {
			res = ioctl(m_sockfd, SIOCGIFINDEX, &ifrs[i]);
			if (res == SOCKET_ERROR) {
				TRC_WARNING("Interface index ioctl failed: [" << errno << "]: " << strerror(errno));
				break;
			}

			if (ifrs[i].ifr_ifindex != idx) {
				continue;
			}

			res = ioctl(m_sockfd, SIOCGIFADDR, &ifrs[i]);
			if (res == SOCKET_ERROR) {
				TRC_WARNING("Interface IP ioctl failed: [" << errno << "]: " << strerror(errno));
				break;
			}

			char addr[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &((struct sockaddr_in *)&ifrs[i].ifr_addr)->sin_addr, addr, sizeof(addr));
			address = std::string(addr);

			uint8_t macBytes[6];
			res = ioctl(m_sockfd, SIOCGIFHWADDR, &ifrs[i]);
			if (res == SOCKET_ERROR) {
				TRC_WARNING("Interface MAC ioctl failed: [" << errno << "]: " << strerror(errno));
				break;
			}

			memcpy(macBytes, ifrs[i].ifr_hwaddr.sa_data, sizeof(macBytes));
			mac = convertToMacString(macBytes);
			break;
		}
	}
	
	m_receivingIp = address;
	m_receivingMac = mac;
}

std::string UdpChannel::parseReceivingIpAddress() {
	std::string address = "0.0.0.0";
	for (m_cmsg = CMSG_FIRSTHDR(&m_recHeader); m_cmsg != NULL; m_cmsg = CMSG_NXTHDR(&m_recHeader, m_cmsg)) {
		if (m_cmsg->cmsg_level == IPPROTO_IP && m_cmsg->cmsg_type == IP_PKTINFO) {
			struct in_pktinfo *pi = (struct in_pktinfo *)CMSG_DATA(m_cmsg);
			char addr[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &pi->ipi_addr, addr, sizeof(addr));
			address = std::string(addr);
			break;
		}
	}
	return address;
}

std::string UdpChannel::matchReceivingMacAddress(const std::string &ip) {
	std::string mac = "00-00-00-00-00-00";
	if (m_receivingIp != "0.0.0.0") {
		struct ifreq ifrs[32];
		struct ifconf ifc;
		std::string ifaceIp;
		int res;

		memset(&ifc, 0, sizeof(ifconf));
		ifc.ifc_req = ifrs;
		ifc.ifc_len = sizeof(ifrs);

		res = ioctl(m_sockfd, SIOCGIFCONF, (char *)&ifc);
		if (res == SOCKET_ERROR) {
			TRC_WARNING("Interface configuration ioctl failed: " << errno << ": " << strerror(errno));
			return mac;
		}

		for (unsigned int i = 0; i < ifc.ifc_len/sizeof(struct ifreq); ++i) {
			ifaceIp = std::string(inet_ntoa(((struct sockaddr_in *)&ifrs[i].ifr_addr)->sin_addr));
			if (ifaceIp != ip) {
				continue;
			}
			uint8_t macBytes[6];
			res = ioctl(m_sockfd, SIOCGIFHWADDR, &ifrs[i]);
			if (res != SOCKET_ERROR) {
				memcpy(macBytes, ifrs[i].ifr_hwaddr.sa_data, sizeof(macBytes));
				mac = convertToMacString(macBytes);
			}
			break;
		}
	}
	return mac;
}

void UdpChannel::deduceReceivingInterface(const uint32_t &sender) {
	std::string ip = "0.0.0.0", mac = "00-00-00-00-00-00";
	struct ifreq ifrs[32];
	struct ifconf ifc;
	int res;
	uint32_t address, mask;

	memset(&ifc, 0, sizeof(ifconf));
	ifc.ifc_req = ifrs;
	ifc.ifc_len = sizeof(ifrs);

	res = ioctl(m_sockfd, SIOCGIFCONF, (char *)&ifc);
	if (res == SOCKET_ERROR) {
		TRC_WARNING("Interface configuration ioctl failed: " << errno << ": " << strerror(errno));
		m_receivingIp = ip;
		m_receivingMac = mac;
		return;
	}

	for (unsigned int i = 0; i < ifc.ifc_len/sizeof(struct ifreq); ++i) {
		if (!(ifrs[i].ifr_flags & IFF_BROADCAST)) {
			continue;
		}

		char ipAddr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &((struct sockaddr_in *)&ifrs[i].ifr_addr)->sin_addr, ipAddr, sizeof(ipAddr));
		address = htonl(((struct sockaddr_in *)&ifrs[i].ifr_addr)->sin_addr.s_addr);

		res = ioctl(m_sockfd, SIOCGIFNETMASK, (char *)&ifrs[i]);
		if (res == SOCKET_ERROR) {
			TRC_WARNING("Netmask ioctl failed for interface " << ifrs[i].ifr_name << ":" << errno << ": " << strerror(errno));
			continue;
		}
		mask = htonl(((struct sockaddr_in *)&ifrs[i].ifr_netmask)->sin_addr.s_addr);

		if ((sender & mask) != (address & mask)) {
			continue;
		}
		ip = std::string(ipAddr);

		res = ioctl(m_sockfd, SIOCGIFHWADDR, &ifrs[i]);
		if (res == SOCKET_ERROR) {
			TRC_WARNING("MAC address ioctl failed for interface " << ifrs[i].ifr_name << ":" << errno << ": " << strerror(errno));
			continue;
		}
		uint8_t macBytes[6];
		memcpy(macBytes, ifrs[i].ifr_hwaddr.sa_data, sizeof(macBytes));
		mac = convertToMacString(macBytes);
		break;
	}

	m_receivingIp = ip;
	m_receivingMac = mac;
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
