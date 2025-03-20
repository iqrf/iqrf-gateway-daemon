/**
 * IQRF Gateway Daemon
 * Copyright (C) 2015-2025 IQRF Tech s.r.o., 2019-2025 MICRORISC s.r.o.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
      rapidjson::Document param;
      param.CopyFrom(*reqParamObj, param.GetAllocator());
      rapidjson::StringBuffer buffer;
      rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
      param.Accept(writer);
      m_param = buffer.GetString();
      std::string mType = getMType();
      if (mType == "iqrfSensor_ReadSensorsWithTypes") {
        Value *sensorIndexes = Pointer("/sensorIndexes").Get(param);
        if (sensorIndexes) {
          m_sensorIndexes.CopyFrom(*sensorIndexes, m_sensorIndexes.GetAllocator());
        }
      }
      if (mType == "iqrfEmbedFrc_SendSelective" || mType == "iqrfEmbedOs_SelectiveBatch") {
        Value* selectedNodes = Pointer("/selectedNodes").Get(param);
        if (selectedNodes) {
          m_selectedNodes.CopyFrom(*selectedNodes, m_selectedNodes.GetAllocator());
        }
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

    void setPnum(const uint8_t &pnum) {
      auto packet = m_request.DpaPacket();
      packet.DpaRequestPacket_t.PNUM = pnum;
      m_request.DataToBuffer(packet.Buffer, sizeof(packet.Buffer));
    }

    void setPcmd(const uint8_t &pcmd) {
      auto packet = m_request.DpaPacket();
      packet.DpaRequestPacket_t.PCMD = pcmd;
      m_request.DataToBuffer(packet.Buffer, sizeof(packet.Buffer));
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
      m_hasPayload = true;
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
      Pointer("/data/rsp/nAdr").Set(doc, m_nadr);
      if (res.isResponded()) {
        Pointer("/data/rsp/pnum").Set(doc, res.getResponse().DpaPacket().DpaResponsePacket_t.PNUM);
        Pointer("/data/rsp/pcmd").Set(doc, res.getResponse().DpaPacket().DpaResponsePacket_t.PCMD);
        Pointer("/data/rsp/hwpId").Set(doc, res.getResponse().DpaPacket().DpaResponsePacket_t.HWPID);
        Pointer("/data/rsp/rCode").Set(doc, res.getResponse().DpaPacket().DpaResponsePacket_t.ResponseCode);
        Pointer("/data/rsp/dpaVal").Set(doc, res.getResponse().DpaPacket().DpaResponsePacket_t.DpaValue);
      } else {
        int pnum = res.getRequest().DpaPacket().DpaRequestPacket_t.PNUM;
        int pcmd = res.getRequest().DpaPacket().DpaRequestPacket_t.PCMD + 0x80;
        if (m_requestDriverConvertFailure && m_unresolvablePerCmd) {
          pnum = -1;
          pcmd = -1;
        }
        Pointer("/data/rsp/pnum").Set(doc, pnum);
        Pointer("/data/rsp/pcmd").Set(doc, pcmd);
        Pointer("/data/rsp/hwpId").Set(doc, -1);
        Pointer("/data/rsp/rCode").Set(doc, -1);
        Pointer("/data/rsp/dpaVal").Set(doc, -1);
      }

      // TODO: CHANGE THIS, THIS IS BAD AND SHOULDN'T BE DONE THIS WAY, SET PAYLOAD ACCORDING TO STATUS, NOT EVERYTIME
      // FIX TO PREVENT REWRITE OF JSON ROOT
      if (m_hasPayload && (!m_payloadOnlyForVerbose || getVerbose())) {
        Pointer(m_payloadKey.c_str()).Set(doc, m_payload);
      }

      if (getStatus() == 0) {
        if (!m_selectedNodes.IsNull()) {
          Pointer("/data/rsp/result/selectedNodes").Set(doc, m_selectedNodes);
        }
        if (!m_sensorIndexes.IsNull()) {
          Pointer("/data/rsp/result/sensorIndexes").Set(doc, m_sensorIndexes);
        }
      }

      if (m_appendMidMetaData) {
        Pointer("/data/rsp/metaData").Set(doc, m_midMetaData);
      }
    }

  private:
    int m_nadr = -1;
    int m_hwpid = -1;
    std::string m_param;
    std::string m_payloadKey;
    rapidjson::Document m_payload;
    bool m_hasPayload = false;
    bool m_payloadOnlyForVerbose = true;
    bool m_appendMidMetaData = false;
    rapidjson::Document m_midMetaData;
    rapidjson::Document m_selectedNodes;
    rapidjson::Document m_sensorIndexes;
  };

}
