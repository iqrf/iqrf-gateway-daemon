#pragma once

#include "JsDriverDpaCommandSolver.h"
#include "EmbedOS.h"
#include "JsonUtils.h"
#include <vector>
#include <sstream>
#include <iomanip>

namespace iqrf
{
  namespace embed
  {
    namespace os
    {
      ////////////////
      class JsDriverRead : public Read, public JsDriverDpaCommandSolver
      {
      public:
        JsDriverRead(IJsRenderService* iJsRenderService, uint16_t nadr)
          :JsDriverDpaCommandSolver(iJsRenderService, nadr)
        {}

        virtual ~JsDriverRead()
        {}

      protected:
        std::string functionName() const override
        {
          return "iqrf.embed.os.Read";
        }

        std::string requestParameter() const override
        {
          return "{}";
        }

        void parseResponse(const rapidjson::Value& v) override
        {
          using namespace rapidjson;
          m_mid = jutils::getMemberAs<unsigned>("mid", v);
          m_osVersion = jutils::getMemberAs<int>("osVersion", v);
          m_trMcuType = jutils::getMemberAs<int>("trMcuType", v);
          m_osBuild = jutils::getMemberAs<int>("osBuild", v);
          m_rssi = jutils::getMemberAs<int>("rssi", v);
          m_supplyVoltage = jutils::getMemberAs<double>("supplyVoltage", v);
          m_flags = jutils::getMemberAs<int>("flags", v);
          std::vector<int> arr;
          arr = jutils::getPossibleMemberAsVector<int>("ibk", v, arr);
          m_ibk = std::vector<uint8_t>(arr.begin(), arr.end());
        }
      };

      ////////////////
      class JsDriverReadCfg : public ReadCfg, public JsDriverDpaCommandSolver
      {
      public:
        JsDriverReadCfg(IJsRenderService* iJsRenderService, uint16_t nadr)
          :JsDriverDpaCommandSolver(iJsRenderService, nadr)
        {}

        virtual ~JsDriverReadCfg()
        {}

      protected:
        std::string functionName() const override
        {
          return "iqrf.embed.os.ReadCfg";
        }

        std::string requestParameter() const override
        {
          return "{}";
        }

        void parseResponse(const rapidjson::Value& v) override
        {
          m_checkum = jutils::getMemberAs<int>("checksum", v);
          std::vector<int> arr;
          arr = jutils::getPossibleMemberAsVector<int>("configuration", v, arr);
          m_configuration = std::vector<uint8_t>(arr.begin(), arr.end());
          m_rfpgm = jutils::getMemberAs<int>("rfpgm", v);
          m_undocumented = jutils::getMemberAs<int>("undocumented", v);
        }

      };

      ////////////////
      class JsDriverRestart : public ReadCfg, public JsDriverDpaCommandSolver
      {
      public:
        JsDriverRestart(IJsRenderService* iJsRenderService, uint16_t nadr)
          :JsDriverDpaCommandSolver(iJsRenderService, nadr)
        {}

        virtual ~JsDriverRestart()
        {}

      protected:
        std::string functionName() const override
        {
          return "iqrf.embed.os.Restart";
        }

        std::string requestParameter() const override
        {
          return "{}";
        }

        void parseResponse(const rapidjson::Value& v) override
        {}

      };

    } //namespace os
  } //namespace embed
} //namespace iqrf
