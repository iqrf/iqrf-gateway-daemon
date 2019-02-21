#pragma once

#include "ComBaseExt.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

namespace iqrf {
  //-------------------------------------------------------
  class ComIqrfStandardExt : public ComBaseExt
  {
  public:
    ComIqrfStandardExt() = delete;
    ComIqrfStandardExt(const rapidjson::Document& doc)
      :ComBaseExt(doc)
    {
      using namespace rapidjson;
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

    int getNadr() const { return m_nadr; }
    int getHwpid() const { return m_hwpid; }
    std::string getParamAsString() const { return m_param; }
    
    const DpaMessage& getDpaRequest() const { return m_dpaRequest; }
    void setDpaRequest(const DpaMessage& dpaRequest) { m_dpaRequest = dpaRequest; }
    
    void setPayload(const std::string& payloadKey, rapidjson::Value&& val)
    {
      m_payloadKey = payloadKey;
      m_payload.CopyFrom(val, m_payload.GetAllocator());
    }

    virtual ~ComIqrfStandardExt()
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
    int m_nadr = -1;
    int m_hwpid = -1;
    std::string m_param;
    DpaMessage m_dpaRequest;
  };

  class IqrfSensorFrc : public ComIqrfStandardExt
  {
  public:
    IqrfSensorFrc() = delete;
    IqrfSensorFrc(const rapidjson::Document& doc)
      :ComIqrfStandardExt(doc)
    {
      using namespace rapidjson;
      m_sensorType = Pointer("/data/req/param/sensorType").Get(doc)->GetInt();
      m_frcCommand = Pointer("/data/req/param/frcCommand").Get(doc)->GetInt();
      m_sensorType = Pointer("/data/req/param/sensorType").Get(doc)->GetInt();
      const Value *valGetEx = Pointer("/data/req/param/getExtraResult").Get(doc);
      if (valGetEx && valGetEx->IsBool()) {
        m_getExtraResult = valGetEx->GetBool();
      }
    }

    int getSensorType() const { return m_sensorType; }
    int getFrcCommand() const { return m_frcCommand; }
    bool getExtraResult() const { return m_getExtraResult; }
    const DpaMessage& getDpaRequestExtra() const { return m_dpaRequestExtra; }
    void setDpaRequestExtra(const DpaMessage& dpaRequest) { m_dpaRequestExtra = dpaRequest; }
    
    const DpaMessage* getDpaResponse() const
    {
      if (m_res) return &m_res->getResponse();
      return nullptr;
    }
    
    const DpaMessage* getDpaResponseExtra() const
    {
      if (m_extraRes) return &m_extraRes->getResponse();
      return nullptr;
    }

    void setDpaTransactionExtraResult(std::unique_ptr<IDpaTransactionResult2> extraRes)
    {
      m_extraRes = std::move(extraRes);
    }

    virtual ~IqrfSensorFrc()
    {
    }

  protected:
    void createResponsePayload(rapidjson::Document& doc) override
    {
      using namespace rapidjson;
      ComIqrfStandardExt::createResponsePayload(doc);
      
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

  private:
    int m_sensorType = 0;
    int m_frcCommand = 0;
    DpaMessage m_dpaRequestExtra;
    bool m_getExtraResult = true;
    std::unique_ptr<IDpaTransactionResult2> m_extraRes;
  };

  class RawHdp
  {
  public:
    RawHdp()
    {}
    int getNadr() const { return m_nadr;  }
    int getPnum() const { return m_pnum; }
    int getPcmd() const { return m_pcmd; }
    int getHwpid() const { return m_hwpid; }
    const rapidjson::Document& getDocument() const { return m_doc; }
    const std::string& getString() const { return m_str; }
    bool empty() const { return m_empty; }
  protected:
    int m_nadr = 0;
    int m_pnum = 0;
    int m_pcmd = 0;
    int m_hwpid = 0;
    rapidjson::Document m_doc;
    std::string m_str;
    bool m_empty = true;
  };

  class RawHdpRequest : public RawHdp
  {
  public:
    RawHdpRequest()
    {}

    RawHdpRequest(const std::string& str, uint16_t inadr, uint16_t ihwpid)
    {
      rapidjson::Document doc;
      doc.Parse(str);
      parse(doc, inadr, ihwpid);
    }

    RawHdpRequest(const rapidjson::Value& value, uint16_t inadr, uint16_t ihwpid)
    {
      parse(value, inadr, ihwpid);
    }

