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

  // OtaUpload input parameters
  typedef struct TOtaUploadInputParams
  {
    TOtaUploadInputParams()
    {
      hwpId = HWPID_DoNotCheck;
      repeat = 1;
      uploadEepromData = false;
      uploadEeepromData = false;
    }
    uint16_t deviceAddress;
    uint16_t hwpId;
    std::string fileName;
    uint16_t repeat;
    uint16_t startMemAddr;
    std::string loadingAction;
    bool uploadEepromData;
    bool uploadEeepromData;
  }TOtaUploadInputParams;

  class ComIqmeshNetworkOtaUpload : public ComBase
  {
  public:
    ComIqmeshNetworkOtaUpload() = delete;
    explicit ComIqmeshNetworkOtaUpload(rapidjson::Document& doc)
      :ComBase(doc)
    {
      parse(doc);
    }

    virtual ~ComIqmeshNetworkOtaUpload()
    {
    }

    const TOtaUploadInputParams getOtaUploadInputParams() const
    {
      return m_otaUploadInputParams;
    }

  protected:
    void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) override
    {
      rapidjson::Pointer("/data/rsp/response")
        .Set(doc, HexStringConversion::encodeBinary(res.getResponse().DpaPacket().Buffer, res.getResponse().GetLength()));
    }


  private:
    TOtaUploadInputParams m_otaUploadInputParams;

    // Parses document into data fields
    void parse(rapidjson::Document& doc)
    {
      rapidjson::Value* jsonVal;

      // Repeat
      if ((jsonVal = rapidjson::Pointer("/data/repeat").Get(doc)))
        m_otaUploadInputParams.repeat = (uint16_t)jsonVal->GetInt();

      // Device address
      if ((jsonVal = rapidjson::Pointer("/data/req/deviceAddr").Get(doc)))
        m_otaUploadInputParams.deviceAddress = (uint16_t)jsonVal->GetInt();

      // hwpId
      if ((jsonVal = rapidjson::Pointer("/data/req/hwpId").Get(doc)))
        m_otaUploadInputParams.hwpId = (uint16_t)jsonVal->GetInt();

      // File name
      if ((jsonVal = rapidjson::Pointer("/data/req/fileName").Get(doc)))
        m_otaUploadInputParams.fileName = jsonVal->GetString();

      // Start memory address
      if ((jsonVal = rapidjson::Pointer("/data/req/startMemAddr").Get(doc)))
        m_otaUploadInputParams.startMemAddr = (uint16_t)jsonVal->GetInt();

      // Loading action
      if ((jsonVal = rapidjson::Pointer("/data/req/loadingAction").Get(doc)))
        m_otaUploadInputParams.loadingAction = jsonVal->GetString();

      // Upload eeprom data
      if ((jsonVal = rapidjson::Pointer("/data/req/uploadEepromData").Get(doc)))
        m_otaUploadInputParams.uploadEepromData = jsonVal->GetBool();

      // Upload eeeprom data
      if ((jsonVal = rapidjson::Pointer("/data/req/uploadEeepromData").Get(doc)))
        m_otaUploadInputParams.uploadEeepromData = jsonVal->GetBool();

    }
  };
}
