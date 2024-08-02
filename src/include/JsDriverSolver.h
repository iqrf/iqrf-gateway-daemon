/**
 * Copyright 2015-2025 IQRF Tech s.r.o.
 * Copyright 2019-2025 MICRORISC s.r.o.
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

    // request processing
    rapidjson::Document m_requestParamDoc;
    std::string m_requestParamStr;
    rapidjson::Document m_requestResultDoc;
    std::string m_requestResultStr;

    // response processing
    rapidjson::Document m_responseParamDoc;
    std::string m_responseParamStr;
    rapidjson::Document m_responseResultDoc;
    std::string m_responseResultStr;

  public:
    virtual std::string functionName() const = 0;
    virtual uint16_t getNadrDrv() const = 0;
    virtual uint16_t getHwpidDrv() const = 0;
    virtual void preRequest(rapidjson::Document & requestParamDoc) = 0;
    virtual void postRequest(const rapidjson::Document & requestResultDoc) = 0;
    virtual void preResponse(rapidjson::Document & responseParamDoc) = 0;
    virtual void postResponse(const rapidjson::Document & responseResultDoc) = 0;

    // request processing
  protected:
    // used by JsDriverStandardFrcSolver
    void setRequestParamDoc(const rapidjson::Value & requestParamVal) { m_requestParamDoc.CopyFrom(requestParamVal, m_requestParamDoc.GetAllocator()); }
  public:
    const rapidjson::Document & getRequestParamDoc() const { return m_requestParamDoc; }
    const std::string & getRequestParamStr() const { return m_requestParamStr; }
    const rapidjson::Document & getRequestResultDoc() const { return m_requestResultDoc; }
    const std::string & getRequestResultStr() const { return m_requestResultStr; }

    // response processing
    const rapidjson::Document & getResponseParamDoc() const { return m_responseParamDoc; }
    const std::string & getResponseParamStr() const { return m_responseParamStr; }
    const rapidjson::Document & getResponseResultDoc() const { return m_responseResultDoc; }
    const std::string & getResponseResultStr() const { return m_responseResultStr; }

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

      preRequest(m_requestParamDoc);

      StringBuffer buffer;
      Writer<rapidjson::StringBuffer> writer(buffer);
      m_requestParamDoc.Accept(writer);
      m_requestParamStr = buffer.GetString();

      TRC_DEBUG(PAR(m_requestParamStr));

      try {
        m_iJsRenderService->callContext(getNadrDrv(), getHwpidDrv(), functionNameReq, m_requestParamStr, m_requestResultStr);
      }
      catch (std::exception &e) {
        //TODO use dedicated exception to distinguish driver error (BAD_REQUEST)
        CATCH_EXC_TRC_WAR(std::exception, e, "Driver request failure: ");
        THROW_EXC_TRC_WAR(std::logic_error, "Driver request failure: " << e.what());
      }

      TRC_DEBUG(PAR(m_requestResultStr));

      m_requestResultDoc.Parse(m_requestResultStr);

      postRequest(m_requestResultDoc);

      TRC_FUNCTION_LEAVE("");
    }

    void processResponseDrv()
    {
      TRC_FUNCTION_ENTER("");

      using namespace rapidjson;

      std::string functionNameRsp(functionName());
      functionNameRsp += "_Response_rsp";
      TRC_DEBUG(PAR(functionNameRsp));

      preResponse(m_responseParamDoc);

      StringBuffer buffer;
      Writer<rapidjson::StringBuffer> writer(buffer);
      m_responseParamDoc.Accept(writer);
      m_responseParamStr = buffer.GetString();

      TRC_DEBUG(PAR(m_responseParamStr));

      try {
        m_iJsRenderService->callContext(getNadrDrv(), getHwpidDrv(), functionNameRsp, m_responseParamStr, m_responseResultStr);

      }
      catch (std::exception &e) {
        //TODO use dedicated exception to distinguish driver error (BAD_RESPONSE)
        CATCH_EXC_TRC_WAR(std::exception, e, "Driver response failure: ");
        THROW_EXC_TRC_WAR(std::logic_error, "Driver response failure: " << e.what());
      }

      TRC_DEBUG(PAR(m_responseResultStr));

      m_responseResultDoc.Parse(m_responseResultStr);

      postResponse(m_responseResultDoc);

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
        const Value *val = Pointer("/pNum").Get(rawHdpRequestVal);
        if (!(val && val->IsString())) {
          THROW_EXC_TRC_WAR(std::logic_error, "Expected: string /pNum");
        }
        HexStringConversion::parseHexaNum(pnum, val->GetString());
      }

      {
        const Value *val = Pointer("/pCmd").Get(rawHdpRequestVal);
        if (!(val && val->IsString())) {
          THROW_EXC_TRC_WAR(std::logic_error, "Expected: string /pCmd");
        }
        HexStringConversion::parseHexaNum(pcmd, val->GetString());
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
        len += HexStringConversion::parseBinary(dpaRequest.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData, val->GetString(), DPA_MAX_DATA_LENGTH);
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
      pnumStr = HexStringConversion::encodeHexaNum(pnum);
      pcmdStr = HexStringConversion::encodeHexaNum(pcmd);
      rcodeStr = HexStringConversion::encodeHexaNum(rcode);
      dpavalStr = HexStringConversion::encodeHexaNum(dpaval);

      //nadr, hwpid is not interesting for drivers
      Pointer("/pNum").Set(rawHdpResponseVal, pnumStr, a);
      Pointer("/pCmd").Set(rawHdpResponseVal, pcmdStr, a);
      Pointer("/rcode").Set(rawHdpResponseVal, rcodeStr, a);
      Pointer("/dpaval").Set(rawHdpResponseVal, rcodeStr, a);

      int rsphdr = (int)(sizeof(TDpaIFaceHeader) + 2); // sizeof(rcode) + sizeof(dpaval)
      if (dpaResponse.GetLength() > rsphdr) {
        Pointer("/rdata").Set(rawHdpResponseVal, HexStringConversion::encodeBinary(
          dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData, dpaResponse.GetLength() - rsphdr), a);
      }

      TRC_FUNCTION_LEAVE("");
    }


  };

}
