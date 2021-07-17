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

    virtual void setMidMetaData(const rapidjson::Value& val)
    {
      m_appendMidMetaData = true;
      m_midMetaData.CopyFrom(val, m_midMetaData.GetAllocator());
    }

    virtual ~ComNadr()
    {
    }

  protected:
    bool m_appendMidMetaData = false;
    rapidjson::Document m_midMetaData;
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

    virtual ~ComRaw()
    {
    }

  protected:
    void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) override
    {
      auto response = res.getResponse();
      rapidjson::Pointer("/data/rsp/rData").Set(doc, encodeBinary(response.DpaPacket().Buffer, response.GetLength()));
      if (m_appendMidMetaData) {
        rapidjson::Pointer("/data/rsp/metaData").Set(doc, m_midMetaData);
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
        int sz = rdata.size() <= DPA_MAX_DATA_LENGTH ? static_cast<int>(rdata.size()) : DPA_MAX_DATA_LENGTH;
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

      if (m_appendMidMetaData) {
        rapidjson::Pointer("/data/rsp/metaData").Set(doc, m_midMetaData);
      }

    }

  };

}
