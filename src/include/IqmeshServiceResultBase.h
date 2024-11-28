#pragma once

#include "IDpaTransactionResult2.h"

#include <memory>
#include <list>
#include <string>

namespace iqrf {

  /**
   * IQMESH service result base class
   */
  class IqmeshServiceResultBase {
  protected:
    /// Status code
    int m_status = 0;
    /// Status string
    std::string m_statusStr = "ok";
    /// List of DPA transactions
    std::list<std::unique_ptr<IDpaTransactionResult2>> m_transResults;
  public:
    /**
     * Returns status code
     * @return Status code
     */
    int getStatus() const {
      return m_status;
    }

    /**
     * Sets status code
     * @param status Status code
     */
    void setStatus(const int status) {
      m_status = status;
    }

    /**
     * Returns status string
     * @return Status string
     */
    std::string getStatusStr() const {
      return m_statusStr;
    }

    /**
     * Sets status string
     * @param statusStr Status string
     */
    void setStatusStr(const std::string statusStr) {
      m_statusStr = statusStr;
    }

    /**
     * Sets status code and string
     * @param status Status code
     * @param statusStr Status string
     */
    void setStatus(const int status, const std::string statusStr) {
      m_status = status;
      m_statusStr = statusStr;
    }

    /**
     * Add transaction result
     * @param transResult Transaction result
     */
    void addTransactionResult(std::unique_ptr<IDpaTransactionResult2> transResult) {
      m_transResults.push_back(std::move(transResult));
    }

    /**
     * Add transaction result by reference
     * @param transResult Transaction result
     */
    void addTransactionResultRef(std::unique_ptr<IDpaTransactionResult2> &transResult) {
      m_transResults.push_back(std::move(transResult));
    }

    /**
     * Check if there are transaction results left to process
     * @return true if there are transaction results to process, false otherwise
     */
    bool isNextTransactionResult() {
      return (m_transResults.size() > 0);
    }

    /**
     * Pops and returns a transaction result for processing
     * @return Next transaction result
     */
    std::unique_ptr<IDpaTransactionResult2> consumeNextTransactionResult() {
      std::list<std::unique_ptr<IDpaTransactionResult2>>::iterator iter = m_transResults.begin();
      std::unique_ptr<IDpaTransactionResult2> tranResult = std::move(*iter);
      m_transResults.pop_front();
      return tranResult;
    }
  };

}
