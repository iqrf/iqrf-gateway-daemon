/*
 * Parse file in hex format into internal representation.
 * Author: Vlastimil Kosar <kosar@rehivetrch.com>
 * License: TBD
 */
#include <vector>
#include <string>
#include <fstream>
#include <array>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <iomanip>

#include "string_operations.h"
#include "HexFmtParser.h"
#include "TrFmtException.h"

#include "Trace.h"

const size_t MIN_LINE_LEN = 11;
const size_t MAX_LINE_LEN = 521;
const size_t TR_LINE_LEN = 32; // Any data uploaded to TR flash and external memory must be 32B long
const size_t TR_LINE_LEN_MIN = 1; // Minimal write data length
const size_t TR_LINE_LEN_MAX = 32; // Maximal write data length

const size_t TR_MEMORY_SIZE = 65536; // Maximal memory size of TR - 16b addressing


// address ranges accodrding to various types of memories of TR module
const uint16_t EEEPROM_ADDR_BEGIN = 0x0200;
const uint16_t EEEPROM_ADDR_END = 0x09FF;

const uint16_t LICENCED_FLASH_ADDR_BEGIN = 0x2C00;
const uint16_t LICENCED_FLASH_ADDR_END = 0x37BF;

const uint16_t CONFIG_AREA_ADDR_BEGIN = 0x37C0;
const uint16_t CONFIG_AREA_ADDR_END = 0x37DF;

const uint16_t MAIN_FLASH_ADDR_BEGIN = 0x3A00;
const uint16_t MAIN_FLASH_ADDR_END = 0x3FFF;

const uint16_t EEPROM_ADDR_BEGIN = 0xF000;
const uint16_t EEPROM_ADDR_END = 0xF0FF;




bool verify_record_csum(const std::string& str) {
    size_t len = str.length() - 1;
    unsigned int sum = 0;
    
    std::string data = str.substr(1, len);
    for (int i = 0; i < len / 2; i++) {
        sum += std::stoul(data.substr(i * 2, 2), nullptr, 16);
    }
    
    return (sum & 0xff) == 0;
}

void generateRecordCsum(std::basic_string<unsigned char> &str) {
    unsigned int sum = 0;
    std::basic_string<unsigned char>::iterator itr;
    
    for (itr = str.begin(); itr != str.end(); itr++) {
        sum += *itr;
    }
    sum = (~sum) & 0xff;
    str.push_back(sum);
}

// returns type of memory
TrMemory getMemoryType(uint16_t address) {
  if ((address >= EEEPROM_ADDR_BEGIN) && (address <= EEEPROM_ADDR_END)) {
    return TrMemory::EXTERNAL_EEPROM;
  }

  if ((address >= LICENCED_FLASH_ADDR_BEGIN) && (address <= LICENCED_FLASH_ADDR_END)) {
    return TrMemory::FLASH;
  }

  if ((address >= MAIN_FLASH_ADDR_BEGIN) && (address <= MAIN_FLASH_ADDR_END)) {
    return TrMemory::FLASH;
  }

  if ((address >= EEPROM_ADDR_BEGIN) && (address <= EEPROM_ADDR_END)) {
    return TrMemory::INTERNAL_EEPROM;
  }

  return TrMemory::ERROR;
}

// selects and returns lines according to specified memory type
std::vector<HexDataRecord> selectLines(const TrMemory memType, const std::vector<HexDataRecord>& lines) 
{
  std::vector<HexDataRecord> selectedLines;

  for (const HexDataRecord& dataRecord : lines) {
    if (dataRecord.memory == memType) {
      selectedLines.push_back(dataRecord);
    }
  }

  return selectedLines;
}

