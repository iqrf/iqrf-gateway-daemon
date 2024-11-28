#pragma once

#include "IqmeshServiceResultBase.h"
#include "RawDpaEmbedOS.h"

#include <memory>
#include <list>
#include <string>

namespace iqrf {

  /**
   * Smart connect code generate result class
   */
  class SmartConnectCodeGenerateResult : public IqmeshServiceResultBase {
  private:
    /// Parsed OS read response
    iqrf::embed::os::RawDpaReadPtr m_osRead;
    /// Generated smart connect code
    std::basic_string<uint8_t> m_code;
  public:
    /**
     * Returns OS read response
     * @return Parsed OS read response
     */
    const embed::os::RawDpaReadPtr& getOsRead() const {
      return m_osRead;
    };

    /**
     * Sets OS read response
     * @param osReadPtr Parsed OS read response
     */
    void setOsRead(embed::os::RawDpaReadPtr &osReadPtr) {
      m_osRead = std::move(osReadPtr);
    }

    /**
     * Returns generated smart connect code
     * @return Smart connect code
     */
    const std::basic_string<uint8_t> getSmartConnectCode() const {
      return m_code;
    }

    /**
     * Sets generated smart connect code
     * @param code Smart connect code
     */
    void setSmartConnectCode(const std::basic_string<uint8_t> &code) {
      m_code = code;
    }
  };

}
