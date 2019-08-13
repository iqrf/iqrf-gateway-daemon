#pragma once

#include "ApiMsg.h"
#include "DpaMessage.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

namespace iqrf {
  //TODO this class shall replace all other obsolete ComIqrfStandard, ComIqrfStandardExt, etc ...
  //-------------------------------------------------------
  class ApiMsgIqrfStandard : public ApiMsg
  {
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
      Document param;
      param.CopyFrom(*reqParamObj, param.GetAllocator());
      StringBuffer buffer;
      Writer<rapidjson::StringBuffer> writer(buffer);
      param.Accept(writer);
      m_param = buffer.GetString();
    }

    int getTimeout() const { return m_timeout; }
    int getNadr() const { return m_nadr; }
    int getHwpid() const { return m_hwpid; }
    std::string getParamAsString() const { return m_param; }
    
    const DpaMessage& getDpaRequest() const { return m_dpaRequest; }
    void setDpaRequest(const DpaMessage& dpaRequest) { m_dpaRequest = dpaRequest; }
    
    void setPayload(const std::string& payloadKey, const rapidjson::Value& val)
    {
      m_payloadKey = payloadKey;
      m_payload.CopyFrom(val, m_payload.GetAllocator());
    }

    virtual ~ApiMsgIqrfStandard()
    {
    }

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

  protected:
    std::unique_ptr<IDpaTransactionResult2> m_res;
    std::string m_payloadKey;
    rapidjson::Document m_payload;

  private:
    int m_timeout = -1;
    int m_nadr = -1;
    int m_hwpid = 0;
    std::string m_param;
    DpaMessage m_dpaRequest;
  };

}
