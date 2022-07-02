/**
 * Copyright 2015-2021 IQRF Tech s.r.o.
 * Copyright 2019-2021 MICRORISC s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include "Trace.h"
#include <exception>
#include <algorithm>
#include <sstream>
#include <chrono>
#include <set>
#include <vector>

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

  int parseBinary(std::vector<uint8_t> & to, const std::string& from, int maxlen)
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
        to.push_back((uint8_t)val);
        retval++;
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
      auto fromMs = std::chrono::duration_cast<std::chrono::milliseconds>(from.time_since_epoch()).count() % 1000;
      auto time = std::chrono::system_clock::to_time_t(from);
      //auto tm = *std::gmtime(&time);
      auto tm = *std::localtime(&time);

      char buf[80];
      strftime(buf, sizeof(buf), "%FT%T.mmm%z", &tm);
      std::string str(buf);

      // convert to ISO8601 Date (Extend) format
      std::ostringstream os;
      os.fill('0'); os.width(3);
      os << fromMs;
      str.replace(str.find("mmm"), 3, os.str());
      str.insert(str.size() - 2, 1, ':');

      to = str;

    }
    return to;
  }

  /// \brief Returns an set of indexes of set bits in the bitmap
  /// \param [in] bitmap - array of bytes
  /// \param [in] indexFrom - starting index of the byte in the bitmap
  /// \param [in] indexTo - ending index of the byte in the bitmap
  /// \param [in] offset - value to start indexing from
  /// \return set of integers specifying the offset indexes of bits set in the bitmap
  std::set<int> bitmapToIndexes(const uint8_t* bitmap, int indexFrom, int indexTo, int offset)
  {
    std::set<int> retval;

    for (int index = indexFrom; index <= indexTo; index++) {
      unsigned bitmapByte = bitmap[index];
      if (0 == bitmapByte) {
        offset += 8;
      }
      else {
        for (unsigned bitMask = 0x01; bitMask != 0x100; bitMask <<= 1) {
          if ((bitmapByte & bitMask) != 0) {
            retval.insert(offset);
          }
          offset++;
        }
      }
    }
    return retval;
  }

  /// \brief Returns a vector of bytes with the selected indexes
  /// \param [in] indexes - set of integers each specifying the index of bit to set in the bitmap
  /// \return vector of bytes with the bit set according to the indexes parameter
  std::vector<uint8_t> indexesToBitmap(const std::set<int> & indexes, int bitmapSize)
  {
    std::vector<uint8_t> retval(bitmapSize, 0);
    for (int idx : indexes) {
      int bitmapIdx = idx / (int)8;
      if (bitmapIdx >= bitmapSize) {
        THROW_EXC_TRC_WAR(std::logic_error, PAR(idx) << " is out of size: " << PAR(bitmapSize))
      }
      retval[bitmapIdx] |= 1 << (idx % 8);
    }
    return retval;
  }

  //aux class to convert from dot notation to ustring and back
  class DotMsg
  {
  public:
    DotMsg(std::basic_string<unsigned char> msg)
      :m_msg(msg)
    {}

    DotMsg(std::string dotMsg)
    {
      if (!dotMsg.empty()) {
        std::string buf = dotMsg;
        std::replace(buf.begin(), buf.end(), '.', ' ');

        std::istringstream istr(buf);

        int val;
        while (true) {
          if (!(istr >> std::hex >> val)) {
            if (istr.eof()) break;
            THROW_EXC_TRC_WAR(std::logic_error, "Unexpected format: " << PAR(dotMsg));
          }
          m_msg.push_back((uint8_t)val);
        }
      }
    }

    operator std::basic_string<unsigned char>() { return m_msg; }

    operator std::vector<unsigned char>() { return std::vector<unsigned char>(m_msg.begin(), m_msg.end()); }

    operator std::string() const
    {
      std::string to;
      if (!m_msg.empty()) {
        std::ostringstream ostr;
        ostr.setf(std::ios::hex, std::ios::basefield);
        ostr.fill('0');
        for (uint8_t c : m_msg) {
          ostr << std::setw(2) << (short int)c;
          ostr << '.';
        }
        to = ostr.str();
        to.pop_back();
      }
      return to;
    }

    friend std::ostream & operator << (std::ostream &o, const DotMsg &a)
    {
      o << (std::string)a;
      return o;
    }

  private:
    std::basic_string<unsigned char> m_msg;
  };

}
