#include "DataPreparer.h"
#include <fstream>
#include <sstream>
#include <list>

namespace {

  // simple implementation of abstract class PreparedData
  class PreparedDataImpl : public iqrf::PreparedData {
  private:
    Data m_data;
    uint16_t m_length;
    uint16_t m_checksum;

  public:
    PreparedDataImpl(const Data& data, uint16_t length, uint16_t checksum) {
      this->m_data = data;
      this->m_length = length;
      this->m_checksum = checksum;
    }

    Data getData() override { return m_data; };
    uint16_t getChecksum() override { return m_checksum; };
    uint16_t getLength() override { return m_length; };

  };

  // encapsulates block of code 
  class CodeBlock {
  private:
    std::basic_string<uint8_t> m_code;
    uint16_t m_startAddress;
    uint16_t m_endAddress;

  public:
    CodeBlock(
      const std::basic_string<uint8_t>& code,
      uint16_t startAddress,
      uint16_t endAddress
    )
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


  // parses specified text as a byte in hexa format and returns its value
  static uint8_t parseHexaByte(const std::string& text, int pos) {
    std::string byteStr = text.substr(pos, 2);

    try {
      return std::stoi(byteStr, nullptr, 16);
    }
    catch (std::exception& ex) {
      std::stringstream errstream;
      errstream << "Invalid Intex HEX record: cannot parse string:  ";
      errstream << byteStr << " to hexa value.";
      throw std::logic_error(errstream.str().c_str());
    }
  }

  // parses hex files and provides data from them
  class IntelHexParser {
  private:
    static uint16_t offset;

    // trims leading and trailing whitespaces from string
    static void trim(std::string& str) {
      const std::string whitespace = " \t";
      const auto strBegin = str.find_first_not_of(whitespace);

      // no content
      if (strBegin == std::string::npos) {
        return;
      }

      str.erase(0, strBegin);

      const auto strEnd = str.find_last_not_of(whitespace);
      if (strEnd == str.length() - 1) {
        return;
      }

      str.erase(strEnd + 1);
    }

    // constructs error string - puts together error msg with line and line number
    static std::string addLineInfo(
      const std::string& msg, std::string& line, uint16_t lineNumber
    )
    {
      std::stringstream ss;
      ss << msg << ", line number : " << lineNumber << ", line : " << line;
      return ss.str().c_str();
    }

    // parses specified line
    // if the line is data records, returns corresponding code block
    static std::unique_ptr<CodeBlock> parseLine(std::string& line)
    {
      trim(line);

      if (line.find_first_of(':') != 0) {
        throw std::logic_error(
          "Invalid Intel HEX record: line does not star with colon"
        );
      }

      if ((line.length() % 2 != 1) || (line.length() < (1 + 5 * 2))) {
        throw std::logic_error(
          "Invalid Intel HEX record: invalid line length"
        );
      }

      uint8_t recordType = 0;
      uint8_t byteCount = parseHexaByte(line, 1);
      uint8_t lineAddressH = parseHexaByte(line, 3);
      uint8_t lineAddressL = parseHexaByte(line, 5);
      recordType = parseHexaByte(line, 7);
      uint8_t checksum
        = (uint8_t)(
          byteCount
          + lineAddressH
          + lineAddressL
          + recordType
          + parseHexaByte(line, byteCount * 2 + 9)
          );
      uint16_t realAddress = 0;

      switch (recordType) {
      case 0x00:
        realAddress = (IntelHexParser::offset + ((lineAddressH & 0xFF) << 8) + (lineAddressL & 0xFF));
        break;
      case 0x01:
        break;
      case 0x02:
      case 0x04:
        IntelHexParser::offset = ((parseHexaByte(line, 9) & 0xFF) << 8) + (parseHexaByte(line, 11) & 0xFF);
        IntelHexParser::offset *= (recordType == 0x02) ? 16 : 65536;
        break;
      default:
        throw std::logic_error("Invalid Intel HEX record: unknown record type");
      }

      std::basic_string<uint8_t> data;

      for (int i = 0; i < byteCount; i++) {
        uint8_t b = parseHexaByte(line, i * 2 + 9);
        checksum += b;

        if (recordType == 0x00) {
          data.push_back(b);
        }
      }

      if (checksum != 0) {
        throw std::logic_error("Invalid Intel HEX record: invalid checksum");
      }

      if (recordType == 0x00) {
        uint16_t endAddress = realAddress + byteCount - 1;

        if (endAddress > (2 * 0x3D7F)) {
          throw std::range_error("End block address out of bound.");
        }

        return std::unique_ptr<CodeBlock>(
          new CodeBlock(data, realAddress, endAddress)
          );
      }

      return nullptr;
    }

