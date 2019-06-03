#pragma once

#include "IDpaTransactionResult2.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "stdint.h"
#include <string>
#include <memory>

namespace iqrf {

  class JsDriverRequest
  {
  protected:
    uint16_t m_nadr;
    uint16_t m_hwpid;
    std::string m_storeRequest;
    std::unique_ptr<IDpaTransactionResult2> m_dpaTransactionResult2;

  public:
    virtual ~JsDriverRequest() {}
    virtual std::string functionName() const = 0;
    //must override if request function requires parameter
    virtual std::string requestParameter() const { return "{}"; }
    virtual void parseResponse(const rapidjson::Value& v) = 0;

    JsDriverRequest(uint16_t nadr)
      :m_nadr(nadr)
      , m_hwpid(0xffff)
    {}

    std::string & storeRequest() { return m_storeRequest; }
    uint16_t getNadr() const { return m_nadr; }
    uint16_t getHwpid() const { return m_hwpid; }
    void setHwpid(uint16_t hwpid) { m_hwpid = hwpid; }
    const std::unique_ptr<IDpaTransactionResult2> & getResult() const
    {
      return m_dpaTransactionResult2;
    }
    void setResult(std::unique_ptr<IDpaTransactionResult2> dpaTransactionResult2)
    {
      m_dpaTransactionResult2 = std::move(dpaTransactionResult2);
    }

  };
}
