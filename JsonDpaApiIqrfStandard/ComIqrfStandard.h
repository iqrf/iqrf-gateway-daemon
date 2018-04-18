#pragma once

#include "ComBase.h"

namespace iqrf {
  //-------------------------------------------------------
  class ComIqrfStandard : public ComBase
  {
  public:
    ComIqrfStandard() = delete;
    ComIqrfStandard(rapidjson::Document& doc)
      :ComBase(doc)
    {
    }

    void setDpaMessage(std::vector<uint8_t> dpaVect)
    {
      if (!dpaVect.empty()) {
        std::copy(dpaVect.data(), dpaVect.data() + dpaVect.size(), m_request.DpaPacket().Buffer);
        m_request.SetLength(dpaVect.size());
      }
      else {
        THROW_EXC_TRC_WAR(std::logic_error, "Unexpected format of data");
      }
    }

    void setPayload(rapidjson::Document&& doc)
    {
      m_payload.Swap(doc);
    }

    virtual ~ComIqrfStandard()
    {
    }

  protected:
    void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) override
    {
      bool r = res.isResponded();
      rapidjson::Pointer("/data/rsp").Set(doc, m_payload);
      rapidjson::Pointer("/data/rsp/rCode").Set(doc, r ? res.getResponse().DpaPacket().DpaResponsePacket_t.ResponseCode : 0);
      rapidjson::Pointer("/data/rsp/dpaVal").Set(doc, r ? res.getResponse().DpaPacket().DpaResponsePacket_t.DpaValue : 0);
    }

  private:
    rapidjson::Document m_payload;
  };

}
