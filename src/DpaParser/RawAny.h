#pragma once

#include "DpaCommandSolver.h"

namespace iqrf
{
  namespace raw
  {
    ////////////////
    //speciality for handling async responses
    class AnyAsyncResponse : public DpaCommandSolver
    {
    public:
      AnyAsyncResponse() = delete;

      AnyAsyncResponse(const DpaMessage& dpaMessage)
        :DpaCommandSolver(dpaMessage)
      {
        if (!isAsyncRcode()) {
          THROW_EXC_TRC_WAR(std::logic_error, "Invalid async response code:"
            << NAME_PAR(expected, (int)STATUS_ASYNC_RESPONSE) << NAME_PAR(delivered, (int)getRcode()));
        }
      }

      virtual ~AnyAsyncResponse()
      {}

    protected:
      void encodeRequest(DpaMessage & dpaRequest) override
      {
        (void)dpaRequest; //silence -Wunused-parameter
      }

      void parseResponse(const DpaMessage & dpaResponse) override
      {
        (void)dpaResponse; //silence -Wunused-parameter
      }
    };
    typedef std::unique_ptr<AnyAsyncResponse> AnyPtr;

  } //namespace raw
} //namespace iqrf
