#include "WriteResult.h"

void iqrf::WriteResult::putResult(uint16_t nodeAddr, const NodeWriteResult& result) {
  resultsMap[nodeAddr] = result;
}