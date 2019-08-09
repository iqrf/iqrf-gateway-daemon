#pragma once

#include "JsDriverDpaCommandSolver.h"
#include "Dali.h"
#include "JsonUtils.h"

namespace iqrf
{
  namespace dali
  {
    ////////////////
    class JsDriverFrc : public Frc, public JsDriverDpaCommandSolver
    {
    public:

      JsDriverFrc(IJsRenderService* iJsRenderService, int command, const std::vector<int> & selectedNodes)
        :JsDriverDpaCommandSolver(iJsRenderService, 0)
        , Frc(command, selectedNodes)
      {}

      virtual ~JsDriverFrc() {}

    protected:
      std::string functionName() const override
      {
        return "iqrf.dali.Frc";
      }

      void requestParameter(rapidjson::Document& par) const override
      {
        using namespace rapidjson;

        Pointer("/command").Set(par, (int)m_command);
        
        Value selectedNodesArr;
        selectedNodesArr.SetArray();
        for (auto n : m_selectedNodes) {
          Value nVal;
          nVal.SetInt(n);
          selectedNodesArr.PushBack(nVal, par.GetAllocator());
        }
        Pointer("/selectedNodes").Set(par, selectedNodesArr);
      }

      void parseResponse(const rapidjson::Value& v) override
      {
        using namespace rapidjson;

        //TODO
        //const auto val = Pointer("/sensors").Get(v)->GetArray();
        //int index = 0; //use index in name as there may be more sensors of the same type;
        //for (auto itr = val.Begin(); itr != val.End(); ++itr) {
        //  SensorPtr sen;
        //  if (!itr->IsNull()) {
        //    sen.reset(shape_new JsDriverSensor(*itr));
        //  }
        //  m_sensors.push_back(std::move(sen));
        //}
      }
    };
    typedef std::unique_ptr<JsDriverFrc> JsDriverFrcPtr;

  } //namespace dali
} //namespace iqrf
