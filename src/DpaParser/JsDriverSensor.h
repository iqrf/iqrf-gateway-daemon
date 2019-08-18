#pragma once

#include "JsDriverDpaCommandSolver.h"
#include "JsDriverStandardFrcSolver.h"
#include "Sensor.h"
#include "JsonUtils.h"

namespace iqrf
{
  namespace sensor
  {
    namespace item {
      ///////
      class JsDriverSensor : public Sensor
      {
      public:
        JsDriverSensor(const rapidjson::Value& v)
          : Sensor()
        {
          using namespace rapidjson;

          //TODO id is not set by driver
          m_sid = jutils::getPossibleMemberAs<std::string>("id", v, m_sid);
          m_type = jutils::getMemberAs<int>("type", v);
          m_name = jutils::getMemberAs<std::string>("name", v);
          m_shortName = jutils::getMemberAs<std::string>("shortName", v);
          m_unit = jutils::getMemberAs<std::string>("unit", v);
          //TODO id is not set by driver
          m_decimalPlaces = jutils::getPossibleMemberAs<int>("decimalPlaces", v, m_decimalPlaces);
          {
            auto vect = jutils::getPossibleMemberAsVector<int>("frcs", v);
            m_frcs = std::set<int>(vect.begin(), vect.end());
          }
          const Value *val = Pointer("/value").Get(v);
          if (val) {
              m_value = val->GetDouble();
              m_valueSet = true;
          }
        }

        virtual ~JsDriverSensor() {}
      };
    } // namespace item

    ////////////////
    class JsDriverEnumerate : public Enumerate, public JsDriverDpaCommandSolver
    {
    public:
      JsDriverEnumerate(IJsRenderService* iJsRenderService, uint16_t nadr)
        :JsDriverDpaCommandSolver(iJsRenderService, nadr)
      {}

      virtual ~JsDriverEnumerate() {}

    protected:
      std::string functionName() const override
      {
        return "iqrf.sensor.Enumerate";
      }

      void parseResponse(const rapidjson::Value& v) override
      {
        using namespace rapidjson;

        const auto val = Pointer("/sensors").Get(v)->GetArray();
        int index = 0; //use index in name as there may be more sensors of the same type;
        for (auto itr = val.Begin(); itr != val.End(); ++itr) {
          item::SensorPtr sen;
          if (!itr->IsNull()) {
            sen.reset(shape_new item::JsDriverSensor(*itr));
          }
          m_sensors.push_back(std::move(sen));
        }
      }
    };
    typedef std::unique_ptr<JsDriverEnumerate> JsDriverEnumeratePtr;

    ////////////////
    class JsDriverFrc : public Frc, public JsDriverStandardFrcSolver
    {
    public:
      JsDriverFrc(IJsRenderService* iJsRenderService, int sensorType, int sensorIndex, uint8_t frcCommand)
        :JsDriverStandardFrcSolver(iJsRenderService)
        , Frc(sensorType, sensorIndex, frcCommand)
      {}

      JsDriverFrc(IJsRenderService* iJsRenderService, int sensorType, int sensorIndex, uint8_t frcCommand, const std::vector<int> & selectedNodes)
        :JsDriverStandardFrcSolver(iJsRenderService)
        , Frc(sensorType, sensorIndex, frcCommand, selectedNodes)
      {}

      JsDriverFrc(IJsRenderService* iJsRenderService, int sensorType, int sensorIndex, uint8_t frcCommand, const std::vector<int> & selectedNodes, int time, int control)
        :JsDriverStandardFrcSolver(iJsRenderService)
        , Frc(sensorType, sensorIndex, frcCommand, selectedNodes, time, control)
      {}

      JsDriverFrc(IJsRenderService* iJsRenderService, int sensorType, int sensorIndex, uint8_t frcCommand, int time, int control)
        :JsDriverStandardFrcSolver(iJsRenderService)
        , Frc(sensorType, sensorIndex, frcCommand, time, control)
      {}

      // for using with ApiMsgIqrfSensorFrc - members are not interpreted
      JsDriverFrc(IJsRenderService* iJsRenderService, const rapidjson::Value & val)
        :JsDriverStandardFrcSolver(iJsRenderService)
        , Frc()
      {}

      virtual ~JsDriverFrc() {}

    protected:
      std::string functionName() const override
      {
        return "iqrf.sensor.Frc";
      }

      void preRequest(rapidjson::Document& requestParamDoc) override
      {
        using namespace rapidjson;

        Pointer("/sensorType").Set(requestParamDoc, m_sensorType);
        Pointer("/sensorIndex").Set(requestParamDoc, m_sensorIndex);
        Pointer("/frcCommand").Set(requestParamDoc, m_frcCommand);
        if (m_selectedNodesSet) {
          Value selectedNodesArr;
          selectedNodesArr.SetArray();
          for (auto n : m_selectedNodes) {
            Value nVal;
            nVal.SetInt(n);
            selectedNodesArr.PushBack(nVal, requestParamDoc.GetAllocator());
          }
          Pointer("/selectedNodes").Set(requestParamDoc, selectedNodesArr);
        }
        if (m_sleepAfterFrcSet) {
          Pointer("/sleepAfterFrc/control").Set(requestParamDoc, m_control);
          Pointer("/sleepAfterFrc/time").Set(requestParamDoc, m_time);
        }
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

          //CommandResponse commandResponse(status, value);
          //m_items.push_back(commandResponse);
        }
      }

      //void parseResponse(const rapidjson::Value& v) override
      //{
      //  using namespace rapidjson;

      //  const auto val = Pointer("/sensors").Get(v)->GetArray();
      //  int index = 0; //use index in name as there may be more sensors of the same type;
      //  for (auto itr = val.Begin(); itr != val.End(); ++itr) {
      //    SensorPtr sen;
      //    if (!itr->IsNull()) {
      //      sen.reset(shape_new JsDriverSensor(*itr));
      //    }
      //    m_sensors.push_back(std::move(sen));
      //  }
      //}
    };
    typedef std::unique_ptr<JsDriverFrc> JsDriverFrcPtr;

  } //namespace sensor
} //namespace iqrf
