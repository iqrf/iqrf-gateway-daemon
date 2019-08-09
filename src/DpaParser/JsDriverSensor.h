#pragma once

#include "JsDriverDpaCommandSolver.h"
#include "Sensor.h"
#include "JsonUtils.h"

namespace iqrf
{
  namespace sensor
  {
    ////////////////
    class JsDriverEnumerate : public Enumerate, public JsDriverDpaCommandSolver
    {
    public:
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
            auto vect = jutils::getMemberAsVector<int>("frcs", v);
            m_frcs = std::set<int>(vect.begin(), vect.end());
          }
        }

        virtual ~JsDriverSensor() {}
      };

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
          SensorPtr sen;
          if (!itr->IsNull()) {
            sen.reset(shape_new JsDriverSensor(*itr));
          }
          m_sensors.push_back(std::move(sen));
        }
      }
    };
    typedef std::unique_ptr<JsDriverEnumerate> JsDriverEnumeratePtr;

  } //namespace sensor
} //namespace iqrf
