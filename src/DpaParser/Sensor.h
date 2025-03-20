/**
 * IQRF Gateway Daemon
 * Copyright (C) 2015-2025 IQRF Tech s.r.o., 2019-2025 MICRORISC s.r.o.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include <string>
#include <set>
#include <vector>
#include <memory>

namespace iqrf
{
  namespace sensor
  {
    //FRC command to return 2 - bits sensor data of the supporting sensor types.
    const int STD_SENSOR_FRC_2BITS = 0x10;
    //FRC command to return 1-byte wide sensor data of the supporting sensor types.
    const int STD_SENSOR_FRC_1BYTE = 0x90;
    //FRC command to return 2-bytes wide sensor data of the supporting sensor types.
    const int STD_SENSOR_FRC_2BYTES = 0xE0;
    //FRC command to return 4-bytes wide sensor data of the supporting sensor types.
    const int STD_SENSOR_FRC_4BYTES = 0xF9;

    namespace item {
      ////////
      class Sensor
      {
      protected:
        int m_idx;
        std::string m_sid;
        int m_type;
        std::string m_name;
        std::string m_shortName;
        std::string m_unit;
        int m_decimalPlaces = 1;
        std::set<int> m_frcs;
        double m_value = 0;
        std::vector<uint8_t> m_valueArray;
        bool m_valueSet = false;
        std::string m_breakdownName;
        std::string m_breakdownShortname;
        std::string m_breakdownUnit;
        uint8_t m_breakdownDecimalPlaces = 1;
        double m_breakdownValue = 0;
        std::vector<uint8_t> m_breakdownValueArray;
        int m_addr = -1;

      public:
        int getAddr() const { return m_addr; }
        int getIdx() const { return m_idx; }
        const std::string & getSid() const { return m_sid; }
        int getType() const { return m_type; }
        const std::string & getName() const { return m_name; }
        const std::string & getShortName() const { return m_shortName; }
        const std::string & getUnit() const { return m_unit; }
        int getDecimalPlaces() const { return m_decimalPlaces; }
        bool hasBreakdown() const { return m_breakdownName.length() > 0 && m_breakdownShortname.length() > 0 && m_breakdownUnit.length() > 0; }
        const std::string& getBreakdownName() const { return m_breakdownName; }
        const std::string& getBreakdownShortName() const { return m_breakdownShortname; }
        const std::string& getBreakdownUnit() const { return m_breakdownUnit; }
        const uint8_t& getBreakdownDecimalPlaces() const { return m_breakdownDecimalPlaces; }
        const std::set<int> & getFrcs() const { return m_frcs; }
        bool isValueSet() const { return m_valueSet; }
        double getValue() const { return m_value; }
        const std::vector<uint8_t>& getValueArray() const { return m_valueArray; }
        double getBreakdownValue() const { return m_breakdownValue; }
        const std::vector<uint8_t>& getBreakdownValueArray() const { return m_breakdownValueArray; }
        void setIdx(const uint8_t &idx) { m_idx = idx; }

        virtual ~Sensor() {}
      };
      typedef std::unique_ptr<Sensor> SensorPtr;
    }

    ////////////////
    class Enumerate
    {
    protected:
      std::vector<item::SensorPtr> m_sensors;

      Enumerate()
      {}

    public:
      virtual ~Enumerate() {}

      // get data as returned from driver
      const std::vector<item::SensorPtr> & getSensors() const { return m_sensors; }

    };
    typedef std::unique_ptr<Enumerate> EnumeratePtr;

    class ReadSensorsWithTypes {
    protected:
      ReadSensorsWithTypes() {}

      /// response
      std::vector<item::SensorPtr> m_sensors;
    public:
      virtual ~ReadSensorsWithTypes() {};

			const std::vector<item::SensorPtr>& getSensors() const { return m_sensors; }
    };
    typedef std::unique_ptr<ReadSensorsWithTypes> ReadSensorsWithTypesPtr;

    ////////////////
    class Frc
    {
    protected:
      //params
      int m_sensorType = 0;
      int m_sensorIndex = 0;
      uint8_t m_frcCommand = 0;
      bool m_selectedNodesSet = false;
      std::set<uint8_t> m_selectedNodes;
      bool m_sleepAfterFrcSet = false;
      int m_time = 0;
      int m_control = 0;

      //response
      std::vector<item::SensorPtr> m_sensors;

      Frc()
      {}

      Frc(int sensorType, int sensorIndex, uint8_t frcCommand)
        :m_sensorType(sensorType)
        , m_sensorIndex(sensorIndex)
        , m_frcCommand(frcCommand)
      {}

      Frc(uint8_t sensorType, uint8_t sensorIndex, uint8_t frcCommand, const std::set<uint8_t> &selectedNodes = {})
        : m_sensorType(sensorType),
        m_sensorIndex(sensorIndex),
        m_frcCommand(frcCommand)
      {
        if (selectedNodes.size() > 0) {
          m_selectedNodes = selectedNodes;
          m_selectedNodesSet = true;
        }
      }

      Frc(int sensorType, int sensorIndex, uint8_t frcCommand, const std::set<uint8_t> &selectedNodes)
        :m_sensorType(sensorType)
        , m_sensorIndex(sensorIndex)
        , m_frcCommand(frcCommand)
        , m_selectedNodesSet(true)
        , m_selectedNodes(selectedNodes)
      {}

      Frc(int sensorType, int sensorIndex, uint8_t frcCommand, const std::set<uint8_t> &selectedNodes, int time, int control)
        :m_sensorType(sensorType)
        , m_sensorIndex(sensorIndex)
        , m_frcCommand(frcCommand)
        , m_selectedNodesSet(true)
        , m_selectedNodes(selectedNodes)
        , m_sleepAfterFrcSet(true)
        , m_time(time)
        , m_control(control)
      {}

      Frc(int sensorType, int sensorIndex, uint8_t frcCommand, int time, int control)
        :m_sensorType(sensorType)
        , m_sensorIndex(sensorIndex)
        , m_frcCommand(frcCommand)
        , m_sleepAfterFrcSet(true)
        , m_time(time)
        , m_control(control)
      {}

    public:
      virtual ~Frc() {}

      // get param data passes by ctor
      int getSensorType() const { return m_sensorType; }
      int getSensorIndex() const { return m_sensorIndex; }
      int getFrcCommand() const { return m_frcCommand; }
      const std::set<uint8_t>& getSelectedNodes() const { return m_selectedNodes; }
      int getTime() const { return m_time; }
      int getControl() const { return m_control; }

      // get data as returned from driver
      const std::vector<item::SensorPtr> & getSensors() const { return m_sensors; }

    };
    typedef std::unique_ptr<Frc> FrcPtr;

  } //namespace sensor
} //namespace iqrf
