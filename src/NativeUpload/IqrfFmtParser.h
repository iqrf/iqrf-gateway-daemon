/*
 * Parse file in iqrf format into internal representation.
 * Author: Vlastimil Kosar <kosar@rehivetrch.com>
 * License: TBD
 */

#ifndef __IQRFFMTPARSER_H__
#define __IQRFFMTPARSER_H__

#include <vector>
#include <string>
#include <array>
#include <map>
#include <TrTypes.h>

class IqrfPrgHeader {
private:
    int index;
    TrMcu mcu;
    TrSeries series;
    std::map<TrOsVersion, std::pair<TrOsBuild, TrOsBuild>> supportedOs;
public:
    IqrfPrgHeader() {index = 0; mcu = TrMcu::NONE; series = TrSeries::NONE;}
    void add(std::string line);
    bool check(TrModuleInfo& info);
};

class IqrfFmtParser {
private:
    std::string file_name;
    std::vector<std::basic_string<unsigned char>> blines;
    IqrfPrgHeader prgHeader;
public:
    IqrfFmtParser(std::string name) : file_name(name) {}
    void parse();
    bool check(TrModuleInfo& info) {return prgHeader.check(info);}
    typedef std::vector<std::basic_string<unsigned char>>::iterator iterator;
    typedef std::vector<std::basic_string<unsigned char>>::const_iterator const_iterator;
    iterator begin() { return blines.begin(); }
    iterator end() { return blines.end(); }
};

#endif // __IQRFFMTPARSER_H__
