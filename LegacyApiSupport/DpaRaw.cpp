/**
 * Copyright 2015-2017 MICRORISC s.r.o.
 * Copyright 2017 IQRF Tech s.r.o.
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

#include "DpaRaw.h"

const std::string DpaRaw::PRF_NAME("raw");
const std::string STR_CMD_UNKNOWN("unknown");

DpaRaw::DpaRaw() : DpaTask(DpaRaw::PRF_NAME, 0) {}

DpaRaw::DpaRaw(const DpaMessage& request) : DpaTask(PRF_NAME, 0) {
  setRequest(request);
}

DpaRaw::~DpaRaw() {}

void DpaRaw::setRequest(const DpaMessage& request)
{
  m_request = request;
}

//from IQRF
void DpaRaw::parseResponse(const DpaMessage& response)
{
  m_response = response;
}

//from Messaging
void DpaRaw::parseCommand(const std::string& command)
{
  (void)command; //silence -Wunused-parameter
}

const std::string& DpaRaw::encodeCommand() const {
  return STR_CMD_UNKNOWN;
}
