#pragma once

#include <string>
#include <set>
#include <vector>

namespace iqrf
{
  namespace sensor
  {
    ////////////////
    class Enumerate
    {
    public:
      class Sensor
      {
      protected:
        std::string m_sid;
        int m_type;
        std::string m_name;
        std::string m_shortName;
        std::string m_unit;
        int m_decimalPlaces = 1;
        std::set<int> m_frcs;
        //TODO breakdown - array : [optional] see <iqrf.sensor.ReadSensorsWithTypes_Response> for more information.

      public:
        const std::string & getSid() const { return m_sid; }
        int getType() const { return m_type; }
        const std::string & getName() const { return m_name; }
        const std::string & getShortName() const { return m_shortName; }
        const std::string & getUnit() const { return m_unit; }
        int getDecimalPlaces() const { return m_decimalPlaces; }
        const std::set<int> & getFrcs() const { return m_frcs; }
        //TODO breakdown - array : [optional] see <iqrf.sensor.ReadSensorsWithTypes_Response> for more information.

        virtual ~Sensor() {}
      };
      typedef std::unique_ptr<Sensor> SensorPtr;

    protected:
      std::vector<SensorPtr> m_sensors;

      Enumerate()
      {}

    public:
      virtual ~Enumerate() {}

      // get data as returned from driver
      const std::vector<SensorPtr> & getSensors() const { return m_sensors; }

    };
    typedef std::unique_ptr<Enumerate> EnumeratePtr;

  } //namespace sensor
} //namespace iqrf
