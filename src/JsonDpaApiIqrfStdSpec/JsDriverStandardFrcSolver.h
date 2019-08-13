#pragma once

#include "IDpaTransactionResult2.h"
#include "JsDriverSolver.h"
#include "Dali.h"
#include "JsonUtils.h"
#include <vector>

namespace iqrf
{
  // use to handle API msg to cope with Standard FRC (iqrf.sensor.Frc, iqrf.dali.Frc, ...)
  class JsDriverStandardFrcSolver : public JsDriverSolver
  {
  private:
    DpaMessage m_frcRequest;
    DpaMessage m_frcExtraRequest;
    DpaMessage m_frcResponse;
    DpaMessage m_frcExtraResponse;
    std::unique_ptr<IDpaTransactionResult2> m_frcDpaTransactionResult;
    std::unique_ptr<IDpaTransactionResult2> m_frcExtraDpaTransactionResult;
    rapidjson::Document m_responseResultDoc;

  public:

    JsDriverStandardFrcSolver(IJsRenderService* iJsRenderService)
      :JsDriverSolver(iJsRenderService)
    {}

    virtual ~JsDriverStandardFrcSolver() {}

    const DpaMessage & getFrcRequest() const { return m_frcRequest; }
    const DpaMessage & getFrcExtraRequest() const { return m_frcExtraRequest; }

    void setFrcDpaTransactionResult(std::unique_ptr<IDpaTransactionResult2> res)
    {
      m_frcDpaTransactionResult = std::move(res);

      if (!m_frcDpaTransactionResult->isResponded()) {
        THROW_EXC_TRC_WAR(std::logic_error, "No Frc response");
      }

      m_frcResponse = m_frcDpaTransactionResult->getResponse();
    }

    void setFrcExtraDpaTransactionResult(std::unique_ptr<IDpaTransactionResult2> res)
    {
      m_frcExtraDpaTransactionResult = std::move(res);

      if (!m_frcExtraDpaTransactionResult->isResponded()) {
        THROW_EXC_TRC_WAR(std::logic_error, "No Frc Extra response");
      }

      m_frcExtraResponse = m_frcExtraDpaTransactionResult->getResponse();
    }

    std::unique_ptr<IDpaTransactionResult2> moveFrcDpaTransactionResult()
    {
      return std::move(m_frcDpaTransactionResult);
    }

    std::unique_ptr<IDpaTransactionResult2> moveFrcExtraDpaTransactionResult()
    {
      return std::move(m_frcExtraDpaTransactionResult);
    }

    const rapidjson::Document & getResponseResultDoc() { return m_responseResultDoc; }

    //// TODO function name
    //void setFunctionName(const std::string& functionName)
    //{
    //  //set function name to be used by driver *_Request, *_Response
    //}

    //// TODO function name
    //void setParam()
    //{
    //  //set param to be passed to driver _Request
    //}

  protected:
    //std::string functionName() const override
    //{
    //  return "iqrf.dali.Frc";
    //}

    uint16_t getNadrDrv() const override
    {
      return (uint16_t)0; // coordinator
    }

    uint16_t getHwpidDrv() const
    {
      return (uint16_t)0xFFFF; // any
    }

    //void preRequest(rapidjson::Document& requestParamDoc) override
    //{
    //  using namespace rapidjson;

    //  // set passed params
    //}

    void postRequest(rapidjson::Document& requestResultDoc) override
    {
      using namespace rapidjson;

      if (const Value *val0 = Pointer("/retpars/0").Get(requestResultDoc)) {
        uint8_t pnum, pcmd;
        rawHdp2dpaRequest(m_frcRequest, getNadrDrv(), pnum, pcmd, getHwpidDrv(), *val0);
      }
      else {
        THROW_EXC_TRC_WAR(std::logic_error, "Expected: Json Array .../retpars[0]");
      }

      if (const Value *val1 = Pointer("/retpars/1").Get(requestResultDoc)) {
        uint8_t pnum, pcmd;
        rawHdp2dpaRequest(m_frcExtraRequest, getNadrDrv(), pnum, pcmd, getHwpidDrv(), *val1);
      }
      else {
        THROW_EXC_TRC_WAR(std::logic_error, "Expected: Json Array .../retpars[1]");
      }
    }

    void preResponse(rapidjson::Document& responseParamDoc) override
    {
      using namespace rapidjson;

      if (!m_frcDpaTransactionResult->isResponded()) {
        THROW_EXC_TRC_WAR(std::logic_error, "No Frc response");
      }
      {
        Value val;
        dpa2rawHdpResponse(m_frcResponse, val, responseParamDoc.GetAllocator());
        Pointer("/responseFrcSend").Set(responseParamDoc, val);
      }

      if (!m_frcExtraDpaTransactionResult->isResponded()) {
        THROW_EXC_TRC_WAR(std::logic_error, "No Frc response");
      }
      {
        Value val;
        dpa2rawHdpResponse(m_frcExtraResponse, val, responseParamDoc.GetAllocator());
        Pointer("/responseFrcExtraResult").Set(responseParamDoc, val);
      }
    }

    void postResponse(rapidjson::Document& responseResultDoc) override
    {
      using namespace rapidjson;

      m_responseResultDoc.CopyFrom(responseResultDoc, m_responseResultDoc.GetAllocator());
    }

  };

} //namespace iqrf
