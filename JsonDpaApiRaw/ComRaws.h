#pragma once

#include "ComBase.h"

namespace iqrf {
  class ComRaw : public ComBase
  {
  public:
    ComRaw() = delete;
    ComRaw(rapidjson::Document& doc)
      :ComBase(doc)
    {
      int len = parseBinary(m_request.DpaPacket().Buffer,
        rapidjson::Pointer("/data/req/request").Get(doc)->GetString(),
        DPA_MAX_DATA_LENGTH);
      m_request.SetLength(len);
    }

    virtual ~ComRaw()
    {
    }

  protected:
    void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) override
    {
      rapidjson::Pointer("/data/rsp/response").Set(doc, encodeBinary(res.getResponse().DpaPacket().Buffer, res.getResponse().GetLength()));
    }

  private:
  };

  class ComRawHdp : public ComBase
  {
  public:
    ComRawHdp() = delete;
    ComRawHdp(rapidjson::Document& doc)
      :ComBase(doc)
    {
      m_request.DpaPacket().DpaRequestPacket_t.NADR = rapidjson::Pointer("/data/req/nAdr").Get(doc)->GetInt();
      m_request.DpaPacket().DpaRequestPacket_t.PNUM = rapidjson::Pointer("/data/req/pNum").Get(doc)->GetInt();
      m_request.DpaPacket().DpaRequestPacket_t.PCMD = rapidjson::Pointer("/data/req/pCmd").Get(doc)->GetInt();
      m_request.DpaPacket().DpaRequestPacket_t.HWPID = rapidjson::Pointer("/data/req/hwpId").Get(doc)->GetInt();
      int len = parseBinary(m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData,
        rapidjson::Pointer("/data/req/rData").Get(doc)->GetString(),
        DPA_MAX_DATA_LENGTH);
      m_request.SetLength(sizeof(TDpaIFaceHeader) + len);
    }

    virtual ~ComRawHdp()
    {
    }

  protected:
    void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) override
    {
      bool r = res.isResponded();
      rapidjson::Pointer("/data/rsp/nAdr").Set(doc, r ? res.getResponse().DpaPacket().DpaResponsePacket_t.NADR : 0);
      rapidjson::Pointer("/data/rsp/pNum").Set(doc, r ? res.getResponse().DpaPacket().DpaResponsePacket_t.PNUM : 0);
      rapidjson::Pointer("/data/rsp/pCmd").Set(doc, r ? res.getResponse().DpaPacket().DpaResponsePacket_t.PCMD : 0);
      rapidjson::Pointer("/data/rsp/hwpId").Set(doc, r ? res.getResponse().DpaPacket().DpaResponsePacket_t.HWPID : 0);
      rapidjson::Pointer("/data/rsp/rCode").Set(doc, r ? res.getResponse().DpaPacket().DpaResponsePacket_t.ResponseCode : 0);
      rapidjson::Pointer("/data/rsp/dpaVal").Set(doc, r ? res.getResponse().DpaPacket().DpaResponsePacket_t.DpaValue : 0);
      rapidjson::Pointer("/data/rsp/rData").Set(doc, r ? encodeBinary(res.getResponse().DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData,
        res.getResponse().GetLength() - sizeof(TDpaIFaceHeader) - 2) : "");
    }
  };

}
