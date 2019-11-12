#pragma once

#include "IDpaTransactionResult2.h"
#include "JsDriverSolver.h"
#include "Dali.h"
#include "JsonUtils.h"
#include <vector>

namespace iqrf
{
  // use as common functionality predecessor for JsDriverDali, JsDriverSensor
  class JsDriverStandardFrcSolver : public JsDriverSolver
  {
  private:
    std::string m_functionName;
    DpaMessage m_frcRequest;
    DpaMessage m_frcExtraRequest;
    DpaMessage m_frcResponse;
    DpaMessage m_frcExtraResponse;
    std::unique_ptr<IDpaTransactionResult2> m_frcDpaTransactionResult;
    std::unique_ptr<IDpaTransactionResult2> m_frcExtraDpaTransactionResult;
    rapidjson::Document m_frcRequestResult0Doc;
    rapidjson::Document m_frcResponseResultDoc;

  protected:
    // used by sensor::JsDriverFrc, dali::JsDriverFrc
    JsDriverStandardFrcSolver(IJsRenderService* iJsRenderService)
      :JsDriverSolver(iJsRenderService)
    {}

  public:
    JsDriverStandardFrcSolver(IJsRenderService* iJsRenderService, const std::string & functionName, const rapidjson::Value & val)
      :JsDriverSolver(iJsRenderService)
      , m_functionName(functionName)
    {
      setRequestParamDoc(val);
    }

    virtual ~JsDriverStandardFrcSolver() {}

    const DpaMessage & getFrcRequest() const { return m_frcRequest; }
    const DpaMessage & getFrcExtraRequest() const { return m_frcExtraRequest; }

    const rapidjson::Document & getRequestResult0Doc() const { return m_frcRequestResult0Doc; }
    const rapidjson::Document & getResponseResultDoc() const { return m_frcResponseResultDoc; }

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
    
    // partial JSON parse to get nadrs of returned results
    rapidjson::Document getExtFormat(const std::string & resultArrayKey, const std::string & resultItemKey)
    {
      using namespace rapidjson;
      Document doc;
      doc.SetArray();
      
      // get nadrs from selectedNodes if applied
      std::set<int> selectedNodesSet;
      const Value *selectedNodesVal = Pointer("/selectedNodes").Get(getRequestParamDoc());
      if (selectedNodesVal && selectedNodesVal->IsArray()) {
        for (const Value *nadrVal = selectedNodesVal->Begin(); nadrVal != selectedNodesVal->End(); nadrVal++) {
          if (nadrVal->IsInt()) {
            selectedNodesSet.insert(nadrVal->GetInt());
          }
          else {
            THROW_EXC_TRC_WAR(std::logic_error, "Expected: Json Array of Int .../selectedNodes[]");
          }
        }
      }

      Value *arrayVal = Pointer(resultArrayKey).Get(m_frcResponseResultDoc);
      if (!(arrayVal && arrayVal->IsArray())) {
        THROW_EXC_TRC_WAR(std::logic_error, "Expected: Json Array ..." << resultArrayKey << "[]");
      }

      if (arrayVal->Size() > 0) { // is there something in the array?
        if (selectedNodesSet.size() <= 0) {
          // selective FRC
          auto nadrIt = selectedNodesSet.begin();
          for (Value *itemVal = arrayVal->Begin() + 1; //skip index 0 as driver returns first null as a result of general FRC 
            itemVal != arrayVal->End(); itemVal++) {

            if (nadrIt == selectedNodesSet.end()) {
              THROW_EXC_TRC_WAR(std::logic_error, "Inconsistent .../selectedNodes[] and ..." << resultArrayKey << "[]");
            }

            Value sensorVal;
            Pointer("/nAdr").Set(sensorVal, *nadrIt++, doc.GetAllocator());
            Pointer(resultItemKey).Set(sensorVal, *itemVal, doc.GetAllocator());
            doc.PushBack(sensorVal, doc.GetAllocator());
          }
        }
        else {
          // non-selective FRC //TODO does FRC always starts from nadr=0?
          int nadr = 0;
          for (Value *itemVal = arrayVal->Begin(); itemVal != arrayVal->End(); itemVal++) {
            Value sensorVal;
            Pointer("/nAdr").Set(sensorVal, nadr++, doc.GetAllocator());
            Pointer(resultItemKey).Set(sensorVal, *itemVal, doc.GetAllocator());
            doc.PushBack(sensorVal, doc.GetAllocator());
          }
        }
      }

      return doc;
    }

  protected:

    uint16_t getNadrDrv() const override
    {
      return (uint16_t)0; // coordinator
    }

    uint16_t getHwpidDrv() const
    {
      return (uint16_t)0xFFFF; // any
    }

    std::string functionName() const override
    {
      return m_functionName;
    }

    void preRequest(rapidjson::Document& requestResultDoc) override
    {
      // set in ctor by setRequestParamDoc(val);
    }

    void postRequest(const rapidjson::Document& requestResultDoc) override
    {
      using namespace rapidjson;

      if (const Value *val0 = Pointer("/retpars/0").Get(requestResultDoc)) {
        uint8_t pnum, pcmd;
        rawHdp2dpaRequest(m_frcRequest, getNadrDrv(), pnum, pcmd, getHwpidDrv(), *val0);
        m_frcRequestResult0Doc.CopyFrom(*val0, m_frcRequestResult0Doc.GetAllocator());
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

      // some std FRC needs requestParam to parse response
      responseParamDoc.CopyFrom(getRequestParamDoc(), responseParamDoc.GetAllocator());

      if (!m_frcDpaTransactionResult->isResponded()) {
        THROW_EXC_TRC_WAR(std::logic_error, "No Frc response");
      }
      {
        Value val;
        dpa2rawHdpResponse(m_frcResponse, val, responseParamDoc.GetAllocator());
        Pointer("/responseFrcSend").Set(responseParamDoc, val);
      }

      if (m_frcExtraDpaTransactionResult) {
        // optional extra result
        if (!m_frcExtraDpaTransactionResult->isResponded()) {
          THROW_EXC_TRC_WAR(std::logic_error, "No Frc response");
        }
        {
          Value val;
          dpa2rawHdpResponse(m_frcExtraResponse, val, responseParamDoc.GetAllocator());
          Pointer("/responseFrcExtraResult").Set(responseParamDoc, val);
        }
      }

      Pointer("/frcSendRequest").Set(responseParamDoc, m_frcRequestResult0Doc);
    }

    void postResponse(const rapidjson::Document& responseResultDoc) override
    {
      m_frcResponseResultDoc.CopyFrom(responseResultDoc, m_frcResponseResultDoc.GetAllocator());
    }

  };

} //namespace iqrf