    // reads lines from specified file
    static std::list<std::string> readLinesFromFile(const std::string& fileName) {
      std::ifstream sourceFile(fileName);

      if (!sourceFile.is_open()) {
        std::stringstream ss;
        ss << "Could not open source code file: " << fileName;
        throw std::logic_error(ss.str().c_str());
      }

      std::list<std::string> lines;
      std::string line;

      while (std::getline(sourceFile, line)) {
        if (line[line.length() - 1] == '\r') {
          line.erase(line.length()-1, 1);
        }
        lines.push_back(line);
      }

      sourceFile.close();
      return lines;
    }

    static bool areAdjacent(const CodeBlock& block1, const CodeBlock& block2) {
      if (((block1.getEndAddress() + 1) == block2.getStartAddress())
        || ((block2.getEndAddress() + 1) == block1.getStartAddress())
        ) {
        return true;
      }

      return false;
    }

    static std::unique_ptr<CodeBlock> mergeBlock(const CodeBlock& b1, const CodeBlock& b2) {
      if ((b1.getEndAddress() + 1) == b2.getStartAddress()) {
        return std::unique_ptr<CodeBlock>(
          new CodeBlock(b1.getCode() + b2.getCode(), b1.getStartAddress(), b2.getEndAddress())
          );
      }

      return std::unique_ptr<CodeBlock>(
        new CodeBlock(b2.getCode() + b1.getCode(), b2.getStartAddress(), b1.getEndAddress())
        );
    }

    static void addCodeBlock(CodeBlock& newBlock, std::list<CodeBlock>& codeBlocks) {
      std::list<CodeBlock>::iterator codeBlockIter = codeBlocks.begin();

      while (codeBlockIter != codeBlocks.end()) {
        if (areAdjacent(*codeBlockIter, newBlock)) {
          std::unique_ptr<CodeBlock> pMergedBlock = mergeBlock(newBlock, *codeBlockIter);
          codeBlocks.erase(codeBlockIter++);
          codeBlocks.push_back(*pMergedBlock);
          return;
        }
        else {
          codeBlockIter++;
        }
      }
      codeBlocks.push_back(newBlock);
    }


  public:
    static std::list<CodeBlock> parse(const std::string& fileName) {
      std::list<std::string> lines = readLinesFromFile(fileName);

      std::list<CodeBlock> codeBlocks;
      IntelHexParser::offset = 0;
      uint16_t lineIndex = 0;

      for (std::string line : lines) {
        std::unique_ptr<CodeBlock> pCodeBlock;
        try {
          pCodeBlock = parseLine(line);
        }
        catch (std::exception& ex) {
          throw std::logic_error(addLineInfo(ex.what(), line, lineIndex).c_str());
        }

        if (pCodeBlock) {
          addCodeBlock(*pCodeBlock, codeBlocks);
        }

        lineIndex++;
      }
      return codeBlocks;
    }
  };

  // for definition of static member
  uint16_t IntelHexParser::offset = 0;


  // parses iqrf files and provides data from them
  class IqrfParser {
  private:

    // length of line in IQRF plugin file
    static const int LINE_LENGTH = 40;

