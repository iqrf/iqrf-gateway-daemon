/**
 * Copyright 2015-2023 IQRF Tech s.r.o.
 * Copyright 2019-2023 MICRORISC s.r.o.
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
          Sensor(const rapidjson::Value& v, uint8_t idx)
          {
            using namespace rapidjson;

            //TODO id is not set by driver
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
              m_value = val->GetDouble();
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
                m_breakdownValue = val->GetDouble();
              } else {
                m_breakdownValue = m_value;
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
              sen.reset(shape_new jsdriver::item::Sensor(*itr, itr - val.begin()));
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
              sensor.reset(new jsdriver::item::Sensor(*itr, itr - val.begin()));
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

      /*
      ////////////////
      class JsDriverFrc : public Frc, public JsDriverStandardFrcSolver
      {
      public:
        JsDriverFrc(IJsRenderService* iJsRenderService, int sensorType, int sensorIndex, uint8_t frcCommand)
          :Frc(sensorType, sensorIndex, frcCommand)
          , JsDriverStandardFrcSolver(iJsRenderService)
        {}

        JsDriverFrc(IJsRenderService* iJsRenderService, int sensorType, int sensorIndex, uint8_t frcCommand, const std::vector<int> & selectedNodes)
          :Frc(sensorType, sensorIndex, frcCommand, selectedNodes)
          , JsDriverStandardFrcSolver(iJsRenderService)
        {}

        JsDriverFrc(IJsRenderService* iJsRenderService, int sensorType, int sensorIndex, uint8_t frcCommand, const std::vector<int> & selectedNodes, int time, int control)
          :Frc(sensorType, sensorIndex, frcCommand, selectedNodes, time, control)
          , JsDriverStandardFrcSolver(iJsRenderService)
        {}

        JsDriverFrc(IJsRenderService* iJsRenderService, int sensorType, int sensorIndex, uint8_t frcCommand, int time, int control)
          :Frc(sensorType, sensorIndex, frcCommand, time, control)
          , JsDriverStandardFrcSolver(iJsRenderService)
        {}

        // for using with ApiMsgIqrfSensorFrc - members are not interpreted
        JsDriverFrc(IJsRenderService* iJsRenderService, const rapidjson::Value & val)
          :Frc()
          , JsDriverStandardFrcSolver(iJsRenderService)
        {
          (void)val; //silence -Wunused-parameter
        }

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
          }
        }
      };
      typedef std::unique_ptr<JsDriverFrc> JsDriverFrcPtr;
      */

    } //namespace jsdriver
  } //namespace sensor
} //namespace iqrf
