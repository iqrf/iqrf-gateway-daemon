/*
 * Usefull std::string operations.
 * Author: Vlastimil Kosar <kosar@rehivetrch.com>
 * License: TBD
 */

#include <string>
#include <vector>

std::string trim(const std::string& str, const std::string& whitespace = " \t\r\n");
std::string uncomment(const std::string& str);
std::vector<std::string> tokenize(const std::string& str, const std::string& delimiters = ";");
