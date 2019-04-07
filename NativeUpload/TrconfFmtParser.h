/*
 * Parse file in trconf format into internal representation.
 * Author: Vlastimil Kosar <kosar@rehivetrch.com>
 * License: TBD
 */

#ifndef __TRCONFFMTPARSER_H__
#define __TRCONFFMTPARSER_H__

#include <vector>
#include <string>
#include <array>

class TrconfFmtParser {
private:
    std::string file_name;
    unsigned char rfpgm;
    std::basic_string<unsigned char> data;
    std::basic_string<unsigned char> cfg_data_1of2;
    std::basic_string<unsigned char> cfg_data_2of2;
    bool parsed;
public:
    TrconfFmtParser(std::string name) : file_name(name) {parsed = false;}
    void parse();
    void checkChannels(unsigned char rfband);
    unsigned char getRFPMG(void);
    std::basic_string<unsigned char> getRawData(void);
    std::basic_string<unsigned char> getCfgData1of2(void);
    std::basic_string<unsigned char> getCfgData2of2(void);
};

#endif // __TRCONFFMTPARSER_H__
