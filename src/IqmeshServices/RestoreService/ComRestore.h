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

  // Restore input paramaters
  typedef struct
  {
    uint16_t deviceAddress;
    std::string data;
    bool restartCoodinator;
  } TRestoreInputParams;

  class ComRestore : public ComBase
  {
  public:
    ComRestore() = delete;
    ComRestore(rapidjson::Document& doc) : ComBase(doc)
    {
      parse(doc);
    }

    virtual ~ComRestore()
    {
    }

    const TRestoreInputParams getRestoreParams() const
    {
      return m_restoreParams;
    }

  protected:
    void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) override
    {
      rapidjson::Pointer("/data/rsp/response").Set(doc, HexStringConversion::encodeBinary( res.getResponse().DpaPacket().Buffer, res.getResponse().GetLength()));
    }

  private:
    TRestoreInputParams m_restoreParams;

    // Parses document into data fields
    void parse(rapidjson::Document& doc)
    {
      rapidjson::Value* jsonValue;

      // deviceAddress
      m_restoreParams.deviceAddress = COORDINATOR_ADDRESS;
      if ((jsonValue = rapidjson::Pointer("/data/req/deviceAddr").Get(doc)))
      {
        uint32_t addr = jsonValue->GetInt();
        if (addr < MAX_ADDRESS)
          m_restoreParams.deviceAddress = (uint16_t)addr;
      }

      // data
      if ((jsonValue = rapidjson::Pointer("/data/req/data").Get(doc)))
      {
        m_restoreParams.data = jsonValue->GetString();
      }

      // restartCoordinator
      if ((jsonValue = rapidjson::Pointer("/data/req/restartCoordinator").Get(doc)))
        m_restoreParams.restartCoodinator = jsonValue->GetBool();
      else
        m_restoreParams.restartCoodinator = false;
    }
  };
}
