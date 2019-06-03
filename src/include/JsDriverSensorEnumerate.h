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
    class Enumerate : public JsDriverRequest
    {
    public:
      class Sensor
      {
      public:
        /*
        *id - string : Value type as "identifier" text.
          * type - number : Value type of the sensor(quantity).See IQRF Sensor standard for details.
          * name - string : Name of the sensor(quantity).
          * shortName - string : Short name of the sensor(quantity).Typically it is a symbol used at physics.
          * unit - string : Unit of the quantity.Dimensionless quantity has empty string "".
          * decimalPlaces - number : Number of valid decimal places.
          * frcs - array : Array of FRC commands supported by the sensor.
          * breakdown - array : [optional] see <iqrf.sensor.ReadSensorsWithTypes_Response> for more information.
        */
        std::string id;
        int type;
        std::string name;
        std::string shortName;
        std::string unit;
        int decimalPlaces;
        std::set<int> frcs;
        //TODO breakdown

        Sensor(const rapidjson::Value& v)
        {
          using namespace rapidjson;

          type = jutils::getMemberAs<int>("type", v);
          name = jutils::getMemberAs<std::string>("name", v);
          shortName = jutils::getMemberAs<std::string>("shortName", v);
          decimalPlaces = jutils::getMemberAs<int>("type", v);
          unit = jutils::getMemberAs<std::string>("unit", v);
          {
            auto vect = jutils::getMemberAsVector<int>("frcs", v);
            frcs = std::set<int>(vect.begin(), vect.end());
          }
        }
      };

    private:
      std::vector<std::shared_ptr<Sensor>> m_sensors;

    public:
      Enumerate(uint16_t nadr)
        :JsDriverRequest(nadr)
      {
      }

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
          std::shared_ptr<Sensor> sen;
          if (!itr->IsNull()) {
            sen.reset(shape_new Sensor(*itr));
          }
          m_sensors.push_back(sen);
        }
      }

      // get data as returned from driver
      const std::vector<std::shared_ptr<Sensor>> & getSensors() const { return m_sensors; }

    };


  } //namespace sensor
} //namespace iqrf
