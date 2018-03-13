#pragma once

#include "DpaHandler2.h"
#include "rapidjson/schema.h"
#include "HexStringCoversion.h"
#include "Trace.h"
#include <exception>
#include <algorithm>
#include <sstream>

namespace iqrf {
  class ComBase
  {
  public:
    ComBase() {}
    ComBase(rapidjson::Document& doc)
    {
      m_msgId = rapidjson::Pointer("/data/msgId").Get(doc)->GetString();
      m_timeout = rapidjson::Pointer("/data/timeout").GetWithDefault(doc, (int)m_timeout).GetInt();
      m_verbose = rapidjson::Pointer("/data/returnVerbose").GetWithDefault(doc, m_verbose).GetBool();
    }

    virtual ~ComBase()
    {

    }

    const std::string& getMsgId() const
    {
      return m_msgId;
    }

    const int32_t getTimeout() const
    {
      return m_timeout;
    }

    const bool getVerbose() const
    {
      return m_verbose;
    }

    const DpaMessage& getDpaRequest() const
    {
      return m_request;
    }

    void createResponse(rapidjson::Document& doc, const IDpaTransactionResult2& res)
    {
      rapidjson::Pointer("/mType").Set(doc, "TODO-mType"); //TODO
      rapidjson::Pointer("/data/msgId").Set(doc, m_msgId);
      if (m_verbose) {
        rapidjson::Pointer("/data/timeout").Set(doc, m_timeout);
      }

      createResponsePayload(doc, res);

      if (m_verbose) {
        rapidjson::Pointer("/data/raw/request").Set(doc, encodeBinary(res.getRequest().DpaPacket().Buffer, res.getRequest().GetLength()));
        rapidjson::Pointer("/data/raw/requestTs").Set(doc, encodeTimestamp(res.getRequestTs()));
        rapidjson::Pointer("/data/raw/confirmation").Set(doc, encodeBinary(res.getConfirmation().DpaPacket().Buffer, res.getConfirmation().GetLength()));
        rapidjson::Pointer("/data/raw/confirmationTs").Set(doc, encodeTimestamp(res.getConfirmationTs()));
        rapidjson::Pointer("/data/raw/response").Set(doc, encodeBinary(res.getResponse().DpaPacket().Buffer, res.getResponse().GetLength()));
        rapidjson::Pointer("/data/raw/responseTs").Set(doc, encodeTimestamp(res.getResponseTs()));

        rapidjson::Pointer("/data/insId").Set(doc, "TODO-insId"); //TODO
        rapidjson::Pointer("/data/statusStr").Set(doc, res.getErrorString());
      }

      rapidjson::Pointer("/data/status").Set(doc, res.getErrorCode());
    }

  protected:
    virtual void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) = 0;
    DpaMessage m_request;

  private:
    std::string m_msgId;
    int32_t m_timeout = -1;
    bool m_verbose = false;
  };

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
