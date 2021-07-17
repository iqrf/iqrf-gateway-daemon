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

#include <memory>
#include "PreparedData.h"
#include "IOtaUploadService.h"
#include "iqrf_header_parser.h"
#include <list>

namespace iqrf 
{
  // Encapsulates block of code 
  class CodeBlock
  {
  private:
    std::basic_string<uint8_t> m_code;
    uint16_t m_startAddress;
    uint16_t m_endAddress;

  public:
    CodeBlock(const std::basic_string<uint8_t>& code, uint16_t startAddress, uint16_t endAddress)
    {
      this->m_code = code;
      this->m_startAddress = startAddress;
      this->m_endAddress = endAddress;
    }

    const std::basic_string<uint8_t>& getCode() const { return m_code; };
    uint16_t getStartAddress() const { return m_startAddress; };
    uint16_t getEndAddress() const { return m_endAddress; };
    uint16_t getLength() const { return m_endAddress - m_startAddress + 1; };
  };

  /// Class DataPreparer
  class DataPreparer
  {
  public:
    static std::unique_ptr<PreparedData> prepareData(IOtaUploadService::LoadingContentType loadingContent, const std::string& fileName, bool isForBroadcast, const IOtaUploadService::ModuleInfo &module);
    static std::list<CodeBlock> getEepromData(const std::string& fileName);
    static std::list<CodeBlock> getEeepromData(const std::string& fileName);
  private:
    class Imp;
    static Imp* m_imp;
  };
}