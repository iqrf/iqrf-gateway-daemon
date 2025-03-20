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

#include "ComBase.h"

namespace iqrf {

  // BackupService input paramaters
  typedef struct
  {
    uint16_t deviceAddress = 0;
    bool wholeNetwork = false;
  } TBackupInputParams;

  class ComBackup : public ComBase
  {
  public:
    ComBackup() = delete;
    ComBackup(rapidjson::Document& doc) : ComBase(doc)
    {
      parse(doc);
    }

    virtual ~ComBackup()
    {
    }

    const TBackupInputParams getBackupParams() const
    {
      return m_backupParams;
    }

  protected:
    void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) override
    {
      rapidjson::Pointer("/data/rsp/response").Set(doc, HexStringConversion::encodeBinary(res.getResponse().DpaPacket().Buffer, res.getResponse().GetLength()));
    }

  private:
    TBackupInputParams m_backupParams;

    // Parses document into data fields
    void parse(rapidjson::Document& doc)
    {
      rapidjson::Value* jsonValue;

      // deviceAddress
      m_backupParams.deviceAddress = COORDINATOR_ADDRESS;
      if ((jsonValue = rapidjson::Pointer("/data/req/deviceAddr").Get(doc)))
      {
        uint32_t addr = jsonValue->GetInt();
        if (addr < MAX_ADDRESS)
          m_backupParams.deviceAddress = (uint16_t)addr;
      }

      // wholeNetwork
      if ((jsonValue = rapidjson::Pointer("/data/req/wholeNetwork").Get(doc)))
        m_backupParams.wholeNetwork = jsonValue->GetBool();
      else
        m_backupParams.wholeNetwork = false;
    }
  };
}
