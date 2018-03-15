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
      parseHexaNum(m_request.DpaPacket().DpaRequestPacket_t.NADR, rapidjson::Pointer("/data/req/nAdr").Get(doc)->GetString());
      parseHexaNum(m_request.DpaPacket().DpaRequestPacket_t.PNUM, rapidjson::Pointer("/data/req/pNum").Get(doc)->GetString());
      parseHexaNum(m_request.DpaPacket().DpaRequestPacket_t.PCMD, rapidjson::Pointer("/data/req/pCmd").Get(doc)->GetString());
      parseHexaNum(m_request.DpaPacket().DpaRequestPacket_t.HWPID, rapidjson::Pointer("/data/req/hwpId").Get(doc)->GetString());
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
      rapidjson::Pointer("/data/rsp/nAdr").Set(doc, r ? encodeHexaNum(res.getResponse().DpaPacket().DpaResponsePacket_t.NADR) : "");
      rapidjson::Pointer("/data/rsp/pNum").Set(doc, r ? encodeHexaNum(res.getResponse().DpaPacket().DpaResponsePacket_t.PNUM) : "");
      rapidjson::Pointer("/data/rsp/pCmd").Set(doc, r ? encodeHexaNum(res.getResponse().DpaPacket().DpaResponsePacket_t.PCMD) : "");
      rapidjson::Pointer("/data/rsp/hwpId").Set(doc, r ? encodeHexaNum(res.getResponse().DpaPacket().DpaResponsePacket_t.HWPID) : "");
      rapidjson::Pointer("/data/rsp/rCode").Set(doc, r ? encodeHexaNum(res.getResponse().DpaPacket().DpaResponsePacket_t.ResponseCode) : "");
      rapidjson::Pointer("/data/rsp/dpaVal").Set(doc, r ? encodeHexaNum(res.getResponse().DpaPacket().DpaResponsePacket_t.DpaValue) : "");
      rapidjson::Pointer("/data/rsp/rData").Set(doc, r ? encodeBinary(res.getResponse().DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData,
        res.getResponse().GetLength() - sizeof(TDpaIFaceHeader) - 2) : "");
    }
  };

}
