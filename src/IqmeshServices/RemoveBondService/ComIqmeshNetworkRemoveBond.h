/**
 * Copyright 2015-2025 IQRF Tech s.r.o.
 * Copyright 2019-2025 MICRORISC s.r.o.
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
#include <vector>

namespace iqrf {

  // RemoveBond input paramaters
  typedef struct
  {
    uint8_t deviceAddr = 0;
    uint16_t hwpId = HWPID_DoNotCheck;
    bool wholeNetwork = false;
    int repeat = 1;
    std::basic_string<uint8_t> deviceAddrList;
    bool clearAllBonds = false;
  } TRemoveBondInputParams;

  class ComIqmeshNetworkRemoveBond : public ComBase
  {
  public:
    ComIqmeshNetworkRemoveBond() = delete;
    explicit ComIqmeshNetworkRemoveBond(rapidjson::Document& doc) : ComBase(doc)
    {
      parse(doc);
    }

    virtual ~ComIqmeshNetworkRemoveBond()
    {
    }

    const TRemoveBondInputParams getRomveBondParams() const
    {
      return m_removeBondInputParams;
    }

  protected:
    void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) override
    {
      rapidjson::Pointer("/data/rsp/response").Set(doc, HexStringConversion::encodeBinary(res.getResponse().DpaPacket().Buffer, res.getResponse().GetLength()));
    }

  private:
    TRemoveBondInputParams m_removeBondInputParams;

    // parses document into data fields
    void parse(rapidjson::Document& doc)
    {
      rapidjson::Value* jsonValue;

      // deviceAddress
      if ((jsonValue = rapidjson::Pointer("/data/req/deviceAddr").Get(doc)))
      {
        m_removeBondInputParams.deviceAddrList.clear();

        if (jsonValue->IsInt())
        {
          uint32_t addr = jsonValue->GetInt();
          m_removeBondInputParams.deviceAddr = (uint8_t)addr;
        }

        if (jsonValue->IsArray())
        {
          for (auto itr = jsonValue->Begin(); itr != jsonValue->End(); ++itr)
          {
            if (itr->IsInt())
            {
              uint8_t addr = (uint8_t)itr->GetInt();
              m_removeBondInputParams.deviceAddrList.push_back(addr);
            }
          }
        }
      }

      // hwpId
      if ((jsonValue = rapidjson::Pointer("/data/req/hwpId").Get(doc)))
        m_removeBondInputParams.hwpId = (uint16_t)jsonValue->GetInt();

      // wholeNetwork
      if ((jsonValue = rapidjson::Pointer("/data/req/wholeNetwork").Get(doc)))
        m_removeBondInputParams.wholeNetwork = jsonValue->GetBool();

      // clearAllBonds
      if ((jsonValue = rapidjson::Pointer("/data/req/clearAllBonds").Get(doc)))
        m_removeBondInputParams.clearAllBonds = jsonValue->GetBool();

      // repeat
      if (rapidjson::Value* repeatJsonVal = rapidjson::Pointer("/data/repeat").Get(doc))
        m_removeBondInputParams.repeat = repeatJsonVal->GetInt();
    }
  };
}
