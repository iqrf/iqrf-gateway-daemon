#pragma once

#include <memory>
#include "PreparedData.h"
#include "IOtaUploadService.h"
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
    static std::unique_ptr<PreparedData> prepareData(IOtaUploadService::LoadingContentType loadingContent, const std::string& fileName, bool isForBroadcast);
    static std::list<CodeBlock> getEepromData(const std::string& fileName);
    static std::list<CodeBlock> getEeepromData(const std::string& fileName);
  private:
    class Imp;
    static Imp* m_imp;
  };
}