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
      rapidjson::Pointer("/data/rsp/response").Set(doc, HexStringConversion::encodeBinary(res.getResponse().DpaPacket().Buffer, res.getResponse().GetLength()));
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
