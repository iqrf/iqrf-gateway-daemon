#pragma once
#include <map>
#include <vector>
#include "WriteError.h"
#include "HwpConfigByte.h"

namespace iqrf {

  /// \class NodeWriteResult
  /// \brief Holds information about configuration writing result on one node
  class NodeWriteResult {
  private:

    // write error
    WriteError writeError;

    // map of configuration bytes, which failed to write
    // indexed by byte addresses
    std::map<uint8_t, HWP_ConfigByte> failedBytesMap;


  public:
    /// \brief Default constructor.
    /// \details
    /// Write error is set to "No error" and the are no failed bytes.
    NodeWriteResult() {};

    /// \brief Returns error.
    /// \return write error
    WriteError getError() const { return writeError; };

    /// \bried Sets specified write error.
    /// \param[in] error    error to set
    void setError(const WriteError& error);

    /// \brief Puts specified configuration byte, which failed to write.
    /// \param [in] failedByte      failed byte
    void putFailedByte(const HWP_ConfigByte& failedByte);

    /// \brief Puts specified configuration bytes, which failed to write.
    /// \param [in] failedBytes      failed bytes
    void putFailedBytes(const std::vector<HWP_ConfigByte>& failedBytes);

    /// \brief Returns map of failed bytes indexed by their addresses
    /// \return map of failed bytes indexed by their addresses
    std::map<uint8_t, HWP_ConfigByte> getFailedBytesMap() const { return failedBytesMap; };
  };
}
