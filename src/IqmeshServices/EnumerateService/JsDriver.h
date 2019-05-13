#pragma once

#include "Trace.h"
#include "JsonUtils.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/pointer.h"
#include "JsdConversion.h"
#include <vector>
#include <string>

namespace iqrf {
  class JsDpaRequest
  {
  protected:
    uint16_t m_nadr;
    uint16_t m_hwpid;
    rapidjson::Document m_requestParameter;
    DpaMessage m_dpaRequest;
    std::string m_rawHdpRequest;
    bool m_valid;
    IJsRenderService* m_iJsRenderService = nullptr;

  public:
    virtual std::string getFunctionName() const = 0;
    virtual void parseResponse(const rapidjson::Value& v) = 0;

    //JsDpaRequest() delete;

    JsDpaRequest(uint16_t nadr, uint16_t hwpid, IJsRenderService* iJsRenderService)
      :m_nadr(nadr)
      ,m_hwpid(hwpid)
      ,m_valid(false)
      ,m_iJsRenderService(iJsRenderService)
    {
      m_requestParameter.SetObject();
    }

    bool getValid() const { return m_valid; }

    const DpaMessage & dpaRequest()
    {
      TRC_FUNCTION_ENTER("")

      using namespace rapidjson;

      std::string functionNameReq(getFunctionName());
      functionNameReq += "_Request_req";

      // call request driver func, it returns rawHdpRequest format in text form
      try {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        m_requestParameter.Accept(writer);

        m_iJsRenderService->call(functionNameReq, buffer.GetString(), m_rawHdpRequest);
      }
      catch (std::exception &e) {
        CATCH_EXC_TRC_WAR(std::exception, e, "Driver request failure: ");
        //TODO special request error exc
        THROW_EXC_TRC_WAR(std::exception, "Driver request failure: " << e.what());
      }

      TRC_DEBUG(PAR(m_rawHdpRequest));

      // convert from rawHdpRequest to dpaRequest and pass nadr and hwpid to be in dapaRequest (driver doesn't set them)
      Document doc;
      doc.Parse(m_rawHdpRequest);

      uint8_t pnum = 0, pcmd = 0;

      if (Value *val = Pointer("/pnum").Get(doc)) {
        parseHexaNum(pnum, val->GetString());
      }
      if (Value *val = Pointer("/pcmd").Get(doc)) {
        parseHexaNum(pcmd, val->GetString());
      }

      uint8_t* p0 = m_dpaRequest.DpaPacket().Buffer;
      uint8_t* p = p0;

      *p++ = m_nadr & 0xff;
      *p++ = (m_nadr >> 8) & 0xff;
      *p++ = pnum;
      *p++ = pcmd;
      *p++ = m_hwpid & 0xff;
      *p++ = (m_hwpid >> 8) & 0xff;

      if (Value *val = Pointer("/rdata").Get(doc)) {
        int len = parseBinary(p, val->GetString(), 0xFF);
        p += len;
      }

      m_dpaRequest.SetLength(p - p0);

      TRC_FUNCTION_LEAVE("");
      return m_dpaRequest;
    }

    void dpaResponse(const DpaMessage & dpaResponse)
    {
      TRC_FUNCTION_ENTER("");

      using namespace rapidjson;

      std::string functionNameRsp(getFunctionName());
      functionNameRsp += "_Response_rsp";

      //process response
      int rcode = -1;

      // get rawHdpResponse in text form
      Document doc;
      const uint8_t* p = dpaResponse.DpaPacket().Buffer;
      int len = dpaResponse.GetLength();

      if (len < 8) {
        THROW_EXC_TRC_WAR(std::exception, "Invalid dpaResponse");
      }
      uint8_t pnum = 0, pcmd = 0, rcode8 = 0, dpaval = 0;
      std::string pnumStr, pcmdStr, rcodeStr, dpavalStr;

      m_nadr = p[0];
      m_nadr += p[1] << 8;
      pnum = p[2];
      pcmd = p[3];
      m_hwpid = p[4];
      m_hwpid += p[5] << 8;
      rcode8 = p[6];
      rcode = rcode8;
      dpaval = p[7];

      pnumStr = encodeHexaNum(pnum);
      pcmdStr = encodeHexaNum(pcmd);
      rcodeStr = encodeHexaNum(rcode8);
      dpavalStr = encodeHexaNum(dpaval);

      //nadr, hwpid is not interesting for drivers
      Pointer("/pnum").Set(doc, pnumStr);
      Pointer("/pcmd").Set(doc, pcmdStr);
      Pointer("/rcode").Set(doc, rcodeStr);
      Pointer("/dpaval").Set(doc, rcodeStr);

      if (len > 8) {
        Pointer("/rdata").Set(doc, encodeBinary(p + 8, static_cast<int>(len) - 8));
      }

      // original rawHdpRequest request passed for additional driver processing, e.g. sensor breakdown parsing
      if (m_rawHdpRequest.size() > 0) {
        Document rawHdpRequestDoc;
        rawHdpRequestDoc.Parse(m_rawHdpRequest);
        const Value & val = rawHdpRequestDoc;
        Pointer("/originalRequest").Set(doc, val);
      }

      std::string rawHdpResponse;
      rapidjson::StringBuffer buffer;
      rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
      doc.Accept(writer);
      rawHdpResponse = buffer.GetString();

      TRC_DEBUG(PAR(rawHdpResponse))

      if (0 != rcode) {
        //TODO special rcode error exc
        THROW_EXC_TRC_WAR(std::exception, "No response");
      }

      try {
        std::string rsp;
        //m_iJsRenderService->call(methodResponseName, rawHdpResponse, rspObjStr);
        m_iJsRenderService->callFenced(m_hwpid, functionNameRsp, rawHdpResponse, rsp);
        
        Document rspDoc;
        rspDoc.Parse(rsp);
        
        parseResponse(rspDoc);
      }
      catch (std::exception &e) {
        CATCH_EXC_TRC_WAR(std::exception, e, "Driver response failure: ");
        //TODO special response error exc
        THROW_EXC_TRC_WAR(std::exception, "Driver response failure: " << e.what());
      }

      TRC_FUNCTION_LEAVE("");
    }

  };

}
