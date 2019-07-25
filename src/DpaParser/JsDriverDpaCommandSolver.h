#pragma once

#include "DpaCommandSolver.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include <string>

namespace iqrf {

  class JsDriverDpaCommandSolver : public DpaCommandSolver
  {
  protected:
    std::string m_storeRequest;

  public:
    virtual ~JsDriverDpaCommandSolver() {}
    virtual std::string functionName() const = 0;
    //must override if request function requires parameter
    virtual std::string requestParameter() const { return "{}"; }
    virtual void parseResponse(const rapidjson::Value& v) = 0;

    JsDriverDpaCommandSolver(uint16_t nadr)
      :DpaCommandSolver(nadr)
    {}

    JsDriverDpaCommandSolver(uint16_t nadr, uint16_t hwpid)
      :DpaCommandSolver(nadr, hwpid)
    {}

    std::string & storeRequest() { return m_storeRequest; }
  };
}
