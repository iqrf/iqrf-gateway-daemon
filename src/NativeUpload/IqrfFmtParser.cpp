/*
 * Parse file in iqrf format into internal representation.
 * Author: Vlastimil Kosar <kosar@rehivetrch.com>
 * License: TBD
 */
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <array>
#include <map>
#include <locale>

#include "string_operations.h"
#include "IqrfFmtParser.h"
#include "TrFmtException.h"

const size_t LINE_LEN = 40;

static bool isCommentHeader(const std::string& str) {
    size_t pos = str.find_first_of('#');
    if (str[pos + 1] == '$') {
        return true;
    } else {
        return false;
    }
}

static std::string getHeader(const std::string& str) {
    size_t pos = str.find_first_of('#');
    size_t last = str.length() - 1;
    int i;
    std::locale loc;

    for (i = last; i >= 0; i--) {
        if (!std::isspace(str[i], loc)) {
            break;
        }
    }

    return str.substr(pos + 2, i - (pos + 2) + 1);
}

static int getLineCounter(const std::string& str) {
    return std::stoi(str.substr(str.length() - 4), nullptr, 16);
}

void IqrfPrgHeader::add(std::string line) {
    if (!isCommentHeader(line)) {
        return;
    }

    std::string header = getHeader(line);

    index++;
    switch (index) {
        case 1: {
            if (header.length() != 2) {
                TR_THROW_EXCEPTION(TrException, "Invalid size of first programming header!");
            }
            switch(header[0]) {
                case '4':
                    mcu = TrMcu::PIC16F1938;
                    break;
                default:
                    TR_THROW_EXCEPTION(TrException, "Invalid type of MCU in first programming header - " + std::string(1, header[0]) + "!");
                    break;
            }
            switch(header[1]) {
                case '0':
                    serie = TrSerie::DCTR_5xD;
                    break;
                case '1':
                    serie = TrSerie::DCTR_7xD;
                    break;
                default:
                    TR_THROW_EXCEPTION(TrException, "Invalid type of Tr serie in first programming header - " + std::string(1, header[1]) + "!");
                    break;
            }
            break;
        }
        case 2: {
            std::vector<std::string>::iterator itr;
            std::vector<std::string> parts = tokenize(header, ";");
            TrOsVersion osVersion;
            for (itr = parts.begin(); itr != parts.end(); itr++) {
                switch ((*itr).length()) {
                    case 2: {
                        osVersion = std::stoul(*itr, nullptr, 16);
                        std::pair<TrOsBuild, TrOsBuild> osBuild(0, 0xffff);
                        supportedOs.insert(std::pair<TrOsVersion, std::pair<TrOsBuild, TrOsBuild>>(osVersion, osBuild));
                        break;
                    }
                    case 6: {
                        osVersion = std::stoul((*itr).substr(0, 2), nullptr, 16);
                        TrOsBuild build = std::stoul((*itr).substr(2, 4), nullptr, 16);
                        std::pair<TrOsBuild, TrOsBuild> osBuild(build, build);
                        supportedOs.insert(std::pair<TrOsVersion, std::pair<TrOsBuild, TrOsBuild>>(osVersion, osBuild));
                        break;
                    }
                    case 10: {
                        osVersion = std::stoul((*itr).substr(0, 2), nullptr, 16);
                        TrOsBuild buildMin = std::stoul((*itr).substr(2, 4), nullptr, 16);
                        TrOsBuild buildMax = std::stoul((*itr).substr(6, 4), nullptr, 16);
                        std::pair<TrOsBuild, TrOsBuild> osBuild(buildMin, buildMax);
                        supportedOs.insert(std::pair<TrOsVersion, std::pair<TrOsBuild, TrOsBuild>>(osVersion, osBuild));
                        break;
                    }
                    default:
                        TR_THROW_EXCEPTION(TrException, "Mallformed OS version and build record in second programming header! Offending value is: " + (*itr) + "");
                        break;
                }
            }
            break;
        }
        case 3:
            std::cerr << "Note: Build date & time: " << header << "\n";
            break;
        case 4:
            std::cerr << "Note: Description: " << header << "\n";
            break;
        default:
            std::cerr << "Warning: Unrecognized programming header: \n" << "[" << index << "] " << header << " is ignored!\n";
            break;
    }
}

bool IqrfPrgHeader::check(TrModuleInfo& info) {
    std::map<TrOsVersion, std::pair<TrOsBuild, TrOsBuild>>::iterator itr;
    if (mcu != info.mcu) {
        return false;
    }
    if (serie != info.serie) {
        return false;
    }
    itr = supportedOs.find(info.osVersion);
    if (itr == supportedOs.end()) {
        return false;
    }
    if (info.osBuild < (*itr).second.first) {
        return false;
    }
    if (info.osBuild > (*itr).second.second) {
        return false;
    }
    return true;
}

void IqrfFmtParser::parse() {
    std::string line;
    std::ifstream infile(file_name);
    size_t line_no = 0;
    size_t position;
    int last_cnt = -1;
    int cnt;

    while (std::getline(infile, line))
    {
        std::basic_string<unsigned char> bdata;

        line_no++;

        // Check for programming header in comments
        if (isCommentHeader(line)) {
            prgHeader.add(line);
            continue;
        }

        // Remove comments and trim whitespace
        line = trim(uncomment(line));

        // Skip empty line
        if (line.length() == 0)
            continue;

        // Get line counter
        cnt = getLineCounter(line);

        // Every line in iqrf file which is not a comment has exactly LINE_LEN (40) chars
        if (line.length() != LINE_LEN) {
            TR_THROW_FMT_EXCEPTION(file_name, line_no, 0, "Invalid line length in iqrf file - expected 36!");
        }

        // Check for invalid characters
        if ((position = line.find_first_not_of("0123456789abcdefABCDEF")) != std::string::npos) {
            TR_THROW_FMT_EXCEPTION(file_name,  line_no, position, "Invalid character in iqrf file!");
        }

        // Check line counter sequence
        if (last_cnt + 1 != cnt) {
            TR_THROW_FMT_EXCEPTION(file_name, line_no, 0, "Invalid line counter sequence!");
        } else {
            last_cnt = cnt;
        }

        bdata.resize(LINE_LEN / 2);

        // Convert hexadecimal values to bytes
        for (unsigned int i = 0; i < LINE_LEN / 2; i++) {
            bdata[i] = std::stoul(line.substr(i * 2, 2), nullptr, 16);
        }

        // Store data
        blines.push_back(bdata);
    }
}
