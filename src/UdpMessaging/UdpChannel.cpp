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
  m_bufsize(bufsize)
{
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

  //try to get local IP by trm and rec to itself
  getMyAddress();
  TRC_INFORMATION("UDP listening on: " <<
    NAME_PAR(IP, m_myIpAdress) << NAME_PAR(port, localPort) << NAME_PAR(MAC, m_myMacAdress));

  //iqrfUdpSocket = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
  m_iqrfUdpSocket = socket(AF_INET, SOCK_DGRAM, 0);
  if (m_iqrfUdpSocket == -1)
    THROW_EXC_TRC_WAR(UdpChannelException, "socket failed: " << GetLastError());

  opttype broadcastEnable = 1;                                // Enable sending broadcast packets
  if (0 != setsockopt(m_iqrfUdpSocket, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)))
  {
    closesocket(m_iqrfUdpSocket);
    THROW_EXC_TRC_WAR(UdpChannelException, "setsockopt failed: " << GetLastError());
  }

  // Remote server, packets are send as a broadcast until the first packet is received
  memset(&m_iqrfUdpTalker, 0, sizeof(m_iqrfUdpTalker));
  m_iqrfUdpTalker.sin_family = AF_INET;
  m_iqrfUdpTalker.sin_port = htons(m_remotePort);
  m_iqrfUdpTalker.sin_addr.s_addr = htonl(INADDR_BROADCAST);

  // Local server, packets are received from any IP
  memset(&m_iqrfUdpListener, 0, sizeof(m_iqrfUdpListener));
  m_iqrfUdpListener.sin_family = AF_INET;
  m_iqrfUdpListener.sin_port = htons(m_localPort);
  m_iqrfUdpListener.sin_addr.s_addr = htonl(INADDR_ANY);

  if (SOCKET_ERROR == bind(m_iqrfUdpSocket, (struct sockaddr *)&m_iqrfUdpListener, sizeof(m_iqrfUdpListener)))
  {
    closesocket(m_iqrfUdpSocket);
    THROW_EXC_TRC_WAR(UdpChannelException, "bind failed: " << GetLastError());
  }

  m_rx = shape_new unsigned char[m_bufsize];
  memset(m_rx, 0, m_bufsize);

  m_listenThread = std::thread(&UdpChannel::listen, this);
  TRC_FUNCTION_LEAVE("");
}

UdpChannel::~UdpChannel()
{
  shutdown(m_iqrfUdpSocket, SHUT_RD);
  closesocket(m_iqrfUdpSocket);

  TRC_DEBUG("joining udp listening thread");
  if (m_listenThread.joinable())
    m_listenThread.join();
  TRC_DEBUG("listening thread joined");

#ifdef SHAPE_PLATFORM_WINDOWS
  WSACleanup();
#endif

  delete[] m_rx;
}

void UdpChannel::listen()
{
  TRC_FUNCTION_ENTER("thread starts");

  int recn = -1;
  socklen_t iqrfUdpListenerLength = sizeof(m_iqrfUdpListener);

  try {
    m_isListening = true;
    while (m_runListenThread)
    {
      recn = recvfrom(m_iqrfUdpSocket, (char*)m_rx, m_bufsize, 0, (struct sockaddr *)&m_iqrfUdpListener, &iqrfUdpListenerLength);

      if (recn == SOCKET_ERROR) {
        THROW_EXC_TRC_WAR(UdpChannelException, "recvfrom returned: " << WSAGetLastError());
      }

      if (recn > 0) {
        if (m_receiveFromFunc) {
          std::basic_string<unsigned char> message(m_rx, recn);
          if (0 == m_receiveFromFunc(message)) {
            m_iqrfUdpTalker.sin_addr.s_addr = m_iqrfUdpListener.sin_addr.s_addr;    // Change the destination to the address of the last received packet
          }
        }
        else {
          TRC_WARNING("Unregistered receiveFrom() handler");
        }
      }
    }
  }
  catch (UdpChannelException& e) {
    CATCH_EXC_TRC_WAR(UdpChannelException, e, "listening thread finished");
    m_runListenThread = false;
  }
  m_isListening = false;
  TRC_FUNCTION_LEAVE("thread stopped");
}

