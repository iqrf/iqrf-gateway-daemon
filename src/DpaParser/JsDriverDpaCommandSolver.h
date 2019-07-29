#pragma once

#include "IJsRenderService.h"
#include "DpaCommandSolver.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/pointer.h"

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

    JsDriverDpaCommandSolver(IJsRenderService* iJsRenderService, uint16_t nadr)
      :DpaCommandSolver(nadr)
      , m_iJsRenderService(iJsRenderService)
    {}

    JsDriverDpaCommandSolver(IJsRenderService* iJsRenderService, uint16_t nadr, uint16_t hwpid)
      :DpaCommandSolver(nadr, hwpid)
      , m_iJsRenderService(iJsRenderService)
    {}

    DpaMessage encodeRequest() override
    {
      TRC_FUNCTION_ENTER("");

      using namespace rapidjson;

      std::string functionNameReq(functionName());
      functionNameReq += "_Request_req";

      // call request driver func, it returns rawHdpRequest format in text form
      try {
        m_iJsRenderService->callFenced(getNadr(), getHwpid(), functionNameReq, requestParameter(), m_rawHdpRequestAsStr);
      }
      catch (std::exception &e) {
        CATCH_EXC_TRC_WAR(std::exception, e, "Driver request failure: ");
        //TODO special request error exc
        THROW_EXC_TRC_WAR(std::logic_error, "Driver request failure: " << e.what());
      }

      TRC_DEBUG(PAR(m_rawHdpRequestAsStr));

      // convert from rawHdpRequest to dpaRequest and pass nadr and hwpid to be in dapaRequest (driver doesn't set them)
      Document doc;
      doc.Parse(m_rawHdpRequestAsStr);

      if (Value *val = Pointer("/pnum").Get(doc)) {
        parseHexaNum(m_pnum, val->GetString());
      }
      if (Value *val = Pointer("/pcmd").Get(doc)) {
        parseHexaNum(m_pcmd, val->GetString());
      }

      DpaMessage dpaRequest;
      initRequestHeader(dpaRequest);

      int len = (int)sizeof(TDpaIFaceHeader);
      if (Value *val = Pointer("/rdata").Get(doc)) {
        len += parseBinary(dpaRequest.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData, val->GetString(), DPA_MAX_DATA_LENGTH);
        dpaRequest.SetLength(sizeof(TDpaIFaceHeader) + len);
      }
      dpaRequest.SetLength(len);

      TRC_FUNCTION_LEAVE("");
      return dpaRequest;
    }

  protected:
    virtual std::string functionName() const = 0;

    virtual std::string requestParameter() const = 0;

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
  };
}
