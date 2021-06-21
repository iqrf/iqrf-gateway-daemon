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

#include "ApiMsg.h"
#include "IDpaTransactionResult2.h"
#include "HexStringCoversion.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

namespace iqrf {
  //TODO this class shall replace all other obsolete ComIqrfStandard, ComIqrfStandardExt, etc ...
  //-------------------------------------------------------
  class ApiMsgIqrfStandard : public ApiMsg
  {
  protected:
    std::unique_ptr<IDpaTransactionResult2> m_res;
    std::string m_payloadKey;
    rapidjson::Document m_payload;

  private:
    int m_timeout = -1;
    int m_nadr = -1;
    int m_hwpid = 0;
    rapidjson::Document m_requestParamDoc;
    std::string m_requestParamStr;
    DpaMessage m_dpaRequest;

  public:
    ApiMsgIqrfStandard() = delete;
    ApiMsgIqrfStandard(const rapidjson::Document& doc)
      :ApiMsg(doc)
    {
      using namespace rapidjson;

      const Value* timeoutVal = Pointer("/data/timeout").Get(doc);
      if (timeoutVal && timeoutVal->IsInt())
        m_timeout = timeoutVal->GetInt();
      
      m_nadr = Pointer("/data/req/nAdr").Get(doc)->GetInt();
      const Value* hwpidVal = Pointer("/data/req/hwpId").Get(doc);
      if (hwpidVal && hwpidVal->IsInt())
        m_hwpid = hwpidVal->GetInt();
      
      const Value* reqParamObj = Pointer("/data/req/param").Get(doc);
      m_requestParamDoc.CopyFrom(*reqParamObj, m_requestParamDoc.GetAllocator());
      StringBuffer buffer;
      Writer<rapidjson::StringBuffer> writer(buffer);
      m_requestParamDoc.Accept(writer);
      m_requestParamStr = buffer.GetString();
    }

    int getTimeout() const { return m_timeout; }
    int getNadr() const { return m_nadr; }
    int getHwpid() const { return m_hwpid; }
    
    const rapidjson::Document & getRequestParamDoc() { return m_requestParamDoc; }
    std::string getRequestParamStr() const { return m_requestParamStr; }
    
    const DpaMessage& getDpaRequest() const { return m_dpaRequest; }
    //void setDpaRequest(const DpaMessage& dpaRequest) { m_dpaRequest = dpaRequest; }
    
    void setPayload(const std::string& payloadKey, const rapidjson::Value& val)
    {
      m_payloadKey = payloadKey;
      m_payload.CopyFrom(val, m_payload.GetAllocator());
    }

    virtual ~ApiMsgIqrfStandard()
    {}

    void setDpaTransactionResult(std::unique_ptr<IDpaTransactionResult2> res)
    {
      m_res = std::move(res);
    }

  protected:
    void createResponsePayload(rapidjson::Document& doc) override
    {
      using namespace rapidjson;

      if (getVerbose()) {
        if (m_timeout != -1) {
          rapidjson::Pointer("/data/timeout").Set(doc, m_timeout);
        }
      }

      bool r = (bool)m_res && m_res->isResponded();
      Pointer("/data/rsp/nAdr").Set(doc, m_nadr);
      Pointer("/data/rsp/hwpId").Set(doc, r ? m_res->getResponse().DpaPacket().DpaResponsePacket_t.HWPID : m_hwpid);
      Pointer("/data/rsp/rCode").Set(doc, r ? m_res->getResponse().DpaPacket().DpaResponsePacket_t.ResponseCode : 0);
      Pointer("/data/rsp/dpaVal").Set(doc, r ? m_res->getResponse().DpaPacket().DpaResponsePacket_t.DpaValue : 0);
      Pointer(m_payloadKey.c_str()).Set(doc, m_payload);

      if (getVerbose()) {
        r = (bool)m_res;
        rapidjson::Pointer("/data/raw/0/request").Set(doc, r ? encodeBinary(m_res->getRequest().DpaPacket().Buffer, m_res->getRequest().GetLength()) : "");
        rapidjson::Pointer("/data/raw/0/requestTs").Set(doc, r ? encodeTimestamp(m_res->getRequestTs()) : "");
        rapidjson::Pointer("/data/raw/0/confirmation").Set(doc, r ? encodeBinary(m_res->getConfirmation().DpaPacket().Buffer, m_res->getConfirmation().GetLength()) : "");
        rapidjson::Pointer("/data/raw/0/confirmationTs").Set(doc, r ? encodeTimestamp(m_res->getConfirmationTs()) : "");
        rapidjson::Pointer("/data/raw/0/response").Set(doc, r ? encodeBinary(m_res->getResponse().DpaPacket().Buffer, m_res->getResponse().GetLength()) : "");
        rapidjson::Pointer("/data/raw/0/responseTs").Set(doc, r ? encodeTimestamp(m_res->getResponseTs()) : "");
      }
    }

  };

}