void UdpChannel::sendTo(const std::basic_string<unsigned char>& message)
{
  //TRC_DEBUG("Send to UDP: " << std::endl << FORM_HEX(message.data(), message.size()));

  int trmn = sendto(m_iqrfUdpSocket, (const char*)message.data(), static_cast<int>(message.size()), 0, (struct sockaddr *)&m_iqrfUdpTalker, sizeof(m_iqrfUdpTalker));

  if (trmn < 0) {
    THROW_EXC_TRC_WAR(UdpChannelException, "sendto failed: " << WSAGetLastError());
  }

}

void UdpChannel::registerReceiveFromHandler(ReceiveFromFunc receiveFromFunc)
{
  m_receiveFromFunc = receiveFromFunc;
}

void UdpChannel::unregisterReceiveFromHandler()
{
  m_receiveFromFunc = ReceiveFromFunc();
}

void UdpChannel::getMyAddress()
{
  TRC_FUNCTION_ENTER("");

  m_myIpAdress = "0.0.0.0";
  m_myMacAdress = "00-00-00-00-00-00";

  SOCKET sockfd = socket(PF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    THROW_EXC_TRC_WAR(UdpChannelException, "socket failed: " << GetLastError() << ": " << strerror(errno));
  }

  sockaddr_in hints;
  memset(&hints, 0, sizeof(hints));
  hints.sin_family = AF_INET;
  hints.sin_port = htons(9);
  hints.sin_addr.s_addr = INADDR_LOOPBACK;

  int res;

  res = connect(sockfd, (sockaddr *)&hints, sizeof(hints));
  if (res < 0) {
    closesocket(sockfd);
    THROW_EXC_TRC_WAR(UdpChannelException, "connect failed: " << GetLastError() << ": " << strerror(errno));
  }

  socklen_t hintsLen = sizeof(hints);
  res = getsockname(sockfd, (sockaddr *)&hints, &hintsLen);
  if (res < 0) {
    closesocket(sockfd);
    THROW_EXC_TRC_WAR(UdpChannelException, "getsockname failed: " << GetLastError() << ": " << strerror(errno));
  }

  closesocket(sockfd);

  char address[INET_ADDRSTRLEN];
  if (inet_ntop(AF_INET, &hints.sin_addr, address, INET_ADDRSTRLEN) == NULL) {
    THROW_EXC_TRC_WAR(UdpChannelException, "inet_ntop failed: " << GetLastError() << ": " << strerror(errno));
  }
  m_myIpAdress = std::string(address);

  sockfd = socket(PF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    THROW_EXC_TRC_WAR(UdpChannelException, "socket failed: " << GetLastError() << ": " << strerror(errno));
  }

  memset(&hints, 0, sizeof(hints));
  hints.sin_family = AF_INET;
  hints.sin_port = htons(m_localPort);
  hints.sin_addr.s_addr = htonl(INADDR_ANY);

  opttype broadcastEnable = 1;
  res = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));
  if (res < 0) {
    closesocket(sockfd);
    THROW_EXC_TRC_WAR(UdpChannelException, "setsockopt failed: " << GetLastError() << ": " << strerror(errno));
  }

  res = bind(sockfd, (struct sockaddr *)&hints, sizeof(hints));
  if (res < 0) {
    closesocket(sockfd);
    THROW_EXC_TRC_WAR(UdpChannelException, "bind failed: " << GetLastError() << ": " << strerror(errno));
  }

  TRC_INFORMATION("IP from inet_ntop: " << m_myIpAdress);

  getMyMacAddress(sockfd);

  std::ostringstream os;
  for (auto adapter : m_adapters) {
    os << std::endl << NAME_PAR(ip, adapter.second.mIpAddr) << NAME_PAR(mac, adapter.second.mMac);
  }
  TRC_INFORMATION("Detected network adapters:" << os.str());

  auto found = m_adapters.find(m_myIpAdress);
  if (found != m_adapters.end()) {
    m_myMacAdress = found->second.mMac;
  }

  shutdown(sockfd, SHUT_RD);
  closesocket(sockfd);

  TRC_FUNCTION_LEAVE("");
}

