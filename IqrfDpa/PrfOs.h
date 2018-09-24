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

  class PrfOs : public Prf
  {
  public:
    enum class Cmd {
      READ = CMD_OS_READ,
      RESET = CMD_OS_RESET,
      READ_CFG = CMD_OS_READ_CFG,
      RFPGM = CMD_OS_RFPGM,
      SLEEP = CMD_OS_SLEEP,
      BATCH = CMD_OS_BATCH,
      SET_SECURITY = CMD_OS_SET_SECURITY,
      RESTART = CMD_OS_RESTART,
      WRITE_CFG_BYTE = CMD_OS_WRITE_CFG_BYTE,
      LOAD_CODE = CMD_OS_LOAD_CODE,
      WRITE_CFG = CMD_OS_WRITE_CFG
    };

    enum class TimeControl {
      WAKEUP_PB4_NEGATIVE = 1,
      RUN_CALIB = 2,
      LEDG_FLASH = 4,
      WAKEUP_PB4_POSITIVE = 8
    };

    PrfOs(uint16_t address = 0);

    ~PrfOs() override;

    // parse response data
    void parseResponse(const DpaMessage& response) override;

    // periphery commands (not complete implementation)
    void setSleepCmd(const std::chrono::seconds& sec, uint8_t ctrl = 0);
    void setSleepCmd(const std::chrono::milliseconds& milis, uint8_t ctrl = 0);
    void setSleepCmd(); //calibration
    void setResetCmd();
    void setReadCmd();

    const std::string& getModuleId() const { return m_moduleId; }
    const std::string& getOsVersion()  const { return m_osVersion; }
    const std::string& getTrType()  const { return m_trType; }
    const std::string& getMcuType()  const { return m_mcuType; }
    const std::string& getOsBuild()  const { return m_osBuild; }

    Cmd getCmd() const;

  private:
    typedef std::chrono::duration<unsigned long, std::ratio<2097, 1000>> milis2097;
    typedef std::chrono::duration<unsigned long, std::ratio<32768, 1000000>> micros32768;

    void setCmd(Cmd cmd);
    Cmd m_cmd = Cmd::READ;

    uint16_t m_time = 0;
    uint8_t m_timeCtrl = 0;

    std::string m_moduleId;
    std::string m_osVersion;
    uint16_t m_osVersionWord;
    std::string m_trType;
    bool m_fcc = false;
    std::string m_mcuType;
    std::string m_osBuild;
    
  };
}
