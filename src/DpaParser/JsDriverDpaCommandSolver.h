#pragma once

#include "IJsRenderService.h"
#include "DpaCommandSolver.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/pointer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "HexStringCoversion.h"
#include "Trace.h"

#include <string>

namespace iqrf {

  class JsDriverDpaCommandSolver : public DpaCommandSolver
  {
  protected:
    std::string m_rawHdpRequestAsStr;
    IJsRenderService* m_iJsRenderService = nullptr;

  public:
    virtual ~JsDriverDpaCommandSolver() {}

    JsDriverDpaCommandSolver(IJsRenderService* iJsRenderService, uint16_t nadr = -1)
      :DpaCommandSolver(nadr)
      , m_iJsRenderService(iJsRenderService)
    {}

    JsDriverDpaCommandSolver(IJsRenderService* iJsRenderService, uint16_t nadr, uint16_t hwpid)
      :DpaCommandSolver(nadr, hwpid)
      , m_iJsRenderService(iJsRenderService)
    {}

    // encode DpaMessage by driver function *_Request returning directly RawHdp
    DpaMessage encodeRequest() override
    {
      TRC_FUNCTION_ENTER("");

      using namespace rapidjson;

      driverRequest(m_rawHdpRequestAsStr);

      // convert from rawHdpRequest to dpaRequest and pass nadr and hwpid to be in dapaRequest (driver doesn't set them)
      Document doc;
      doc.Parse(m_rawHdpRequestAsStr);

      DpaMessage dpaRequest = encodeRequest(doc);

      TRC_FUNCTION_LEAVE("");
      return dpaRequest;
    }

    
    // encode vector of DpaMessage by driver function *_Request returning vector of RawHdp
    std::vector<DpaMessage> encodeMultiRequest()
    {
      TRC_FUNCTION_ENTER("");

      using namespace rapidjson;

      std::vector<DpaMessage> dpaRequestVect;
      std::string requestResult;

      driverRequest(requestResult);

      Document doc;
      doc.Parse(requestResult);

      const Value *parsVal = Pointer("/retpars").Get(doc);
      if (!(parsVal && parsVal->IsArray())) {
        THROW_EXC_TRC_WAR(std::logic_error, "Expected: Json Array .../retpars[]");
      }

      for (const Value* parVal = parsVal->Begin(); parVal != parsVal->End(); parVal++) {
        dpaRequestVect.push_back(encodeRequest(*parVal));
      }

      TRC_FUNCTION_LEAVE("");
      return dpaRequestVect;
    }

  protected:
    virtual std::string functionName() const = 0;

    // override if non empty par is required
    virtual void requestParameter(rapidjson::Document& par) const
    {
      par.SetObject();
    }

    virtual void parseResponse(const rapidjson::Value& v) = 0;

    void parseResponse(const DpaMessage & dpaResponse) override
    {
      TRC_FUNCTION_ENTER("");

      using namespace rapidjson;

      Document doc;

      std::string functionNameRsp(functionName());
      functionNameRsp += "_Response_rsp";

      std::string pnumStr, pcmdStr, rcodeStr, dpavalStr;
      pnumStr = encodeHexaNum(getPnum());
      pcmdStr = encodeHexaNum(uint8_t(getPcmd() | 0x80));  // set highest bit to satisfy driver checker
      rcodeStr = encodeHexaNum(getRcode());
      dpavalStr = encodeHexaNum(getDpaval());

      //nadr, hwpid is not interesting for drivers
      Pointer("/pnum").Set(doc, pnumStr);
      Pointer("/pcmd").Set(doc, pcmdStr);
      Pointer("/rcode").Set(doc, rcodeStr);
      Pointer("/dpaval").Set(doc, rcodeStr);

      if (getRdata().size() > 0) {
        Pointer("/rdata").Set(doc, encodeBinary(getRdata().data(), (int)getRdata().size()));
      }

      // original rawHdpRequest request passed for additional driver processing, e.g. sensor breakdown parsing
      if (m_rawHdpRequestAsStr.size() > 0) {
        Document rawHdpRequestDoc;
        rawHdpRequestDoc.Parse(m_rawHdpRequestAsStr);
        const Value & val = rawHdpRequestDoc;
        Pointer("/originalRequest").Set(doc, rawHdpRequestDoc);
      }

      std::string rawHdpResponse;
      StringBuffer buffer;
      Writer<rapidjson::StringBuffer> writer(buffer);
      doc.Accept(writer);
      rawHdpResponse = buffer.GetString();

      TRC_DEBUG(PAR(rawHdpResponse));

      try {
        std::string rsp;
        m_iJsRenderService->callFenced(getNadr(), getHwpid(), functionNameRsp, rawHdpResponse, rsp);

        Document rspDoc;
        rspDoc.Parse(rsp);

        parseResponse(rspDoc);
      }
      catch (std::exception &e) {
        CATCH_EXC_TRC_WAR(std::exception, e, "Driver response failure: ");
        //TODO special response error exc
        THROW_EXC_TRC_WAR(std::logic_error, "Driver response failure: " << e.what());
      }

      TRC_FUNCTION_LEAVE("");
    }

    private:
      // encode parameters for calling driver *_Request
      std::string requestParameter() const
      {
        using namespace rapidjson;
        Document par;
        requestParameter(par);

        std::string parStr;
        StringBuffer buffer;
        Writer<rapidjson::StringBuffer> writer(buffer);
        par.Accept(writer);
        parStr = buffer.GetString();

        return parStr;
      }

