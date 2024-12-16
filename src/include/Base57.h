#pragma once

#include <stdexcept>
#include <string>
#include <sstream>

namespace iqrf::base57 {
  /// Base number
  static inline const int BASE = 57;
  /// Alphabet
  static inline const std::string ALPHABET = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstvwxyz";
  /// Number of base57 characters required to encode a piece (8 bytes)
  static inline const int MAXIMUM_PIECE_LENGTH = 11;
  /// Mapping of base57 characters required to encode respective number of bytes (bytes are represented by index in array)
  static inline const int PIECE_LENGTHS[] = {0, 2, 3, 5, 6, 7, 9, 10, MAXIMUM_PIECE_LENGTH};

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

  /**
   * Encodes bytes of data into base57 string
   * @param data Data to encode
   * @return Encoded base57 data string
   */
  static std::string encode(const std::basic_string<uint8_t> &data) {
    if (data.size() == 0) {
      throw std::logic_error("No data to encode");
    }

    std::ostringstream oss;
    for (std::size_t pieceIndex = 0; pieceIndex < data.size(); pieceIndex += sizeof(uint64_t)) {
      int pieceLength = std::min(sizeof(uint64_t), data.size() - pieceIndex);

      uint64_t piece = 0;
      for (int inPieceIndex = 0; inPieceIndex < pieceLength; inPieceIndex++) {
        piece = (piece << 8) | data[pieceIndex + inPieceIndex];
      }

      for (int piece57Length = PIECE_LENGTHS[pieceLength]; piece57Length != 0; piece57Length--) {
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
      throw std::logic_error("IQRF Code has incorrect length: " + std::to_string(input.length()));
    }

    std::basic_string<uint8_t> data;
    data.resize(sizeof(uint64_t) * (input.length() / MAXIMUM_PIECE_LENGTH) + lastPieceLenIndex);

    for (unsigned int piece57Index = 0, pieceIndex = 0; piece57Index < input.length(); piece57Index += MAXIMUM_PIECE_LENGTH, pieceIndex += sizeof(uint64_t)) {
      int piece57Length = std::min((size_t)MAXIMUM_PIECE_LENGTH, input.length() - piece57Index);

      uint64_t piece = 0;
      for (int inPiece57Index = piece57Length; --inPiece57Index >= 0;) {
        char char57 = input[piece57Index + inPiece57Index];
        size_t alphabet57Index = ALPHABET.find(char57);
        if (alphabet57Index == std::string::npos) {
          throw std::logic_error("IQRF Code contains an incorrect character " + std::to_string(char57) + " at position: " + std::to_string(piece57Index + inPiece57Index));
        }
        piece = piece * BASE + (uint64_t)alphabet57Index;
      }

      for (int pieceLenIndex = getIndexOfPieceLen(piece57Length); --pieceLenIndex >= 0;) {
        data[pieceIndex + pieceLenIndex] = (uint8_t)(piece & 0xFF);
        piece >>= 8;
      }
    }

    return data;
  }
}
