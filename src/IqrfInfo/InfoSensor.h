/**
 * Copyright 2015-2024 IQRF Tech s.r.o.
 * Copyright 2019-2024 MICRORISC s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
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
