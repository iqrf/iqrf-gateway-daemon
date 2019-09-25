#pragma once

#include "Sensor.h"

namespace iqrf
{
  namespace sensor
  {
    ////////////////
    class InfoEnumerate : public Enumerate
    {
    public:
      class InfoSensor : public item::Sensor
      {
      public:
        InfoSensor(int idx, std::string sid, int stype, std::string name, std::string sname, std::string unit, int dplac, std::set<int> frcs)
          : Sensor()
        {
          m_idx = idx;
          m_sid = sid;
          m_type = stype;
          m_name = name;
          m_shortName = sname;
          m_unit = unit;
          m_decimalPlaces = dplac;
          m_frcs = frcs;
        }

        virtual ~InfoSensor() {}
      };
      typedef std::unique_ptr<InfoSensor> InfoSensorPtr;

      InfoEnumerate()
      {}

      void addInfoSensor(InfoSensorPtr infoSensorPtr)
      {
        m_sensors.push_back(std::move(infoSensorPtr));
      }

      virtual ~InfoEnumerate() {}
    };

    typedef std::unique_ptr<InfoEnumerate> InfoEnumeratePtr;

  } //namespace sensor
} //namespace iqrf
