/**
 * Copyright 2015-2024 IQRF Tech s.r.o.
 * Copyright 2019-2024 MICRORISC s.r.o.
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
