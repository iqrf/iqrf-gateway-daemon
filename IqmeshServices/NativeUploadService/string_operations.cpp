/*
 * Usefull std::string operations.
 * Author: Vlastimil Kosar <kosar@rehivetrch.com>
 * License: TBD
 */

#include <string>
#include <vector>
#include "string_operations.h"

std::string trim(const std::string& str, const std::string& whitespace) {
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::string::npos)
        return "";
    size_t stop = str.find_last_not_of(whitespace);

    return str.substr(start, stop - start + 1);
}

std::string uncomment(const std::string& str) {
    size_t pos = str.find_first_of('#');
    if (pos == std::string::npos)
        return str;
    return str.substr(0, pos);
}

std::vector<std::string> tokenize(const std::string& str, const std::string& delimiters)
{
    std::vector<std::string> vect;
    size_t last = str.find_first_not_of(delimiters, 0);
    size_t pos = str.find_first_of(delimiters, last);

    while (std::string::npos != pos || std::string::npos != last) {
        vect.push_back(str.substr(last, pos - last));
        last = str.find_first_not_of(delimiters, pos);
        pos = str.find_first_of(delimiters, last);
    }
    return vect;
}
