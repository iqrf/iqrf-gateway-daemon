/**
 * Copyright 2015-2021 IQRF Tech s.r.o.
 * Copyright 2019-2021 MICRORISC s.r.o.
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
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "rapidjson/prettywriter.h"

namespace iqrf {
  //-------------------------------------------------------
  class ComIqrfStandard : public ComBase
  {
  public:
    ComIqrfStandard()
      :ComBase()
    {}

    ComIqrfStandard(rapidjson::Document& doc)
      :ComBase(doc)
    {
      using namespace rapidjson;
      m_nadr = rapidjson::Pointer("/data/req/nAdr").Get(doc)->GetInt();
      m_hwpid = rapidjson::Pointer("/data/req/hwpId").GetWithDefault(doc, m_hwpid).GetInt();
      Value* reqParamObj = Pointer("/data/req/param").Get(doc);
      if (reqParamObj != nullptr) {
        rapidjson::Document param;
        param.Swap(*reqParamObj);
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        param.Accept(writer);
        m_param = buffer.GetString();
      }
    }

    int getNadr() const
    {
      return m_nadr;
    }
    
    int getHwpid() const
    {
      return m_hwpid;
    }

    std::string getParamAsString() const
    {
      return m_param;
    }

    void setDpaMessage(std::vector<uint8_t> dpaVect)
    {
      if (!dpaVect.empty()) {
        std::copy(dpaVect.data(), dpaVect.data() + dpaVect.size(), m_request.DpaPacket().Buffer);
        m_request.SetLength(static_cast<int>(dpaVect.size()));
      }
      else {
        THROW_EXC_TRC_WAR(std::logic_error, "Unexpected format of data");
      }
    }

    void setPayload(const std::string& payloadKey, const rapidjson::Value& val, bool onlyForVerbose)
    {
      m_payloadKey = payloadKey;
      m_payload.CopyFrom(val, m_payload.GetAllocator());
      m_payloadOnlyForVerbose = onlyForVerbose;
    }

    //db metadata
    void setMidMetaData(const rapidjson::Value& val)
    {
      m_appendMidMetaData = true;
      m_midMetaData.CopyFrom(val, m_midMetaData.GetAllocator());
    }

    virtual ~ComIqrfStandard()
    {
    }

  protected:
    void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) override
    {
      using namespace rapidjson;
      bool r = res.isResponded();
      Pointer("/data/rsp/nAdr").Set(doc, m_nadr);
      Pointer("/data/rsp/hwpId").Set(doc, r ? res.getResponse().DpaPacket().DpaResponsePacket_t.HWPID : m_hwpid);
      Pointer("/data/rsp/rCode").Set(doc, r ? res.getResponse().DpaPacket().DpaResponsePacket_t.ResponseCode : 0);
      Pointer("/data/rsp/dpaVal").Set(doc, r ? res.getResponse().DpaPacket().DpaResponsePacket_t.DpaValue : 0);
      if ((!m_payloadOnlyForVerbose || getVerbose()) && (m_payload.IsObject() && !m_payload.ObjectEmpty())) {
        Pointer(m_payloadKey.c_str()).Set(doc, m_payload);
      }
      if (m_appendMidMetaData) {
        Pointer("/data/rsp/metaData").Set(doc, m_midMetaData);
      }
    }

  private:
    int m_nadr = -1;
    int m_hwpid = -1;
    std::string m_param = "{}";
    std::string m_payloadKey;
    rapidjson::Document m_payload;
    bool m_payloadOnlyForVerbose = true;
    bool m_appendMidMetaData = false;
    rapidjson::Document m_midMetaData;
  };

}
