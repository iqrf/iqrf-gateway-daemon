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

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <netpacket/packet.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
typedef int SOCKET;
typedef void * SOCKADDR_STORAGE;
typedef size_t clientlen_t;

#endif

#include "IChannel.h"
#include <stdint.h>
#include <exception>
#include <thread>
#include <vector>
#include <atomic>
#include <map>
#include <stdexcept>

class UdpChannel : public IChannel
{
public:
  UdpChannel(unsigned short remotePort, unsigned short localPort, unsigned bufsize);
  virtual ~UdpChannel();
  void sendTo(const std::basic_string<unsigned char>& message) override;
  void registerReceiveFromHandler(ReceiveFromFunc receiveFromFunc) override;
  void unregisterReceiveFromHandler() override;
  State getState() override;

  const std::string& getListeningIpAddress() const { return m_myIpAdress; }
  unsigned short getListeningIpPort() const { return m_localPort; }
  const std::string& getListeningMacAddress() const { return m_myMacAdress; }
  bool isListening() const { return m_isListening; }

private:
  class MyAdapter {
  public:
    MyAdapter() = delete;
    MyAdapter(const std::string& ip, const std::string& mac)
      :mIpAddr(ip)
      , mMac(mac)
    {}
    std::string mIpAddr;
    std::string mMac;
  };

  UdpChannel();
  ReceiveFromFunc m_receiveFromFunc;

  std::atomic_bool m_isListening;
  bool m_runListenThread;
  std::thread m_listenThread;
  void listen();
  void getMyAddress();
  void getMyMacAddress(SOCKET soc);

  SOCKET m_iqrfUdpSocket = -1;
  sockaddr_in m_iqrfUdpListener;
  sockaddr_in m_iqrfUdpTalker;

  unsigned short m_remotePort;
  unsigned short m_localPort;

  unsigned char* m_rx;
  unsigned m_bufsize;

  std::string m_myIpAdress;
  std::string m_myMacAdress;
  std::map<std::string, MyAdapter> m_adapters;
};

class UdpChannelException : public std::logic_error
{
public:
  UdpChannelException(const std::string& cause) : logic_error(cause) {}
  UdpChannelException(const char* cause) : logic_error(cause) {}
};
