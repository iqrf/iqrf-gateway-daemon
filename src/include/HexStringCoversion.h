/**
 * IQRF Gateway Daemon
 * Copyright (C) 2015-2025 IQRF Tech s.r.o., 2019-2025 MICRORISC s.r.o.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include "Trace.h"
#include <exception>
#include <algorithm>
#include <sstream>
#include <chrono>
#include <set>
#include <vector>

class HexStringConversion {
public:
  /// \brief Parse binary data encoded hexa
  /// \param [out] to buffer for result binary data
  /// \param [in] from hexadecimal string
  /// \param [in] maxlen maximal length of binary data
  /// \return length of result
  /// \details
  /// Gets hexadecimal string in form e.g: "00 a5 b1" (space separation) or "00.a5.b1" (dot separation) and parses to binary data
  static int parseBinary(uint8_t* to, const std::string& from, int maxlen)
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

  static int parseBinary(std::vector<uint8_t> & to, const std::string& from, int maxlen)
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
  static void parseHexaNum(T& to, const char* from)
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

  /// \brief Encode uint_8 to hexa string
  /// \param [in] from value to be encoded
  /// \return encoded string
  static std::string encodeHexaNum(uint8_t from)
  {
    std::ostringstream os;
    os.fill('0'); os.width(2);
    os << std::hex << (int)from;
    return os.str();
  }

  /// \brief Encode uint_16 to hexa string
  /// \param [in] from value to be encoded
  /// \return encoded string
  static std::string encodeHexaNum(uint16_t from)
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
  static std::string encodeBinary(const uint8_t* from, int len)
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

  /// \brief Returns an set of indexes of set bits in the bitmap
  /// \param [in] bitmap - array of bytes
  /// \param [in] indexFrom - starting index of the byte in the bitmap
  /// \param [in] indexTo - ending index of the byte in the bitmap
  /// \param [in] offset - value to start indexing from
  /// \return set of integers specifying the offset indexes of bits set in the bitmap
  static std::set<int> bitmapToIndexes(const uint8_t* bitmap, int indexFrom, int indexTo, int offset)
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
  static std::vector<uint8_t> indexesToBitmap(const std::set<int> & indexes, int bitmapSize)
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

};
