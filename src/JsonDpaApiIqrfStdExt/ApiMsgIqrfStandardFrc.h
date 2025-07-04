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

#include "IJsRenderService.h"
#include "ApiMsgIqrfStandard.h"
#include "JsDriverSensor.h"

namespace iqrf {
  class ApiMsgIqrfStandardFrc : public ApiMsgIqrfStandard
  {
  private:
    bool m_getExtraResult = true;
    bool m_getNadr = false;
    bool m_getExtFormat = false;
    DpaMessage m_dpaRequestExtra;
    std::unique_ptr<IDpaTransactionResult2> m_extraRes;
    std::set<uint8_t> selectedNodes;
    bool m_hasSensorIndex = false;
    uint8_t m_sensorIndex = 0;
    uint16_t m_ldiCommand = 0;
    rapidjson::Document m_selectedNodes;
    const std::string mTypeDaliFrc = "iqrfDali_Frc";
    const std::string mTypeLightFrcLaiRead = "iqrfLight_FrcLaiRead";
    const std::string mTypeLightFrcLdiSend = "iqrfLight_FrcLdiSend";
    const std::string mTypeSensorFrc = "iqrfSensor_Frc";

  public:
    ApiMsgIqrfStandardFrc() = delete;
    ApiMsgIqrfStandardFrc(const rapidjson::Document& doc)
      :ApiMsgIqrfStandard(doc)
    {
      using namespace rapidjson;


      const Value *val = Pointer("/data/req/param/getExtraResult").Get(doc);
      if (val && val->IsBool()) {
        m_getExtraResult = val->GetBool();
      }
      val = Pointer("/data/req/param/extFormat").Get(doc);
      if (val && val->IsBool()) {
        m_getExtFormat = val->GetBool();
      }
      const std::string mType = getMType();
      if (mType == mTypeLightFrcLdiSend || mType == mTypeDaliFrc) {
        val = Pointer("/data/req/param/command").Get(doc);
        if (val && val->IsUint()) {
          m_ldiCommand = static_cast<uint16_t>(val->GetUint());
        }
      } else if (mType == mTypeSensorFrc) {
        val = Pointer("/data/req/param/sensorIndex").Get(doc);
        if (val && val->IsUint()) {
          m_hasSensorIndex = true;
          m_sensorIndex = val->GetUint();
        }
      }

      val = Pointer("/data/req/param/selectedNodes").Get(doc);
      if (val) {
        m_selectedNodes.CopyFrom(*val, m_selectedNodes.GetAllocator());
        for (Value *item = m_selectedNodes.Begin(); item != m_selectedNodes.End(); item++) {
          selectedNodes.insert(item->GetUint());
        }
      }
    }

    bool hasSelectedNodes() const {
      return selectedNodes.size() > 0;
    }

    std::set<uint8_t> getSelectedNodes() const {
      return selectedNodes;
    }

    std::string getArrayKeyByMessageType(const std::string mType) {
      if (mType == mTypeLightFrcLdiSend || mType == mTypeDaliFrc) {
        return "answers";
      } else if (mType == mTypeLightFrcLaiRead) {
        return "voltages";
      } else if (mType == mTypeSensorFrc) {
        return "sensors";
      }
      return "";
    }

    std::string getItemKeyByMessageType(const std::string mType) {
      if (mType == mTypeLightFrcLdiSend || mType == mTypeDaliFrc) {
        return "answer";
      } else if (mType == mTypeLightFrcLaiRead) {
        return "voltage";
      } else if (mType == mTypeSensorFrc) {
        return "sensor";
      }
      return "";
    }

    bool getExtraResult() const { return m_getExtraResult; }
    bool getExtFormat() const { return m_getExtFormat; }

    void setDpaTransactionExtraResult(std::unique_ptr<IDpaTransactionResult2> extraRes)
    {
      m_extraRes = std::move(extraRes);
    }

    virtual ~ApiMsgIqrfStandardFrc()
    {
    }

  protected:
    void createResponsePayload(rapidjson::Document& doc) override
    {
      using namespace rapidjson;
      ApiMsgIqrfStandard::createResponsePayload(doc);

      if (getStatus() == 0) {
        const std::string mType = getMType();
        if (mType == mTypeLightFrcLdiSend || mType == mTypeDaliFrc) {
          Pointer("/data/rsp/result/command").Set(doc, m_ldiCommand);
        } else if (mType == mTypeSensorFrc && m_hasSensorIndex) {
          Pointer("/data/rsp/result/sensorIndex").Set(doc, m_sensorIndex);
        }

        if (!m_selectedNodes.IsNull()) {
          Pointer("/data/rsp/result/selectedNodes").Set(doc, m_selectedNodes);
        }
      }

      bool r = (bool)m_extraRes;
      if (getVerbose() && r) {
        rapidjson::Pointer("/data/raw/1/request").Set(doc, r ? HexStringConversion::encodeBinary(m_extraRes->getRequest().DpaPacket().Buffer, m_extraRes->getRequest().GetLength()) : "");
        rapidjson::Pointer("/data/raw/1/requestTs").Set(doc, r ? TimeConversion::encodeTimestamp(m_extraRes->getRequestTs()) : "");
        rapidjson::Pointer("/data/raw/1/confirmation").Set(doc, r ? HexStringConversion::encodeBinary(m_extraRes->getConfirmation().DpaPacket().Buffer, m_extraRes->getConfirmation().GetLength()) : "");
        rapidjson::Pointer("/data/raw/1/confirmationTs").Set(doc, r ? TimeConversion::encodeTimestamp(m_extraRes->getConfirmationTs()) : "");
        rapidjson::Pointer("/data/raw/1/response").Set(doc, r ? HexStringConversion::encodeBinary(m_extraRes->getResponse().DpaPacket().Buffer, m_extraRes->getResponse().GetLength()) : "");
        rapidjson::Pointer("/data/raw/1/responseTs").Set(doc, r ? TimeConversion::encodeTimestamp(m_extraRes->getResponseTs()) : "");
      }
    }

  };

}
