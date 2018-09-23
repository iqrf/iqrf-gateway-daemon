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

#pragma once

#include "Prf.h"
#include <chrono>

namespace iqrf {

  class PrfEnum : public Prf
  {
  public:
    enum class Cmd {
      INFO = CMD_GET_PER_INFO
    };

    PrfEnum(uint16_t address = 0);

    ~PrfEnum() override;

    // parse response data
    void parseResponse(const DpaMessage& response) override;
    bool parseCoordinatorResetAsyncResponse(const DpaMessage& response);

    // periphery commands
    void setEnumerationCmd();

    Cmd getCmd() const;

    uint16_t getDpaVerWord() const { return m_dpaVerWord; }
    const std::string& getDpaVerWordAsStr() const { return m_dpaVerWordAsStr; }
    const std::string& getDpaVer() const { return m_dpaVer; }
    int getDpaVerMajor() const { return m_dpaVerMajor; }
    int getDpaVerMinor() const { return m_dpaVerMinor; }
    bool getDemoFlag() const { return m_demo; }
    bool getStdModeSupportFlag() const { return m_stdModeSupport; }
    bool getLpModeSupportFlag() const { return m_lpModeSupport; }

  private:
    void setCmd(Cmd cmd);
    Cmd m_cmd = Cmd::INFO;

    TEnumPeripheralsAnswer m_resp = { 0 };

    uint16_t m_dpaVerWord;
    std::string m_dpaVerWordAsStr;
    std::string m_dpaVer;
    int m_dpaVerMajor = 0;
    int m_dpaVerMinor = 0;
    bool m_demo = false;
    bool m_stdModeSupport = false;
    bool m_lpModeSupport = false;

  };
}
