/*
 * Copyright 2016-2017 MICRORISC s.r.o.
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

#include "Trace.h"

#include <string>
#include <sstream>
#include <algorithm>

namespace iqrf {

  //aux class to convert from dot notation to ustring and back
  class DotMsg
  {
  public:
    DotMsg(std::basic_string<unsigned char> msg)
      :m_msg(msg)
    {}

    DotMsg(std::string dotMsg)
    {
      if (!dotMsg.empty()) {
        std::string buf = dotMsg;
        std::replace(buf.begin(), buf.end(), '.', ' ');

        std::istringstream istr(buf);

        int val;
        while (true) {
          if (!(istr >> std::hex >> val)) {
            if (istr.eof()) break;
            THROW_EXC_TRC_WAR(std::logic_error, "Unexpected format: " << PAR(dotMsg));
          }
          m_msg.push_back((uint8_t)val);
        }
      }
    }

    operator std::basic_string<unsigned char>() { return m_msg; }

    operator std::string()
    {
      std::string to;
      if (!m_msg.empty()) {
        std::ostringstream ostr;
        ostr.setf(std::ios::hex, std::ios::basefield);
        ostr.fill('0');
        for (uint8_t c : m_msg) {
          ostr << std::setw(2) << (short int)c;
          ostr << '.';
        }
        to = ostr.str();
        to.pop_back();
      }
      return to;
    }

  private:
    std::basic_string<unsigned char> m_msg;
  };
}
