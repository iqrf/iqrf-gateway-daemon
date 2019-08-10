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

  class JsDriverSolver
  {
  protected:
    
    IJsRenderService* m_iJsRenderService = nullptr;

  public:
    virtual std::string functionNameDrv() const = 0;
    virtual uint16_t getNadrDrv() const = 0;
    virtual uint16_t getHwpidDrv() const = 0;

    virtual void preRequest(rapidjson::Document & requestParamDoc) = 0;
    virtual void postRequest(rapidjson::Document & requestResultDoc) = 0;

    void processRequest()
    {
      TRC_FUNCTION_ENTER("");

      using namespace rapidjson;

      std::string functionNameReq(functionNameDrv());
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
    
    virtual void preResponse(rapidjson::Document & responseParamDoc) = 0;
    virtual void postResponse(rapidjson::Document & responseResultDoc) = 0;

    void processResponse()
    {
      TRC_FUNCTION_ENTER("");

      using namespace rapidjson;

      std::string functionNameRsp(functionNameDrv());
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
        std::string rsp;
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

      postRequest(responseResultDoc);

      TRC_FUNCTION_LEAVE("");
    }

  };
}
