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

#include "JsDriverDpaCommandSolver.h"
#include "JsDriverStandardFrcSolver.h"
#include "Sensor.h"
#include "JsonUtils.h"

namespace iqrf
{
  namespace sensor
  {
    namespace jsdriver
    {
      namespace item {
        ///////
        class Sensor : public sensor::item::Sensor
        {
        public:
          Sensor(const rapidjson::Value& v, uint8_t idx, int addr = -1)
          {
            using namespace rapidjson;

            //TODO id is not set by driver
            m_addr = addr;
            m_sid = jutils::getPossibleMemberAs<std::string>("id", v, m_sid);
            m_type = jutils::getMemberAs<int>("type", v);
            m_name = jutils::getMemberAs<std::string>("name", v);
            m_shortName = jutils::getMemberAs<std::string>("shortName", v);
            m_unit = jutils::getMemberAs<std::string>("unit", v);
            m_idx = idx;
            //TODO id is not set by driver
            m_decimalPlaces = jutils::getPossibleMemberAs<int>("decimalPlaces", v, m_decimalPlaces);
            {
              auto vect = jutils::getPossibleMemberAsVector<int>("frcs", v);
              m_frcs = std::set<int>(vect.begin(), vect.end());
            }
            const Value *val = Pointer("/value").Get(v);
            if (val && !val->IsNull()) {
              if (m_type == 192) {
                auto vect = jutils::getPossibleMemberAsVector<int>("value", v);
                m_valueArray = std::vector<uint8_t>(vect.begin(), vect.end());
              } else {
                m_value = val->GetDouble();
              }
              m_valueSet = true;
            }
            const Value *breakdown = Pointer("/breakdown/0").Get(v);
            if (breakdown) {
              m_breakdownName = jutils::getMemberAs<std::string>("name", *breakdown);
              m_breakdownShortname = jutils::getMemberAs<std::string>("shortName", *breakdown);
              m_breakdownUnit = jutils::getMemberAs<std::string>("unit", *breakdown);
              m_breakdownDecimalPlaces = (uint8_t)jutils::getPossibleMemberAs<int>("decimalPlaces", *breakdown, m_breakdownDecimalPlaces);
              val = Pointer("/value").Get(*breakdown);
              if (val && !val->IsNull()) {
                if (m_type == 192) {
                  auto vect = jutils::getPossibleMemberAsVector<int>("value", *breakdown);
                  m_breakdownValueArray = std::vector<uint8_t>(vect.begin(), vect.end());
                } else {
                  m_breakdownValue = val->GetDouble();
                }
              } else {
                m_breakdownValue = m_value;
                m_breakdownValueArray = m_valueArray;
              }
            }
          }

          virtual ~Sensor() {}
        };
        typedef std::unique_ptr<Sensor> SensorPtr;

      } // namespace jsdriver::item

      ////////////////
      class Enumerate : public sensor::Enumerate, public JsDriverDpaCommandSolver
      {
      public:
        Enumerate(IJsRenderService* iJsRenderService, uint16_t nadr)
          :JsDriverDpaCommandSolver(iJsRenderService, nadr)
        {}

        virtual ~Enumerate() {}

      protected:
        std::string functionName() const override
        {
          return "iqrf.sensor.Enumerate";
        }

        void parseResponse(const rapidjson::Value& v) override
        {
          using namespace rapidjson;

          const auto val = Pointer("/sensors").Get(v)->GetArray();
          for (auto itr = val.Begin(); itr != val.End(); ++itr) {
            jsdriver::item::SensorPtr sen;
            if (!itr->IsNull()) {
              sen.reset(shape_new jsdriver::item::Sensor(*itr, itr - val.begin(), m_nadr));
            }
            m_sensors.push_back(std::move(sen));
          }
        }
      };
      typedef std::unique_ptr<Enumerate> EnumeratePtr;

      class ReadSensorsWithTypes : public sensor::ReadSensorsWithTypes, public JsDriverDpaCommandSolver {
      public:
        ReadSensorsWithTypes(IJsRenderService* iJsRenderService, uint16_t nadr, const rapidjson::Value &params) : JsDriverDpaCommandSolver(iJsRenderService, nadr) {
          setRequestParamDoc(params);
        }

        virtual ~ReadSensorsWithTypes() {}
      protected:
        std::string functionName() const override {
          return "iqrf.sensor.ReadSensorsWithTypes";
        }

