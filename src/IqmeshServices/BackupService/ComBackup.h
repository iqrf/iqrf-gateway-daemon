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
