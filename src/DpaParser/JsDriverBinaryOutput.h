#pragma once

#include "JsDriverDpaCommandSolver.h"
#include "BinaryOutput.h"
#include "JsonUtils.h"

namespace iqrf
{
  namespace binaryoutput
  {
    namespace jsdriver {
      ////////////////
      class Enumerate : public binaryoutput::Enumerate, public JsDriverDpaCommandSolver
      {
      public:
        Enumerate(IJsRenderService* iJsRenderService, uint16_t nadr)
          :JsDriverDpaCommandSolver(iJsRenderService, nadr)
        {}

        virtual ~Enumerate() {}

      protected:
        std::string functionName() const override
        {
          return "iqrf.binaryoutput.Enumerate";
        }

        void parseResponse(const rapidjson::Value& v) override
        {
          m_outputsNum = jutils::getMemberAs<int>("binOuts", v);
        }
      };
      typedef std::unique_ptr<Enumerate> EnumeratePtr;

    } //namespace jsdriver
  } //namespace binaryoutput
} //namespace iqrf
