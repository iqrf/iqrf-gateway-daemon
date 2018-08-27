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
    bool parsed;
public:
    TrconfFmtParser(std::string name) : file_name(name) {parsed = false;}
    void parse();
    void checkChannels(unsigned char rfband);
    unsigned char getRFPMG(void);
    std::basic_string<unsigned char> getData(void);
};

#endif // __TRCONFFMTPARSER_H__
