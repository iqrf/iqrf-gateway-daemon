#pragma once

#include "IJsRenderService.h"
#include "DpaMessage.h"
#include "Trace.h"
#include "HexStringCoversion.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/pointer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include <string>

namespace iqrf {
  // auxiliar class to support JS driver functions invoking and processing of result
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
        THROW_EXC_TRC_WAR(std::logic_error, "Driver response failure: " << e.what());
      }

      TRC_DEBUG(PAR(responseResultStr));

      Document responseResultDoc;
      responseResultDoc.Parse(responseResultStr);

      postResponse(responseResultDoc);

      TRC_FUNCTION_LEAVE("");
    }

    // convert rawHdpRequest from driver to DpaRequest
    // out: dpaRequest - return result of conversion
    // in: nadr - pass nadr as RawHdp from driver doesn't contain it
    // out: pnum - return pnum as set by driver
    // out: pcmd return pcmd as set by driver
    // in: hwpid - pass hwpid as RawHdp from driver doesn't contain it 
    // in: rawHdpRequestDoc - RawHdp to be converted
    static void rawHdp2dpaRequest(DpaMessage & dpaRequest, uint16_t nadr, uint8_t & pnum, uint8_t & pcmd, uint16_t hwpid, const rapidjson::Value & rawHdpRequestVal)
    {
      TRC_FUNCTION_ENTER("");

      using namespace rapidjson;

      {
        const Value *val = Pointer("/pnum").Get(rawHdpRequestVal);
        if (!(val && val->IsString())) {
          THROW_EXC_TRC_WAR(std::logic_error, "Expected: string /pnum");
        }
        parseHexaNum(pnum, val->GetString());
      }

      {
        const Value *val = Pointer("/pcmd").Get(rawHdpRequestVal);
        if (!(val && val->IsString())) {
          THROW_EXC_TRC_WAR(std::logic_error, "Expected: string /pcmd");
        }
        parseHexaNum(pcmd, val->GetString());
      }

      dpaRequest.DpaPacket().DpaRequestPacket_t.NADR = nadr;
      dpaRequest.DpaPacket().DpaRequestPacket_t.PNUM = pnum;
      dpaRequest.DpaPacket().DpaRequestPacket_t.PCMD = pcmd;
      dpaRequest.DpaPacket().DpaRequestPacket_t.HWPID = hwpid;

      int len = (int)sizeof(TDpaIFaceHeader);
      if (const Value *val = Pointer("/rdata").Get(rawHdpRequestVal)) {
        if (!val->IsString()) {
          THROW_EXC_TRC_WAR(std::logic_error, "Expected: string /rdata");
        }
        len += parseBinary(dpaRequest.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData, val->GetString(), DPA_MAX_DATA_LENGTH);
        dpaRequest.SetLength(sizeof(TDpaIFaceHeader) + len);
      }
      dpaRequest.SetLength(len);

      TRC_FUNCTION_LEAVE("");
    }

    // convert DpaResponse to RawHdpResponse to be processed by driver
    // in: dpaResponse - to be converted
    // out: rawHdpResponseDoc - return result of conversion
    static void dpa2rawHdpResponse(const DpaMessage & dpaResponse, rapidjson::Value & rawHdpResponseVal, rapidjson::Document::AllocatorType & a)
    {
      TRC_FUNCTION_ENTER("");

      using namespace rapidjson;

      uint8_t pnum = dpaResponse.DpaPacket().DpaResponsePacket_t.PNUM;
      uint8_t pcmd = dpaResponse.DpaPacket().DpaResponsePacket_t.PCMD;
      uint8_t rcode = dpaResponse.DpaPacket().DpaResponsePacket_t.ResponseCode;
      uint8_t dpaval = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaValue;

      std::string pnumStr, pcmdStr, rcodeStr, dpavalStr;
      pnumStr = encodeHexaNum(pnum);
      pcmdStr = encodeHexaNum(pcmd);
      rcodeStr = encodeHexaNum(rcode);
      dpavalStr = encodeHexaNum(dpaval);

      //nadr, hwpid is not interesting for drivers
      Pointer("/pnum").Set(rawHdpResponseVal, pnumStr, a);
      Pointer("/pcmd").Set(rawHdpResponseVal, pcmdStr, a);
      Pointer("/rcode").Set(rawHdpResponseVal, rcodeStr, a);
      Pointer("/dpaval").Set(rawHdpResponseVal, rcodeStr, a);

      int rsphdr = (int)(sizeof(TDpaIFaceHeader) + 2); // sizeof(rcode) + sizeof(dpaval)
      if (dpaResponse.GetLength() > rsphdr) {
        Pointer("/rdata").Set(rawHdpResponseVal, encodeBinary(
          dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData, dpaResponse.GetLength() - rsphdr), a);
      }

      TRC_FUNCTION_LEAVE("");
    }


  };

}