        void preRequest(rapidjson::Document& params) override {
          (void)params;
        }

        void parseResponse(const rapidjson::Value &v) override {
          using namespace rapidjson;

          const auto val = Pointer("/sensors").Get(v)->GetArray();
          for (auto itr = val.Begin(); itr != val.End(); ++itr) {
            jsdriver::item::SensorPtr sensor;
            if (!itr->IsNull()) {
              sensor.reset(new jsdriver::item::Sensor(*itr, itr - val.begin(), m_nadr));
            }
            m_sensors.push_back(std::move(sensor));
          }
        }
      };
      typedef std::unique_ptr<ReadSensorsWithTypes> ReadSensorsWithTypesPtr;

      ////////////////
      class SensorFrc {
      public:
        SensorFrc(const rapidjson::Document& parameters) {
          using namespace rapidjson;
          m_type = Pointer("/sensorType").Get(parameters)->GetUint();
          m_idx = Pointer("/sensorIndex").Get(parameters)->GetUint();
          m_command = Pointer("/frcCommand").Get(parameters)->GetUint();
          {
            const Value *v = Pointer("/selectedNodes").Get(parameters);
            if (v && v->IsArray()) {
              const auto &arr = v->GetArray();
              for (SizeType i = 0; i < arr.Size(); ++i) {
                m_selectedNodes.insert(arr[i].GetUint());
              }
            }
          }
          {
            const Value *v = Pointer("/getExtraResult").Get(parameters);
            if (v && v->IsBool()) {
              m_extraResult = v->GetBool();
            } else {
              m_extraResult = false;
            }
          }
        }

        uint8_t& getIndex() {
          return m_idx;
        }

        uint8_t& getType() {
          return m_type;
        }

        uint8_t& getCommand() {
          return m_command;
        }

        std::set<uint8_t>& getSelectedNodes() {
          return m_selectedNodes;
        }

        bool getExtraResult() {
          return m_extraResult;
        }

        virtual ~SensorFrc() {}
      private:
        uint8_t m_idx;
        uint8_t m_type;
        uint8_t m_command;
        std::set<uint8_t> m_selectedNodes;
        bool m_extraResult;
      };

      class SensorFrcJs : public sensor::Frc, public JsDriverStandardFrcSolver {
      public:
        SensorFrcJs(IJsRenderService* iJsRenderService, const uint8_t &type, const uint8_t &index, const uint8_t frcCommand, const std::set<uint8_t> &selectedNodes)
          : sensor::Frc(type, index, frcCommand, selectedNodes), JsDriverStandardFrcSolver(iJsRenderService)
        {}

        virtual ~SensorFrcJs() {}
      protected:
        std::string functionName() const override {
          return "iqrf.sensor.Frc";
        }

        void preRequest(rapidjson::Document &requestParamDoc) override {
          using namespace rapidjson;

          Pointer("/sensorType").Set(requestParamDoc, m_sensorType);
          Pointer("/sensorIndex").Set(requestParamDoc, m_sensorIndex);
          Pointer("/frcCommand").Set(requestParamDoc, m_frcCommand);
          if (m_selectedNodes.size() > 0) {
            Value arr(kArrayType);
            for (const auto &addr : m_selectedNodes) {
              arr.PushBack(Value(addr), requestParamDoc.GetAllocator());
            }
            Pointer("/selectedNodes").Set(requestParamDoc, arr);
          }
        }

        void postResponse(const rapidjson::Document &responseDoc) override {
          using namespace rapidjson;

          std::vector<uint8_t> requested(m_selectedNodes.begin(), m_selectedNodes.end());
          uint8_t cnt = 0;
          const auto val = Pointer("/sensors").Get(responseDoc)->GetArray();
          auto itr = val.Begin();
          std::advance(itr, 1);
          for (; itr != val.End(); ++itr) {
            if (!itr->IsNull()) {
              jsdriver::item::SensorPtr sensor;
              if (m_selectedNodesSet) {
                sensor.reset(new jsdriver::item::Sensor(*itr, m_sensorIndex, requested[cnt]));
              } else {
                sensor.reset(new jsdriver::item::Sensor(*itr, m_sensorIndex, cnt));
              }
              m_sensors.push_back(std::move(sensor));
            }
            cnt++;
          }
        }
      };
      typedef std::unique_ptr<SensorFrcJs> SensorFrcJsPtr;
    } //namespace jsdriver
  } //namespace sensor
} //namespace iqrf