      // calling driver *_Request
      void driverRequest(std::string & driverRequestResult)
      {
        TRC_FUNCTION_ENTER("");

        using namespace rapidjson;

        std::string functionNameReq(functionName());
        functionNameReq += "_Request_req";

        // call request driver func, it returns result json in text form
        try {
          m_iJsRenderService->callFenced(getNadr(), getHwpid(), functionNameReq, requestParameter(), driverRequestResult);
        }
        catch (std::exception &e) {
          CATCH_EXC_TRC_WAR(std::exception, e, "Driver request failure: ");
          //TODO special request error exc
          THROW_EXC_TRC_WAR(std::logic_error, "Driver request failure: " << e.what());
        }

        TRC_DEBUG(PAR(driverRequestResult));
        TRC_FUNCTION_LEAVE("");
      }

      // convert RawHdp request to DpaMessage
      DpaMessage encodeRequest(const rapidjson::Value & msgVal)
      {
        TRC_FUNCTION_ENTER("");

        using namespace rapidjson;

        // convert from rawHdpRequest to dpaRequest and pass nadr and hwpid to be in DpaRequest (driver doesn't set them)
        if (const Value *val = Pointer("/pnum").Get(msgVal)) {
          parseHexaNum(m_pnum, val->GetString());
        }
        if (const Value *val = Pointer("/pcmd").Get(msgVal)) {
          parseHexaNum(m_pcmd, val->GetString());
        }

        DpaMessage dpaRequest;
        initRequestHeader(dpaRequest);

        int len = (int)sizeof(TDpaIFaceHeader);
        if (const Value *val = Pointer("/rdata").Get(msgVal)) {
          len += parseBinary(dpaRequest.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData, val->GetString(), DPA_MAX_DATA_LENGTH);
          dpaRequest.SetLength(sizeof(TDpaIFaceHeader) + len);
        }
        dpaRequest.SetLength(len);

        TRC_FUNCTION_LEAVE("");
        return dpaRequest;
      }

      // convert DpaMessage to RawHdp response
      void encodeResponse(const DpaMessage & response, rapidjson::Value & responseVal, rapidjson::Document::AllocatorType & a)
      {
        TRC_FUNCTION_ENTER("");

        using namespace rapidjson;

        int len = response.GetLength();

        if (len < sizeof(TDpaIFaceHeader) + 2 || len > sizeof(TDpaIFaceHeader) + DPA_MAX_DATA_LENGTH) {
          THROW_EXC_TRC_WAR(std::logic_error, "Invalid dpaResponse lenght: " << PAR(len));
        }

        std::string pnumStr, pcmdStr, rcodeStr, dpavalStr;
        pnumStr = encodeHexaNum(response.DpaPacket().DpaResponsePacket_t.PNUM);
        pcmdStr = encodeHexaNum(response.DpaPacket().DpaResponsePacket_t.PCMD);
        rcodeStr = encodeHexaNum(response.DpaPacket().DpaResponsePacket_t.ResponseCode);
        dpavalStr = encodeHexaNum(response.DpaPacket().DpaResponsePacket_t.DpaValue);

        //nadr, hwpid is not interesting for drivers
        Pointer("/pnum").Set(responseVal, pnumStr, a);
        Pointer("/pcmd").Set(responseVal, pcmdStr, a);
        Pointer("/rcode").Set(responseVal, rcodeStr, a);
        Pointer("/dpaval").Set(responseVal, rcodeStr, a);

        //if (len > sizeof(TDpaIFaceHeader) + 2) { //+ rcode + dpaval
        //  m_rdata = std::vector<uint8_t>(response.DpaMessage.Response.PData, rp.DpaMessage.Response.PData + static_cast<int>(len) - sizeof(TDpaIFaceHeader) - 2);
        //}

        //if (getRdata().size() > 0) {
        //  Pointer("/rdata").Set(doc, encodeBinary(getRdata().data(), (int)getRdata().size()));
        //}

        TRC_FUNCTION_LEAVE("");
      }
      
  };

  class RawHdp
  {
  public:
    RawHdp()
    {}
    int getNadr() const { return m_nadr; }
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

    const rapidjson::Document& encode()
    {
      using namespace rapidjson;

      uint16_t nadr16 = 0, hwpid16 = 0;
      uint8_t pnum = 0, pcmd = 0, rcode8 = 0, dpaval = 0;
      std::string nadrStr, pnumStr, pcmdStr, hwpidStr, rcodeStr, dpavalStr;

      nadr16 = m_dpaRequest.DpaPacket().DpaRequestPacket_t.NADR;
      pnum = m_dpaRequest.DpaPacket().DpaRequestPacket_t.PNUM;
      pcmd = m_dpaRequest.DpaPacket().DpaRequestPacket_t.PCMD;
      hwpid16 = m_dpaRequest.DpaPacket().DpaRequestPacket_t.HWPID;

      nadrStr = encodeHexaNum(nadr16);
      pnumStr = encodeHexaNum(pnum);
      pcmdStr = encodeHexaNum(pcmd);
      hwpidStr = encodeHexaNum(hwpid16);

      Pointer("/nadr").Set(m_doc, nadrStr);
      Pointer("/pnum").Set(m_doc, pnumStr);
      Pointer("/pcmd").Set(m_doc, pcmdStr);
      Pointer("/hwpid").Set(m_doc, hwpidStr);

      if (m_dpaRequest.GetLength() > 8) {
        Pointer("/rdata").Set(m_doc, encodeBinary(m_dpaRequest.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData, m_dpaRequest.GetLength() - 8));
      }

      return m_doc;
    }

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
