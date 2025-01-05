/**
 * Copyright 2015-2025 IQRF Tech s.r.o.
 * Copyright 2019-2025 MICRORISC s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
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
