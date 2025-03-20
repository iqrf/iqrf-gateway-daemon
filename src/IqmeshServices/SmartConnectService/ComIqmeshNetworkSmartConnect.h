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
#include "Trace.h"
#include <list>

namespace iqrf {

  // SmartConnect input parameters
  typedef struct TSmartConnectInputParams
  {
    TSmartConnectInputParams()
    {
      userData.clear();
      MID.clear();
      IBK.clear();
      bondingRetries = 1;
      repeat = 1;
    }
    uint16_t deviceAddress;
    std::string smartConnectCode;
    int bondingRetries;
    std::basic_string<uint8_t> userData;
    std::basic_string<uint8_t> MID;
    std::basic_string<uint8_t> IBK;
    uint16_t hwpId;
    int repeat;
  }TSmartConnectInputParams;

  class ComIqmeshNetworkSmartConnect : public ComBase
  {
  public:
    ComIqmeshNetworkSmartConnect() = delete;
    explicit ComIqmeshNetworkSmartConnect(rapidjson::Document& doc)
      :ComBase(doc)
    {
      parse(doc);
    }

    virtual ~ComIqmeshNetworkSmartConnect()
    {
    }

    const TSmartConnectInputParams getSmartConnectInputParams() const
    {
      return m_smartConnectInputParams;
    }

  protected:
    void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) override
    {
      rapidjson::Pointer("/data/rsp/response")
        .Set(doc, HexStringConversion::encodeBinary(res.getResponse().DpaPacket().Buffer, res.getResponse().GetLength()));
    }


  private:
    TSmartConnectInputParams m_smartConnectInputParams;

    // parses document into data fields
    void parse(rapidjson::Document& doc)
    {
      rapidjson::Value* jsonVal;

      // Repeat
      if ((jsonVal = rapidjson::Pointer("/data/repeat").Get(doc))) {
        m_smartConnectInputParams.repeat = jsonVal->GetInt();
      }

      // Device address
      if ((jsonVal = rapidjson::Pointer("/data/req/deviceAddr").Get(doc))) {
        m_smartConnectInputParams.deviceAddress = (uint16_t)jsonVal->GetInt();
      }

      // smartConnectCode
      if ((jsonVal = rapidjson::Pointer("/data/req/smartConnectCode").Get(doc))) {
        m_smartConnectInputParams.smartConnectCode = jsonVal->GetString();
      }

      // bondingTestRetries
      if ((jsonVal = rapidjson::Pointer("/data/req/bondingTestRetries").Get(doc))) {
        m_smartConnectInputParams.bondingRetries = jsonVal->GetInt();
      }

      // userData
      if ((jsonVal = rapidjson::Pointer("/data/req/userData").Get(doc))) {
        for (rapidjson::SizeType i = 0; i < jsonVal->Size(); i++)
          m_smartConnectInputParams.userData.push_back((uint8_t)(*jsonVal)[i].GetInt());
      }
    }
  };
}
