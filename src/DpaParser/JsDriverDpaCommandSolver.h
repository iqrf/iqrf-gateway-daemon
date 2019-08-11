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

  class JsDriverSolver
  {
  protected:

    IJsRenderService* m_iJsRenderService = nullptr;

  public:
    virtual std::string functionName() const = 0;
    virtual uint16_t getNadrDrv() const = 0;
    virtual uint16_t getHwpidDrv() const = 0;
    virtual void preRequest(rapidjson::Document & requestParamDoc) = 0;
    virtual void postRequest(rapidjson::Document & requestResultDoc) = 0;
    virtual void preResponse(rapidjson::Document & responseParamDoc) = 0;
    virtual void postResponse(rapidjson::Document & responseResultDoc) = 0;

    JsDriverSolver() = delete;
    JsDriverSolver(IJsRenderService* iJsRenderService)
      :m_iJsRenderService(iJsRenderService)
    {}
    
    void processRequestDrv()
    {
      TRC_FUNCTION_ENTER("");

      using namespace rapidjson;

      std::string functionNameReq(functionName());
      functionNameReq += "_Request_req";
      TRC_DEBUG(PAR(functionNameReq));

      Document requestParamDoc;

      preRequest(requestParamDoc);

      std::string requestParamStr;
      StringBuffer buffer;
      Writer<rapidjson::StringBuffer> writer(buffer);
      requestParamDoc.Accept(writer);
      requestParamStr = buffer.GetString();

      TRC_DEBUG(PAR(requestParamStr));

      std::string requestResultStr;
      try {
        m_iJsRenderService->callFenced(getNadrDrv(), getHwpidDrv(), functionNameReq, requestParamStr, requestResultStr);
      }
      catch (std::exception &e) {
        CATCH_EXC_TRC_WAR(std::exception, e, "Driver request failure: ");
        //TODO special request error exc
        THROW_EXC_TRC_WAR(std::logic_error, "Driver request failure: " << e.what());
      }

      TRC_DEBUG(PAR(requestResultStr));

      Document requestResultDoc;
      requestResultDoc.Parse(requestResultStr);

      postRequest(requestResultDoc);

      TRC_FUNCTION_LEAVE("");
    }

    void processResponseDrv()
    {
      TRC_FUNCTION_ENTER("");

      using namespace rapidjson;

      std::string functionNameRsp(functionName());
      functionNameRsp += "_Response_rsp";
      TRC_DEBUG(PAR(functionNameRsp));

      Document responseParamDoc;

      preResponse(responseParamDoc);

      std::string responseParamStr;
      StringBuffer buffer;
      Writer<rapidjson::StringBuffer> writer(buffer);
      responseParamDoc.Accept(writer);
      responseParamStr = buffer.GetString();

      TRC_DEBUG(PAR(responseParamStr));

      std::string responseResultStr;
      try {
        m_iJsRenderService->callFenced(getNadrDrv(), getHwpidDrv(), functionNameRsp, responseParamStr, responseResultStr);

      }
      catch (std::exception &e) {
        CATCH_EXC_TRC_WAR(std::exception, e, "Driver response failure: ");
        //TODO special response error exc
        THROW_EXC_TRC_WAR(std::logic_error, "Driver response failure: " << e.what());
      }

      TRC_DEBUG(PAR(responseResultStr));

      Document responseResultDoc;
      responseResultDoc.Parse(responseResultStr);

      postResponse(responseResultDoc);

      TRC_FUNCTION_LEAVE("");
    }
  };

  ///////////////////////////
  class JsDriverDpaCommandSolver : public JsDriverSolver, public DpaCommandSolver
  {
  protected:
    //std::string m_rawHdpRequestAsStr;
    IJsRenderService* m_iJsRenderService = nullptr;
    DpaMessage m_dpaRequest;
    rapidjson::Document m_originalRawHdpRequestDoc;

  public:
    virtual ~JsDriverDpaCommandSolver() {}

    JsDriverDpaCommandSolver(IJsRenderService* iJsRenderService, uint16_t nadr = -1)
      :DpaCommandSolver(nadr)
      ,JsDriverSolver(iJsRenderService)
      , m_iJsRenderService(iJsRenderService)
    {}

    JsDriverDpaCommandSolver(IJsRenderService* iJsRenderService, uint16_t nadr, uint16_t hwpid)
      :DpaCommandSolver(nadr, hwpid)
      , JsDriverSolver(iJsRenderService)
      , m_iJsRenderService(iJsRenderService)
    {}

    // override from JsDriverSolver
    //////////
    virtual uint16_t getNadrDrv() const { return getNadr(); }
    virtual uint16_t getHwpidDrv() const { return getHwpid(); }
    
    void preRequest(rapidjson::Document & requestParamDoc) override
    {
      TRC_FUNCTION_ENTER("");
      requestParameter(requestParamDoc);
      TRC_FUNCTION_LEAVE("");
    }
    
    void postRequest(rapidjson::Document & requestResultDoc) override
    {
      TRC_FUNCTION_ENTER("");

      using namespace rapidjson;

      // convert from rawHdpRequest to dpaRequest and pass nadr and hwpid to be in DpaRequest (driver doesn't set them)
      if (const Value *val = Pointer("/pnum").Get(requestResultDoc)) {
        parseHexaNum(m_pnum, val->GetString());
      }
      if (const Value *val = Pointer("/pcmd").Get(requestResultDoc)) {
        parseHexaNum(m_pcmd, val->GetString());
      }

      initRequestHeader(m_dpaRequest);

      int len = (int)sizeof(TDpaIFaceHeader);
      if (const Value *val = Pointer("/rdata").Get(requestResultDoc)) {
        len += parseBinary(m_dpaRequest.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData, val->GetString(), DPA_MAX_DATA_LENGTH);
        m_dpaRequest.SetLength(sizeof(TDpaIFaceHeader) + len);
      }
      m_dpaRequest.SetLength(len);

      m_rawHdpRequestDoc.Swap(requestResultDoc);

      TRC_FUNCTION_LEAVE("");
    }

    void preResponse(rapidjson::Document & responseParamDoc) override
    {
      TRC_FUNCTION_ENTER("");

      using namespace rapidjson;

      std::string pnumStr, pcmdStr, rcodeStr, dpavalStr;
      pnumStr = encodeHexaNum(getPnum());
      pcmdStr = encodeHexaNum(uint8_t(getPcmd() | 0x80));  // set highest bit to satisfy driver checker
      rcodeStr = encodeHexaNum(getRcode());
      dpavalStr = encodeHexaNum(getDpaval());

      //nadr, hwpid is not interesting for drivers
      Pointer("/pnum").Set(responseParamDoc, pnumStr);
      Pointer("/pcmd").Set(responseParamDoc, pcmdStr);
      Pointer("/rcode").Set(responseParamDoc, rcodeStr);
      Pointer("/dpaval").Set(responseParamDoc, rcodeStr);

      if (getRdata().size() > 0) {
        Pointer("/rdata").Set(responseParamDoc, encodeBinary(getRdata().data(), (int)getRdata().size()));
      }

      // original rawHdpRequest request passed for additional driver processing, e.g. sensor breakdown parsing
      Pointer("/originalRequest").Set(responseParamDoc, m_originalRawHdpRequestDoc);

      TRC_FUNCTION_LEAVE("");
    }

    void postResponse(rapidjson::Document & responseResultDoc) override
    {
      TRC_FUNCTION_ENTER("");
      parseResponse(responseResultDoc);
      TRC_FUNCTION_LEAVE("");
    }

    // encode DpaMessage by driver function *_Request returning directly RawHdp
    DpaMessage encodeRequest() override
    {
      TRC_FUNCTION_ENTER("");
      processRequestDrv();
      TRC_FUNCTION_LEAVE("");
      return m_dpaRequest;
    }

  protected:

    // override if non empty par is required
    virtual void requestParameter(rapidjson::Document& par) const
    {
      par.SetObject();
    }

    virtual void parseResponse(const rapidjson::Value& v) = 0;

    void parseResponse(const DpaMessage & dpaResponse) override
    {
      TRC_FUNCTION_ENTER("");
      processResponseDrv();
      TRC_FUNCTION_LEAVE("");
    }

    private:
  };
}
