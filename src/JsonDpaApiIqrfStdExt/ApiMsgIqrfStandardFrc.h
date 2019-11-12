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

  public:
    ApiMsgIqrfStandardFrc() = delete;
    ApiMsgIqrfStandardFrc(const rapidjson::Document& doc)
      :ApiMsgIqrfStandard(doc)
    {
      using namespace rapidjson;
      
      {
        const Value *val = Pointer("/data/req/param/getExtraResult").Get(doc);
        if (val && val->IsBool()) {
          m_getExtraResult = val->GetBool();
        }
      }
      {
        const Value *val = Pointer("/data/req/param/extFormat").Get(doc);
        if (val && val->IsBool()) {
          m_getExtFormat = val->GetBool();
        }
      }
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

      bool r = (bool)m_extraRes;
      if (getVerbose() && r) {
        rapidjson::Pointer("/data/raw/1/request").Set(doc, r ? encodeBinary(m_extraRes->getRequest().DpaPacket().Buffer, m_extraRes->getRequest().GetLength()) : "");
        rapidjson::Pointer("/data/raw/1/requestTs").Set(doc, r ? encodeTimestamp(m_extraRes->getRequestTs()) : "");
        rapidjson::Pointer("/data/raw/1/confirmation").Set(doc, r ? encodeBinary(m_extraRes->getConfirmation().DpaPacket().Buffer, m_extraRes->getConfirmation().GetLength()) : "");
        rapidjson::Pointer("/data/raw/1/confirmationTs").Set(doc, r ? encodeTimestamp(m_extraRes->getConfirmationTs()) : "");
        rapidjson::Pointer("/data/raw/1/response").Set(doc, r ? encodeBinary(m_extraRes->getResponse().DpaPacket().Buffer, m_extraRes->getResponse().GetLength()) : "");
        rapidjson::Pointer("/data/raw/1/responseTs").Set(doc, r ? encodeTimestamp(m_extraRes->getResponseTs()) : "");
      }
    }

  };

}
