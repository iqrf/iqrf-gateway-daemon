#pragma once

#include "JsDriverDpaCommandSolver.h"

namespace iqrf {

  class IJsDriverService
  {
  public:
    virtual DpaMessage createDpaRequest(JsDriverDpaCommandSolver & jsRequest) = 0;
    virtual void processDpaTransactionResult(JsDriverDpaCommandSolver & JsDriverDpaCommandSolver, std::unique_ptr<IDpaTransactionResult2> res) = 0;
    virtual ~IJsDriverService() {}
  };
}
