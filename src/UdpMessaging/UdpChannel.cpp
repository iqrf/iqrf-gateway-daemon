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

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd == -1) {
		THROW_EXC_TRC_WAR(UdpChannelException, "socket failed: " << GetLastError());
	}

	int ret;
	opttype opt = 1;
	// Enable packet info
	ret = setsockopt(sockfd, IPPROTO_IP, IP_PKTINFO, &opt, sizeof(opt));;
	if (ret == SOCKET_ERROR) {
		closesocket(sockfd);
		THROW_EXC_TRC_WAR(UdpChannelException, "setsockopt failed: " << GetLastError());
	}

	// Local server, packets are received from any IP
	memset(&m_listener, 0, sizeof(m_listener));
	m_listener.sin_family = AF_INET;
	m_listener.sin_port = htons(m_localPort);
	m_listener.sin_addr.s_addr = htonl(INADDR_ANY);

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
	ret = bind(sockfd, (struct sockaddr *)&m_listener, sizeof(m_listener));
	if (ret == SOCKET_ERROR) {
		closesocket(sockfd);
		THROW_EXC_TRC_WAR(UdpChannelException, "bind failed: " << GetLastError());
	}

	m_listenThread = std::thread(&UdpChannel::listen, this);
	TRC_FUNCTION_LEAVE("");
}

UdpChannel::~UdpChannel() {
	shutdown(sockfd, SHUT_RD);
	closesocket(sockfd);

	TRC_DEBUG("joining udp listening thread");
	if (m_listenThread.joinable())
		m_listenThread.join();
	TRC_DEBUG("listening thread joined");

	#ifdef SHAPE_PLATFORM_WINDOWS
	WSACleanup();
	#endif

	delete[] m_dataBuff;
}

void UdpChannel::listen() {
	TRC_FUNCTION_ENTER("thread starts");

	int recn = -1;
	//socklen_t iqrfUdpListenerLength = sizeof(m_listener);

	try {
		m_isListening = true;
		while (m_runListenThread) {

		recn = recvmsg(sockfd, &m_recHeader, 0);
		m_receivingIp = parseReceivingIpAddress();
		m_receivingMac = matchReceivingMacAddress(m_receivingIp);
		TRC_DEBUG("Received UDP datagram at IP" << m_receivingIp << ", MAC " << m_receivingMac);
		/*if (recn > 0) {
			if (m_receiveFromFunc) {
				std::basic_string<unsigned char> message(m_dataBuff, recn);
				if (0 == m_receiveFromFunc(message)) {
					m_sender.sin_addr.s_addr = m_listener.sin_addr.s_addr;    // Change the destination to the address of the last received packet
				}
			}
			else {
				TRC_WARNING("Unregistered receiveFrom() handler");
			}
		}*/
		}
	}
	catch (UdpChannelException& e) {
		CATCH_EXC_TRC_WAR(UdpChannelException, e, "listening thread finished");
		m_runListenThread = false;
	}
	m_isListening = false;
	TRC_FUNCTION_LEAVE("thread stopped");
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

		res = ioctl(sockfd, SIOCGIFCONF, (char *)&ifc);
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
			res = ioctl(sockfd, SIOCGIFHWADDR, &ifrs[i]);
			if (res != SOCKET_ERROR) {
				memcpy(macBytes, ifrs[i].ifr_hwaddr.sa_data, sizeof(macBytes));
				mac = convertToMacString(macBytes);
			}
			break;
		}
	}
	return mac;
}

std::string UdpChannel::convertToMacString(const uint8_t *macBytes) {
	char buffer[18];
	std::sprintf(buffer, "%02X-%02X-%02X-%02X-%02X-%02X", macBytes[0], macBytes[1], macBytes[2], macBytes[3], macBytes[4], macBytes[5]);
	return std::string(buffer);
}

void UdpChannel::sendTo(const std::basic_string<unsigned char>& message) {
	int trmn = sendto(sockfd, (const char*)message.data(), static_cast<int>(message.size()), 0, (struct sockaddr *)&m_sender, sizeof(m_sender));
	if (trmn < 0) {
		THROW_EXC_TRC_WAR(UdpChannelException, "sendto failed: " << WSAGetLastError());
	}
}

void UdpChannel::registerReceiveFromHandler(ReceiveFromFunc receiveFromFunc) {
	m_receiveFromFunc = receiveFromFunc;
}

void UdpChannel::unregisterReceiveFromHandler() {
	m_receiveFromFunc = ReceiveFromFunc();
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