// split programming data lines into chunks according to the type of target memory
// and put them into the specified result container
void splitLines(
  const TrMemory memType, 
  std::vector<HexDataRecord>& lines, 
  std::vector<HexDataRecord>& result
)
{
  std::array<unsigned char, TR_MEMORY_SIZE> prgData;
  std::array<bool, TR_MEMORY_SIZE> prgDataValid;
  
  if ((memType == TrMemory::FLASH) || (memType == TrMemory::EXTERNAL_EEPROM)) 
  {
    // Convert to Tr upload data
    // Init fake data memory for programming data grouping
    prgData.fill(0);
    prgDataValid.fill(false);
    std::vector<HexDataRecord>::iterator itr;
    std::array<bool, TR_MEMORY_SIZE>::iterator itrPrgDataValid;

    // Programm the fake memory and note programmed positions
    for (itr = lines.begin(); itr != lines.end(); itr++) {
      std::copy((*itr).data.begin(), (*itr).data.end(), prgData.begin() + (*itr).addr);
      std::fill_n(prgDataValid.begin() + (*itr).addr, (*itr).data.size(), true);
    }

    // Create Tr prg data lines with width 32B
    for (itrPrgDataValid = prgDataValid.begin(); itrPrgDataValid < prgDataValid.end(); itrPrgDataValid += TR_LINE_LEN) {
      if (std::any_of(itrPrgDataValid, itrPrgDataValid + TR_LINE_LEN, [](bool b) {return b == true; })) {
        std::basic_string<unsigned char> data;
        size_t addr = std::distance(prgDataValid.begin(), itrPrgDataValid);
        data.resize(TR_LINE_LEN);
        std::copy_n(prgData.begin() + addr, TR_LINE_LEN, data.begin());
        result.push_back(HexDataRecord(addr, data, memType));
      }
    }
  }
  else if (memType == TrMemory::INTERNAL_EEPROM) {
    // Check length of programming data and splir if length exceeds limit
    std::vector<HexDataRecord>::iterator itr;
    for (itr = lines.begin(); itr != lines.end(); itr++) {
      size_t dLen = (*itr).data.size();
      if (dLen > TR_LINE_LEN_MAX) {
        // Split programming line which is too long
        int parts = static_cast<int>(ceil(dLen / static_cast<double>(TR_LINE_LEN_MAX)));
        for (int i = 0; i < parts; i++) {
          size_t addr = (*itr).addr + i * TR_LINE_LEN_MAX;
          size_t len = (i*TR_LINE_LEN_MAX + TR_LINE_LEN_MAX > dLen) ? (dLen - i*TR_LINE_LEN_MAX) : TR_LINE_LEN_MAX;
          std::basic_string<unsigned char> data;
          data.resize(len);
          std::copy_n((*itr).data.begin() + i*TR_LINE_LEN_MAX, len, data.begin());
          result.push_back(HexDataRecord(addr, data, memType));
        }
      }
      else if (dLen < TR_LINE_LEN_MIN) {
        THROW_EXC(std::logic_error, "Empty data line in hex file!\n");
      }
      else {
        result.push_back(*itr);
      }
    }
  }
  else {
    THROW_EXC(std::logic_error, "Invalid TR memory type for HEX file!\n");
  }
}

