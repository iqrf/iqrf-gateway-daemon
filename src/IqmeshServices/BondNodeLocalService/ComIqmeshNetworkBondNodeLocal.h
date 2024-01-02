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

  // SmartConnect input parameters
  typedef struct TBondNodeInputParams
  {
    TBondNodeInputParams()
    {
      bondingMask = 255;
      bondingTestRetries = 1;
      repeat = 1;
    }
    uint16_t deviceAddress;
    int bondingMask;
    int bondingTestRetries;
    int repeat;
  }TBondNodeInputParams;

  class ComIqmeshNetworkBondNodeLocal : public ComBase
  {
  public:
    ComIqmeshNetworkBondNodeLocal() = delete;
    explicit ComIqmeshNetworkBondNodeLocal(rapidjson::Document& doc)
      :ComBase(doc)
    {
      parse(doc);
    }

    virtual ~ComIqmeshNetworkBondNodeLocal()
    {
    }

    const TBondNodeInputParams getBondNodeInputParams() const
    {
      return m_bondNodeInputParams;
    }

  protected:
    void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) override
    {
      rapidjson::Pointer("/data/rsp/response").Set(doc, encodeBinary(res.getResponse().DpaPacket().Buffer, res.getResponse().GetLength()));
    }

  private:
    TBondNodeInputParams m_bondNodeInputParams;

    // parses document into data fields
    void parse(rapidjson::Document& doc)
    {
      rapidjson::Value* jsonVal;

      // Repeat
      if ((jsonVal = rapidjson::Pointer("/data/repeat").Get(doc))) {
        m_bondNodeInputParams.repeat = jsonVal->GetInt();
      }

      // Device address
      if ((jsonVal = rapidjson::Pointer("/data/req/deviceAddr").Get(doc))) {
        m_bondNodeInputParams.deviceAddress = (uint16_t)jsonVal->GetInt();
      }

      // bondingMak
      if ((jsonVal = rapidjson::Pointer("/data/req/bondingMak").Get(doc))) {
        m_bondNodeInputParams.bondingMask = jsonVal->GetInt();
      }

      // bondingTestRetries
      if ((jsonVal = rapidjson::Pointer("/data/req/bondingTestRetries").Get(doc))) {
        m_bondNodeInputParams.bondingTestRetries = jsonVal->GetInt();
      }
    }
  };
}
