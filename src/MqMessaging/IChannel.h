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

#include <string>
#include <functional>

class IChannel
{
public:
  enum class State
  {
    Ready,
    NotReady
  };

  // receive data handler
  typedef std::function<int(const std::basic_string<unsigned char>&)> ReceiveFromFunc;

  //dtor
  virtual ~IChannel() {};

  /**
  Sends a request.

  @param [in]	      message	Data to be sent.

  @return	Result of the data send operation. 0 - Data was sent successfully, negative value means some error
  occurred.
  */
  virtual void sendTo(const std::basic_string<unsigned char>& message) = 0;

  /**
  Registers the receive data handler, a functional that is called when a message is received.

  @param [in]	receiveFromFunc	The functional.
  */
  virtual void registerReceiveFromHandler(ReceiveFromFunc receiveFromFunc) = 0;

  /**
  Unregisters data handler. The handler remains empty. All icoming data are silently discarded
  */
  virtual void unregisterReceiveFromHandler() = 0;

  virtual State getState() = 0;
};
