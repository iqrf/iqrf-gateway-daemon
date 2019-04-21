/*
 * Parse file in hex format into internal representation.
 * Author: Vlastimil Kosar <kosar@rehivetrch.com>
 * License: TBD
 */

#ifndef __HEXFMTPARSER_H__
#define __HEXFMTPARSER_H__

#include <vector>
#include <string>
#include <array>

#include <TrTypes.h>

struct HexDataRecord {
    uint16_t addr;
    std::basic_string<uint8_t> data;
    TrMemory memory;

    HexDataRecord(const uint16_t a, const std::basic_string<uint8_t>& d, const TrMemory mem)
      : addr(a), data(d), memory(mem) {}
};

class HexFmtParser {
private:
    std::string file_name;
    std::vector<HexDataRecord> blines;
    TrMemory memory;

public:
    HexFmtParser(const std::string& name) : file_name(name) {}
    void parse();
    typedef std::vector<HexDataRecord>::iterator iterator;
    typedef std::vector<HexDataRecord>::const_iterator const_iterator;
    iterator begin() { return blines.begin(); }
    iterator end() { return blines.end(); }
};

#endif // __HEXFMTPARSER_H__
