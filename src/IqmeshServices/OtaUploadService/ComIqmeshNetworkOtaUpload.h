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
