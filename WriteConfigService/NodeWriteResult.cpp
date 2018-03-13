#include "NodeWriteResult.h"

void iqrf::NodeWriteResult::putFailedByte(const HWP_ConfigByte& failedByte) {
  failedBytesMap[failedByte.address] = failedByte;
}

void iqrf::NodeWriteResult::putFailedBytes(const std::vector<HWP_ConfigByte>& failedBytes) 
{
  for (const HWP_ConfigByte failedByte : failedBytes) {
    failedBytesMap[failedByte.address] = failedByte;
  }
}

void iqrf::NodeWriteResult::setError(const WriteError& error) {
  this->writeError = error;
}

