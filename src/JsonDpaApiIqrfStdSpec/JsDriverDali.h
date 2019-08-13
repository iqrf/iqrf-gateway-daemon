#pragma once

#include "IDpaTransactionResult2.h"
#include "JsDriverSolver.h"
#include "Dali.h"
#include "JsonUtils.h"
#include <vector>

namespace iqrf
{
  namespace dali
  {
    ////////////////
    class JsDriverFrc : public Frc, public JsDriverSolver
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

      JsDriverFrc(IJsRenderService* iJsRenderService, int command, const std::vector<int> & selectedNodes)
        :JsDriverSolver(iJsRenderService)
        , Frc(command, selectedNodes)
      {}

      virtual ~JsDriverFrc() {}

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

    protected:
      std::string functionName() const override
      {
        return "iqrf.dali.Frc";
      }

      uint16_t getNadrDrv() const override
      {
        return (uint16_t)0; // coordinator
      }

      uint16_t getHwpidDrv() const
      {
        return (uint16_t)0xFFFF; // any
      }

      void preRequest(rapidjson::Document& requestParamDoc) override
      {
        using namespace rapidjson;

        Pointer("/command").Set(requestParamDoc, (int)m_daliCommand);
        
        Value selectedNodesArr;
        selectedNodesArr.SetArray();
        for (auto n : m_selectedNodes) {
          Value nVal;
          nVal.SetInt(n);
          selectedNodesArr.PushBack(nVal, requestParamDoc.GetAllocator());
        }
        Pointer("/selectedNodes").Set(requestParamDoc, selectedNodesArr);
      }

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

        const Value *itemsVal = Pointer("/answers").Get(responseResultDoc);
        if (!(itemsVal && itemsVal->IsArray())) {
          THROW_EXC_TRC_WAR(std::logic_error, "Expected: Json Array .../answers[]");
        }

        for (const Value *itemVal = itemsVal->Begin(); itemVal != itemsVal->End(); itemVal++) {
          
          if (!(itemVal && itemVal->IsObject())) {
            THROW_EXC_TRC_WAR(std::logic_error, "Expected: Json Array of Objects.../answers[{}]");
          }
          
          uint8_t status, value;
          status = jutils::getMemberAs<int>("status", *itemVal);
          value = jutils::getMemberAs<int>("value", *itemVal);

          CommandResponse commandResponse(status, value);
          m_items.push_back(commandResponse);
        }
      }

    };
    typedef std::unique_ptr<JsDriverFrc> JsDriverFrcPtr;

  } //namespace dali
} //namespace iqrf
