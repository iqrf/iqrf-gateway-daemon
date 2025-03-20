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
