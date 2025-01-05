/**
 * Copyright 2015-2025 IQRF Tech s.r.o.
 * Copyright 2019-2025 MICRORISC s.r.o.
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
#include "IqrfCodeDecoder.h"
#include "Trace.h"
#include "DPA.h"
#include <algorithm>

namespace {

  // Base (number of characters in the alphabet) used to encode.
  static const int base57 = 57;

  // Alphabet used for alphanumeric coding.
  // Contains digits and uppercase+lowercase letters except 0, I, O, l, u.
  static const std::string base57alphabet = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstvwxyz";

  // Number of base57 character to encode full piece (8 bytes) .
  static const int maximumPiece57Length = 11;

  // Array to convert from number of bytes into number of base57 to encode them.
  ///
  /// base57chars base2bites  base2bitsRoundedDown
  /// 2           11.67       8
  /// 3           17.50       16
  /// 5           29.16       24
  /// 6           35.00       32
  /// 7           40.83       40
  /// 9           52.50       48
  /// 10          58.33       56
  /// 11          64.16       64
  static int piece57Lengths[] = { 0, 2, 3, 5, 6, 7, 9, 10, maximumPiece57Length };

  // returns INDEX of specified piece
  static int getIndexOfPieceLen(int piece57Len) {
    for (int i = 0; i < 9; i++) {
      if (piece57Lengths[i] == piece57Len) {
        return i;
      }
    }
    return -1;
  }

  static char checkChar(const std::string& text)
  {
    int sum = 0;
    for (int index = text.length() - 1, factor = 2; index >= 0; index--, factor = (1 + 2) - factor)
    {
      int addend = factor * base57alphabet.find(text[index]);
      sum += addend / base57 + (addend % base57);
    }

    return base57alphabet[(base57 - (sum % base57)) % base57];
  }

  // Tags to mark important IQRF data objects.
  enum class Tags
  {
    // Marks end of the nibble stream.
    End = 0x0,
    MID = 0x1,
    IBK = 0x2,
    HWPID = 0x3,
    BondingChannel = 0x4,
    Nop = 0x5,
    DataBlock = 0x6,
    Text = 0x7,

    // For future tags count expansion.
    TAG_RESERVED = 0xF
  };
}

namespace iqrf {

  // Nibble "stream" reader.
  class NibbleReader {

  private:
    std::basic_string<uint8_t> m_stream;
    int m_nibbleIndex = 0;

  public:
    NibbleReader(const std::basic_string<uint8_t>& stream) {
      this->m_stream = stream;
    }

    // Reads one nibble.
    uint8_t readNibble()
    {
      unsigned int streamIndex = m_nibbleIndex / 2;
      if (streamIndex >= m_stream.length()) {
        THROW_EXC(std::logic_error, "No more data available to read.");
      }

      uint8_t nibble = m_stream[streamIndex];
      if (m_nibbleIndex++ % 2 == 1) {
        nibble >>= 4;
      }
      return (uint8_t)(nibble & 0x0F);
    }

    // Reads one byte.
    uint8_t readByte() {
      return (uint8_t)(readNibble() | (readNibble() << 4));
    }

    // Reads specified number of bytes.
    std::basic_string<uint8_t> readBytes(int length)
    {
      std::basic_string<uint8_t> bytes;
      for (int index = 0; index < length; index++) {
        bytes.push_back(readByte());
      }
      return bytes;
    }

  };

  // initialization of static members of IqrfCodeDecoder
  std::basic_string<uint8_t> IqrfCodeDecoder::m_mid;
  std::basic_string<uint8_t> IqrfCodeDecoder::m_ibk;
  uint16_t IqrfCodeDecoder::m_hwpId = HWPID_Default;

  void IqrfCodeDecoder::decode(const std::string& iqrfCodeOrig) {
    if (iqrfCodeOrig.empty()) {
      throw std::logic_error("IQRF Code cannot be empty.");
    }

    char checkedChar = iqrfCodeOrig[iqrfCodeOrig.length() - 1];

    std::string iqrfCode = iqrfCodeOrig.substr(0, iqrfCodeOrig.length() - 1);
    if (checkedChar != checkChar(iqrfCode)) {
      throw std::logic_error("IQRF Code ends by an incorrect check character.");
    }

    int lastPiece57Length = iqrfCode.length() % maximumPiece57Length;

    int lastPieceLenIndex = getIndexOfPieceLen(lastPiece57Length);
    if (lastPieceLenIndex == -1) {
      THROW_EXC(std::logic_error, "IQRF Code has incorrect length: " << PAR(iqrfCode.length()));
    }

    std::basic_string<uint8_t> result;
    result.resize(sizeof(uint64_t) * (iqrfCode.length() / maximumPiece57Length) + lastPieceLenIndex);

    for (unsigned int piece57Index = 0, pieceIndex = 0; piece57Index < iqrfCode.length(); piece57Index += maximumPiece57Length, pieceIndex += sizeof(uint64_t))
    {
      int piece57Length = std::min((size_t)maximumPiece57Length, iqrfCode.length() - piece57Index);

      uint64_t piece = 0;
      for (int inPiece57Index = piece57Length; --inPiece57Index >= 0; )
      {
        char char57 = iqrfCode[piece57Index + inPiece57Index];
        size_t alphabet57Index = base57alphabet.find(char57);
        if (alphabet57Index == std::string::npos) {
          THROW_EXC(std::logic_error, "IQRF Code contains an incorrect character " << PAR(char57) << " at position: " << PAR(piece57Index + inPiece57Index));
        }
        piece = piece * base57 + (uint64_t)alphabet57Index;
      }

      for (int pieceLenIndex = getIndexOfPieceLen(piece57Length); --pieceLenIndex >= 0; )
      {
        result[pieceIndex + pieceLenIndex] = (uint8_t)(piece & 0xFF);
        piece >>= 8;
      }
    }

    NibbleReader nibbleReader(result);

    // sequence of bytes created
    for (Tags tag; (tag = (Tags)nibbleReader.readNibble()) != Tags::End; )
    {
      switch (tag)
      {
      case Tags::MID:
        m_mid = nibbleReader.readBytes(4);
        break;

      case Tags::IBK:
        m_ibk = nibbleReader.readBytes(16);
        break;

      case Tags::HWPID:
        m_hwpId = nibbleReader.readByte() << 8 | nibbleReader.readByte();
        break;

      case Tags::BondingChannel:
        // obsolete
        break;

      case Tags::Nop:
        // no data
        break;

      case Tags::DataBlock:
        // non IQRF data
        break;

      case Tags::Text:
        // non IQRF data
        break;

      default:
        THROW_EXC(std::logic_error, "Unknown tag value.");
      }
    }
  }
}