  public:
    static CodeBlock parse(const std::string& fileName) {
      std::ifstream sourceFile(fileName);

      if (!sourceFile.is_open()) {
        std::stringstream ss;
        ss << "Could not open source code file: " << fileName;
        throw std::logic_error(ss.str().c_str());
      }

      std::string line;
      std::basic_string<uint8_t> data;

      while (std::getline(sourceFile, line)) {
        // comment
        if (line.find_first_of('#') == 0) {
          continue;
        }

        if (line.size() != LINE_LENGTH) {
          throw std::logic_error("Illegal length of IQRF plugin file.");
        }

        for (int i = 0; i < LINE_LENGTH; i += 2) {
          uint8_t b = parseHexaByte(line, i);
          data.push_back(b);
        }
      }

      sourceFile.close();

      CodeBlock codeBlock(data, 0, data.size() - 1);
      return codeBlock;
    }
  };

  // size of memory block according to DPA specification
  static const int BLOCK_SIZE = 64;

  // initial values for calculating checksum
  static const int CRC_INIT_VALUE_HEX = 0x01;
  static const int CRC_INIT_VALUE_IQRF = 0x03;
}


namespace iqrf {

  // definition
  DataPreparer::Imp* DataPreparer::m_imp = nullptr;

  class DataPreparer::Imp {
  public:
    Imp() {}
    ~Imp() {}

    void checkFileName(const std::string& fileName) {
      if (fileName.empty()) {
        throw std::logic_error("Empty file name.");
      }
    }

    // finds block with custom DPA handler
    const CodeBlock* findHandlerBlock(const std::list<CodeBlock>& codeBlocks) {
      for (const CodeBlock& block : codeBlocks) {
        if (block.getStartAddress() == (0x3A20*2)) {
          return &block;
        }
      }
      return nullptr;
    }

    // calculates checksum with defined initial value and length of code block
    uint16_t calculateChecksum(
      const CodeBlock& block, uint16_t initialValue, uint16_t length
    )
    {
      uint16_t checksum = initialValue;

      size_t byteIndex = 0;
      for (
        uint16_t address = block.getStartAddress();
        address < block.getStartAddress() + length;
        address++, byteIndex++
        )
      {
        uint8_t oneByte = 0x00;
        if (block.getEndAddress() - address >= 0) {
          oneByte = block.getCode()[byteIndex] & 0xFF;
        }

        // One’s Complement Fletcher Checksum
        uint16_t tempL = checksum & 0xff;
        tempL += oneByte;

        if ((tempL & 0x100) != 0) {
          tempL++;
        }

        uint16_t tempH = checksum >> 8;
        tempH += tempL & 0xff;

        if ((tempH & 0x100) != 0) {
          tempH++;
        }

        checksum = (tempL & 0xff) | (tempH & 0xff) << 8;
      }

      return checksum;
    }

    // fills byte block from specified source and offset from the beggining
    void fillByteBlock(
      std::basic_string<uint8_t>& byteBlock,
      const std::basic_string<uint8_t>& source,
      uint8_t blockSize,
      uint16_t sourceOffset
    )
    {
      for (uint8_t i = 0; i < blockSize; i++) {
        if ((sourceOffset + i) >= source.size()) {
          byteBlock[i] = 0x00;
        }
        else {
          byteBlock[i] = source[sourceOffset + i];
        }
      }

      // update size of the byte block
      byteBlock.resize(blockSize);
    }

    // prepares specified code block as 16 byte blocks
    void prepareAs16ByteBlocks(const CodeBlock& block, PreparedData::Data& data)
    {
      std::basic_string<uint8_t> byteBlock;
      uint16_t byteCounter = 0;

      for (
        uint16_t address = block.getStartAddress();
        address <= block.getEndAddress();
        )
      {
        fillByteBlock(byteBlock, block.getCode(), 16, byteCounter);
        data.push_back(byteBlock);
        byteCounter += 16;
        address += 16;
      }

      // it is necessary to align block with DPA specification
      while (((data.size() * 16) % BLOCK_SIZE) != 0) {
        for (uint8_t i = 0; i < 16; i++) {
          byteBlock[i] = 0x00;
        }
        data.push_back(byteBlock);
      }
    }

