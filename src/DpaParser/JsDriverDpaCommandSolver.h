/**
 * Copyright 2015-2023 IQRF Tech s.r.o.
 * Copyright 2019-2023 MICRORISC s.r.o.
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
#include "JsDriverSolver.h"
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

  ///////////////////////////
  class JsDriverDpaCommandSolver : public JsDriverSolver, public DpaCommandSolver
  {
  private:
    IJsRenderService* m_iJsRenderService = nullptr;
    DpaMessage m_dpaRequest;

  public:
    virtual ~JsDriverDpaCommandSolver() {}

    JsDriverDpaCommandSolver(IJsRenderService* iJsRenderService, uint16_t nadr = -1)
      :JsDriverSolver(iJsRenderService)
      , DpaCommandSolver(nadr)
      , m_iJsRenderService(iJsRenderService)
    {}

    JsDriverDpaCommandSolver(IJsRenderService* iJsRenderService, uint16_t nadr, uint16_t hwpid)
      :JsDriverSolver(iJsRenderService)
      , DpaCommandSolver(nadr, hwpid)
      , m_iJsRenderService(iJsRenderService)
    {}

  protected:
    // overrides from JsDriverSolver
    //////////
    virtual uint16_t getNadrDrv() const { return getNadr(); }
    virtual uint16_t getHwpidDrv() const { return getHwpid(); }

    void preRequest(rapidjson::Document & requestParamDoc) override
    {
      TRC_FUNCTION_ENTER("");
      requestParameter(requestParamDoc);
      TRC_FUNCTION_LEAVE("");
    }

    void postRequest(const rapidjson::Document & requestResultDoc) override
    {
      TRC_FUNCTION_ENTER("");
      rawHdp2dpaRequest(m_dpaRequest, getNadrDrv(), m_pnum, m_pcmd, getHwpidDrv(), requestResultDoc);
      TRC_FUNCTION_LEAVE("");
    }

    void preResponse(rapidjson::Document & responseParamDoc) override
    {
      TRC_FUNCTION_ENTER("");

      dpa2rawHdpResponse(m_dpaTransactionResult2->getResponse(), responseParamDoc, responseParamDoc.GetAllocator());

      // original rawHdpRequest request passed for additional driver processing, e.g. sensor breakdown parsing
      rapidjson::Pointer("/originalRequest").Set(responseParamDoc, getRequestResultDoc());

      TRC_FUNCTION_LEAVE("");
    }

    void postResponse(const rapidjson::Document & responseResultDoc) override
    {
      TRC_FUNCTION_ENTER("");
      parseResponse(responseResultDoc);
      TRC_FUNCTION_LEAVE("");
    }

  protected:
    // overrides from DpaCommandSolver
    ////////
    // uses driver function *_Request function to get RawHdp and convert to DpaMessage
    void encodeRequest(DpaMessage & dpaRequest) override
    {
      TRC_FUNCTION_ENTER("");
      processRequestDrv();
      TRC_FUNCTION_LEAVE("");
      dpaRequest = m_dpaRequest;
    }

    // convert DpaMessage to RawHdp and uses driver function *_Response function to process
    void parseResponse(const DpaMessage & dpaResponse) override
    {
      TRC_FUNCTION_ENTER("");
      (void)dpaResponse; //silence -Wunused-parameter
      processResponseDrv();
      TRC_FUNCTION_LEAVE("");
    }

  protected:
    // virtuals to be overriden by successor classes
    ////////

    // override if non empty par is required
    virtual void requestParameter(rapidjson::Document& par) const
    {
      par.SetObject();
    }

    // provide parsing from Json after driver *_Response processing
    virtual void parseResponse(const rapidjson::Value& v) = 0;

  };
}
