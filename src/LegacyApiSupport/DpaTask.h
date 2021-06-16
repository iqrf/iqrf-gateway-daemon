/**
 * Copyright 2015-2021 IQRF Tech s.r.o.
 * Copyright 2019-2021 MICRORISC s.r.o.
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
#include <sstream>
#include <memory>
#include <chrono>

#include "DpaMessage.h"

class DpaTask
{
public:
  DpaTask() = delete;
  DpaTask(const std::string& prfName, uint8_t prfNum);
  DpaTask(const std::string& prfName, uint8_t prfNum, uint16_t address, uint8_t command);
  virtual ~DpaTask();

  void handleConfirmation(const DpaMessage& confirmation);
  void handleResponse(const DpaMessage& response);

  virtual void parseResponse(const DpaMessage& response) = 0;
  virtual std::string encodeResponse(const std::string& errStr) { 
    (void)errStr; //silence -Wunused-parameter
    return std::string();
  }
  virtual std::string encodeRequest() const { return std::string(); }

  virtual void parseCommand(const std::string& command) = 0;
  virtual const std::string& encodeCommand() const = 0;
  
  const std::string& getPrfName() const { return m_prfName; }
  const std::string& getClid() const { return m_clid; }
  void setClid(const std::string& clid) { m_clid = clid; }

  uint16_t getAddress() const;
  void setAddress(uint16_t address);
  uint16_t getHwpid() const;
  void setHwpid(uint16_t hwpid);
  uint8_t getPcmd() const;
  void setPcmd(uint8_t command);
  int getTimeout() const { return m_timeout; }
  void setTimeout(int timeout) { m_timeout = timeout; }

  const DpaMessage& getRequest() const { return m_request;  }
  const DpaMessage& getConfirmation() const { return m_confirmation; }
  const DpaMessage& getResponse() const { return m_response; }

  const std::chrono::time_point<std::chrono::system_clock>& getRequestTs() const { return m_request_ts; }
  const std::chrono::time_point<std::chrono::system_clock>& getConfirmationTs() const { return m_confirmation_ts; }
  const std::chrono::time_point<std::chrono::system_clock>& getResponseTs() const { return m_response_ts; }
  void timestampRequest(const std::chrono::time_point<std::chrono::system_clock>& ts = std::chrono::system_clock::now());
  void timestampConfirmation(const std::chrono::time_point<std::chrono::system_clock>& ts = std::chrono::system_clock::now());
  void timestampResponse(const std::chrono::time_point<std::chrono::system_clock>& ts = std::chrono::system_clock::now());

protected:
  DpaMessage m_request;
  DpaMessage m_response;

private:
  DpaMessage m_confirmation;
  std::string m_prfName;
  std::string m_clid;
  int m_timeout = -1;

  std::chrono::time_point<std::chrono::system_clock> m_request_ts;
  std::chrono::time_point<std::chrono::system_clock> m_confirmation_ts;
  std::chrono::time_point<std::chrono::system_clock> m_response_ts;
};
