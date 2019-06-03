#pragma once

#include "JsDriverRequest.h"
#include "JsonUtils.h"
#include <vector>
#include <sstream>
#include <iomanip>
#include <set>

namespace iqrf
{
  namespace sensor
  {
    ////////////////
    class Enumerate : public JsDriverRequest, public IEnumerateService::IStandardSensorData
    {
    public:
      class Sensor : public IEnumerateService::IStandardSensorData::ISensor
      {
      private:
        std::string m_sid;
        int m_type;
        std::string m_name;
        std::string m_shortName;
        std::string m_unit;
        int m_decimalPlaces;
        std::set<int> m_frcs;
        //TODO breakdown - array : [optional] see <iqrf.sensor.ReadSensorsWithTypes_Response> for more information.
      
      public:
        Sensor(const rapidjson::Value& v)
        {
          using namespace rapidjson;

          m_sid = jutils::getMemberAs<std::string>("id", v);
          m_type = jutils::getMemberAs<int>("type", v);
          m_name = jutils::getMemberAs<std::string>("name", v);
          m_shortName = jutils::getMemberAs<std::string>("shortName", v);
          m_unit = jutils::getMemberAs<std::string>("unit", v);
          m_decimalPlaces = jutils::getMemberAs<int>("decimalPlaces", v);
          {
            auto vect = jutils::getMemberAsVector<int>("frcs", v);
            m_frcs = std::set<int>(vect.begin(), vect.end());
          }
        }

        const std::string & getSid() const override { return m_sid; }
        int getType() const  override { return m_type; }
        const std::string & getName() const  override { return m_name; }
        const std::string & getShortName() const  override { return m_shortName; }
        const std::string & getUnit() const  override { return m_unit; }
        int getDecimalPlaces() const  override { return m_decimalPlaces; }
        const std::set<int> & getFrcs() const  override { return m_frcs; }
        //TODO breakdown - array : [optional] see <iqrf.sensor.ReadSensorsWithTypes_Response> for more information.

        virtual ~Sensor() {}
      };

    private:
      std::vector<IEnumerateService::IStandardSensorData::ISensorPtr> m_sensors;

    public:
      Enumerate(uint16_t nadr)
        :JsDriverRequest(nadr)
      {
      }

      virtual ~Enumerate() {}

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
          std::unique_ptr<IEnumerateService::IStandardSensorData::ISensor> sen;
          if (!itr->IsNull()) {
            sen.reset(shape_new Sensor(*itr));
          }
          m_sensors.push_back(std::move(sen));
        }
      }

      // get data as returned from driver
      const std::vector<IEnumerateService::IStandardSensorData::ISensorPtr> & getSensors() const { return m_sensors; }

    };


  } //namespace sensor
} //namespace iqrf
