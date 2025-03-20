/**
 * IQRF Gateway Daemon
 * Copyright (C) 2015-2025 IQRF Tech s.r.o., 2019-2025 MICRORISC s.r.o.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
