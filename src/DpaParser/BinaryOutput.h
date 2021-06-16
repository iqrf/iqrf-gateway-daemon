/**
 * Copyright 2015-2021 IQRF Tech s.r.o.
 * Copyright 2019-2021 MICRORISC s.r.o.
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

#include <string>

namespace iqrf
{
  namespace binaryoutput
  {
    ////////////////
    class Enumerate
    {
    protected:
      int m_outputsNum;

      Enumerate()
      {}

    public:
      virtual ~Enumerate() {}

      int getBinaryOutputsNum() const { return m_outputsNum; }
    };
    typedef std::unique_ptr<Enumerate> EnumeratePtr;

  } //namespace binaryoutput
} //namespace iqrf