    // prepares specified code block as most effective for transfer into EEEPROM
    void prepareAsMostEffective(const CodeBlock& block, PreparedData::Data& data)
    {
      std::basic_string<uint8_t> byteBlock;
      uint16_t byteCounter = 0;

      for (
        uint16_t address = block.getStartAddress();
        address <= block.getEndAddress();
        )
      {
        fillByteBlock(byteBlock, block.getCode(), 48, byteCounter);
        data.push_back(byteBlock);
        byteCounter += 48;

        fillByteBlock(byteBlock, block.getCode(), 16, byteCounter);
        data.push_back(byteBlock);
        byteCounter += 16;

        fillByteBlock(byteBlock, block.getCode(), 16, byteCounter);
        data.push_back(byteBlock);
        byteCounter += 16;

        fillByteBlock(byteBlock, block.getCode(), 48, byteCounter);
        data.push_back(byteBlock);
        byteCounter += 48;

        address += 128;
      }
    }


    std::unique_ptr<PreparedData> prepareDataFromHex(
      const std::string& fileName, bool isForBroadcast
    )
    {
      std::list<CodeBlock> codeBlocks = IntelHexParser::parse(fileName);

      const CodeBlock* handlerBlock = findHandlerBlock(codeBlocks);
      if (handlerBlock == nullptr) {
        throw std::logic_error(
          "Selected.hex file does not include Custom DPA "
          "handler section or the code does not start with "
          "clrwdt() marker."
        );
      }

      // calculating rounded length of handler in memory
      uint16_t length = (handlerBlock->getLength() + (64 - 1)) & ~(64 - 1);

      // calculating checksum with initial value 1 (defined for DPA handler)
      uint16_t checksum = calculateChecksum(*handlerBlock, CRC_INIT_VALUE_HEX, length);

      PreparedData::Data data;
      if (isForBroadcast) {
        prepareAs16ByteBlocks(*handlerBlock, data);
      }
      else {
        prepareAsMostEffective(*handlerBlock, data);
      }

      return std::unique_ptr<PreparedData>(new PreparedDataImpl(data, length, checksum));
    }


    std::unique_ptr<PreparedData> prepareDataFromIqrf(
      const std::string& fileName, bool isForBroadcast
    )
    {
      CodeBlock codeBlock = IqrfParser::parse(fileName);

      uint16_t length = codeBlock.getLength();

      // calculating checksum with initial value 1 (defined for DPA handler)
      uint16_t checksum = calculateChecksum(codeBlock, CRC_INIT_VALUE_IQRF, length);

      PreparedData::Data data;
      if (isForBroadcast) {
        prepareAs16ByteBlocks(codeBlock, data);
      }
      else {
        prepareAsMostEffective(codeBlock, data);
      }

      return std::unique_ptr<PreparedData>(new PreparedDataImpl(data, length, checksum));
    }

  };

  std::unique_ptr<PreparedData> DataPreparer::prepareData(
    IOtaUploadService::LoadingContentType loadingContent,
    const std::string& fileName,
    bool isForBroadcast
  )
  {
    m_imp = shape_new DataPreparer::Imp();

    m_imp->checkFileName(fileName);

    std::unique_ptr<PreparedData> preparedData;

    switch (loadingContent) {
    case IOtaUploadService::LoadingContentType::Hex:
      preparedData = m_imp->prepareDataFromHex(fileName, isForBroadcast);
      break;
    case IOtaUploadService::LoadingContentType::Iqrf_plugin:
      preparedData = m_imp->prepareDataFromIqrf(fileName, isForBroadcast);
      break;
    default:
      throw std::logic_error("Unsupported type of loading content");
    }

    delete m_imp;

    return preparedData;
  }

}
