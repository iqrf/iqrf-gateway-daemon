#pragma once

#include "JsDriverDpaCommandSolver.h"
#include "BinaryOutput.h"
#include "JsonUtils.h"

namespace iqrf
{
  namespace binaryoutput
  {
    ////////////////
    class JsDriverEnumerate : public Enumerate, public JsDriverDpaCommandSolver
    {
    public:
      JsDriverEnumerate(IJsRenderService* iJsRenderService, uint16_t nadr)
        :JsDriverDpaCommandSolver(iJsRenderService, nadr)
      {}

      virtual ~JsDriverEnumerate() {}

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
    typedef std::unique_ptr<JsDriverEnumerate> JsDriverEnumeratePtr;

  } //namespace binaryoutput
} //namespace iqrf
