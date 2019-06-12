#pragma once

#include "JsDriverDpaCommandSolver.h"
#include "EmbedExplore.h"
#include "JsonUtils.h"
#include <sstream>
#include <iomanip>

namespace iqrf
{
  namespace embed
  {
    namespace explore
    {
      ////////////////
      class JsDriverEnumerate : public Enumerate, public JsDriverDpaCommandSolver
      {

      public:
        JsDriverEnumerate(uint16_t nadr)
          :JsDriverDpaCommandSolver(nadr)
        {
        }

        std::string functionName() const override
        {
          return "iqrf.embed.explore.Enumerate";
        }

        void parseResponse(const rapidjson::Value& v) override
        {
          m_dpaVer = jutils::getMemberAs<int>("dpaVer", v);
          m_perNr = jutils::getMemberAs<int>("perNr", v);
          {
            auto vect = jutils::getMemberAsVector<int>("embeddedPers", v);
            m_embedPer = std::set<int>(vect.begin(), vect.end());
          }
          m_hwpid = jutils::getMemberAs<int>("hwpid", v);
          m_hwpidVer = jutils::getMemberAs<int>("hwpidVer", v);
          m_flags = jutils::getMemberAs<int>("flags", v);
          {
            auto vect = jutils::getMemberAsVector<int>("userPer", v);
            m_userPer = std::set<int>(vect.begin(), vect.end());
          }
        }

      };

      ////////////////
      class JsDriverPeripheralInformation : public PeripheralInformation, public JsDriverDpaCommandSolver
      {
      public:
        JsDriverPeripheralInformation(uint16_t nadr, int per)
          :PeripheralInformation(per)
          , JsDriverDpaCommandSolver(nadr)
        {}

        std::string functionName() const override
        {
          return "iqrf.embed.explore.PeripheralInformation";
        }

        std::string requestParameter() const override
        {
          using namespace rapidjson;
          Document par;

          Pointer("/per").Set(par, (int)m_per);

          std::string parStr;
          StringBuffer buffer;
          Writer<rapidjson::StringBuffer> writer(buffer);
          par.Accept(writer);
          parStr = buffer.GetString();

          return parStr;
        }

        void parseResponse(const rapidjson::Value& v) override
        {
          using namespace rapidjson;

          m_perTe = jutils::getMemberAs<int>("perTe", v);
          m_perT = jutils::getMemberAs<int>("perT", v);
          m_par1 = jutils::getMemberAs<int>("par1", v);
          m_par2 = jutils::getMemberAs<int>("par2", v);
        }
      };
    } //namespace explore
  } //namespace embed
} //namespace iqrf
