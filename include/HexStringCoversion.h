#pragma once

#include "Trace.h"
#include <exception>
#include <algorithm>
#include <sstream>
#include <chrono>

namespace iqrf {
  /// \brief Parse binary data encoded hexa
  /// \param [out] to buffer for result binary data
  /// \param [in] from hexadecimal string
  /// \param [in] maxlen maximal length of binary data
  /// \return length of result
  /// \details
  /// Gets hexadecimal string in form e.g: "00 a5 b1" (space separation) or "00.a5.b1" (dot separation) and parses to binary data
  int parseBinary(uint8_t* to, const std::string& from, int maxlen)
  {
    int retval = 0;
    if (!from.empty()) {
      std::string buf = from;
      std::replace(buf.begin(), buf.end(), '.', ' ');
      std::istringstream istr(buf);

      int val;
      while (retval < maxlen) {
        if (!(istr >> std::hex >> val)) {
          if (istr.eof()) break;
          THROW_EXC_TRC_WAR(std::logic_error, "Unexpected format: " << PAR(from));

        }
        to[retval++] = (uint8_t)val;
      }
    }
    return retval;
  }

  /// \brief Parse templated ordinary type T encoded hexa
  /// \param [out] to result binary data
  /// \param [in] from hexadecimal string
  /// \details
  /// Gets hexadecimal string in form e.g: "00a5b1" and inerpret it as templated ordinary type
  template<typename T>
  void parseHexaNum(T& to, const char* from)
  {
    int val = 0;
    std::istringstream istr(from);
    if (istr >> std::hex >> val) {
      to = (T)val;
    }
    else {
      THROW_EXC_TRC_WAR(std::logic_error, "Unexpected format: " << PAR(from));
    }
  }

  /// \brief parse time up to seconds granularity in format YYY-MM-DDThh:mm:ss
  /// \param [in] from string to be parsed
  /// \return encoded time point
  std::chrono::time_point<std::chrono::system_clock> parseTimestamp(const std::string& from)
  {
    std::chrono::time_point<std::chrono::system_clock> retval = std::chrono::system_clock::now();

    if (!from.empty()) {
      int tm_year = 0, tm_mon = 1;

      time_t rawtime;
      tm * tm1;
      time(&rawtime);
      tm1 = localtime(&rawtime);

      std::string buf(from);
      std::replace(buf.begin(), buf.end(), '-', ' ');
      std::replace(buf.begin(), buf.end(), 'T', ' ');
      std::replace(buf.begin(), buf.end(), ':', ' ');
      std::replace(buf.begin(), buf.end(), '.', ' ');

      std::istringstream is(buf);
      is >> tm_year >> tm_mon >> tm1->tm_mday >> tm1->tm_hour >> tm1->tm_min >> tm1->tm_sec;
      tm1->tm_year = tm_year - 1900;
      tm1->tm_mon = tm_mon - 1;

      time_t tt = mktime(tm1);

      std::chrono::time_point<std::chrono::system_clock> retval = std::chrono::system_clock::now();

      if (tt >= 0) {
        retval = std::chrono::system_clock::from_time_t(tt);
      }
    }

    return retval;
  }

  /// \brief Encode uint_8 to hexa string
  /// \param [in] from value to be encoded
  /// \return encoded string
  std::string encodeHexaNum(uint8_t from)
  {
    std::ostringstream os;
    os.fill('0'); os.width(2);
    os << std::hex << (int)from;
    return os.str();
  }

  /// \brief Encode uint_16 to hexa string
  /// \param [in] from value to be encoded
  /// \return encoded string
  std::string encodeHexaNum(uint16_t from)
  {
    std::ostringstream os;
    os.fill('0'); os.width(4);
    os << std::hex << (int)from;
    return os.str();
  }

  /// \brief Encode binary data to hexa string
  /// \param [in] len length of dat to be encoded
  /// \return encoded string
  /// \details
  /// Encode binary data to hexadecimal string in form e.g: "00 a5 b1" (space separation) or "00.a5.b1" (dot separation)
  /// Used separation is controlled by member m_dotNotation and it is hardcoded as dot separation it this version
  std::string encodeBinary(const uint8_t* from, int len)
  {
    std::string to;
    if (len > 0) {
      std::ostringstream ostr;
      ostr << shape::TracerMemHex(from, len, '.');
      to = ostr.str();
      if (to[to.size() - 1] == '.') {
        to.pop_back();
      }
    }
    return to;
  }

  /// \brief Encode timestamp
  /// \param [in] from timestamp to be encoded
  /// \return encoded string
  std::string encodeTimestamp(std::chrono::time_point<std::chrono::system_clock> from)
  {
    using namespace std::chrono;

    std::string to;
    if (from.time_since_epoch() != system_clock::duration()) {
      auto fromUs = std::chrono::duration_cast<std::chrono::microseconds>(from.time_since_epoch()).count() % 1000000;
      auto time = std::chrono::system_clock::to_time_t(from);
      //auto tm = *std::gmtime(&time);
      auto tm = *std::localtime(&time);

      char buf[80];
      strftime(buf, sizeof(buf), "%FT%T", &tm);

      std::ostringstream os;
      os.fill('0'); os.width(6);
      //os << std::put_time(&tm, "%F %T.") <<  fromUs; // << std::put_time(&tm, " %Z\n");
      os << buf << "." << fromUs;

      to = os.str();
    }
    return to;
  }
}