void HexFmtParser::parse() {
    std::string line;
    std::ifstream infile(file_name);
    size_t line_no = 0;
    size_t position;
    bool finished = false;
    std::vector<HexDataRecord> variableLines;
    std::array<unsigned char, TR_MEMORY_SIZE> prgData;
    std::array<bool, TR_MEMORY_SIZE> prgDataValid;
    
    // base address - segment
    uint16_t base = 0;

    while (std::getline(infile, line))
    {
        size_t len;
        uint16_t offset;
        uint16_t addr;
        size_t data_len;
        unsigned char type;
        std::basic_string<uint8_t> data;
        
        line_no++;
        
        // Trim whitespace
        line = trim(line);
     
        len = line.length();
        
        // Skip empty line
        if (len == 0)
            continue;
        
        // Check if file continue after End Of File record
        if (finished) {
            TR_THROW_FMT_EXCEPTION(file_name, line_no, 0, "Hex file continues after End Of File record!");
        }
        
        // Every line in hex file must be at least 11 characters long
        if (len < MIN_LINE_LEN) {
            TR_THROW_FMT_EXCEPTION(file_name, line_no, 0, "Invalid length of record in hex file - line is too short!");
        }
        
        // Every line in hex file must be at most 521 characters long
        if (len > MAX_LINE_LEN) {
            TR_THROW_FMT_EXCEPTION(file_name, line_no, 0, "Invalid length of record in hex file - line is too long!");
        }
        
        // Every line must be odd
        if (len % 2 != 1) {
            TR_THROW_FMT_EXCEPTION(file_name, line_no, 0, "Invalid length of record in hex file - line length is not odd!");
        }
        
        // Check for invalid characters
        if ((position = line.find_first_not_of(":0123456789abcdefABCDEF")) != std::string::npos) {
            TR_THROW_FMT_EXCEPTION(file_name, line_no, position, "Invalid length character in hex file!");
        }
        
        // Check for record start code
        if (line[0] != ':') {
            TR_THROW_FMT_EXCEPTION(file_name, line_no, 1, "Missing record start code : in hex file!");
        }
        
        // Check checksum
        if (!verify_record_csum(line)) {
            TR_THROW_FMT_EXCEPTION(file_name, line_no, len - 2, "Invalid checksum of record in hex file!");
        }
        
        // Get length
        data_len = std::stoul(line.substr(1, 2), nullptr, 16);
        // Get offset
        offset = std::stoul(line.substr(3, 4), nullptr, 16);
        // Get type
        type = std::stoul(line.substr(7, 2), nullptr, 16);
        
        // Check data length of record
        if (2 * data_len + 11 != len) {
            TR_THROW_FMT_EXCEPTION(file_name, line_no, 2, "Actual length of record in hex file is different from indicated length!");
        }
        
        // type of memory
        TrMemory memType;

        switch(type) {
            case 0:
                // Data record
                for (size_t i = 0; i < data_len / 2; i++) {
                    data.push_back(std::stoul(line.substr(9 + i * 2, 2), nullptr, 16));
                }
                addr = base + offset;
                
                // each address in hex is x2!
                addr /= 2;

                memType = getMemoryType(addr);
                if (memType == TrMemory::ERROR) {
                  //TR_THROW_EXCEPTION(std::logic_error, "Unknown memory type." << NAME_PAR(address, addr));
                  TRC_WARNING("Unknown memory type." << NAME_PAR(address, addr));
                  break;
                }

                variableLines.push_back(HexDataRecord(addr, data, memType));
                break;
            case 1:
                // End Of File record
                if (data_len != 0) {
                    TR_THROW_FMT_EXCEPTION(file_name, line_no, 2, "Data length of End Of File record in hex file must be 0!");
                }
                finished = true;
                break;
            case 2:
                // Extended Segment Address record
                if (data_len != 2) {
                    TR_THROW_FMT_EXCEPTION(file_name, line_no, 2, "Data length of Extended Segment Address record in hex file must be 2!");
                }
                base = std::stoul(line.substr(9, 4), nullptr, 16) * 16;
                break;
            case 3:
                // Start Segment Address record
                if (data_len != 4) {
                    TR_THROW_FMT_EXCEPTION(file_name, line_no, 2, "Data length of Start Segment Address record in hex file must be 4!");
                }
                std::cerr << "Warning: Start Segment Address record (type 03) on line " << line_no << " is ignored!\n";
                std::cerr << "         This type of record has no effect on IQRF TR device.\n";
                break;
            case 4:
                // Extended Linear Address record
                if (data_len != 2) {
                    TR_THROW_FMT_EXCEPTION(file_name, line_no, 2, "Data length of Extended Linear Address record in hex file must be 2!");
                }
                base = std::stoul(line.substr(9, 4), nullptr, 16) << 16;
                break;
            case 5:
                // Start Linear Address record
                if (data_len != 4) {
                    TR_THROW_FMT_EXCEPTION(file_name, line_no, 2, "Data length of Start Linear Address record in hex file must be 4!");
                }
                std::cerr << "Warning: Start Linear Address record (type 05) on line " << line_no << " is ignored!\n";
                std::cerr << "         This type of record has no effect on IQRF TR device.";
                break;
            default:
                TR_THROW_FMT_EXCEPTION(file_name, line_no, 8, "Unknown type of record in hex file!\n");
                break;
        }
    }
    
    // put together data records of the same type of target memory
    std::vector<HexDataRecord> flashVariableLines = selectLines(TrMemory::FLASH, variableLines);
    std::vector<HexDataRecord> eepromVariableLines = selectLines(TrMemory::INTERNAL_EEPROM, variableLines);
    std::vector<HexDataRecord> eeepromVariableLines = selectLines(TrMemory::EXTERNAL_EEPROM, variableLines);

    // split lines
    splitLines(TrMemory::FLASH, flashVariableLines, blines);
    splitLines(TrMemory::INTERNAL_EEPROM, eepromVariableLines, blines);
    splitLines(TrMemory::EXTERNAL_EEPROM, eeepromVariableLines, blines);
}

void HexFmtParser::pushBack(const uint16_t addr, const std::basic_string<uint8_t>& data, const TrMemory mem) 
{
    blines.push_back(HexDataRecord(addr, data, mem));
}

