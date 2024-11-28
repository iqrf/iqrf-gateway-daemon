#pragma once

#include "IqmeshServiceResultBase.h"
#include "RawDpaEmbedOS.h"

namespace iqrf {

  class SmartConnectResult : public IqmeshServiceResultBase {
  private:
    /// HWPID
    uint16_t m_hwpId;
    /// HWPID version
    uint16_t m_hwpIdVer;
    /// Assigned address
    uint8_t m_bondedAddr;
    /// Number of bonded nodes
    uint8_t m_bondedNodesNum;
    /// Manufacturer
    std::string m_manufacturer = "";
    /// Product
    std::string m_product = "";
    /// Standards
    std::list<std::string> m_standards = { "" };
    // Peripheral enumeration data
    TEnumPeripheralsAnswer m_enumPer;
    /// Parsed OS read data
    embed::os::RawDpaReadPtr m_osRead;
    /// OS build
    uint16_t m_osBuild;
    /// Generated smart connect code
    std::string m_code;
  public:
    /**
     * Returns HWPID
     * @return HWPID
     */
    uint16_t getHwpId() const {
      return m_hwpId;
    };

    /**
     * Sets HWPID
     * @param hwpId HWPID
     */
    void setHwpId(uint16_t hwpId) {
      m_hwpId = hwpId;
    }

    /**
     * Returns HWPID version
     * @return uint16_t
     */
    uint16_t getHwpIdVersion() const {
      return m_hwpIdVer;
    };

    /**
     * Sets HWPID version
     * @param hwpIdVer HWPID version
     */
    void setHwpIdVersion(uint16_t hwpIdVer) {
      m_hwpIdVer = hwpIdVer;
    }

    /**
     * Returns assigned address
     * @return Assigned address
     */
    uint8_t getBondedAddr() const {
      return m_bondedAddr;
    };

    /**
     * Sets assigned address
     * @param bondedAddr Assigned address
     */
    void setBondedAddr(uint8_t bondedAddr) {
      m_bondedAddr = bondedAddr;
    }

    /**
     * Returns number of bonded nodes
     * @return Number of bonded nodes
     */
    uint8_t getBondedNodesNum() const {
      return m_bondedNodesNum;
    };

    /**
     * Sets number of bonded nodes
     * @param bondedNodesNum Number of bonded nodes
     */
    void setBondedNodesNum(uint8_t bondedNodesNum) {
      m_bondedNodesNum = bondedNodesNum;
    }

    /**
     * Returns manufacturer
     * @return Manufacturer
     */
    std::string getManufacturer() const {
      return m_manufacturer;
    };

    /**
     * Sets manufacturer
     * @param manufacturer Manufacturer
     */
    void setManufacturer(const std::string& manufacturer) {
      m_manufacturer = manufacturer;
    }

    /**
     * Returns product
     *
     * @return std::string
     */
    std::string getProduct() const {
      return m_product;
    };

    /**
     * Sets product
     * @param product Product
     */
    void setProduct(const std::string& product) {
      m_product = product;
    }

    /**
     * Returns standards
     * @return Standards
     */
    std::list<std::string> getStandards() const {
      return m_standards;
    };

    /**
     * Sets standards
     * @param standards
     */
    void setStandards(std::list<std::string> standards) {
      m_standards = standards;
    }

    /**
     * Returns OS build
     * @return OS build
     */
    uint16_t getOsBuild() const {
      return m_osBuild;
    };

    /**
     * Sets OS build
     * @param osBuild OS build
     */
    void setOsBuild(uint16_t osBuild) {
      m_osBuild = osBuild;
    }

    /**
     * Returns peripheral enumeration response
     * @return Peripheral enumeration response
     */
    TEnumPeripheralsAnswer getEnumPer() const {
      return m_enumPer;
    };

    /**
     * Sets peripheral enumeration response
     * @param enumPer Peripheral enumeration response
     */
    void setEnumPer(TEnumPeripheralsAnswer enumPer) {
      m_enumPer = enumPer;
    }

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
    const std::string getSmartConnectCode() const {
      return m_code;
    }

    /**
     * Sets generated smart connect code
     * @param code Smart connect code
     */
    void setSmartConnectCode(const std::string &code) {
      m_code = code;
    }
  };
}
