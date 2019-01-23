#pragma once

#include "ComBase.h"
#include "JsonUtils.h"

namespace iqrf {
  class ComNadr : public ComBase
  {
  public:
    ComNadr() = delete;
    ComNadr(rapidjson::Document& doc)
      :ComBase(doc)
    {
    }

    virtual uint16_t getNadr() const = 0;

    virtual void setMetaData(const rapidjson::Value& val)
    {
      m_appendMetaData = true;
      m_metaData.CopyFrom(val, m_metaData.GetAllocator());
    }

    virtual ~ComNadr()
    {
    }

  protected:

    bool m_appendMetaData = false;
    rapidjson::Document m_metaData;

  };

  class ComRaw : public ComNadr
  {
  public:
    ComRaw() = delete;
    ComRaw(rapidjson::Document& doc)
      :ComNadr(doc)
    {
      int len = parseBinary(m_request.DpaPacket().Buffer,
        rapidjson::Pointer("/data/req/rData").Get(doc)->GetString(),
        DPA_MAX_DATA_LENGTH);
      m_request.SetLength(len);
    }

    uint16_t getNadr() const override
    {
      return m_request.DpaPacket().DpaRequestPacket_t.NADR;
    }

    void setMetaData(const std::string& metaDataKey, rapidjson::Value& val)
    {
      m_appendMetaData = true;
      m_metaData.CopyFrom(val, m_metaData.GetAllocator());
    }

    virtual ~ComRaw()
    {
    }

  protected:
    void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) override
    {
      rapidjson::Pointer("/data/rsp/rData").Set(doc, encodeBinary(res.getResponse().DpaPacket().Buffer, res.getResponse().GetLength()));
      if (m_appendMetaData) {
        rapidjson::Pointer("/data/rsp/metaData").Set(doc, m_metaData);
      }
    }

  };

  class ComRawHdp : public ComNadr
  {
  public:
    ComRawHdp() = delete;
    ComRawHdp(rapidjson::Document& doc)
      :ComNadr(doc)
    {
      m_request.DpaPacket().DpaRequestPacket_t.NADR = rapidjson::Pointer("/data/req/nAdr").Get(doc)->GetInt();
      m_request.DpaPacket().DpaRequestPacket_t.PNUM = rapidjson::Pointer("/data/req/pNum").Get(doc)->GetInt();
      m_request.DpaPacket().DpaRequestPacket_t.PCMD = rapidjson::Pointer("/data/req/pCmd").Get(doc)->GetInt();

      int hwpidReq = rapidjson::Pointer("/data/req/hwpId").GetWithDefault(doc, -1).GetInt();
      if (hwpidReq < 0) {
        hwpidReq = 0xffff;
      }
      m_request.DpaPacket().DpaRequestPacket_t.HWPID = (uint16_t)hwpidReq;

      rapidjson::Value* req = rapidjson::Pointer("/data/req").Get(doc);
      if (req) {
        std::vector<int> rdata = jutils::getPossibleMemberAsVector<int>("pData", *req);
        int sz = rdata.size() <= DPA_MAX_DATA_LENGTH ? rdata.size() : DPA_MAX_DATA_LENGTH;
        uint8_t* pdata = m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData;
        for (int i = 0; i < sz; i++) {
          *(pdata + i) = (uint8_t)rdata[i];
        }
        m_request.SetLength(sizeof(TDpaIFaceHeader) + sz);
      }
    }

    uint16_t getNadr() const override
    {
      return m_request.DpaPacket().DpaRequestPacket_t.NADR;
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
      
      rapidjson::Value* req = rapidjson::Pointer("/data/rsp").Get(doc);
      if (req) {
        rapidjson::Value rdata;
        rdata.SetArray();
        int sz = res.getResponse().GetLength() - sizeof(TDpaIFaceHeader) - 2;
        const uint8_t* pdata = res.getResponse().DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData;
        rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
        for (int i = 0; i < sz; i++) {
          rdata.PushBack((int)(*(pdata + i)), allocator);
        }
        req->AddMember("pData", rdata, allocator);
      }

      if (m_appendMetaData) {
        rapidjson::Pointer("/data/rsp/metaData").Set(doc, m_metaData);
      }

    }

  };

}
