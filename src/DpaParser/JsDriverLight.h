#pragma once

#include "JsDriverDpaCommandSolver.h"
#include "JsDriverStandardFrcSolver.h"
#include "Light.h"
#include "JsonUtils.h"
#include <vector>

namespace iqrf
{
  namespace light
  {
    namespace jsdriver
    {
      namespace item {
        class Light : public light::item::Light
        {
        public:
          Light(const rapidjson::Value& v)
            : light::item::Light()
          {
            m_index = (int)jutils::getMemberAs<int>("index", v);
            m_power = (uint8_t)jutils::getMemberAs<int>("power", v);
            m_time = (uint8_t)jutils::getMemberAs<int>("time", v);
          }
        };
      } //namespace item

      ////////////////
      class Enumerate : public light::Enumerate, public JsDriverDpaCommandSolver
      {
      public:
        Enumerate(IJsRenderService* iJsRenderService, uint16_t nadr)
          :JsDriverDpaCommandSolver(iJsRenderService, nadr)
        {}

        virtual ~Enumerate() {}

      protected:
        std::string functionName() const override
        {
          return "iqrf.light.Enumerate";
        }

        void parseResponse(const rapidjson::Value& v) override
        {
          m_lightsNum = jutils::getMemberAs<int>("lights", v);
        }
      };
      typedef std::unique_ptr<Enumerate> JsDriverEnumeratePtr;

      ////////////////
      class SetPower : public light::SetPower, public JsDriverDpaCommandSolver
      {
      public:
        SetPower(IJsRenderService* iJsRenderService, const std::vector<light::item::LightPtr> & lights)
          :JsDriverDpaCommandSolver(iJsRenderService)
          , light::SetPower(lights)
        {}

        virtual ~SetPower() {}

        static void parsePrevVals(std::vector<int> & prevVals, const rapidjson::Value& v)
        {
          using namespace rapidjson;

          const Value *arrayVal = Pointer("/prevVals").Get(v);
          if (!(arrayVal && arrayVal->IsArray())) {
            THROW_EXC_TRC_WAR(std::logic_error, "Expected: Json Array .../prevVals[]");
          }

          for (const Value *itr = arrayVal->Begin(); itr != arrayVal->End(); ++itr) {
            if (!(itr && itr->IsInt())) {
              THROW_EXC_TRC_WAR(std::logic_error, "Expected: integer item in .../prevVals[int]");
            }
            prevVals.push_back(itr->GetInt());
          }
        }

      protected:
        std::string functionName() const override
        {
          return "iqrf.light.SetPower";
        }

        void parseResponse(const rapidjson::Value& v) override
        {
          jsdriver::SetPower::parsePrevVals(m_prevVals, v);
        }
      };
      typedef std::unique_ptr<SetPower> SetPowerPtr;

      ////////////////
      class IncrementPower : public light::IncrementPower, public JsDriverDpaCommandSolver
      {
      public:
        IncrementPower(IJsRenderService* iJsRenderService, const std::vector<light::item::LightPtr> & lights)
          :JsDriverDpaCommandSolver(iJsRenderService)
          , light::IncrementPower(lights)
        {}

        virtual ~IncrementPower() {}

      protected:
        std::string functionName() const override
        {
          return "iqrf.light.IncrementPower";
        }

        void parseResponse(const rapidjson::Value& v) override
        {
          jsdriver::SetPower::parsePrevVals(m_prevVals, v);
        }
      };
      typedef std::unique_ptr<IncrementPower> IncrementPowerPtr;
      
      ////////////////
      class DecrementPower : public light::DecrementPower, public JsDriverDpaCommandSolver
      {
      public:
        DecrementPower(IJsRenderService* iJsRenderService, const std::vector<light::item::LightPtr> & lights)
          :JsDriverDpaCommandSolver(iJsRenderService)
          , light::DecrementPower(lights)
        {}

        virtual ~DecrementPower() {}

      protected:
        std::string functionName() const override
        {
          return "iqrf.light.DecrementPower";
        }

        void parseResponse(const rapidjson::Value& v) override
        {
          jsdriver::SetPower::parsePrevVals(m_prevVals, v);
        }
      };
      typedef std::unique_ptr<DecrementPower> DecrementPowerPtr;

    } //namespace jsdriver
  } //namespace light
} //namespace iqrf
