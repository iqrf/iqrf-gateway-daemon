#pragma once

#include "JsDriverStandardFrcSolver.h"
#include "Dali.h"
#include "JsonUtils.h"
#include <vector>

namespace iqrf
{
  namespace dali
  {
    ////////////////
    class JsDriverFrc : public Frc, public JsDriverStandardFrcSolver
    {
    public:

      JsDriverFrc(IJsRenderService* iJsRenderService, int command, const std::vector<int> & selectedNodes)
        :JsDriverStandardFrcSolver(iJsRenderService)
        , Frc(command, selectedNodes)
      {}

      virtual ~JsDriverFrc() {}

    protected:
      std::string functionName() const override
      {
        return "iqrf.dali.Frc";
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

      void postResponse(const rapidjson::Document& responseResultDoc) override
      {
        using namespace rapidjson;

        JsDriverStandardFrcSolver::postResponse(responseResultDoc);

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
