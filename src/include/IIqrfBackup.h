/**
 * Copyright 2015-2023 IQRF Tech s.r.o.
 * Copyright 2019-2023 MICRORISC s.r.o.
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
#include <map>
#include <vector>
#include <list>
#include <cmath>
#include <thread>
#include <bitset>
#include <chrono>
#include "IDpaTransactionResult2.h"

#ifdef IIqrfBackup_EXPORTS
#define IIqrfBackup_DECLSPEC SHAPE_ABI_EXPORT
#else
#define IIqrfBackup_DECLSPEC SHAPE_ABI_IMPORT
#endif

namespace iqrf {

  // Result of backup algorithm
  class DeviceBackupData
  {
  private:
    uint16_t m_address;
    bool m_online;
    uint32_t m_mid;
    uint16_t m_dpaVersion;
    std::basic_string<uint8_t> m_data;

  public:
    DeviceBackupData()
      : m_address(0),
      m_online(false),
      m_mid(0),
      m_dpaVersion(0)
    {
    }

    DeviceBackupData(const uint16_t address, const bool online, const uint32_t mid, const uint16_t dpaVersion, const std::basic_string<uint8_t> &data)
      : m_address(address),
      m_online(online),
      m_mid(mid),
      m_dpaVersion(dpaVersion),
      m_data(data)
    {
    }

    DeviceBackupData(const uint16_t address)
      : m_address(address),
      m_online(false),
      m_mid(0),
      m_dpaVersion(0)
    {
    }

    uint16_t getAddress()
    {
      return(m_address);
    }

    bool getOnlineStatus() const { return m_online; }
    uint32_t getMid() const { return m_mid; }
    uint16_t getDpaVersion() const { return m_dpaVersion; }
    std::basic_string<uint8_t> getBackupData() const { return m_data; }

    void setAddress(const uint16_t address)
    {
      m_address = address;
    }

    void setOnlineStatus(const bool online)
    {
      m_online = online;
    }

    void setMid(const uint32_t mid)
    {
      m_mid = mid;
    }

    void setDpaVersion(uint16_t dpaVersion)
    {
      m_dpaVersion = dpaVersion;
    }

    void setBackupData(const std::basic_string<uint8_t> &data)
    {
      m_data = data;
    }
  };

  class IIqrfBackup_DECLSPEC IIqrfBackup
  {
  public:
    virtual ~IIqrfBackup() {}
    virtual void backup(const uint16_t address, DeviceBackupData& backupData) = 0;
    virtual void getTransResults(std::list<std::unique_ptr<IDpaTransactionResult2>>& transResult) = 0;
    virtual std::basic_string<uint16_t> getBondedNodes(void) = 0;
    virtual int getErrorCode(void) = 0;
  };
}
