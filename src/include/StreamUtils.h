#pragma once

#include <stdexcept>
#include <string>

/// stream utilities namespace
namespace iqrf::utils::stream {

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
        throw std::logic_error("No more data available to read.");
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

}
