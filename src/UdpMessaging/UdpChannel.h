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

#pragma once

#include "ShapeDefines.h"

#ifdef SHAPE_PLATFORM_WINDOWS

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <stdint.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Iphlpapi.h>
typedef int clientlen_t;
#else
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
typedef int SOCKET;
typedef void * SOCKADDR_STORAGE;
typedef size_t clientlen_t;
#endif

#include "IChannel.h"
#include "NetworkInterface.h"
#include "UdpChannelException.h"
#include <stdint.h>

#include <atomic>
#include <ctime>
#include <map>
#include <stdexcept>
#include <thread>
#include <vector>

/// UDP channel class
class UdpChannel : public IChannel {
public:
	/**
	 * UDP channel constructor
	 * @param remotePort Port to send to
	 * @param localPort Port to listen on
	 * @param dataBuffSize Data buffer size
	 */
	UdpChannel(unsigned short remotePort, unsigned short localPort, unsigned int expiration, unsigned dataBuffSize);
	
	/**
	 * Destructor
	 */
	virtual ~UdpChannel();

	/**
	 * Sends message to client
	 * @param message Message to send
	 */
	void sendTo(const std::basic_string<unsigned char>& message) override;
	
	/**
	 * Sets handler for received messages
	 * @param messageHandler Function to pass received message to
	 */
	void registerReceiveFromHandler(ReceiveFromFunc messageHandler) override;

	/**
	 * Clears handler for received messages
	 */
	void unregisterReceiveFromHandler() override;

	/**
	 * Returns channel state
	 * @return channel state
	 */
	State getState() override;

	/**
	 * Returns IP address of receiving interface
	 * @return IP address
	 */
	const std::string& getListeningIpAddress() const;

	/**
	 * Returns MAC address of receiving interface
	 * @return MAC address
	 */
	const std::string& getListeningMacAddress() const;

	/**
	 * Returns listening port
	 * @return Listening port
	 */
	unsigned short getListeningIpPort() const;

	/**
	 * Checks if thread is listening
	 * @return true if thread is listening, false otherwise
	 */
	bool isListening() const;

private:
	/**
	 * Base constructor
	 */
	UdpChannel();

	/**
	 * UDP listening loop
	 */
	void listen();

	/**
	 * Attempts to identify interface that received message
	 */
	void identifyReceivingInterface();

	/**
	 * Checks if the interface has highest priority
	 * @param idx Index of interface
	 */
	bool isPriorityInterface(const int &idx);

	/**
	 * Finds interface at specified index and stores information in interface map
	 * @param idx Index of interface
	 */
	void findInterfaceByIndex(const int &idx);

	/**
	 * Parses and retrieves interface metric from IP route
	 * @param ip Interface IP address
	 * @return Interface metric
	 */
	int getInterfaceMetric(const std::string &ip);

	/**
	 * Splits string by delimiter and returns vector of substrings
	 * @param string String to be split
	 * @param delimiter Characters to split by
	 * @return String split by delimiter in vector
	 */
	std::vector<std::string> split(const std::string &string, const std::string &delimiter);

	/**
	 * Converts bytes to MAC address string
	 * @param macBytes MAC byte array
	 * @return MAC address string
	 */
	std::string convertToMacString(const uint8_t *macBytes);

	/// Handler function for received messages
	ReceiveFromFunc m_messageHandler;
	/// UDP listening thread
	std::thread m_listenThread;
	/// Indicates if thread is listening
	std::atomic_bool m_isListening;
	/// Indicates if thread continue listening
	bool m_runListenThread;
	/// Socket file descriptor
	SOCKET m_sockfd = -1;
	/// Listening transport structure
	sockaddr_in m_listener;
	/// Sending transport structure
	sockaddr_in m_sender;
	/// Port to send to
	unsigned short m_remotePort;
	/// Port to listen on
	unsigned short m_localPort;
#ifndef SHAPE_PLATFORM_WINDOWS
	/// Message header structure for received messages
	msghdr m_recHeader;
	/// Control message header structure for received messages
	cmsghdr *m_cmsg;
	/// Transport structure for receiving interface
	sockaddr_in m_addr;
	/// IO structure
	iovec m_recIov[1];
	/// Control meta buffer
	char *m_controlBuff;
	/// Control meta buffer size
	unsigned m_controlBuffSize = 0x100;
#endif
	/// Data buffer
	char *m_dataBuff;
	/// Data buffer size
	unsigned m_dataBuffSize;
	/// IP address of receiving interface
	std::string m_receivingIp;
	/// MAC address of receiving interface
	std::string m_receivingMac;
	/// Metric of interface route
	int32_t m_receivingMetric;
	/// Network interface map
	std::map<unsigned int, NetworkInterface> m_interfaces;
	/// Interface expiration period
	unsigned int m_expirationPeriod;
};
