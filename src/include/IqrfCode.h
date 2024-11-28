/**
 * Copyright 2015-2024 IQRF Tech s.r.o.
 * Copyright 2019-2024 MICRORISC s.r.o.
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
#include <algorithm>
#include <cstdint>
#include <string>

#include "DPA.h"
#include "Trace.h"

namespace iqrf::code {

  /**
   * IQRF values tags
   */
  enum class Tags {
    /**
     * End of stream
     */
    END,
    /**
     * Module ID
     */
    MID,
    /**
     * Individual bonding key
     */
    IBK,
    /**
     * Hardware profile ID
     */
    HWPID,
    /**
     * Logical address in IQMESH network
     */
    LOGICAL_ADDRESS,
    /**
     * NO operation, used to align data in stream
     */
    NOP,
    /**
     * Used to store custom dat.
     */
    DATA_BLOCK,
    /**
     * Used to store custom text
     */
    TEXT,
    /**
     * Hardware profile ID version
     */
    HWPID_VER
  };

  /**
   * Stream reader class
   */
  class StreamReader {
  private:
    /// Reader byte stream
    std::basic_string<uint8_t> stream;
    /// Current nibble position in stream
    int nibbleIndex = 0;

  public:
    /**
     * Delete default constructor
     */
    StreamReader() = delete;

    /**
     * Constructor
     * @param stream Byte stream to read from
     */
    StreamReader(const std::basic_string<uint8_t>& stream): stream(stream) {}

    /**
     * Reads one nibble of data from stream
     * @return Nibble of stream data
     */
    uint8_t readNibble() {
      unsigned int streamIndex = nibbleIndex / 2;
      if (streamIndex >= stream.length()) {
        THROW_EXC(std::logic_error, "No more data available to read.");
      }

      uint8_t nibble = stream[streamIndex];
      if (nibbleIndex++ % 2 == 1) {
        nibble >>= 4;
      }
      return (uint8_t)(nibble & 0x0F);
    }

    /**
     * Reads one byte of data from stream
     * @return Read data byte
     */
    uint8_t readByte() {
      return (uint8_t)(readNibble() | (readNibble() << 4));
    }

    /**
     * Reads one or more bytes of data from stream
     * @param length Number of bytes to read from stream
     * @return Read data byte(s)
     */
    std::basic_string<uint8_t> readBytes(int length) {
      std::basic_string<uint8_t> bytes;
      for (int index = 0; index < length; index++) {
        bytes.push_back(readByte());
      }
      return bytes;
    }

    /**
     * Resets stream reader to start
     */
    void reset() {
      nibbleIndex = 0;
    }
  };

  /**
   * Stream writer class
   */
  class StreamWriter {
  private:
    /// Writer byte stream
    std::basic_string<uint8_t> stream = {};
    /// Number of bits currently held in byte
    int bitsWritten = 0;
    /// Byte of data to be written to stream
    uint8_t byteToWrite = 0;
  public:
    /**
     * Write nibble value to stream
     * @param nibble Nibble to write
     */
    void writeNibble(uint8_t nibble) {
      if (nibble > 0x0F) {
        throw std::logic_error("Value to write to stream is not a nibble :" + std::to_string(nibble) + ".");
      }

      byteToWrite |= (uint8_t)(nibble << bitsWritten);
      bitsWritten += 4;
      if (bitsWritten >= 8) {
        // 2 nibbles prepared, write byte to stream
        bitsWritten -= 8;
        stream.push_back(byteToWrite);
        byteToWrite = 0;
      }
    }

    /**
     * Write byte to stream
     * @param byte Byte to write
     */
    void writeByte(uint8_t byte) {
      writeNibble(byte & 0x0F);
      writeNibble(byte >> 4);
    }

    /**
     * Write multiple bytes to stream
     * @param bytes Bytes to write
     */
    void writeBytes(std::basic_string<uint8_t> bytes) {
      for (const auto &byte : bytes) {
        writeByte(byte);
      }
    }

    /**
     * Write nibble and bytes
     * @param nibble Nibble to write
     * @param bytes Bytes to write
     */
    void writeNibbleAndBytes(uint8_t nibble, std::basic_string<uint8_t> bytes) {
      writeNibble(nibble);
      writeBytes(bytes);
    }

    /**
     * Finalize writer stream and return written data
     * @return Written data
     */
    std::basic_string<uint8_t> finalize() {
      if (bitsWritten != 0) {
        stream.push_back(byteToWrite);
      }
      return stream;
    }

    /**
     * Resets the writer stream
     */
    void reset() {
      stream.clear();
      bitsWritten = 0;
      byteToWrite = 0;
    }
  };

  class Base57 {
  public:
    /// Base number
    static inline const int BASE = 57;
    /// Alphabet
    static inline const std::string ALPHABET = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstvwxyz";
    /// Number of base57 characters required to encode a piece (8 bytes)
    static inline const int MAXIMUM_PIECE_LENGTH = 11;
    /// Mapping of base57 characters required to encode respective number of bytes (bytes are represented by index in array)
    static inline const int PIECE_LENGTHS[] = { 0, 2, 3, 5, 6, 7, 9, 10, Base57::MAXIMUM_PIECE_LENGTH};

    /**
     * Encodes bytes of data into base57 string
     * @param data Data to encode
     * @return Encoded base57 data string
     */
    static std::string encode(const std::basic_string<uint8_t> &data) {
      if (data.size() == 0) {
        THROW_EXC(std::logic_error, "No data to encode");
      }

      std::ostringstream oss;
      for (std::size_t pieceIndex = 0; pieceIndex < data.size(); pieceIndex += sizeof(uint64_t)) {
        int pieceLength = std::min(sizeof(uint64_t), data.size() - pieceIndex);

        uint64_t piece = 0;
        for (int inPieceIndex = 0; inPieceIndex < pieceLength; inPieceIndex++) {
          piece = (piece << 8) | data[pieceIndex + inPieceIndex];
        }

        for (int piece57Length = PIECE_LENGTHS[pieceLength];  piece57Length != 0; piece57Length--) {
          oss << ALPHABET[(int)(piece % BASE)];
          piece /= BASE;
        }
      }

      auto encoded = oss.str();
      // add check char
      return encoded.append(1, generateCheckCharacter(encoded));
    }

    /**
     * Decodes base57 string into data bytes
     * @param input Input string
     * @return Decoded data
     */
    static std::basic_string<uint8_t> decode(const std::string &input) {
      int lastPieceLength = input.length() % MAXIMUM_PIECE_LENGTH;

      int lastPieceLenIndex = getIndexOfPieceLen(lastPieceLength);
      if (lastPieceLenIndex == -1) {
        THROW_EXC(std::logic_error, "IQRF Code has incorrect length: " << PAR(input.length()));
      }

      std::basic_string<uint8_t> data;
      data.resize(sizeof(uint64_t) * (input.length() / MAXIMUM_PIECE_LENGTH) + lastPieceLenIndex);

      for (unsigned int piece57Index = 0, pieceIndex = 0; piece57Index < input.length(); piece57Index += MAXIMUM_PIECE_LENGTH, pieceIndex += sizeof(uint64_t)) {
        int piece57Length = std::min((size_t)MAXIMUM_PIECE_LENGTH, input.length() - piece57Index);

        uint64_t piece = 0;
        for (int inPiece57Index = piece57Length; --inPiece57Index >= 0; ) {
          char char57 = input[piece57Index + inPiece57Index];
          size_t alphabet57Index = ALPHABET.find(char57);
          if (alphabet57Index == std::string::npos) {
            THROW_EXC(std::logic_error, "IQRF Code contains an incorrect character " << PAR(char57) << " at position: " << PAR(piece57Index + inPiece57Index));
          }
          piece = piece * BASE + (uint64_t)alphabet57Index;
        }

        for (int pieceLenIndex = getIndexOfPieceLen(piece57Length); --pieceLenIndex >= 0; ) {
          data[pieceIndex + pieceLenIndex] = (uint8_t)(piece & 0xFF);
          piece >>= 8;
        }
      }

      return data;
    }

    /**
     * Generate check character for encoded string
     * @param input Input string
     * @return Check character for string
     */
    static char generateCheckCharacter(const std::string &input) {
      int factor = 2;
      int sum = 0;

      for (int index = input.length() - 1; index >= 0; index--) {
        int addend = factor * ALPHABET.find(input[index]);
        // alternate factor, can be re-written as factor = 3 - factor
        factor = (factor == 2) ? 1 : 2;
        sum += addend / BASE + (addend % BASE);
      }

      return ALPHABET[(BASE - (sum % BASE)) % BASE];
    }

    /**
     * Validates check character of input string
     * @param input Input string
     * @return true if check character is correct, false otherwise
     */
    static bool validateCheckCharacter(const std::string &input) {
      int factor = 1;
      int sum = 0;

      for (int index = input.length() - 1; index >= 0; index--) {
        int addend = factor * ALPHABET.find(input[index]);
        // alternate factor, can be re-written as factor = 3 - factor
        factor = (factor == 2) ? 1 : 2;
        sum += (addend / BASE) + (addend % BASE);
      }

      return (sum % BASE) == 0;
    }

  private:
    /**
     * Returns index of piece length
     * @param length Piece length
     * @return Piece length index
     */
    static int getIndexOfPieceLen(int length) {
      for (int i = 0; i < 9; i++) {
        if (PIECE_LENGTHS[i] == length) {
          return i;
        }
      }
      return -1;
    }
  };

  class IqrfValues {
  private:
    /// Module ID
    uint32_t mid;
    /// Individual bonding key
    std::basic_string<uint8_t> ibk;
    /// Hardware profile ID
    uint16_t hwpid;
  public:
    /**
     * Constructor for smart connect code
     * @param mid Module ID
     * @param ibk Individual bonding key
     * @param hwpid Hardware profile ID
     */
    IqrfValues(uint32_t mid, std::basic_string<uint8_t> ibk, uint16_t hwpid): mid(mid), ibk(ibk), hwpid(hwpid) {}

    /**
     * Returns module ID as integer value
     * @return Module ID value
     */
    uint32_t getMid() const {
      return mid;
    }

    /**
     * Returns module ID as bytes
     * @return Module ID btytes
     */
    std::basic_string<uint8_t> getMidBytes() {
      return {
        static_cast<uint8_t>((mid >> 24) & 0xFF),
        static_cast<uint8_t>((mid >> 16) & 0xFF),
        static_cast<uint8_t>((mid >> 8) & 0xFF),
        static_cast<uint8_t>(mid & 0xFF)
      };
    }

    /**
     * Returns individual bonding key bytes
     * @return Individual bonding key
     */
    std::basic_string<uint8_t> getIbk() const {
      return ibk;
    }

    /**
     * Returns hardware profile ID value
     * @return Hardware profile ID value
     */
    uint16_t getHwpid() const {
      return hwpid;
    }

    /**
     * Returns hardware profile ID bytes
     * @return Hardware profile ID bytes
     */
    std::basic_string<uint8_t> getHwpidBytes() {
      return {
        static_cast<uint8_t>((hwpid >> 8) & 0xFF),
        static_cast<uint8_t>(hwpid & 0xFF)
      };
    }
  };

  /**
   * IQRF Code encoder and decoder class
   */
  class IqrfCode {
  public:
    /**
     * Encodes IQRF values to IQRF code
     * @param values IQRF values to encode
     * @return IQRF code with encoded values
     */
    static std::string encode(IqrfValues values) {
      StreamWriter writer;

      // Write MID
      writer.writeNibbleAndBytes(
        static_cast<uint8_t>(Tags::MID),
        values.getMidBytes()
      );
      // Write IBK
      writer.writeNibbleAndBytes(
        static_cast<uint8_t>(Tags::IBK),
        values.getIbk()
      );
      // Write HWPID
      writer.writeNibbleAndBytes(
        static_cast<uint8_t>(Tags::HWPID),
        values.getHwpidBytes()
      );
      // Write END tag
      writer.writeNibble(static_cast<uint8_t>(Tags::END));
      auto data = writer.finalize();

      return Base57::encode(data);
    }

    /**
     * Decodes IQRF code into MID, IBK and HWPID
     * @param iqrfCode IQRF code
     */
    static IqrfValues decode(const std::string& iqrfCode) {
      if (iqrfCode.empty()) {
        THROW_EXC(std::logic_error, "IQRF code is empty (length 0).");
      }

      if (!Base57::validateCheckCharacter(iqrfCode)) {
        THROW_EXC(std::logic_error, "IQRF code contains incorrect check character.");
      }

      std::string code = iqrfCode.substr(0, iqrfCode.length() - 1);
      auto data = Base57::decode(code);

      StreamReader reader(data);
      uint32_t mid = 0;
      std::basic_string<uint8_t> ibk;
      uint16_t hwpid = 0;

      for (Tags tag; (tag = (Tags)reader.readNibble()) != Tags::END; ) {
        switch (tag) {
          case Tags::MID:
            mid = reader.readByte() << 24 | reader.readByte() << 16 | reader.readByte() << 8 | reader.readByte();
            break;
          case Tags::IBK:
            ibk = reader.readBytes(16);
            break;
          case Tags::HWPID:
            hwpid = reader.readByte() << 8 | reader.readByte();
            break;
          case Tags::LOGICAL_ADDRESS:
          case Tags::NOP:
          case Tags::DATA_BLOCK:
          case Tags::TEXT:
          case Tags::HWPID_VER:
            break;
          default:
            THROW_EXC(std::logic_error, "Unknown tag value.");
        }
      }

      return IqrfValues(mid, ibk, hwpid);
    }
  };
}
