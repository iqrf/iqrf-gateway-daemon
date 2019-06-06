#pragma once

#include "JsDriverDpaCommandSolver.h"
#include "IEnumerateService.h"
#include "JsonUtils.h"
#include <vector>
#include <sstream>
#include <iomanip>
#include <set>

namespace iqrf
{
  namespace binaryoutput
  {
    ////////////////
    class Enumerate : public JsDriverDpaCommandSolver, public IEnumerateService::IStandardBinaryOutputData
    {
    private:
      int m_outputsNum;

    public:
      Enumerate(uint16_t nadr)
        :JsDriverDpaCommandSolver(nadr)
      {
      }

      virtual ~Enumerate() {}

      std::string functionName() const override
      {
        return "iqrf.binaryoutput.Enumerate";
      }

      void parseResponse(const rapidjson::Value& v) override
      {
        m_outputsNum = jutils::getMemberAs<int>("binouts", v);
      }

      int getBinaryOutputsNum() const override { return m_outputsNum; }
    };


  } //namespace sensor
} //namespace iqrf
