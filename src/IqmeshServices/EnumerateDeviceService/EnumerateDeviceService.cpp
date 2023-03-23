/**
 * Copyright 2015-2023 IQRF Tech s.r.o.
 * Copyright 2019-2023 MICRORISC s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define IEnumerateDeviceService_EXPORTS

#include "EnumerateDeviceService.h"
#include "RawDpaEmbedOS.h"
#include "RawDpaEmbedExplore.h"
#include "Trace.h"
#include "ComIqmeshNetworkEnumerateDevice.h"
#include "ObjectFactory.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "HexStringCoversion.h"
#include "iqrf__EnumerateDeviceService.hxx"
#include <list>
#include <memory>
#include <math.h>
#include <bitset>

TRC_INIT_MODULE(iqrf::EnumerateDeviceService)

using namespace rapidjson;

namespace {

  // maximum number of repeats
  static const uint8_t REPEAT_MAX = 3;

  // size of discovered devices bitmap
  static const uint8_t DISCOVERED_DEVICES_BITMAP_SIZE = 30;

  // length of MID [in bytes]
  static const uint8_t MID_LEN = 4;

  // length of EmbeddedPers field [in bytes]
  static const uint8_t EMBEDDED_PERS_LEN = 4;

  // length of UserPer field [in bytes]
  static const uint8_t USER_PER_LEN = 12;

  // length of configuration field [in bytes]
  static const uint8_t CONFIGURATION_LEN = 31;

  // number of peripherals in DPA response: get info for more peripherals
  static const uint8_t PERIPHERALS_NUM = 14;

  // baud rates
  static uint8_t BAUD_RATES_SIZE = 9;

  static const int serviceError = 1000;
  static const int parsingRequestError = 1001;
  static const int exclusiveAccessError = 1002;

  static uint32_t BaudRates[] = {
    1200,
    2400,
    4800,
    9600,
    19200,
    38400,
    57600,
    115200,
    230400
  };
}

namespace iqrf {
  // Holds information about result of device enumeration
  class DeviceEnumerateResult {
  private:
    // Status
    int m_status = 0;
    std::string m_statusStr = "ok";

    bool m_discovered;
    uint8_t m_vrn;
    uint8_t m_zone;
    uint8_t m_parent;
    embed::os::RawDpaReadPtr m_osRead;
    uint16_t m_osBuild;
    embed::explore::RawDpaEnumeratePtr m_perEnum;
    TPerOSReadCfg_Response m_hwpConfig;
    embed::explore::RawDpaMorePeripheralInformationPtr m_morePersInfo;
    uint16_t m_hwpId;
    uint16_t m_hwpIdVer;
    std::string m_manufacturer = "";
    std::string m_product = "";
    std::list<std::string> m_standards = { "" };
    // Transaction results
    std::list<std::unique_ptr<IDpaTransactionResult2>> m_transResults;

  public:
    // Status
    int getStatus() const { return m_status; };
    std::string getStatusStr() const { return m_statusStr; };
    void setStatus(const int status) {
      m_status = status;
    }
    void setStatus(const int status, const std::string statusStr) {
      m_status = status;
      m_statusStr = statusStr;
    }
    void setStatusStr(const std::string statusStr) {
      m_statusStr = statusStr;
    }

    void setHwpId(const uint16_t hwpId) {
      m_hwpId = hwpId;
    }
    uint16_t getHwpId() const {
      return m_hwpId;
    }

    void setHwpIdVer(const uint16_t hwpIdVer) {
      m_hwpIdVer = hwpIdVer;
    }
    uint16_t getHwpIdVer() const {
      return m_hwpIdVer;
    }

    std::string getManufacturer() {
      return m_manufacturer;
    }
    void setManufacturer(const std::string& manufacturer) {
      m_manufacturer = manufacturer;
    }

    std::string getProduct() {
      return m_product;
    }
    void setProduct(const std::string& product) {
      m_product = product;
    }

    bool isDiscovered() const {
      return m_discovered;
    }
    void setDiscovered(bool discovered) {
      m_discovered = discovered;
    }

    uint8_t getVrn() const {
      return m_vrn;
    }
    void setVrn(uint8_t vrn) {
      m_vrn = vrn;
    }

    uint8_t getZone() const {
      return m_zone;
    }
    void setZone(uint8_t zone) {
      m_zone = zone;
    }

    uint8_t getParent() const {
      return m_parent;
    }
    void setParent(uint8_t parent) {
      m_parent = parent;
    }

    const embed::os::RawDpaReadPtr& getOsRead() const
    {
      return m_osRead;
    }
    void setOsRead(embed::os::RawDpaReadPtr &osReadPtr) {
      m_osRead = std::move(osReadPtr);
    }

    uint16_t getOsBuild() const {
      return m_osBuild;
    }
    void setOsBuild(uint16_t osBuild) {
      m_osBuild = osBuild;
    }

    const embed::explore::RawDpaEnumeratePtr& getPerEnum() const
    {
      return m_perEnum;
    }

    void setPerEnum(embed::explore::RawDpaEnumeratePtr &perEnumPtr) {
      m_perEnum = std::move(perEnumPtr);
    }

    TPerOSReadCfg_Response getHwpConfig() const {
      return m_hwpConfig;
    }

    void setHwpConfig(TPerOSReadCfg_Response hwpConfig) {
      m_hwpConfig = hwpConfig;
    }

    const embed::explore::RawDpaMorePeripheralInformationPtr& getMorePersInfo() const {
      return m_morePersInfo;
    }

    void setMorePersInfo(embed::explore::RawDpaMorePeripheralInformationPtr &morePersInfoPtr) {
      m_morePersInfo = std::move( morePersInfoPtr );
    }

    std::list<std::string> getStandards() {
      return m_standards;
    }

    void setStandards(const std::list<std::string> &standards) {
      m_standards = standards;
    }

    // Adds transaction result into the list of results
    void addTransactionResult(std::unique_ptr<IDpaTransactionResult2> transResult)
    {
      m_transResults.push_back(std::move(transResult));
    }

    // Adds transaction result into the list of results
    void addTransactionResultRef(std::unique_ptr<IDpaTransactionResult2> &transResult)
    {
      m_transResults.push_back(std::move(transResult));
    }

    bool isNextTransactionResult()
    {
      return (m_transResults.size() > 0);
    }

    // Consumes the first element in the transaction results list
    std::unique_ptr<IDpaTransactionResult2> consumeNextTransactionResult()
    {
      std::list<std::unique_ptr<IDpaTransactionResult2>>::iterator iter = m_transResults.begin();
      std::unique_ptr<IDpaTransactionResult2> tranResult = std::move(*iter);
      m_transResults.pop_front();
      return tranResult;
    }
  };

  // implementation class
  class EnumerateDeviceService::Imp {
  private:
    // parent object
    EnumerateDeviceService& m_parent;

    // message type: IQMESH Network Enumerate Device
    // for temporal reasons
    const std::string m_mTypeName_iqmeshNetworkEnumerateDevice = "iqmeshNetwork_EnumerateDevice";

    iqrf::IJsCacheService* m_iJsCacheService = nullptr;
    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    IIqrfDpaService *m_iIqrfDpaService = nullptr;
    std::unique_ptr<IIqrfDpaService::ExclusiveAccess> m_exclusiveAccess;
    const std::string* m_messagingId = nullptr;
    const IMessagingSplitterService::MsgType* m_msgType = nullptr;
    const ComIqmeshNetworkEnumerateDevice* m_comEnumerateDevice = nullptr;

    // Service input parameters
    TEnumerateDeviceInputParams m_enumerateDeviceParams;

  public:
    explicit Imp(EnumerateDeviceService& parent) : m_parent(parent)
    {
    }

    ~Imp()
    {
    }

  private:

    // Returns 1 byte of discovery data read from the coordinator private external EEPROM storage
    uint8_t readDiscoveryByte(DeviceEnumerateResult& deviceEnumerateResult, const uint16_t address)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        DpaMessage eeepromReadRequest;
        DpaMessage::DpaPacket_t eeepromReadPacket;
        eeepromReadPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        eeepromReadPacket.DpaRequestPacket_t.PNUM = PNUM_EEEPROM;
        eeepromReadPacket.DpaRequestPacket_t.PCMD = CMD_EEEPROM_XREAD;
        eeepromReadPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        // Read the byte from specified address
        eeepromReadPacket.DpaRequestPacket_t.DpaMessage.XMemoryRequest.Address = address;
        eeepromReadPacket.DpaRequestPacket_t.DpaMessage.XMemoryRequest.ReadWrite.Read.Length = sizeof(uint8_t);
        eeepromReadRequest.DataToBuffer(eeepromReadPacket.Buffer, sizeof(TDpaIFaceHeader) + sizeof(uint16_t) + sizeof(uint8_t));
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(eeepromReadRequest, transResult, m_enumerateDeviceParams.repeat);
        TRC_DEBUG("Result from CMD_EEEPROM_XREAD transaction as string:" << PAR(transResult->getErrorString()));
        // because of the move-semantics
        DpaMessage dpaResponse = transResult->getResponse();
        deviceEnumerateResult.addTransactionResultRef(transResult);
        TRC_INFORMATION("CMD_EEEPROM_XREAD successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(eeepromReadRequest.PeripheralType(), eeepromReadRequest.NodeAddress())
          << PAR(eeepromReadRequest.PeripheralCommand())
        );
        TRC_FUNCTION_LEAVE("");
        return dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData[0];
      }
      catch (const std::exception& e)
      {
        deviceEnumerateResult.setStatus(transResult->getErrorCode(), e.what());
        deviceEnumerateResult.addTransactionResultRef(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    // Check the node is discovered
    uint8_t nodeDiscovered(DeviceEnumerateResult& deviceEnumerateResult, const uint16_t deviceAddr)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        DpaMessage getDiscoveredRequest;
        DpaMessage::DpaPacket_t getDiscoveredPacket;
        getDiscoveredPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        getDiscoveredPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
        getDiscoveredPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_DISCOVERED_DEVICES;
        getDiscoveredPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        getDiscoveredRequest.DataToBuffer(getDiscoveredPacket.Buffer, sizeof(TDpaIFaceHeader));
        // Execute DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(getDiscoveredRequest, transResult, m_enumerateDeviceParams.repeat);
        TRC_DEBUG("Result from CMD_COORDINATOR_DISCOVERED_DEVICES transaction as string:" << PAR(transResult->getErrorString()));
        // Parse DPA response
        DpaMessage response = transResult->getResponse();
        deviceEnumerateResult.addTransactionResultRef(transResult);
        TRC_INFORMATION("CMD_COORDINATOR_DISCOVERED_DEVICES successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(getDiscoveredRequest.PeripheralType(), getDiscoveredRequest.NodeAddress())
          << PAR(getDiscoveredRequest.PeripheralCommand())
        );
        TRC_FUNCTION_LEAVE("");
        return response.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData[deviceAddr];
      }
      catch (const std::exception& e)
      {
        deviceEnumerateResult.setStatus(transResult->getErrorCode(), e.what());
        deviceEnumerateResult.addTransactionResultRef(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    // Read discovery data
    void discoveryData(DeviceEnumerateResult& deviceEnumerateResult, const uint16_t deviceAddr)
    {
      // Get discovered indicator
      uint8_t byteIndex = (uint8_t)(deviceAddr / 8);
      uint8_t nodeDiscoveryByte = nodeDiscovered(deviceEnumerateResult, byteIndex);
      uint8_t bitIndex = deviceAddr % 8;
      uint8_t compareByte = uint8_t(pow(2, bitIndex));
      deviceEnumerateResult.setDiscovered((nodeDiscoveryByte & compareByte) == compareByte);
      // VRN
      uint16_t address = 0x5000 + deviceAddr;
      uint8_t vrnByte = readDiscoveryByte(deviceEnumerateResult, address);
      deviceEnumerateResult.setVrn(vrnByte);
      // Zone
      address = 0x5200 + deviceAddr;
      uint8_t zoneByte = readDiscoveryByte(deviceEnumerateResult, address);
      deviceEnumerateResult.setZone(zoneByte);
      // Parent
      address = 0x5300 + deviceAddr;
      uint8_t parentByte = readDiscoveryByte(deviceEnumerateResult, address);
      deviceEnumerateResult.setParent(parentByte);
    }

    //--------------------------------------
    // Reads OS info about enumerated device
    //--------------------------------------
    void osRead(DeviceEnumerateResult& deviceEnumerateResult, const uint16_t deviceAddr)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<embed::os::RawDpaRead> osReadPtr(shape_new embed::os::RawDpaRead(deviceAddr));
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        m_exclusiveAccess->executeDpaTransactionRepeat(osReadPtr->getRequest(), transResult, m_enumerateDeviceParams.repeat);
        osReadPtr->processDpaTransactionResult(std::move(transResult));
        TRC_DEBUG("Result from OS read transaction as string:" << PAR(osReadPtr->getResult()->getErrorString()));
        deviceEnumerateResult.setOsBuild((uint16_t)osReadPtr->getOsBuild());
        deviceEnumerateResult.setHwpId(osReadPtr->getHwpid());
        deviceEnumerateResult.addTransactionResult(osReadPtr->getResultMove());
        deviceEnumerateResult.setOsRead(osReadPtr);
        TRC_INFORMATION("OS read successful!");
        TRC_FUNCTION_LEAVE("");
      }
      catch (const std::exception &e)
      {
        deviceEnumerateResult.setStatus(transResult->getErrorCode(), e.what());
        deviceEnumerateResult.addTransactionResultRef(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //-----------------------
    // Peripheral enumeration
    //-----------------------
    void peripheralEnumeration(DeviceEnumerateResult& deviceEnumerateResult, const uint16_t deviceAddr)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<embed::explore::RawDpaEnumerate> exploreEnumeratePtr(shape_new embed::explore::RawDpaEnumerate(deviceAddr));
      std::unique_ptr<embed::explore::RawDpaMorePeripheralInformation> exploreMorePeripheralInformationPtr(shape_new embed::explore::RawDpaMorePeripheralInformation(deviceAddr, 0));
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(exploreEnumeratePtr->getRequest(), transResult, m_enumerateDeviceParams.repeat);
        exploreEnumeratePtr->processDpaTransactionResult(std::move(transResult));
        TRC_DEBUG("Result from peripheral enumeration transaction as string:" << PAR(exploreEnumeratePtr->getResult()->getErrorString()));
        deviceEnumerateResult.setHwpIdVer((uint16_t)exploreEnumeratePtr->getHwpidVer());
        deviceEnumerateResult.addTransactionResult(exploreEnumeratePtr->getResultMove());
        deviceEnumerateResult.setPerEnum(exploreEnumeratePtr);
        TRC_INFORMATION("Peripheral enumeration successful!");

        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(exploreMorePeripheralInformationPtr->getRequest(), transResult, m_enumerateDeviceParams.repeat);
        exploreMorePeripheralInformationPtr->processDpaTransactionResult(std::move(transResult));
        TRC_DEBUG("Result from more peripheral information transaction as string:" << PAR(exploreMorePeripheralInformationPtr->getResult()->getErrorString()));
        deviceEnumerateResult.addTransactionResult(exploreMorePeripheralInformationPtr->getResultMove());
        deviceEnumerateResult.setMorePersInfo(exploreMorePeripheralInformationPtr);
        TRC_INFORMATION("More peripheral information successful!");
      }
      catch (const std::exception& e)
      {
        deviceEnumerateResult.setStatus(transResult->getErrorCode(), e.what());
        deviceEnumerateResult.addTransactionResultRef(transResult);
        THROW_EXC(std::logic_error, e.what());
      }

      TRC_FUNCTION_LEAVE("");
    }

    // Read HWP configuration
    void readHwpConfiguration(DeviceEnumerateResult& deviceEnumerateResult, const uint16_t deviceAddr)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage readHwpRequest;
        DpaMessage::DpaPacket_t readHwpPacket;
        readHwpPacket.DpaRequestPacket_t.NADR = deviceAddr;
        readHwpPacket.DpaRequestPacket_t.PNUM = PNUM_OS;
        readHwpPacket.DpaRequestPacket_t.PCMD = CMD_OS_READ_CFG;
        readHwpPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        readHwpRequest.DataToBuffer(readHwpPacket.Buffer, sizeof(TDpaIFaceHeader));
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(readHwpRequest, transResult, m_enumerateDeviceParams.repeat);
        TRC_DEBUG("Result from CMD_OS_READ_CFG transaction as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        deviceEnumerateResult.addTransactionResultRef(transResult);
        TRC_INFORMATION("CMD_OS_READ_CFG successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(readHwpRequest.PeripheralType(), readHwpRequest.NodeAddress())
          << PAR((unsigned)readHwpRequest.PeripheralCommand())
        );
        // Parse response pdata
        TPerOSReadCfg_Response readHwpConfig = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerOSReadCfg_Response;
        deviceEnumerateResult.setHwpConfig(readHwpConfig);
      }
      catch (const std::exception& e)
      {
        deviceEnumerateResult.setStatus(transResult->getErrorCode(), e.what());
        deviceEnumerateResult.addTransactionResultRef(transResult);
        THROW_EXC(std::logic_error, e.what());
      }

      TRC_FUNCTION_LEAVE("");
    }

    //-----------------
    // Enumerate device
    //-----------------
    void enumerateDevice(DeviceEnumerateResult& deviceEnumerateResult)
    {
      TRC_FUNCTION_ENTER("");
      try
      {
        // Discovery data
        discoveryData(deviceEnumerateResult, m_enumerateDeviceParams.deviceAddress);

        // OS read info
        osRead(deviceEnumerateResult, m_enumerateDeviceParams.deviceAddress);

        // Obtains HwpId, which in turn is needed to get manufacturer and product
        IJsCacheService::Manufacturer manufacturer = m_iJsCacheService->getManufacturer(deviceEnumerateResult.getHwpId());
        if (manufacturer.m_manufacturerId > -1)
          deviceEnumerateResult.setManufacturer(manufacturer.m_name);
        IJsCacheService::Product product = m_iJsCacheService->getProduct(deviceEnumerateResult.getHwpId());
        if (product.m_manufacturerId > -1)
          deviceEnumerateResult.setProduct(product.m_name);

        // Peripheral enumeration
        peripheralEnumeration(deviceEnumerateResult, m_enumerateDeviceParams.deviceAddress);

        IJsCacheService::Package package;
        package = m_iJsCacheService->getPackage(deviceEnumerateResult.getHwpId(), deviceEnumerateResult.getHwpIdVer(), deviceEnumerateResult.getOsRead()->getOsBuildAsString(), deviceEnumerateResult.getPerEnum()->getDpaVerAsHexaString());
        if (package.m_packageId > -1)
        {
          std::list<std::string> standards;
          for (const IJsCacheService::StdDriver& driver : package.m_stdDriverVect)
            standards.push_back(driver.getName());
          deviceEnumerateResult.setStandards(standards);
        }
        else
          TRC_INFORMATION("Package not found");

        // Read Hwp configuration
        readHwpConfiguration(deviceEnumerateResult, m_enumerateDeviceParams.deviceAddress);
      }
      catch (std::exception& e)
      {
        CATCH_EXC_TRC_WAR(std::exception, e, e.what());
      }
      TRC_FUNCTION_LEAVE("");
    }

    //--------------------------
    // Creates and send response
    //--------------------------
    void createResponse(DeviceEnumerateResult& deviceEnumerateResult)
    {
      Document response;

      // set common parameters
      Pointer("/mType").Set(response, m_msgType->m_type);
      Pointer("/data/msgId").Set(response, m_comEnumerateDevice->getMsgId());

      // Set status
      int status = deviceEnumerateResult.getStatus();
      if (status == 0)
      {
        Pointer("/data/rsp/deviceAddr").Set(response, m_enumerateDeviceParams.deviceAddress);
        Pointer("/data/rsp/manufacturer").Set(response, deviceEnumerateResult.getManufacturer());
        Pointer("/data/rsp/product").Set(response, deviceEnumerateResult.getProduct());

        // Standards - array of strings
        rapidjson::Value standardsJsonArray(kArrayType);
        Document::AllocatorType &allocator = response.GetAllocator();
        for (std::string standard : deviceEnumerateResult.getStandards())
        {
          rapidjson::Value standardJsonString;
          standardJsonString.SetString(standard.c_str(), (SizeType)standard.length(), allocator);
          standardsJsonArray.PushBack(standardJsonString, allocator);
        }
        Pointer("/data/rsp/standards").Set(response, standardsJsonArray);

        // Discovery bytes read
        if (m_enumerateDeviceParams.deviceAddress != COORDINATOR_ADDRESS)
        {
          Pointer("/data/rsp/discovery/discovered").Set(response, deviceEnumerateResult.isDiscovered());
          Pointer("/data/rsp/discovery/vrn").Set(response, deviceEnumerateResult.getVrn());
          Pointer("/data/rsp/discovery/zone").Set(response, deviceEnumerateResult.getZone());
          Pointer("/data/rsp/discovery/parent").Set(response, deviceEnumerateResult.getParent());
        }

        // OS Read data
        Pointer("/data/rsp/osRead/mid").Set(response, deviceEnumerateResult.getOsRead()->getMidAsString());
        Pointer("/data/rsp/osRead/osVersion").Set(response, deviceEnumerateResult.getOsRead()->getOsVersionAsString());
        Pointer("/data/rsp/osRead/trMcuType/value").Set(response, deviceEnumerateResult.getOsRead()->getTrMcuType());
        Pointer("/data/rsp/osRead/trMcuType/trType").Set(response, deviceEnumerateResult.getOsRead()->getTrTypeAsString());
        Pointer("/data/rsp/osRead/trMcuType/fccCertified").Set(response, deviceEnumerateResult.getOsRead()->isFccCertified());
        Pointer("/data/rsp/osRead/trMcuType/mcuType").Set(response, deviceEnumerateResult.getOsRead()->getTrMcuTypeAsString());
        Pointer("/data/rsp/osRead/osBuild").Set(response, deviceEnumerateResult.getOsRead()->getOsBuildAsString());
        // RSSI [dBm]
        Pointer("/data/rsp/osRead/rssi").Set(response, deviceEnumerateResult.getOsRead()->getRssiAsString());
        // Supply voltage [V]
        Pointer("/data/rsp/osRead/supplyVoltage").Set(response, deviceEnumerateResult.getOsRead()->getSupplyVoltageAsString());
        // Flags
        Pointer("/data/rsp/osRead/flags/value").Set(response, deviceEnumerateResult.getOsRead()->getFlags());
        if (deviceEnumerateResult.getOsRead()->getDpaVer() >= 0x0417) {
          rapidjson::Pointer("/data/rsp/osRead/flags/insufficientOsVersion").Set(response, deviceEnumerateResult.getOsRead()->isInsufficientOs());
        } else {
          rapidjson::Pointer("/data/rsp/osRead/flags/insufficientOsBuild").Set(response, deviceEnumerateResult.getOsRead()->isInsufficientOs());
        }
        Pointer("/data/rsp/osRead/flags/interfaceType").Set(response, deviceEnumerateResult.getOsRead()->getInterfaceAsString());
        Pointer("/data/rsp/osRead/flags/dpaHandlerDetected").Set(response, deviceEnumerateResult.getOsRead()->isDpaHandlerDetected());
        Pointer("/data/rsp/osRead/flags/dpaHandlerNotDetectedButEnabled").Set(response, deviceEnumerateResult.getOsRead()->isDpaHandlerNotDetectedButEnabled());
        Pointer("/data/rsp/osRead/flags/noInterfaceSupported").Set(response, deviceEnumerateResult.getOsRead()->isNoInterfaceSupported());
        if(deviceEnumerateResult.getOsRead()->getDpaVer() >= 0x0413)
          rapidjson::Pointer("/data/rsp/osRead/flags/iqrfOsChanged").Set(response, deviceEnumerateResult.getOsRead()->isIqrfOsChanges());
        if((m_enumerateDeviceParams.deviceAddress != COORDINATOR_ADDRESS) && (deviceEnumerateResult.getOsRead()->getDpaVer() >= 0x0416))
          rapidjson::Pointer("/data/rsp/osRead/flags/frcAggregationEnabled").Set(response, deviceEnumerateResult.getOsRead()->isFrcAggregationEnabled());

        // Slot limits
        rapidjson::Pointer("/data/rsp/osRead/slotLimits/value").Set(response, deviceEnumerateResult.getOsRead()->getSlotLimits());
        rapidjson::Pointer("/data/rsp/osRead/slotLimits/shortestTimeslot").Set(response, deviceEnumerateResult.getOsRead()->getShortestTimeSlotAsString());
        rapidjson::Pointer("/data/rsp/osRead/slotLimits/longestTimeslot").Set(response, deviceEnumerateResult.getOsRead()->getLongestTimeSlotAsString());

        // Peripheral enumeration
        Pointer("/data/rsp/peripheralEnumeration/dpaVer").Set(response, deviceEnumerateResult.getPerEnum()->getDpaVerAsString());
        Pointer("/data/rsp/peripheralEnumeration/perNr").Set(response, deviceEnumerateResult.getPerEnum()->getPerNr());
        // embPers
        rapidjson::Value embPersJsonArray(kArrayType);
        for (std::set<int>::iterator it = deviceEnumerateResult.getPerEnum()->getEmbedPer().begin(); it != deviceEnumerateResult.getPerEnum()->getEmbedPer().end(); ++it)
          embPersJsonArray.PushBack(*it, allocator);
        Pointer("/data/rsp/peripheralEnumeration/embPers").Set(response, embPersJsonArray);
        // hwpId
        Pointer("/data/rsp/peripheralEnumeration/hwpId").Set(response, deviceEnumerateResult.getPerEnum()->getHwpid());
        // hwpIdVer
        Pointer("/data/rsp/peripheralEnumeration/hwpIdVer").Set(response, deviceEnumerateResult.getPerEnum()->getHwpidVer());
        // flags - int value
        Pointer("/data/rsp/peripheralEnumeration/flags/value").Set(response, deviceEnumerateResult.getPerEnum()->getFlags());
        // flags - parsed
        bool stdModeSupported = (deviceEnumerateResult.getPerEnum()->getFlags() & 0b1);
        if (stdModeSupported)
        {
          Pointer("/data/rsp/peripheralEnumeration/flags/rfModeStd").Set(response, true);
          Pointer("/data/rsp/peripheralEnumeration/flags/rfModeLp").Set(response, false);
        }
        else
        {
          Pointer("/data/rsp/peripheralEnumeration/flags/rfModeStd").Set(response, false);
          Pointer("/data/rsp/peripheralEnumeration/flags/rfModeLp").Set(response, true);
        }

        // STD+LP network is running, otherwise STD network.
        if (deviceEnumerateResult.getPerEnum()->getDpaVer() >= 0x0400)
        {
          bool stdAndLpModeNetwork = (deviceEnumerateResult.getPerEnum()->getFlags() & 0b100);
          if (stdAndLpModeNetwork)
            Pointer("/data/rsp/peripheralEnumeration/flags/stdAndLpNetwork").Set(response, true);
          else
            Pointer("/data/rsp/peripheralEnumeration/flags/stdAndLpNetwork").Set(response, false);
        }

        // UserPers
        rapidjson::Value userPerJsonArray(kArrayType);
        for (std::set<int>::iterator it = deviceEnumerateResult.getPerEnum()->getUserPer().begin(); it != deviceEnumerateResult.getPerEnum()->getUserPer().end(); ++it)
          userPerJsonArray.PushBack(*it, allocator);
        Pointer("/data/rsp/peripheralEnumeration/userPers").Set(response, userPerJsonArray);

        // HWP configuration
        TPerOSReadCfg_Response hwpConfig = deviceEnumerateResult.getHwpConfig();
        uns8* configuration = hwpConfig.Configuration;
        int dpaVer = deviceEnumerateResult.getPerEnum()->getDpaVer();
        if (dpaVer < 0x0303)
        {
          for (int i = 0; i < CONFIGURATION_LEN; i++)
            configuration[i] = configuration[i] ^ 0x34;
        }
        // Embedded periherals
        rapidjson::Value embPerBitsJsonArray(kArrayType);
        for (int i = 0; i < 4; i++)
          embPerBitsJsonArray.PushBack(configuration[i], allocator);
        Pointer("/data/rsp/trConfiguration/embPers/values").Set(response, embPerBitsJsonArray);
        // Embedded peripherals bits - parsed
        // Byte 0x01
        uint8_t byte01 = configuration[0x00];
        bool coordPresent = ((byte01 & 0b1) == 0b1);
        Pointer("/data/rsp/trConfiguration/embPers/coordinator").Set(response, coordPresent);
        bool nodePresent = ((byte01 & 0b10) == 0b10);
        Pointer("/data/rsp/trConfiguration/embPers/node").Set(response, nodePresent);
        bool osPresent = ((byte01 & 0b100) == 0b100);
        Pointer("/data/rsp/trConfiguration/embPers/os").Set(response, osPresent);
        bool eepromPresent = ((byte01 & 0b1000) == 0b1000);
        Pointer("/data/rsp/trConfiguration/embPers/eeprom").Set(response, eepromPresent);
        bool eeepromPresent = ((byte01 & 0b10000) == 0b10000);
        Pointer("/data/rsp/trConfiguration/embPers/eeeprom").Set(response, eeepromPresent);
        bool ramPresent = ((byte01 & 0b100000) == 0b100000);
        Pointer("/data/rsp/trConfiguration/embPers/ram").Set(response, ramPresent);
        bool ledrPresent = ((byte01 & 0b1000000) == 0b1000000);
        Pointer("/data/rsp/trConfiguration/embPers/ledr").Set(response, ledrPresent);
        bool ledgPresent = ((byte01 & 0b10000000) == 0b10000000);
        Pointer("/data/rsp/trConfiguration/embPers/ledg").Set(response, ledgPresent);
        // Byte 0x02
        uint8_t byte02 = configuration[0x01];
        if (dpaVer < 0x0415)
        {
          bool spiPresent = ((byte02 & 0b1) == 0b1);
          Pointer("/data/rsp/trConfiguration/embPers/spi").Set(response, spiPresent);
        }
        bool ioPresent = ((byte02 & 0b10) == 0b10);
        Pointer("/data/rsp/trConfiguration/embPers/io").Set(response, ioPresent);
        bool thermometerPresent = ((byte02 & 0b100) == 0b100);
        Pointer("/data/rsp/trConfiguration/embPers/thermometer").Set(response, thermometerPresent);
        if (dpaVer < 0x0415)
        {
          bool pwmPresent = ((byte02 & 0b1000) == 0b1000);
          Pointer("/data/rsp/trConfiguration/embPers/pwm").Set(response, pwmPresent);
        }
        bool uartPresent = ((byte02 & 0b10000) == 0b10000);
        Pointer("/data/rsp/trConfiguration/embPers/uart").Set(response, uartPresent);
        if (dpaVer < 0x0400)
        {
          bool frcPresent = ((byte02 & 0b100000) == 0b100000);
          Pointer("/data/rsp/trConfiguration/embPers/frc").Set(response, frcPresent);
        }
        // Byte 0x05
        uint8_t byte05 = configuration[0x04];
        bool customDpaHandler = ((byte05 & 0b1) == 0b1);
        Pointer("/data/rsp/trConfiguration/customDpaHandler").Set(response, customDpaHandler);
        // For DPA v4.00 downwards
        if (dpaVer < 0x0400)
        {
          bool nodeDpaInterface = ((byte05 & 0b00000010) == 0b00000010);
          Pointer("/data/rsp/trConfiguration/nodeDpaInterface").Set(response, nodeDpaInterface);
        }
        // For DPA v4.10 upwards
        if (dpaVer >= 0x0410)
        {
          bool dpaPeerToPeer = ((byte05 & 0b00000010) == 0b00000010);
          Pointer("/data/rsp/trConfiguration/dpaPeerToPeer").Set(response, dpaPeerToPeer);
        }
        bool dpaAutoexec = ((byte05 & 0b100) == 0b100);
        Pointer("/data/rsp/trConfiguration/dpaAutoexec").Set(response, dpaAutoexec);
        bool routingOff = ((byte05 & 0b1000) == 0b1000);
        Pointer("/data/rsp/trConfiguration/routingOff").Set(response, routingOff);
        bool ioSetup = ((byte05 & 0b10000) == 0b10000);
        Pointer("/data/rsp/trConfiguration/ioSetup").Set(response, ioSetup);
        bool peerToPeer = ((byte05 & 0b100000) == 0b100000);
        Pointer("/data/rsp/trConfiguration/peerToPeer").Set(response, peerToPeer);
        // For DPA v3.03 onwards
        if (dpaVer >= 0x0303)
        {
          bool neverSleep = ((byte05 & 0b01000000) == 0b01000000);
          Pointer("/data/rsp/trConfiguration/neverSleep").Set(response, neverSleep);
        }
        // For DPA v4.00 onwards
        if (dpaVer >= 0x0400)
        {
          bool stdAndLpNetwork = ((byte05 & 0b10000000) == 0b10000000);
          Pointer("/data/rsp/trConfiguration/stdAndLpNetwork").Set(response, stdAndLpNetwork);
        }
        // Bytes fields
        Pointer("/data/rsp/trConfiguration/rfChannelA").Set(response, configuration[0x10]);
        Pointer("/data/rsp/trConfiguration/rfChannelB").Set(response, configuration[0x11]);
        if (dpaVer < 0x0400)
        {
          Pointer("/data/rsp/trConfiguration/rfSubChannelA").Set(response, configuration[0x05]);
          Pointer("/data/rsp/trConfiguration/rfSubChannelB").Set(response, configuration[0x06]);
        }
        Pointer("/data/rsp/trConfiguration/txPower").Set(response, configuration[0x07]);
        Pointer("/data/rsp/trConfiguration/rxFilter").Set(response, configuration[0x08]);
        Pointer("/data/rsp/trConfiguration/lpRxTimeout").Set(response, configuration[0x09]);
        Pointer("/data/rsp/trConfiguration/rfAltDsmChannel").Set(response, configuration[0x0B]);
        // BaudRate
        if (configuration[0x0A] <= BAUD_RATES_SIZE)
          Pointer("/data/rsp/trConfiguration/uartBaudrate").Set(response, BaudRates[configuration[0x0A]]);
        else
        {
          TRC_WARNING("Unknown baud rate constant: " << PAR(configuration[0x0A]));
          Pointer("/data/rsp/trConfiguration/uartBaudrate").Set(response, 0);
        }
        // For DPA >= v4.15
        if (dpaVer >= 0x0415)
        {
          // localFrcReception at [N] only
          if (m_enumerateDeviceParams.deviceAddress != COORDINATOR_ADDRESS)
          {
            bool localFrcReception = ((configuration[0x0c] & 0b00000001) == 0b00000001);
            Pointer("/data/rsp/trConfiguration/localFrcReception").Set(response, localFrcReception);
          }
        }
        // RFPGM byte
        uint8_t rfpgm = hwpConfig.RFPGM;
        bool rfPgmDualChannel = ((rfpgm & 0b00000011) == 0b00000011);
        Pointer("/data/rsp/trConfiguration/rfPgmDualChannel").Set(response, rfPgmDualChannel);
        bool rfPgmLpMode = ((rfpgm & 0b00000100) == 0b00000100);
        Pointer("/data/rsp/trConfiguration/rfPgmLpMode").Set(response, rfPgmLpMode);
        bool rfPgmIncorrectUpload = ((rfpgm & 0b00001000) == 0b00001000);
        Pointer("/data/rsp/trConfiguration/rfPgmIncorrectUpload").Set(response, rfPgmIncorrectUpload);
        bool enableAfterReset = ((rfpgm & 0b00010000) == 0b00010000);
        Pointer("/data/rsp/trConfiguration/rfPgmEnableAfterReset").Set(response, enableAfterReset);
        bool rfPgmTerminateAfter1Min = ((rfpgm & 0b01000000) == 0b01000000);
        Pointer("/data/rsp/trConfiguration/rfPgmTerminateAfter1Min").Set(response, rfPgmTerminateAfter1Min);
        bool rfPgmTerminateMcuPin = ((rfpgm & 0b10000000) == 0b10000000);
        Pointer("/data/rsp/trConfiguration/rfPgmTerminateMcuPin").Set(response, rfPgmTerminateMcuPin);
        // RF band - undocumented byte
        std::string rfBand = "";
        switch (hwpConfig.Undocumented[0] & 0x03)
        {
        case 0b00:
          rfBand = "868";
          break;
        case 0b01:
          rfBand = "916";
          break;
        case 0b10:
          rfBand = "433";
          break;
        default:
          TRC_WARNING("Unknown rf band constant: " << PAR((hwpConfig.Undocumented[0] & 0x03)));
        }
        Pointer("/data/rsp/trConfiguration/rfBand").Set(response, rfBand);

        // Undocumented breakdown for DPA 4.17+
        if (dpaVer >= 0x0417) {
          bool thermometerPresent = hwpConfig.Undocumented[0] & 0x10;
          Pointer("/data/rsp/trConfiguration/thermometerSensorPresent").Set(response, thermometerPresent);

          bool eepromPresent = hwpConfig.Undocumented[0] & 0x20;
          Pointer("/data/rsp/trConfiguration/serialEepromPresent").Set(response, eepromPresent);

          bool transcieverIL = hwpConfig.Undocumented[0] & 0x40;
          Pointer("/data/rsp/trConfiguration/transcieverILType").Set(response, transcieverIL);
        }

        // Result of more peripherals info according to request
        if (m_enumerateDeviceParams.morePeripheralsInfo == true)
        {
          // PerInfo objects
          std::map<int, embed::explore::MorePeripheralInformation::Param> mParam = deviceEnumerateResult.getMorePersInfo()->getPerParamsMap();
          // Array of objects
          rapidjson::Value perInfoJsonArray(kArrayType);
          for (std::map<int, embed::explore::MorePeripheralInformation::Param>::iterator it = mParam.begin(); it != mParam.end(); ++it)
          {
            rapidjson::Value perInfoObj(kObjectType);
            perInfoObj.AddMember("perTe", it->second.perTe, allocator);
            perInfoObj.AddMember("perT", it->second.perT, allocator);
            perInfoObj.AddMember("par1", it->second.par1, allocator);
            perInfoObj.AddMember("par2", it->second.par2, allocator);
            perInfoJsonArray.PushBack(perInfoObj, allocator);
          }
          Pointer("/data/rsp/morePeripheralsInfo").Set(response, perInfoJsonArray);
        }
      }

      // Set raw fields, if verbose mode is active
      if (m_comEnumerateDevice->getVerbose())
      {
        rapidjson::Value rawArray(kArrayType);
        Document::AllocatorType& allocator = response.GetAllocator();
        while (deviceEnumerateResult.isNextTransactionResult())
        {
          std::unique_ptr<IDpaTransactionResult2> transResult = deviceEnumerateResult.consumeNextTransactionResult();
          rapidjson::Value rawObject(kObjectType);
          rawObject.AddMember(
            "request",
            encodeBinary(transResult->getRequest().DpaPacket().Buffer, transResult->getRequest().GetLength()),
            allocator
          );
          rawObject.AddMember(
            "requestTs",
            TimeConversion::encodeTimestamp(transResult->getRequestTs()),
            allocator
          );
          rawObject.AddMember(
            "confirmation",
            encodeBinary(transResult->getConfirmation().DpaPacket().Buffer, transResult->getConfirmation().GetLength()),
            allocator
          );
          rawObject.AddMember(
            "confirmationTs",
            TimeConversion::encodeTimestamp(transResult->getConfirmationTs()),
            allocator
          );
          rawObject.AddMember(
            "response",
            encodeBinary(transResult->getResponse().DpaPacket().Buffer, transResult->getResponse().GetLength()),
            allocator
          );
          rawObject.AddMember(
            "responseTs",
            TimeConversion::encodeTimestamp(transResult->getResponseTs()),
            allocator
          );
          // Add object into array
          rawArray.PushBack(rawObject, allocator);
        }
        // Add array into response document
        Pointer("/data/raw").Set(response, rawArray);
      }

      // Set status
      Pointer("/data/status").Set(response, status);
      Pointer("/data/statusStr").Set(response, deviceEnumerateResult.getStatusStr());

      // Send message
      m_iMessagingSplitterService->sendMessage(*m_messagingId, std::move(response));
    }

    //--------------------------
    // Creates and send response
    //--------------------------
    void createResponse(const int status, const std::string statusStr)
    {
      Document response;

      // Set common parameters
      Pointer("/mType").Set(response, m_msgType->m_type);
      Pointer("/data/msgId").Set(response, m_comEnumerateDevice->getMsgId());

      // Set status
      Pointer("/data/status").Set(response, status);
      Pointer("/data/statusStr").Set(response, statusStr);

      // Send message
      m_iMessagingSplitterService->sendMessage(*m_messagingId, std::move(response));
    }

    //-------------------
    // Handle the request
    //-------------------
    void handleMsg(const std::string& messagingId, const IMessagingSplitterService::MsgType& msgType, rapidjson::Document doc)
    {
      TRC_FUNCTION_ENTER(
        PAR(messagingId) <<
        NAME_PAR(mType, msgType.m_type) <<
        NAME_PAR(major, msgType.m_major) <<
        NAME_PAR(minor, msgType.m_minor) <<
        NAME_PAR(micro, msgType.m_micro)
      );

      // Unsupported type of request
      if (msgType.m_type != m_mTypeName_iqmeshNetworkEnumerateDevice)
        THROW_EXC(std::logic_error, "Unsupported message type: " << PAR(msgType.m_type));

      // Creating representation object
      ComIqmeshNetworkEnumerateDevice comEnumerateDevice(doc);
      m_msgType = &msgType;
      m_messagingId = &messagingId;
      m_comEnumerateDevice = &comEnumerateDevice;

      // Parsing and checking service parameters
      try
      {
        m_enumerateDeviceParams = comEnumerateDevice.getEnumerateDevicefParams();
      }
      catch (const std::exception& e)
      {
        CATCH_EXC_TRC_WAR(std::exception, e, "Error while parsing service input parameters.");
        createResponse(parsingRequestError, e.what());
        TRC_FUNCTION_LEAVE("");
        return;
      }

      // Try to establish exclusive access%
      try
      {
        m_exclusiveAccess = m_iIqrfDpaService->getExclusiveAccess();
      }
      catch (const std::exception& e)
      {
        CATCH_EXC_TRC_WAR(std::exception, e, "Exclusive access error.");
        createResponse(exclusiveAccessError, e.what());
        TRC_FUNCTION_LEAVE("");
        return;
      }

      try
      {
        // SmartConnect result
        DeviceEnumerateResult deviceEnumerateResult;

        // Enumerate device
        enumerateDevice(deviceEnumerateResult);

        // Create and send response
        createResponse(deviceEnumerateResult);
      }
      catch (std::exception& e)
      {
        CATCH_EXC_TRC_WAR(std::exception, e, e.what());
      }

      // Release exclusive access
      m_exclusiveAccess.reset();
      TRC_FUNCTION_LEAVE("");
    }

  public:
    void activate(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************************" << std::endl <<
        "EnumerateDeviceService instance activate" << std::endl <<
        "******************************************"
      );

      (void)props;

      // for the sake of register function parameters
      std::vector<std::string> supportedMsgTypes =
      {
        m_mTypeName_iqmeshNetworkEnumerateDevice
      };

      m_iMessagingSplitterService->registerFilteredMsgHandler(
        supportedMsgTypes,
        [&](const std::string & messagingId, const IMessagingSplitterService::MsgType & msgType, rapidjson::Document doc)
      {
        handleMsg(messagingId, msgType, std::move(doc));
      });

      TRC_FUNCTION_LEAVE("");
    }

    void deactivate()
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "**************************************" << std::endl <<
        "EnumerateDeviceService instance deactivate" << std::endl <<
        "**************************************"
      );

      // for the sake of unregister function parameters
      std::vector<std::string> supportedMsgTypes =
      {
        m_mTypeName_iqmeshNetworkEnumerateDevice
      };

      m_iMessagingSplitterService->unregisterFilteredMsgHandler(supportedMsgTypes);

      TRC_FUNCTION_LEAVE("");
    }

    void modify(const shape::Properties *props)
    {
      (void)props;
    }

    void attachInterface(IIqrfDpaService* iface)
    {
      m_iIqrfDpaService = iface;
    }

    void detachInterface(IIqrfDpaService* iface)
    {
      if (m_iIqrfDpaService == iface) {
        m_iIqrfDpaService = nullptr;
      }
    }

    void attachInterface(IJsCacheService* iface)
    {
      m_iJsCacheService = iface;
    }

    void detachInterface(IJsCacheService* iface)
    {
      if (m_iJsCacheService == iface) {
        m_iJsCacheService = nullptr;
      }
    }

    void attachInterface(IMessagingSplitterService* iface)
    {
      m_iMessagingSplitterService = iface;
    }

    void detachInterface(IMessagingSplitterService* iface)
    {
      if (m_iMessagingSplitterService == iface) {
        m_iMessagingSplitterService = nullptr;
      }
    }
  };

  EnumerateDeviceService::EnumerateDeviceService()
  {
    m_imp = shape_new Imp(*this);
  }

  EnumerateDeviceService::~EnumerateDeviceService()
  {
    delete m_imp;
  }

  void EnumerateDeviceService::attachInterface(iqrf::IIqrfDpaService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void EnumerateDeviceService::detachInterface(iqrf::IIqrfDpaService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void EnumerateDeviceService::attachInterface(iqrf::IJsCacheService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void EnumerateDeviceService::detachInterface(iqrf::IJsCacheService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void EnumerateDeviceService::attachInterface(iqrf::IMessagingSplitterService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void EnumerateDeviceService::detachInterface(iqrf::IMessagingSplitterService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void EnumerateDeviceService::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void EnumerateDeviceService::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }


  void EnumerateDeviceService::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void EnumerateDeviceService::deactivate()
  {
    m_imp->deactivate();
  }

  void EnumerateDeviceService::modify( const shape::Properties *props )
  {
    m_imp->modify( props );
  }
}