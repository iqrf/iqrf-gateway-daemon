#pragma once
#include "WriteError.h"
#include "NodeWriteResult.h"
#include <map>

namespace iqrf {

  /// \class WriteResult
  /// \brief Holds information about configuration writing result for each node
  class WriteResult {
  private:

    // map of write results on nodes indexed by node address
    std::map<uint16_t, NodeWriteResult> resultsMap;


  public:

    /// \brief Puts specified write result for specified node into results.
    /// \param [in] node			address of node to put the result for
    /// \param [in] result	  write result to put
    void putResult(uint16_t nodeAddr, const NodeWriteResult& result);

    /// \brief Returns map of write results on nodes indexed by node address
    /// \return map of write results on nodes indexed by node address
    std::map<uint16_t, NodeWriteResult> getResultsMap() const { return resultsMap; };
  };
}