    void parse(const rapidjson::Value& value, uint16_t inadr, uint16_t ihwpid)
    {
      using namespace rapidjson;

      uint16_t nadr16 = inadr, hwpid16 = ihwpid;
      uint8_t pnum = 0, pcmd = 0;

      //set explicitly by param
      //const Value *nadrVal = Pointer("/nadr").Get(value);
      //if (nadrVal && nadrVal->IsString()) {
      //  parseHexaNum(nadr16, nadrVal->GetString());
      //}
      const Value *pnumVal = Pointer("/pnum").Get(value);
      if (pnumVal && pnumVal->IsString()) {
        parseHexaNum(pnum, pnumVal->GetString());
      }
      const Value *pcmdVal = Pointer("/pcmd").Get(value);
      if (pcmdVal && pcmdVal->IsString()) {
        parseHexaNum(pcmd, pcmdVal->GetString());
      }
      //set explicitly by param
      //const Value *hwpidVal = Pointer("/hwpid").Get(value);
      //if (hwpidVal && hwpidVal->IsString()) {
      //  parseHexaNum(hwpid16, hwpidVal->GetString());
      //}

      m_nadr = nadr16;
      m_pnum = pnum;
      m_pcmd = pcmd;
      m_hwpid = hwpid16;

      m_dpaRequest.DpaPacket().DpaRequestPacket_t.NADR = nadr16;
      m_dpaRequest.DpaPacket().DpaRequestPacket_t.PNUM = pnum;
      m_dpaRequest.DpaPacket().DpaRequestPacket_t.PCMD = pcmd;
      m_dpaRequest.DpaPacket().DpaRequestPacket_t.HWPID = hwpid16;

      int len = 0;
      const Value *rdataVal = Pointer("/rdata").Get(value);
      if (rdataVal && rdataVal->IsString()) {
        //uint8_t buf[DPA_MAX_DATA_LENGTH];
        len = parseBinary(m_dpaRequest.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData, rdataVal->GetString(), DPA_MAX_DATA_LENGTH);
      }
      m_dpaRequest.SetLength(len + sizeof(TDpaIFaceHeader));

      m_empty = false;
    }

    virtual ~RawHdpRequest()
    {}

    const DpaMessage& getDpaRequest() { return m_dpaRequest; }
  private:
    DpaMessage m_dpaRequest;
  };

  class RawHdpResponse : public RawHdp
  {
  public:
    RawHdpResponse()
    {}

    RawHdpResponse(const DpaMessage& dpaMessage)
    {
      using namespace rapidjson;

      if (dpaMessage.GetLength() >= 8) {
        uint16_t nadr16 = 0, hwpid16 = 0;
        uint8_t pnum = 0, pcmd = 0, rcode8 = 0, dpaval = 0;
        std::string nadrStr, pnumStr, pcmdStr, hwpidStr, rcodeStr, dpavalStr;

        nadr16 = dpaMessage.DpaPacket().DpaResponsePacket_t.NADR;
        pnum = dpaMessage.DpaPacket().DpaResponsePacket_t.PNUM;
        pcmd = dpaMessage.DpaPacket().DpaResponsePacket_t.PCMD;
        hwpid16 = dpaMessage.DpaPacket().DpaResponsePacket_t.HWPID;
        rcode8 = dpaMessage.DpaPacket().DpaResponsePacket_t.ResponseCode;
        dpaval = dpaMessage.DpaPacket().DpaResponsePacket_t.DpaValue;

        m_nadr = nadr16;
        m_pnum = pnum;
        m_pcmd = pcmd;
        m_hwpid = hwpid16;
        m_rcode = rcode8;
        m_dpaval = m_dpaval;

        nadrStr = encodeHexaNum(nadr16);
        pnumStr = encodeHexaNum(pnum);
        pcmdStr = encodeHexaNum(pcmd);
        hwpidStr = encodeHexaNum(hwpid16);
        rcodeStr = encodeHexaNum(rcode8);
        dpavalStr = encodeHexaNum(dpaval);

        Pointer("/nadr").Set(m_doc, nadrStr);
        Pointer("/pnum").Set(m_doc, pnumStr);
        Pointer("/pcmd").Set(m_doc, pcmdStr);
        Pointer("/hwpid").Set(m_doc, hwpidStr);
        Pointer("/rcode").Set(m_doc, rcodeStr);
        Pointer("/dpaval").Set(m_doc, dpavalStr);

        if (dpaMessage.GetLength() > 8) {
          Pointer("/rdata").Set(m_doc, encodeBinary(dpaMessage.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData, dpaMessage.GetLength() - 8));
        }

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        m_doc.Accept(writer);
        m_str = buffer.GetString();

        m_empty = false;
      }
    }

    virtual ~RawHdpResponse()
    {}

    int getRcode() const { return m_rcode; }
    int getDpaval() const { return m_dpaval; }

  private:
    int m_rcode = 0;
    int m_dpaval = 0;
  };

}
