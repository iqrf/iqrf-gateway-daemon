/**
 * IQRF Gateway Daemon
 * Copyright (C) 2015-2025 IQRF Tech s.r.o., 2019-2025 MICRORISC s.r.o.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include "ShapeDefines.h"

#include "IChannel.h"
#include <string>
#include <exception>
#include <thread>
#include <mutex>
#include <atomic>
#include <stdexcept>

#ifdef SHAPE_PLATFORM_WINDOWS
#include <windows.h>
typedef HANDLE MQDESCR;
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
typedef mqd_t MQDESCR;
#endif

typedef std::basic_string<unsigned char> ustring;

class MqChannel: public IChannel
{
public:
  MqChannel(const std::string& remoteMqName, const std::string& localMqName, const uint8_t &timeout, unsigned bufsize, bool server = false);
  virtual ~MqChannel();

  void sendTo(const std::basic_string<unsigned char>& message) override;
  void registerReceiveFromHandler(ReceiveFromFunc receiveFromFunc) override;
  void unregisterReceiveFromHandler() override;
  State getState() override;

private:
  MqChannel();
  ReceiveFromFunc m_receiveFromFunc;

  std::atomic_bool m_connected;
  bool m_runListenThread;
  std::thread m_listenThread;
  void listen();
  void connect();
  std::mutex m_connectMtx;

  MQDESCR m_localMqHandle;
  MQDESCR m_remoteMqHandle;
  std::string m_localMqName;
  std::string m_remoteMqName;
  uint8_t m_timeout;

  unsigned char* m_rx;
  unsigned m_bufsize;
  bool m_server;
  State m_state = State::NotReady;

};

class MqChannelException : public std::logic_error
{
public:
  MqChannelException(const std::string& cause) : logic_error(cause) {}
  MqChannelException(const char* cause) : logic_error(cause) {}
};
