#pragma once

#include "ShapeDefines.h"
#include "stdint.h"
#include <string>
#include <list>
#include <vector>

#ifdef IEnumerateDeviceService_EXPORTS
#define IEnumerateDeviceService_DECLSPEC SHAPE_ABI_EXPORT
#else
#define IEnumerateDeviceService_DECLSPEC SHAPE_ABI_IMPORT
#endif

namespace iqrf {
  /// \class IEnumerateDeviceService
  class IEnumerateDeviceService_DECLSPEC IEnumerateDeviceService
  {
  public:
    struct NodeEnumeration
    {
      uint8_t m_deviceAddr = 0;
      bool m_discovered = false;
      uint8_t m_vrn = 0;
      uint8_t m_zone = 0;
      uint8_t m_parent = 0;
      uint16_t m_enumeratedNodeHwpIdVer = 0;
      std::string m_osBuild;
      uint16_t m_nodeHwpId = 0;
      std::string m_manufacturer;
      std::string m_product;
      std::list<std::string> m_standards;
      std::string m_trType;
      bool m_fccCertified = false;
      std::string m_mcuType;
      bool m_insufficientOsBuild = false;
      std::string m_interface = false;
      bool m_dpaHandlerDetected = false;
      bool m_dpaHandlerNotDetectedButEnabled = false;
      bool m_noInterfaceSupported = false;
      std::string m_shortestTimeslot;
      std::string m_longestTimeslot;
      std::string m_mid;
      std::string m_osVersion;
      uint8_t m_trMcuTypeVal = 0;
      std::string m_rssi;
      std::string m_supplyVoltage;
      uint8_t m_flagsVal = 0;
      uint8_t m_slotLimitsVal = 0;
      uint16_t m_DpaVersion = 0;
      uint8_t m_UserPerNr = 0;
      std::vector<uint8_t> m_EmbeddedPers;
      std::vector<uint8_t> m_UserPer;
      uint16_t m_HWPID = 0;
      uint16_t m_HWPIDver = 0;
      bool m_rfModeStd = false;
      bool m_rfModeLp = false;
      bool m_stdAndLpNetwork = false;
    };

    NodeEnumeration virtual getEnumerateResult(uint16_t deviceAddr) = 0;
    virtual ~IEnumerateDeviceService() {}
  };
}