void UdpChannel::getMyMacAddress(SOCKET soc)
{
  TRC_FUNCTION_ENTER("");
#ifdef SHAPE_PLATFORM_WINDOWS

  PIP_ADAPTER_INFO AdapterInfo;
  DWORD dwBufLen = sizeof(IP_ADAPTER_INFO);
  char mac_addr[32];

  AdapterInfo = (PIP_ADAPTER_INFO) shape_new uint8_t[dwBufLen];

  if (GetAdaptersInfo(AdapterInfo, &dwBufLen) == ERROR_BUFFER_OVERFLOW) {
    delete[] AdapterInfo;
    AdapterInfo = (PIP_ADAPTER_INFO) shape_new uint8_t[dwBufLen];
  }

  if (GetAdaptersInfo(AdapterInfo, &dwBufLen) == NO_ERROR) {
    PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo;// Contains pointer to current adapter info
    do {
      sprintf(mac_addr, "%02X-%02X-%02X-%02X-%02X-%02X",
        pAdapterInfo->Address[0], pAdapterInfo->Address[1],
        pAdapterInfo->Address[2], pAdapterInfo->Address[3],
        pAdapterInfo->Address[4], pAdapterInfo->Address[5]);

      std::string mac(mac_addr);
      std::string ip(pAdapterInfo->IpAddressList.IpAddress.String);
      m_adapters.insert(std::make_pair(ip, MyAdapter(ip, mac)));

      pAdapterInfo = pAdapterInfo->Next;
    } while (pAdapterInfo);
  }

  delete[] AdapterInfo;

#else
  struct if_nameindex *if_nidxs, *intf;
  struct ifreq ifr;
  if_nidxs = if_nameindex();
  char mac_addr[32];

  if (if_nidxs != NULL) {
    for (intf = if_nidxs; intf->if_index != 0 || intf->if_name != NULL; intf++) {
      memset(&ifr, 0, sizeof(ifr));
      //printf("%s\t", intf->if_name);

      // Type of address to retrieve - IPv4 IP address
      ifr.ifr_addr.sa_family = AF_INET;
      // Copy the interface name in the ifreq structure
      strncpy(ifr.ifr_name, intf->if_name, IFNAMSIZ - 1);

      // Get MAC address
      if (ioctl(soc, SIOCGIFHWADDR, &ifr) < 0) {
        TRC_WARNING("MAC ioctl failed: " << GetLastError() << ": " << strerror(errno));
        continue;
      }
      unsigned char *mac = (unsigned char *)ifr.ifr_hwaddr.sa_data;
      sprintf(mac_addr, "%02X-%02X-%02X-%02X-%02X-%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
      std::string macStr(mac_addr);

      // Get IPv4 address
      if (ioctl(soc, SIOCGIFADDR, &ifr) < 0) {
        TRC_WARNING("IPv4 ioctl failed: " << GetLastError() << ": " << strerror(errno));
        continue;
      }
      if (((struct sockaddr *)&ifr.ifr_addr)->sa_family == AF_INET) {
        std::string ip(inet_ntoa(((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr));

        m_adapters.insert(std::make_pair(ip, MyAdapter(ip, macStr)));
      }
    }
  }
  if_freenameindex(if_nidxs);

#endif
  TRC_FUNCTION_LEAVE("");
}

IChannel::State UdpChannel::getState()
{
  //TODO
  return State::Ready;
}

