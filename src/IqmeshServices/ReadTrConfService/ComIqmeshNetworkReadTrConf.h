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
#include "JsonUtils.h"

namespace iqrf {

  // ReadTrConf input paramaters
  typedef struct
  {
    uint16_t deviceAddress = 0;
    uint16_t hwpId = HWPID_DoNotCheck;
    int repeat = 1;
  }TReadTrConfInputParams;

  class ComIqmeshNetworkReadTrConf : public ComBase
  {
  public:
    ComIqmeshNetworkReadTrConf() = delete;
    explicit ComIqmeshNetworkReadTrConf(rapidjson::Document& doc)
      :ComBase(doc)
    {
      parse(doc);
    }


    virtual ~ComIqmeshNetworkReadTrConf()
    {
    }

    const TReadTrConfInputParams getReadTrConfParams() const
    {
      return m_readTrConfParams;
    }

  protected:
    void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) override
    {
      rapidjson::Pointer("/data/rsp/response")
        .Set(doc, HexStringConversion::encodeBinary(res.getResponse().DpaPacket().Buffer, res.getResponse().GetLength()));
    }


  private:
    TReadTrConfInputParams m_readTrConfParams;

    // parses document into data fields
    void parse(rapidjson::Document& doc)
    {
      rapidjson::Value* jsonVal;

      // Repeat
      if ((jsonVal = rapidjson::Pointer("/data/repeat").Get(doc)))
        m_readTrConfParams.repeat = jsonVal->GetInt();

      // Device address
      if ((jsonVal = rapidjson::Pointer("/data/req/deviceAddr").Get(doc)))
        m_readTrConfParams.deviceAddress = (uint16_t)jsonVal->GetInt();

      // HWPID
      if ((jsonVal = rapidjson::Pointer("/data/req/hwpId").Get(doc)))
        m_readTrConfParams.hwpId = (uint16_t)jsonVal->GetInt();
    }
  };
}
