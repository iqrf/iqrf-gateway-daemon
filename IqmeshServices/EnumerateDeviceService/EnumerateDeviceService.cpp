#define IEnumerateDeviceService_EXPORTS

#include "EnumerateDeviceService.h"
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

TRC_INIT_MODULE(iqrf::EnumerateDeviceService);


using namespace rapidjson;

namespace {

  // helper functions
  std::string encodeHexaNum_CapitalLetters(uint16_t from)
  {
    std::ostringstream os;
    os.fill('0'); os.width(4);
    os << std::hex << std::uppercase << (int)from;
    return os.str();
  }


  // holds parsed data from OS read response
  class OsReadObject {
  public:
    class TrMcuType {
    public:
      uint8_t value;
      std::string trType;
      bool fccCertified;
      std::string mcuType;
    };

    class Flags {
    public:
      uint8_t value;
      bool insufficientOsBuild;
      std::string interface;
      bool dpaHandlerDetected;
      bool dpaHandlerNotDetectedButEnabled;
      bool noInterfaceSupported;
    };

    class SlotLimits {
    public:
      uint8_t value;
      std::string shortestTimeslot;
      std::string longestTimeslot;
    };

    std::string mid;
    std::string osVersion;
    TrMcuType trMcuType;
    std::string osBuild;
    std::string rssi;
    std::string supplyVoltage;
    Flags flags;
    SlotLimits slotLimits;

  };

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

  // service general fail code - may and probably will be changed later in the future
  static const int SERVICE_ERROR = 1000;

  static const int SERVICE_ERROR_NOERROR = 0;

  static const int SERVICE_ERROR_INTERNAL = SERVICE_ERROR + 1;
  static const int SERVICE_ERROR_NODE_NOT_BONDED = SERVICE_ERROR + 2;
  static const int SERVICE_ERROR_INFO_MISSING = SERVICE_ERROR + 3;
};


namespace iqrf {

  // Holds information about errors, which encounter during device enumerate service run
  class DeviceEnumerateError {
  public:
    // Type of error
    enum class Type {
      NoError,
      NotBonded,
      InfoMissing,
      OsRead,
      PerEnum,
      ReadHwp,
      MorePersInfo
    };

    DeviceEnumerateError() : m_type(Type::NoError), m_message("") {};
    DeviceEnumerateError(Type errorType) : m_type(errorType), m_message("") {};
    DeviceEnumerateError(Type errorType, const std::string& message) : m_type(errorType), m_message(message) {};

    Type getType() const { return m_type; };
    std::string getMessage() const { return m_message; };

    DeviceEnumerateError& operator=(const DeviceEnumerateError& error) {
      if (this == &error) {
        return *this;
      }

      this->m_type = error.m_type;
      this->m_message = error.m_message;

      return *this;
    }

  private:
    Type m_type;
    std::string m_message;
  };


  // holds information about result of device enumeration
  class DeviceEnumerateResult {
  private:
    // results of partial DPA requests
    DeviceEnumerateError m_bondedError;
    DeviceEnumerateError m_discoveredError;
    DeviceEnumerateError m_vrnError;
    DeviceEnumerateError m_zoneError;
    DeviceEnumerateError m_parentError;
    DeviceEnumerateError m_osReadError;
    DeviceEnumerateError m_perEnumError;
    DeviceEnumerateError m_readHwpConfigError;
    DeviceEnumerateError m_morePersInfoError;

    uint8_t m_deviceAddr;
    bool m_discovered;
    uint8_t m_vrn;
    uint8_t m_zone;
    uint8_t m_parent;
    uint16_t m_enumeratedNodeHwpIdVer;
    std::vector<uns8> m_osRead;
    uint16_t m_osBuild;
    TEnumPeripheralsAnswer m_perEnum;
    TPerOSReadCfg_Response m_hwpConfig;
    std::vector<TPeripheralInfoAnswer> m_morePersInfo;

    uint16_t m_nodeHwpId;
    std::string m_manufacturer = "";
    std::string m_product = "";

    std::list<std::string> m_standards = { "" };

    // transaction results
    std::list<std::unique_ptr<IDpaTransactionResult2>> m_transResults;

  public:
  
    DeviceEnumerateError getBondedError() const { return m_bondedError; };
    void setBondedError(const DeviceEnumerateError& error) {
      m_bondedError = error;
    }


    DeviceEnumerateError getDiscoveredError() const { return m_discoveredError; };
    void setDiscoveredError(const DeviceEnumerateError& error) {
      m_discoveredError = error;
    }

    DeviceEnumerateError getVrnError() const { return m_vrnError; };
    void setVrnError(const DeviceEnumerateError& error) {
      m_vrnError = error;
    }

    DeviceEnumerateError getZoneError() const { return m_zoneError; };
    void setZoneError(const DeviceEnumerateError& error) {
      m_zoneError = error;
    }

    DeviceEnumerateError getParentError() const { return m_parentError; };
    void setParentError(const DeviceEnumerateError& error) {
      m_parentError = error;
    }

    DeviceEnumerateError getOsReadError() const { return m_osReadError; };
    void setOsReadError(const DeviceEnumerateError& error) {
      m_osReadError = error;
    }

    DeviceEnumerateError getPerEnumError() const { return m_perEnumError; };
    void setPerEnumError(const DeviceEnumerateError& error) {
      m_perEnumError = error;
    }

    DeviceEnumerateError getReadHwpConfigError() const { return m_readHwpConfigError; };
    void setReadHwpConfigError(const DeviceEnumerateError& error) {
      m_readHwpConfigError = error;
    }

    DeviceEnumerateError getMorePersInfoError() const { return m_morePersInfoError; };
    void setMorePersInfoError(const DeviceEnumerateError& error) {
      m_morePersInfoError = error;
    }


    uint8_t getDeviceAddr() const {
      return m_deviceAddr;
    }

    void setDeviceAddr(uint8_t deviceAddr) {
      m_deviceAddr = deviceAddr;
    }

    // sets HwpId of enumerated node
    void setEnumeratedNodeHwpId(const uint16_t hwpId) {
      m_nodeHwpId = hwpId;
    }

    // returns HwpId of enumerated node
    uint16_t getEnumeratedNodeHwpId() const {
      return m_nodeHwpId;
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

    // sets HwpId version of enumerated node
    void setEnumeratedNodeHwpIdVer(const uint16_t hwpIdVer) {
      m_enumeratedNodeHwpIdVer = hwpIdVer;
    }

    // returns HwpId version of enumerated node
    uint16_t getEnumeratedNodeHwpIdVer() const {
      return m_enumeratedNodeHwpIdVer;
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


    const std::vector<uns8> getOsRead() {
      return m_osRead;
    }

    void setOsRead(const uns8* osRead) {
      m_osRead.insert(m_osRead.begin(), osRead, osRead + DPA_MAX_DATA_LENGTH);
    }

    uint16_t getOsBuild() const {
      return m_osBuild;
    }

    void setOsBuild(uint16_t osBuild) {
      m_osBuild = osBuild;
    }

    TEnumPeripheralsAnswer getPerEnum() const {
      return m_perEnum;
    }

    void setPerEnum(TEnumPeripheralsAnswer perEnum) {
      m_perEnum = perEnum;
    }

    TPerOSReadCfg_Response getHwpConfig() const {
      return m_hwpConfig;
    }

    void setHwpConfig(TPerOSReadCfg_Response hwpConfig) {
      m_hwpConfig = hwpConfig;
    }


    std::vector<TPeripheralInfoAnswer> getMorePersInfo() const {
      return m_morePersInfo;
    }

    void setMorePersInfo(const std::vector<TPeripheralInfoAnswer>& morePersInfo) {
      m_morePersInfo = morePersInfo;
    }

    std::list<std::string> getStandards() {
      return m_standards;
    }

    void setStandards(std::list<std::string> standards) {
      m_standards = standards;
    }

    // adds transaction result into the list of results
    void addTransactionResult(std::unique_ptr<IDpaTransactionResult2>& transResult) {
      m_transResults.push_back(std::move(transResult));
    }

    bool isNextTransactionResult() {
      return (m_transResults.size() > 0);
    }

    // consumes the first element in the transaction results list
    std::unique_ptr<IDpaTransactionResult2> consumeNextTransactionResult() {
      std::list<std::unique_ptr<IDpaTransactionResult2>>::iterator iter = m_transResults.begin();
      std::unique_ptr<IDpaTransactionResult2> tranResult = std::move(*iter);
      m_transResults.pop_front();
      return std::move(tranResult);
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
    //IMessagingSplitterService::MsgType* m_msgType_mngIqmeshWriteConfig;

    iqrf::IJsCacheService* m_iJsCacheService = nullptr;
    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    IIqrfDpaService* m_iIqrfDpaService = nullptr;
    std::unique_ptr<IIqrfDpaService::ExclusiveAccess> m_exclusiveAccess;

    // number of repeats
    uint8_t m_repeat;

    // if is set Verbose mode
    bool m_returnVerbose = false;

    bool m_morePeripheralsInfo = false;


  public:
    Imp(EnumerateDeviceService& parent) : m_parent(parent)
    {
      /*
      m_msgType_mngIqmeshWriteConfig
        = shape_new IMessagingSplitterService::MsgType(m_mTypeName_mngIqmeshWriteConfig, 1, 0, 0);
        */
    }

    ~Imp()
    {
    }


  private:
    
    // returns 1 byte of discovery data read from the coordinator private external EEPROM storage
    uint8_t readDiscoveryByte(
      DeviceEnumerateResult& deviceEnumerateResult, 
      uint16_t address 
    ) {
      TRC_FUNCTION_ENTER("");

      DpaMessage eeepromReadRequest;
      DpaMessage::DpaPacket_t eeepromReadPacket;
      eeepromReadPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      eeepromReadPacket.DpaRequestPacket_t.PNUM = PNUM_EEEPROM;
      eeepromReadPacket.DpaRequestPacket_t.PCMD = CMD_EEEPROM_XREAD;
      eeepromReadPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;

      // read data from specified address
      uns8* pData = eeepromReadPacket.DpaRequestPacket_t.DpaMessage.Request.PData;
      pData[0] = address & 0xFF;
      pData[1] = (address >> 8) & 0xFF;

      // length of data to read[in bytes]
      pData[2] = 1;

      eeepromReadRequest.DataToBuffer(eeepromReadPacket.Buffer, sizeof(TDpaIFaceHeader) + 3);

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> eeepromReadTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      for (int rep = 0; rep <= m_repeat; rep++) {
        try {
          //discoveryDataTransaction = m_iIqrfDpaService->executeDpaTransaction(discoveryDataRequest);
          eeepromReadTransaction = m_exclusiveAccess->executeDpaTransaction(eeepromReadRequest);
          transResult = eeepromReadTransaction->get();
        }
        catch (std::exception& e) {
          TRC_WARNING("DPA transaction error : " << e.what());

          if (rep < m_repeat) {
            continue;
          }

          THROW_EXC(std::logic_error, "DPA transaction error : " << e.what());
        }

        TRC_DEBUG("Result from EEEPROM X read transaction as string:" << PAR(transResult->getErrorString()));

        IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

        // because of the move-semantics
        DpaMessage dpaResponse = transResult->getResponse();
        deviceEnumerateResult.addTransactionResult(transResult);

        if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
          TRC_INFORMATION("EEEPROM X read successful!");
          TRC_DEBUG(
            "DPA transaction: "
            << NAME_PAR(eeepromReadRequest.PeripheralType(), eeepromReadRequest.NodeAddress())
            << PAR(eeepromReadRequest.PeripheralCommand())
          );

          TRC_FUNCTION_LEAVE("");
          return dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData[0];
        }

        // transaction error
        if (errorCode < 0) {
          TRC_WARNING("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

          if (rep < m_repeat) {
            continue;
          }

          THROW_EXC(std::logic_error, "Transaction error. " << NAME_PAR_HEX("Error code", errorCode));
        }

        // DPA error
        TRC_WARNING("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

        if (rep < m_repeat) {
          continue;
        }

        THROW_EXC(std::logic_error, "DPA error. " << NAME_PAR_HEX("Error code", errorCode));
      }

      THROW_EXC(std::logic_error, "Internal error ");
    }

    // read discovery data
    void discoveryData(DeviceEnumerateResult& deviceEnumerateResult) 
    {
      // get discovered indicator
      try {
        uint16_t address = 0x20 + deviceEnumerateResult.getDeviceAddr() / 8;

        uint8_t discoveredDevicesByte = readDiscoveryByte(deviceEnumerateResult, address);
        
        uint8_t bitIndex = deviceEnumerateResult.getDeviceAddr() % 8;
        uint8_t compareByte = uint8_t(pow(2, bitIndex));

        deviceEnumerateResult.setDiscovered(
          (discoveredDevicesByte & compareByte) == compareByte
        );
      }
      catch (std::exception& ex) {
        DeviceEnumerateError error(DeviceEnumerateError::Type::InfoMissing, ex.what());
        deviceEnumerateResult.setDiscoveredError(error);
      }

      // VRN
      try {
        uint16_t address = 0x5000 + deviceEnumerateResult.getDeviceAddr();

        uint8_t vrnByte = readDiscoveryByte(deviceEnumerateResult, address);
        deviceEnumerateResult.setVrn(vrnByte);
      }
      catch (std::exception& ex) {
        DeviceEnumerateError error(DeviceEnumerateError::Type::InfoMissing, ex.what());
        deviceEnumerateResult.setVrnError(error);
      }

      // zone
      try {
        uint16_t address = 0x5200 + deviceEnumerateResult.getDeviceAddr();

        uint8_t zoneByte = readDiscoveryByte(deviceEnumerateResult, address);
        deviceEnumerateResult.setZone(zoneByte);
      }
      catch (std::exception& ex) {
        DeviceEnumerateError error(DeviceEnumerateError::Type::InfoMissing, ex.what());
        deviceEnumerateResult.setZoneError(error);
      }

      // parent
      try {
        uint16_t address = 0x5300 + deviceEnumerateResult.getDeviceAddr();

        uint8_t parentByte = readDiscoveryByte(deviceEnumerateResult, address);
        deviceEnumerateResult.setParent(parentByte);
      }
      catch (std::exception& ex) {
        DeviceEnumerateError error(DeviceEnumerateError::Type::InfoMissing, ex.what());
        deviceEnumerateResult.setParentError(error);
      }
    }

    
    // reads OS info about smart connected node
    void osRead(DeviceEnumerateResult& deviceEnumerateResult) {
      TRC_FUNCTION_ENTER("");

      DpaMessage osReadRequest;
      DpaMessage::DpaPacket_t osReadPacket;
      osReadPacket.DpaRequestPacket_t.NADR = deviceEnumerateResult.getDeviceAddr();
      osReadPacket.DpaRequestPacket_t.PNUM = PNUM_OS;
      osReadPacket.DpaRequestPacket_t.PCMD = CMD_OS_READ;
      osReadPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
      osReadRequest.DataToBuffer(osReadPacket.Buffer, sizeof(TDpaIFaceHeader));

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> osReadTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      for (int rep = 0; rep <= m_repeat; rep++) {
        try {
          osReadTransaction = m_exclusiveAccess->executeDpaTransaction(osReadRequest);
          transResult = osReadTransaction->get();
        }
        catch (std::exception& e) {
          TRC_WARNING("DPA transaction error : " << e.what());

          if (rep < m_repeat) {
            continue;
          }

          DeviceEnumerateError error(DeviceEnumerateError::Type::OsRead, e.what());
          deviceEnumerateResult.setOsReadError(error);

          TRC_FUNCTION_LEAVE("");
          return;
        }

        TRC_DEBUG("Result from OS read transaction as string:" << PAR(transResult->getErrorString()));

        IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

        // because of the move-semantics
        DpaMessage dpaResponse = transResult->getResponse();
        deviceEnumerateResult.addTransactionResult(transResult);

        if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
          TRC_INFORMATION("OS read successful!");
          TRC_DEBUG(
            "DPA transaction: "
            << NAME_PAR(osReadRequest.PeripheralType(), osReadRequest.NodeAddress())
            << PAR((unsigned)osReadRequest.PeripheralCommand())
          );

          // get OS data
          uns8* osData = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData;
          deviceEnumerateResult.setOsRead(osData);

          TPerOSRead_Response resp = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerOSRead_Response;
          deviceEnumerateResult.setOsBuild(resp.OsBuild);

          deviceEnumerateResult.setEnumeratedNodeHwpId(dpaResponse.DpaPacket().DpaResponsePacket_t.HWPID);

          TRC_FUNCTION_LEAVE("");
          return;
        }

        // transaction error
        if (errorCode < 0) {
          TRC_WARNING("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

          if (rep < m_repeat) {
            continue;
          }

          DeviceEnumerateError error(DeviceEnumerateError::Type::OsRead, "Transaction error.");
          deviceEnumerateResult.setOsReadError(error);

          TRC_FUNCTION_LEAVE("");
          return;
        }

        // DPA error
        TRC_WARNING("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

        if (rep < m_repeat) {
          continue;
        }

        DeviceEnumerateError error(DeviceEnumerateError::Type::OsRead, "Dpa error.");
        deviceEnumerateResult.setOsReadError(error);

        TRC_FUNCTION_LEAVE("");
      }
    }
    
    void peripheralEnumeration(DeviceEnumerateResult& deviceEnumerateResult) {
      TRC_FUNCTION_ENTER("");

      DpaMessage perEnumRequest;
      DpaMessage::DpaPacket_t perEnumPacket;
      perEnumPacket.DpaRequestPacket_t.NADR = deviceEnumerateResult.getDeviceAddr();
      perEnumPacket.DpaRequestPacket_t.PNUM = 0xFF;
      perEnumPacket.DpaRequestPacket_t.PCMD = 0x3F;
      perEnumPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
      perEnumRequest.DataToBuffer(perEnumPacket.Buffer, sizeof(TDpaIFaceHeader));

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> perEnumTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      for (int rep = 0; rep <= m_repeat; rep++) {
        try {
          //perEnumTransaction = m_iIqrfDpaService->executeDpaTransaction(perEnumRequest);
          perEnumTransaction = m_exclusiveAccess->executeDpaTransaction(perEnumRequest);
          transResult = perEnumTransaction->get();
        }
        catch (std::exception& e) {
          TRC_WARNING("DPA transaction error : " << e.what());

          if (rep < m_repeat) {
            continue;
          }

          DeviceEnumerateError error(DeviceEnumerateError::Type::PerEnum, e.what());
          deviceEnumerateResult.setPerEnumError(error);

          TRC_FUNCTION_LEAVE("");
          return;
        }

        TRC_DEBUG("Result from peripheral enumeration transaction as string:" << PAR(transResult->getErrorString()));

        IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

        // because of the move-semantics
        DpaMessage dpaResponse = transResult->getResponse();
        deviceEnumerateResult.addTransactionResult(transResult);

        if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
          TRC_INFORMATION("Peripheral enumeration successful!");
          TRC_DEBUG(
            "DPA transaction: "
            << NAME_PAR(perEnumRequest.PeripheralType(), perEnumRequest.NodeAddress())
            << PAR(perEnumRequest.PeripheralCommand())
          );

          // get peripheral enumeration
          TEnumPeripheralsAnswer perEnum = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.EnumPeripheralsAnswer;
          deviceEnumerateResult.setPerEnum(perEnum);

          // parsing response pdata
          uns8* respData = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData;
          uint8_t minorHwpIdVer = respData[9];
          uint8_t majorHwpIdVer = respData[10];

          deviceEnumerateResult.setEnumeratedNodeHwpIdVer(minorHwpIdVer + (majorHwpIdVer << 8));

          TRC_FUNCTION_LEAVE("");
          return;
        }

        // transaction error
        if (errorCode < 0) {
          TRC_WARNING("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

          if (rep < m_repeat) {
            continue;
          }

          DeviceEnumerateError error(DeviceEnumerateError::Type::PerEnum, "Transaction error.");
          deviceEnumerateResult.setPerEnumError(error);

          TRC_FUNCTION_LEAVE("");
          return;
        }

        // DPA error
        TRC_WARNING("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

        if (rep < m_repeat) {
          continue;
        }

        DeviceEnumerateError error(DeviceEnumerateError::Type::PerEnum, "Dpa error.");
        deviceEnumerateResult.setPerEnumError(error);

        TRC_FUNCTION_LEAVE("");
      }
    }

    void readHwpConfiguration(DeviceEnumerateResult& deviceEnumerateResult) {
      TRC_FUNCTION_ENTER("");

      DpaMessage readHwpRequest;
      DpaMessage::DpaPacket_t readHwpPacket;
      readHwpPacket.DpaRequestPacket_t.NADR = deviceEnumerateResult.getDeviceAddr();
      readHwpPacket.DpaRequestPacket_t.PNUM = PNUM_OS;
      readHwpPacket.DpaRequestPacket_t.PCMD = CMD_OS_READ_CFG;
      readHwpPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
      readHwpRequest.DataToBuffer(readHwpPacket.Buffer, sizeof(TDpaIFaceHeader));

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> readHwpTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      for (int rep = 0; rep <= m_repeat; rep++) {
        try {
          //readHwpTransaction = m_iIqrfDpaService->executeDpaTransaction(readHwpRequest);
          readHwpTransaction = m_exclusiveAccess->executeDpaTransaction(readHwpRequest);
          transResult = readHwpTransaction->get();
        }
        catch (std::exception& e) {
          TRC_WARNING("DPA transaction error : " << e.what());

          if (rep < m_repeat) {
            continue;
          }

          DeviceEnumerateError error(DeviceEnumerateError::Type::ReadHwp, e.what());
          deviceEnumerateResult.setReadHwpConfigError(error);

          TRC_FUNCTION_LEAVE("");
          return;
        }

        TRC_DEBUG("Result from read HWP config transaction as string:" << PAR(transResult->getErrorString()));

        IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

        // because of the move-semantics
        DpaMessage dpaResponse = transResult->getResponse();
        deviceEnumerateResult.addTransactionResult(transResult);

        if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
          TRC_INFORMATION("Read HWP configuration successful!");
          TRC_DEBUG(
            "DPA transaction: "
            << NAME_PAR(readHwpRequest.PeripheralType(), readHwpRequest.NodeAddress())
            << PAR((unsigned)readHwpRequest.PeripheralCommand())
          );

          // get HWP configuration 
          TPerOSReadCfg_Response readHwpConfig = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerOSReadCfg_Response;
          deviceEnumerateResult.setHwpConfig(readHwpConfig);

          TRC_FUNCTION_LEAVE("");
          return;
        }

        // transaction error
        if (errorCode < 0) {
          TRC_WARNING("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

          if (rep < m_repeat) {
            continue;
          }

          DeviceEnumerateError error(DeviceEnumerateError::Type::ReadHwp, "Transaction error.");
          deviceEnumerateResult.setReadHwpConfigError(error);

          TRC_FUNCTION_LEAVE("");
          return;
        }

        // DPA error
        TRC_WARNING("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

        if (rep < m_repeat) {
          continue;
        }

        DeviceEnumerateError error(DeviceEnumerateError::Type::ReadHwp, "Dpa error.");
        deviceEnumerateResult.setReadHwpConfigError(error);

        TRC_FUNCTION_LEAVE("");
      }
    }
    
    void getInfoForMorePeripherals(DeviceEnumerateResult& deviceEnumerateResult) {
      TRC_FUNCTION_ENTER("");

      DpaMessage morePersInfoRequest;
      DpaMessage::DpaPacket_t morePersInfoPacket;
      morePersInfoPacket.DpaRequestPacket_t.NADR = deviceEnumerateResult.getDeviceAddr();
      morePersInfoPacket.DpaRequestPacket_t.PNUM = 0xFF;
      morePersInfoPacket.DpaRequestPacket_t.PCMD = 0x3F;
      morePersInfoPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
      morePersInfoRequest.DataToBuffer(morePersInfoPacket.Buffer, sizeof(TDpaIFaceHeader));

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> morePersInfoTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      for (int rep = 0; rep <= m_repeat; rep++) {
        try {
          //morePersInfoTransaction = m_iIqrfDpaService->executeDpaTransaction(morePersInfoRequest);
          morePersInfoTransaction = m_exclusiveAccess->executeDpaTransaction(morePersInfoRequest);
          transResult = morePersInfoTransaction->get();
        }
        catch (std::exception& e) {
          TRC_WARNING("DPA transaction error : " << e.what());

          if (rep < m_repeat) {
            continue;
          }

          DeviceEnumerateError error(DeviceEnumerateError::Type::MorePersInfo, e.what());
          deviceEnumerateResult.setMorePersInfoError(error);

          TRC_FUNCTION_LEAVE("");
          return;
        }

        TRC_DEBUG("Result from get info for more peripherals transaction as string:" << PAR(transResult->getErrorString()));

        IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

        // because of the move-semantics
        DpaMessage dpaResponse = transResult->getResponse();
        deviceEnumerateResult.addTransactionResult(transResult);

        if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
          TRC_INFORMATION("Get info for more peripherals successful!");
          TRC_DEBUG(
            "DPA transaction: "
            << NAME_PAR(morePersInfoRequest.PeripheralType(), morePersInfoRequest.NodeAddress())
            << PAR(morePersInfoRequest.PeripheralCommand())
          );

          // get info for more peripherals
          TPeripheralInfoAnswer* persInfoArr = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PeripheralInfoAnswers;
          
          std::vector<TPeripheralInfoAnswer> persInfoList;
          for (int i = 0; i < PERIPHERALS_NUM; i++) {
            persInfoList.push_back(persInfoArr[i]);
          }
          
          deviceEnumerateResult.setMorePersInfo(persInfoList);

          TRC_FUNCTION_LEAVE("");
          return;
        }

        // transaction error
        if (errorCode < 0) {
          TRC_WARNING("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

          if (rep < m_repeat) {
            continue;
          }

          DeviceEnumerateError error(DeviceEnumerateError::Type::MorePersInfo, "Transaction error.");
          deviceEnumerateResult.setMorePersInfoError(error);

          TRC_FUNCTION_LEAVE("");
          return;
        }

        // DPA error
        TRC_WARNING("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

        if (rep < m_repeat) {
          continue;
        }

        DeviceEnumerateError error(DeviceEnumerateError::Type::MorePersInfo, "Dpa error.");
        deviceEnumerateResult.setMorePersInfoError(error);
      }

      TRC_FUNCTION_LEAVE("");
    }
    
    // checks, if the specified address is bonded
    void checkBond(DeviceEnumerateResult& deviceEnumerateResult, const uint8_t deviceAddr)
    {
      TRC_FUNCTION_ENTER("");

      DpaMessage bondedNodesRequest;
      DpaMessage::DpaPacket_t bondedNodesPacket;
      bondedNodesPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      bondedNodesPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
      bondedNodesPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_BONDED_DEVICES;
      bondedNodesPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
      bondedNodesRequest.DataToBuffer(bondedNodesPacket.Buffer, sizeof(TDpaIFaceHeader));

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> bondedNodesTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      for (int rep = 0; rep <= m_repeat; rep++) {
        try {
          bondedNodesTransaction = m_iIqrfDpaService->executeDpaTransaction(bondedNodesRequest);
          transResult = bondedNodesTransaction->get();
        }
        catch (std::exception& e) {
          TRC_WARNING("DPA transaction error : " << e.what());

          if (rep < m_repeat) {
            continue;
          }

          DeviceEnumerateError error(DeviceEnumerateError::Type::NotBonded, e.what());
          deviceEnumerateResult.setBondedError(error);

          TRC_FUNCTION_LEAVE("");
          return;
        }

        TRC_DEBUG("Result from get bonded nodes transaction as string:" << PAR(transResult->getErrorString()));

        IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

        // because of the move-semantics
        DpaMessage dpaResponse = transResult->getResponse();
        deviceEnumerateResult.addTransactionResult(transResult);

        if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
          TRC_INFORMATION("Get bonded nodes successful!");
          TRC_DEBUG(
            "DPA transaction: "
            << NAME_PAR(bondedNodesRequest.PeripheralType(), bondedNodesRequest.NodeAddress())
            << PAR((unsigned)bondedNodesRequest.PeripheralCommand())
          );

          // get bonded nodes
          uns8* bondedNodesArr = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData;
          
          uint8_t byteIndex = deviceAddr / 8;
          uint8_t bitIndex = deviceAddr % 8;
          uint8_t compareByte = uint8_t(pow(2, bitIndex));

          if (!((bondedNodesArr[byteIndex] & compareByte) == compareByte)) {
            DeviceEnumerateError error(DeviceEnumerateError::Type::NotBonded, "Not bonded.");
            deviceEnumerateResult.setBondedError(error);
          }

          TRC_FUNCTION_LEAVE("");
          return;
        }

        // transaction error
        if (errorCode < 0) {
          TRC_WARNING("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

          if (rep < m_repeat) {
            continue;
          }

          DeviceEnumerateError error(DeviceEnumerateError::Type::NotBonded, "Transaction error.");
          deviceEnumerateResult.setBondedError(error);

          TRC_FUNCTION_LEAVE("");
          return;
        }

        // DPA error
        TRC_WARNING("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

        if (rep < m_repeat) {
          continue;
        }

        DeviceEnumerateError error(DeviceEnumerateError::Type::NotBonded, "Dpa error.");
        deviceEnumerateResult.setBondedError(error);

        TRC_FUNCTION_LEAVE("");
      }
    }
    
    
    // creates error response about service general fail
    rapidjson::Document getCheckParamsFailedResponse(
      const std::string& msgId,
      const IMessagingSplitterService::MsgType& msgType,
      const std::string& errorMsg
    )
    {
      rapidjson::Document response;

      Pointer("/mType").Set(response, msgType.m_type);
      Pointer("/data/msgId").Set(response, msgId);
      
      Pointer("/data/status").Set(response, SERVICE_ERROR);
      Pointer("/data/statusStr").Set(response, errorMsg);

      return response;
    }

    // creates error response about failed exclusive access
    rapidjson::Document getExclusiveAccessFailedResponse(
      const std::string& msgId,
      const IMessagingSplitterService::MsgType& msgType,
      const std::string& errorMsg
    ) 
    {
      rapidjson::Document response;

      Pointer("/mType").Set(response, msgType.m_type);
      Pointer("/data/msgId").Set(response, msgId);

      Pointer("/data/status").Set(response, SERVICE_ERROR_INTERNAL);
      Pointer("/data/statusStr").Set(response, errorMsg);

      return response;
    }

    // creates response for the situation, when target node is not bonded
    rapidjson::Document getNodeNotBondedResponse(
      const std::string& msgId,
      const IMessagingSplitterService::MsgType& msgType,
      DeviceEnumerateResult& deviceEnumResult,
      const ComIqmeshNetworkEnumerateDevice& comEnumerateDevice,
      const std::string& errorMsg
    )
    {
      rapidjson::Document response;

      Pointer("/mType").Set(response, msgType.m_type);
      Pointer("/data/msgId").Set(response, msgId);

      Pointer("/data/rsp/deviceAddr").Set(response, deviceEnumResult.getDeviceAddr());

      // set raw fields, if verbose mode is active
      if (comEnumerateDevice.getVerbose()) {
        setVerboseData(response, deviceEnumResult);
      }

      Pointer("/data/status").Set(response, SERVICE_ERROR_NODE_NOT_BONDED);
      Pointer("/data/statusStr").Set(response, errorMsg);

      return response;
    }

    // sets response VERBOSE data
    void setVerboseData(rapidjson::Document& response, DeviceEnumerateResult& deviceEnumerateResult)
    {
      rapidjson::Value rawArray(kArrayType);
      Document::AllocatorType& allocator = response.GetAllocator();

      while (deviceEnumerateResult.isNextTransactionResult()) {
        std::unique_ptr<IDpaTransactionResult2> transResult = deviceEnumerateResult.consumeNextTransactionResult();
        rapidjson::Value rawObject(kObjectType);

        rawObject.AddMember(
          "request",
          encodeBinary(transResult->getRequest().DpaPacket().Buffer, transResult->getRequest().GetLength()),
          allocator
        );

        rawObject.AddMember(
          "requestTs",
          encodeTimestamp(transResult->getRequestTs()),
          allocator
        );

        rawObject.AddMember(
          "confirmation",
          encodeBinary(transResult->getConfirmation().DpaPacket().Buffer, transResult->getConfirmation().GetLength()),
          allocator
        );

        rawObject.AddMember(
          "confirmationTs",
          encodeTimestamp(transResult->getConfirmationTs()),
          allocator
        );

        rawObject.AddMember(
          "response",
          encodeBinary(transResult->getResponse().DpaPacket().Buffer, transResult->getResponse().GetLength()),
          allocator
        );

        rawObject.AddMember(
          "responseTs",
          encodeTimestamp(transResult->getResponseTs()),
          allocator
        );

        // add object into array
        rawArray.PushBack(rawObject, allocator);
      }

      // add array into response document
      Pointer("/data/raw").Set(response, rawArray);
    }

    // sets error status for specified response
    void setResponseStatus(
      rapidjson::Document& response, int32_t errorCode, const std::string& msg
    ) 
    {
      Pointer("/data/status").Set(response, errorCode);
      Pointer("/data/statusStr").Set(response, msg);
    }

    // sets data for discovery response
    void setDiscoveryDataResponse(
      const std::string& messagingId,
      const IMessagingSplitterService::MsgType& msgType,
      DeviceEnumerateResult& deviceEnumerateResult,
      const ComIqmeshNetworkEnumerateDevice& comDeviceEnumerate,
      rapidjson::Document& response
    )
    {
      // if no data was successfully read, return empty message
      Pointer("/data/rsp/discovery/discovered").Set(response, deviceEnumerateResult.isDiscovered());
      Pointer("/data/rsp/discovery/vrn").Set(response, deviceEnumerateResult.getVrn());
      Pointer("/data/rsp/discovery/zone").Set(response, deviceEnumerateResult.getZone());
      Pointer("/data/rsp/discovery/parent").Set(response, deviceEnumerateResult.getParent());
    }

    // parses OS read info
    // TEMPORAL SOLUTION !!!
    OsReadObject parseOsReadResponse(const std::vector<uns8>& osReadInfo)
    {
      TRC_FUNCTION_ENTER(PAR(osReadInfo.size()))
      OsReadObject osReadObject;

      std::ostringstream moduleId;
      moduleId.fill('0');
      moduleId << std::hex << std::uppercase <<
        std::setw(2) << (int)osReadInfo[3] <<
        std::setw(2) << (int)osReadInfo[2] <<
        std::setw(2) << (int)osReadInfo[1] <<
        std::setw(2) << (int)osReadInfo[0];

      osReadObject.mid = moduleId.str();

      // OS version - string
      std::ostringstream osVer;
      uns8 osVersion = osReadInfo[4];

      osVer << std::hex << (int)(osVersion >> 4) << '.';
      osVer.fill('0');
      osVer << std::setw(2) << (int)(osVersion & 0xf) << 'D';

      osReadObject.osVersion = osVer.str();

      // trMcuType
      osReadObject.trMcuType.value = osReadInfo[5];
      uns8 mcuType = osReadInfo[5];

      std::string trTypeStr = "(DC)TR-";
      switch (mcuType >> 4) {
        case 2: trTypeStr += "72Dx";
          break;
        case 4: trTypeStr += "78Dx";
          break;
        case 11: trTypeStr += "76Dx";
          break;
        case 12: trTypeStr += "77Dx";
          break;
        case 13: trTypeStr += "75Dx";
          break;
        default: trTypeStr += "???";
          break;
      }

      osReadObject.trMcuType.trType = trTypeStr;
      osReadObject.trMcuType.fccCertified = ((mcuType & 0x08) == 0x08) ? true : false;
      osReadObject.trMcuType.mcuType = ((mcuType & 0x07) == 0x04) ? "PIC16LF1938" : "UNKNOWN";
    
      // OS build - string
      uint16_t osBuild = (osReadInfo[7] << 8) + osReadInfo[6];
      osReadObject.osBuild = encodeHexaNum_CapitalLetters(osBuild);

      // RSSI [dBm]
      int8_t rssi = osReadInfo[8] - 130;
      std::string rssiStr = std::to_string(rssi) + " dBm";
      osReadObject.rssi = rssiStr;

      // Supply voltage [V]
      float supplyVoltage = 261.12f / (float)(127 - osReadInfo[9]);
      char supplyVoltageStr[8];
      std::sprintf(supplyVoltageStr, "%1.2f V", supplyVoltage);
      osReadObject.supplyVoltage = supplyVoltageStr;

      // Flags
      uns8 flags = osReadInfo[10];
      osReadObject.flags.value = flags;
      osReadObject.flags.insufficientOsBuild = ((flags & 0x01) == 0x01) ? true : false;
      osReadObject.flags.interface = ((flags & 0x02) == 0x02) ? "UART" : "SPI";
      osReadObject.flags.dpaHandlerDetected = ((flags & 0x04) == 0x04) ? true : false;
      osReadObject.flags.dpaHandlerNotDetectedButEnabled = ((flags & 0x08) == 0x08) ? true : false;
      osReadObject.flags.noInterfaceSupported = ((flags & 0x10) == 0x10) ? true : false;

      // Slot limits
      uns8 slotLimits = osReadInfo[11];
      osReadObject.slotLimits.value = slotLimits;
      uint8_t shortestTimeSlot = ((slotLimits & 0x0f) + 3) * 10;
      uint8_t longestTimeSlot = (((slotLimits >> 0x04) & 0x0f) + 3) * 10;

      osReadObject.slotLimits.shortestTimeslot = std::to_string(shortestTimeSlot) + " ms";
      osReadObject.slotLimits.longestTimeslot = std::to_string(longestTimeSlot) + " ms";

      TRC_FUNCTION_LEAVE("")
      return osReadObject;
    }

    // sets OS read part of the response
    void setOsReadResponse(
      const std::string& messagingId,
      const IMessagingSplitterService::MsgType& msgType,
      DeviceEnumerateResult& deviceEnumerateResult,
      const ComIqmeshNetworkEnumerateDevice& comDeviceEnumerate,
      const OsReadObject& osReadObject, 
      rapidjson::Document& response
    )
    {
      rapidjson::Pointer("/data/rsp/osRead/mid").Set(response, osReadObject.mid);
      rapidjson::Pointer("/data/rsp/osRead/osVersion").Set(response, osReadObject.osVersion);

      rapidjson::Pointer("/data/rsp/osRead/trMcuType/value").Set(response, osReadObject.trMcuType.value);
      rapidjson::Pointer("/data/rsp/osRead/trMcuType/trType").Set(response, osReadObject.trMcuType.trType);
      rapidjson::Pointer("/data/rsp/osRead/trMcuType/fccCertified").Set(response, osReadObject.trMcuType.fccCertified);
      rapidjson::Pointer("/data/rsp/osRead/trMcuType/mcuType").Set(response, osReadObject.trMcuType.mcuType);

      rapidjson::Pointer("/data/rsp/osRead/osBuild").Set(response, osReadObject.osBuild);

      // RSSI [dBm]
      rapidjson::Pointer("/data/rsp/osRead/rssi").Set(response, osReadObject.rssi);

      // Supply voltage [V]
      rapidjson::Pointer("/data/rsp/osRead/supplyVoltage").Set(response, osReadObject.supplyVoltage);

      // Flags
      rapidjson::Pointer("/data/rsp/osRead/flags/value").Set(response, osReadObject.flags.value);
      rapidjson::Pointer("/data/rsp/osRead/flags/insufficientOsBuild").Set(response, osReadObject.flags.insufficientOsBuild);
      rapidjson::Pointer("/data/rsp/osRead/flags/interfaceType").Set(response, osReadObject.flags.interface);
      rapidjson::Pointer("/data/rsp/osRead/flags/dpaHandlerDetected").Set(response, osReadObject.flags.dpaHandlerDetected);
      rapidjson::Pointer("/data/rsp/osRead/flags/dpaHandlerNotDetectedButEnabled").Set(response, osReadObject.flags.dpaHandlerNotDetectedButEnabled);
      rapidjson::Pointer("/data/rsp/osRead/flags/noInterfaceSupported").Set(response, osReadObject.flags.noInterfaceSupported);

      // Slot limits
      rapidjson::Pointer("/data/rsp/osRead/slotLimits/value").Set(response, osReadObject.slotLimits.value);
      rapidjson::Pointer("/data/rsp/osRead/slotLimits/shortestTimeslot").Set(response, osReadObject.slotLimits.shortestTimeslot);
      rapidjson::Pointer("/data/rsp/osRead/slotLimits/longestTimeslot").Set(response, osReadObject.slotLimits.longestTimeslot);
    }

    // sets peripheral enumeration part of the response
    void setPeripheralEnumerationResponse(
      const std::string& messagingId,
      const IMessagingSplitterService::MsgType& msgType,
      DeviceEnumerateResult& deviceEnumerateResult,
      const ComIqmeshNetworkEnumerateDevice& comDeviceEnumerate,
      rapidjson::Document& response
    ) 
    {
      // per enum object
      TEnumPeripheralsAnswer perEnum = deviceEnumerateResult.getPerEnum();
      
      // dpa version - string
      std::string dpaVerStr = std::to_string((perEnum.DpaVersion >> 8) & 0xFF)
        + "." + encodeHexaNum((uint8_t)(perEnum.DpaVersion & 0xFF));
      Pointer("/data/rsp/peripheralEnumeration/dpaVer").Set(response, dpaVerStr);
      
      // perNr
      Pointer("/data/rsp/peripheralEnumeration/perNr").Set(response, perEnum.UserPerNr);

      Document::AllocatorType& allocator = response.GetAllocator();

      // embPers
      rapidjson::Value embPersJsonArray(kArrayType);
      for (int i = 0; i < EMBEDDED_PERS_LEN; i++) {
        embPersJsonArray.PushBack(perEnum.EmbeddedPers[i], allocator);
      }
      Pointer("/data/rsp/peripheralEnumeration/embPers").Set(response, embPersJsonArray);

      // hwpId
      Pointer("/data/rsp/peripheralEnumeration/hwpId").Set(response, perEnum.HWPID);

      // hwpIdVer
      Pointer("/data/rsp/peripheralEnumeration/hwpIdVer").Set(response, perEnum.HWPIDver);


      // flags - int value
      Pointer("/data/rsp/peripheralEnumeration/flags/value").Set(response, perEnum.Flags);

      // flags - parsed
      bool stdModeSupported = ((perEnum.Flags & 0b1) == 0b1) ? true : false;
      if (stdModeSupported) {
        Pointer("/data/rsp/peripheralEnumeration/flags/rfModeStd").Set(response, true);
        Pointer("/data/rsp/peripheralEnumeration/flags/rfModeLp").Set(response, false);
      }
      else {
        Pointer("/data/rsp/peripheralEnumeration/flags/rfModeStd").Set(response, false);
        Pointer("/data/rsp/peripheralEnumeration/flags/rfModeLp").Set(response, true);
      }

      // getting DPA version
      IIqrfDpaService::CoordinatorParameters coordParams = m_iIqrfDpaService->getCoordinatorParameters();
      uint16_t dpaVer = (coordParams.dpaVerMajor << 8) + coordParams.dpaVerMinor;

      TPerOSReadCfg_Response hwpConfig = deviceEnumerateResult.getHwpConfig();
      uns8* configuration = hwpConfig.Configuration;

      // STD+LP network is running, otherwise STD network.
      if (dpaVer >= 0x0400) {
        bool stdAndLpModeNetwork = ((perEnum.Flags & 0b100) == 0b100) ? true : false;
        if (stdAndLpModeNetwork) {
          Pointer("/data/rsp/peripheralEnumeration/flags/stdAndLpNetwork").Set(response, true);
        }
        else {
          Pointer("/data/rsp/peripheralEnumeration/flags/stdAndLpNetwork").Set(response, false);
        }
      }

      // userPers
      rapidjson::Value userPerJsonArray(kArrayType);
      for (int i = 0; i < USER_PER_LEN; i++) {
        userPerJsonArray.PushBack(perEnum.UserPer[i], allocator);
      }
      Pointer("/data/rsp/peripheralEnumeration/userPers").Set(response, userPerJsonArray);
    }
    
    uint32_t parseBaudRate(uint8_t baudRateId) {
      if ((baudRateId < 0) || (baudRateId >= BAUD_RATES_SIZE)) {
        THROW_EXC(std::out_of_range, "Baud rate ID out of range: " << PAR(baudRateId));
      }
      return BaudRates[baudRateId];
    }

    // parses RF band
    std::string parseRfBand(const uint8_t rfBand) {
      switch (rfBand) {
        case 0b00:
          return "868";
        case 0b01:
          return "916";
        case 0b10:
          return "433";
        default:
          THROW_EXC(std::out_of_range, "Unsupported coordinator RF band: " << PAR(rfBand));
        }
    }

    // sets Read HWP Configuration part of the response
    void setReadHwpConfigurationResponse(
      const std::string& messagingId,
      const IMessagingSplitterService::MsgType& msgType,
      DeviceEnumerateResult& deviceEnumerateResult,
      const ComIqmeshNetworkEnumerateDevice& comDeviceEnumerate,
      rapidjson::Document& response
    )
    {
      // getting DPA version
      IIqrfDpaService::CoordinatorParameters coordParams = m_iIqrfDpaService->getCoordinatorParameters();
      uint16_t dpaVer = (coordParams.dpaVerMajor << 8) + coordParams.dpaVerMinor;

      TPerOSReadCfg_Response hwpConfig = deviceEnumerateResult.getHwpConfig();
      uns8* configuration = hwpConfig.Configuration;

      if (dpaVer < 0x0303) {
        for (int i = 0; i < CONFIGURATION_LEN; i++) {
          configuration[i] = configuration[i] ^ 0x34;
        }
      }

      Document::AllocatorType& allocator = response.GetAllocator();

      // embedded periherals
      rapidjson::Value embPerBitsJsonArray(kArrayType);
      for (int i = 0; i < 4; i++) {
        embPerBitsJsonArray.PushBack(configuration[i], allocator);
      }
      Pointer("/data/rsp/trConfiguration/embPers/values").Set(response, embPerBitsJsonArray);

      // embedded peripherals bits - parsed
      // byte 0x01
      uint8_t byte01 = configuration[0x00];

      bool coordPresent = ((byte01 & 0b1) == 0b1) ? true : false;
      Pointer("/data/rsp/trConfiguration/embPers/coordinator").Set(response, coordPresent);

      bool nodePresent = ((byte01 & 0b10) == 0b10) ? true : false;
      Pointer("/data/rsp/trConfiguration/embPers/node").Set(response, nodePresent);

      bool osPresent = ((byte01 & 0b100) == 0b100) ? true : false;
      Pointer("/data/rsp/trConfiguration/embPers/os").Set(response, osPresent);

      bool eepromPresent = ((byte01 & 0b1000) == 0b1000) ? true : false;
      Pointer("/data/rsp/trConfiguration/embPers/eeprom").Set(response, eepromPresent);

      bool eeepromPresent = ((byte01 & 0b10000) == 0b10000) ? true : false;
      Pointer("/data/rsp/trConfiguration/embPers/eeeprom").Set(response, eeepromPresent);

      bool ramPresent = ((byte01 & 0b100000) == 0b100000) ? true : false;
      Pointer("/data/rsp/trConfiguration/embPers/ram").Set(response, ramPresent);

      bool ledrPresent = ((byte01 & 0b1000000) == 0b1000000) ? true : false;
      Pointer("/data/rsp/trConfiguration/embPers/ledr").Set(response, ledrPresent);

      bool ledgPresent = ((byte01 & 0b10000000) == 0b10000000) ? true : false;
      Pointer("/data/rsp/trConfiguration/embPers/ledg").Set(response, ledgPresent);


      // byte 0x02
      uint8_t byte02 = configuration[0x01];

      bool spiPresent = ((byte02 & 0b1) == 0b1) ? true : false;
      Pointer("/data/rsp/trConfiguration/embPers/spi").Set(response, spiPresent);

      bool ioPresent = ((byte02 & 0b10) == 0b10) ? true : false;
      Pointer("/data/rsp/trConfiguration/embPers/io").Set(response, ioPresent);

      bool thermometerPresent = ((byte02 & 0b100) == 0b100) ? true : false;
      Pointer("/data/rsp/trConfiguration/embPers/thermometer").Set(response, thermometerPresent);

      bool pwmPresent = ((byte02 & 0b1000) == 0b1000) ? true : false;
      Pointer("/data/rsp/trConfiguration/embPers/pwm").Set(response, pwmPresent);

      bool uartPresent = ((byte02 & 0b10000) == 0b10000) ? true : false;
      Pointer("/data/rsp/trConfiguration/embPers/uart").Set(response, uartPresent);

      bool frcPresent = ((byte02 & 0b100000) == 0b100000) ? true : false;
      Pointer("/data/rsp/trConfiguration/embPers/frc").Set(response, frcPresent);


      // byte 0x05
      uint8_t byte05 = configuration[0x04];

      bool customDpaHandler = ((byte05 & 0b1) == 0b1) ? true : false;
      Pointer("/data/rsp/trConfiguration/customDpaHandler").Set(response, customDpaHandler);


      // for DPA v4.00 downwards
      if (dpaVer < 0x0400) {
        bool nodeDpaInterface = ((byte05 & 0b00000010) == 0b00000010) ? true : false;
        Pointer("/data/rsp/trConfiguration/nodeDpaInterface").Set(response, nodeDpaInterface);
      }

      bool dpaAutoexec = ((byte05 & 0b100) == 0b100) ? true : false;
      Pointer("/data/rsp/trConfiguration/dpaAutoexec").Set(response, dpaAutoexec);

      bool routingOff = ((byte05 & 0b1000) == 0b1000) ? true : false;
      Pointer("/data/rsp/trConfiguration/routingOff").Set(response, routingOff);

      bool ioSetup = ((byte05 & 0b10000) == 0b10000) ? true : false;
      Pointer("/data/rsp/trConfiguration/ioSetup").Set(response, ioSetup);

      bool peerToPeer = ((byte05 & 0b100000) == 0b100000) ? true : false;
      Pointer("/data/rsp/trConfiguration/peerToPeer").Set(response, peerToPeer);


      // for DPA v3.03 onwards
      if (dpaVer >= 0x0303) {
        bool neverSleep = ((byte05 & 0b01000000) == 0b01000000) ? true : false;
        Pointer("/data/rsp/trConfiguration/neverSleep").Set(response, neverSleep);
      }

      // for DPA v4.00 onwards
      if (dpaVer >= 0x0400) {
        bool stdAndLpNetwork = ((byte05 & 0b10000000) == 0b10000000) ? true : false;
        Pointer("/data/rsp/trConfiguration/stdAndLpNetwork").Set(response, stdAndLpNetwork);
      }

      // bytes fields
      Pointer("/data/rsp/trConfiguration/rfChannelA").Set(response, configuration[0x10]);
      Pointer("/data/rsp/trConfiguration/rfChannelB").Set(response, configuration[0x11]);

      if (dpaVer < 0x0400) {
        Pointer("/data/rsp/trConfiguration/rfSubChannelA").Set(response, configuration[0x05]);
        Pointer("/data/rsp/trConfiguration/rfSubChannelB").Set(response, configuration[0x06]);
      }

      Pointer("/data/rsp/trConfiguration/txPower").Set(response, configuration[0x07]);
      Pointer("/data/rsp/trConfiguration/rxFilter").Set(response, configuration[0x08]);
      Pointer("/data/rsp/trConfiguration/lpRxTimeout").Set(response, configuration[0x09]);
      Pointer("/data/rsp/trConfiguration/rfAltDsmChannel").Set(response, configuration[0x0B]);

      try {
        uint32_t baudRate = parseBaudRate(configuration[0x0A]);
        Pointer("/data/rsp/trConfiguration/uartBaudrate").Set(response, baudRate);
      }
      catch (std::exception& ex) {
        TRC_WARNING("Unknown baud rate constant: " << PAR(configuration[0x0A]));
        Pointer("/data/rsp/trConfiguration/uartBaudrate").Set(response, 0);
      }

      // RFPGM byte
      uint8_t rfpgm = hwpConfig.RFPGM;

      bool rfPgmDualChannel = ((rfpgm & 0b00000011) == 0b00000011) ? true : false;
      Pointer("/data/rsp/trConfiguration/rfPgmDualChannel").Set(response, rfPgmDualChannel);

      bool rfPgmLpMode = ((rfpgm & 0b00000100) == 0b00000100) ? true : false;
      Pointer("/data/rsp/trConfiguration/rfPgmLpMode").Set(response, rfPgmLpMode);

      bool rfPgmIncorrectUpload = ((rfpgm & 0b00001000) == 0b00001000) ? true : false;
      Pointer("/data/rsp/trConfiguration/rfPgmIncorrectUpload").Set(response, rfPgmIncorrectUpload);

      bool enableAfterReset = ((rfpgm & 0b00010000) == 0b00010000) ? true : false;
      Pointer("/data/rsp/trConfiguration/rfPgmEnableAfterReset").Set(response, enableAfterReset);

      bool rfPgmTerminateAfter1Min = ((rfpgm & 0b01000000) == 0b01000000) ? true : false;
      Pointer("/data/rsp/trConfiguration/rfPgmTerminateAfter1Min").Set(response, rfPgmTerminateAfter1Min);

      bool rfPgmTerminateMcuPin = ((rfpgm & 0b10000000) == 0b10000000) ? true : false;
      Pointer("/data/rsp/trConfiguration/rfPgmTerminateMcuPin").Set(response, rfPgmTerminateMcuPin);


      // RF band - undocumented byte
      std::string rfBand;
      try {
        rfBand = parseRfBand(hwpConfig.Undocumented[0] & 0x03);
      }
      catch (std::exception& ex) {
        rfBand = "";
      }
      Pointer("/data/rsp/trConfiguration/rfBand").Set(response, rfBand);
    }

    // sets Info for more peripherals part of the response
    void setInfoForMorePeripheralsResponse(
      const std::string& messagingId,
      const IMessagingSplitterService::MsgType& msgType,
      DeviceEnumerateResult& deviceEnumerateResult,
      const ComIqmeshNetworkEnumerateDevice& comDeviceEnumerate,
      rapidjson::Document& response
    ) 
    {
      // per enum object
      std::vector<TPeripheralInfoAnswer> moreInfoPers = deviceEnumerateResult.getMorePersInfo();

      // array of objects
      Document::AllocatorType& allocator = response.GetAllocator();
      rapidjson::Value perInfoJsonArray(kArrayType);

      for (int i = 0; i < PERIPHERALS_NUM; i++) {
        rapidjson::Value perInfoObj(kObjectType);
        perInfoObj.AddMember(
          "perTe",
          moreInfoPers[i].PerTE,
          allocator
        );

        perInfoObj.AddMember(
          "perT",
          moreInfoPers[i].PerT,
          allocator
        );

        perInfoObj.AddMember(
          "par1",
          moreInfoPers[i].Par1,
          allocator
        );

        perInfoObj.AddMember(
          "par2",
          moreInfoPers[i].Par2,
          allocator
        );

        perInfoJsonArray.PushBack(perInfoObj, allocator);
      }
      Pointer("/data/rsp/morePeripheralsInfo").Set(response, perInfoJsonArray);
    }


    // field "valid" is set to "true" for now
    // ma se poslat vsechno, co bylo v predchozich DPA responses dostupne anebo
    // pokud je neco nedostupneho, tak hned SERVICE_ERROR_INFO_MISSING?
    void setValidationAndUpdatesResponse(
      const std::string& messagingId,
      const IMessagingSplitterService::MsgType& msgType,
      DeviceEnumerateResult& deviceEnumerateResult,
      const ComIqmeshNetworkEnumerateDevice& comDeviceEnumerate,
      const OsReadObject osReadObject,
      rapidjson::Document& response
    ) 
    {
      // valid - for now always true
      Pointer("/data/rsp/validationAndUpdates/validation/valid").Set(response, true);
      
      // OS read
      if (deviceEnumerateResult.getOsReadError().getType() == DeviceEnumerateError::Type::NoError) 
      {
        Pointer("/data/rsp/validationAndUpdates/validation/osVer").Set(response, osReadObject.osVersion);
        Pointer("/data/rsp/validationAndUpdates/validation/osBuild").Set(response, osReadObject.osBuild);
      }

      if (
        deviceEnumerateResult.getPerEnumError().getType() == DeviceEnumerateError::Type::NoError
        ) 
      {
        // DPA ver
        TEnumPeripheralsAnswer perEnum = deviceEnumerateResult.getPerEnum();  

        // dpa version - string
        std::string dpaVerStr = std::to_string((perEnum.DpaVersion >> 8) & 0xFF)
          + "." + encodeHexaNum((uint8_t)(perEnum.DpaVersion & 0xFF));
        Pointer("/data/rsp/validationAndUpdates/validation/dpaVer").Set(response, dpaVerStr);
      }

      if (deviceEnumerateResult.getReadHwpConfigError().getType() == DeviceEnumerateError::Type::NoError)
      {
        // configuration - bytes need to be XORED
        TPerOSReadCfg_Response hwpConfig = deviceEnumerateResult.getHwpConfig();

        // txPower
        Pointer("/data/rsp/validationAndUpdates/validation/txPower").Set(response, hwpConfig.Configuration[0x07] ^ 0x34);

        // rxFilter
        Pointer("/data/rsp/validationAndUpdates/validation/rxFilter").Set(response, hwpConfig.Configuration[0x08] ^ 0x34);
      }
    }
    
    // creates response on the basis of write result
    Document createResponse(
      const std::string& messagingId,
      const IMessagingSplitterService::MsgType& msgType,
      DeviceEnumerateResult& deviceEnumResult,
      ComIqmeshNetworkEnumerateDevice comEnumerateDevice
    )
    {
      Document response;

      // set common parameters
      Pointer("/mType").Set(response, msgType.m_type);
      Pointer("/data/msgId").Set(response, messagingId);
 
      Pointer("/data/rsp/deviceAddr").Set(response, deviceEnumResult.getDeviceAddr());

      // manufacturer and product - if OS Read was correct - to get HWP ID which is needed
      if (
        deviceEnumResult.getOsReadError().getType() == DeviceEnumerateError::Type::NoError
        ) {
        Pointer("/data/rsp/manufacturer").Set(response, deviceEnumResult.getManufacturer());
        Pointer("/data/rsp/product").Set(response, deviceEnumResult.getProduct());

        // Enum was correct - to get HWP ID ver
        if (
        deviceEnumResult.getPerEnumError().getType() == DeviceEnumerateError::Type::NoError
        ) {
          // standards - array of strings
          rapidjson::Value standardsJsonArray(kArrayType);
          Document::AllocatorType &allocator = response.GetAllocator();
          for (std::string standard : deviceEnumResult.getStandards())
          {
            rapidjson::Value standardJsonString;
            standardJsonString.SetString(standard.c_str(), standard.length(), allocator);
            standardsJsonArray.PushBack(standardJsonString, allocator);
          }
          Pointer("/data/rsp/standards").Set(response, standardsJsonArray);
        }
      }

      bool isError = false;
      if (
        deviceEnumResult.getDiscoveredError().getType() == DeviceEnumerateError::Type::NoError
        && deviceEnumResult.getVrnError().getType() == DeviceEnumerateError::Type::NoError
        && deviceEnumResult.getZoneError().getType() == DeviceEnumerateError::Type::NoError
        && deviceEnumResult.getParentError().getType() == DeviceEnumerateError::Type::NoError
        )
      {
        setDiscoveryDataResponse(messagingId, msgType, deviceEnumResult, comEnumerateDevice, response);
      }
      else {
        isError = true;
      }

      // OS Read data
      if (
        deviceEnumResult.getOsReadError().getType() == DeviceEnumerateError::Type::NoError
        )
      {
        const std::vector<uns8> osInfo = deviceEnumResult.getOsRead();
        OsReadObject osReadObject = parseOsReadResponse(osInfo);
        setOsReadResponse(messagingId, msgType, deviceEnumResult, comEnumerateDevice, osReadObject, response);
      }
      else {
        isError = true;
      }
      
      if (
        deviceEnumResult.getPerEnumError().getType() == DeviceEnumerateError::Type::NoError
        )
      {
        setPeripheralEnumerationResponse(messagingId, msgType, deviceEnumResult, comEnumerateDevice, response);
      }
      else {
        isError = true;
      }

      if (
        deviceEnumResult.getPerEnumError().getType() == DeviceEnumerateError::Type::NoError
        )
      {
        setReadHwpConfigurationResponse(messagingId, msgType, deviceEnumResult, comEnumerateDevice, response);
      }
      else {
        isError = true;
      }

      // result of more peripherals info according to request
      if (m_morePeripheralsInfo) {
        if (
          deviceEnumResult.getPerEnumError().getType() == DeviceEnumerateError::Type::NoError
          )
        {
          setInfoForMorePeripheralsResponse(messagingId, msgType, deviceEnumResult, comEnumerateDevice, response);
        }
        else {
          isError = true;
        }
      }

      // removed also from JSON api - will be implemented next year
      //setValidationAndUpdatesResponse(messagingId, msgType, deviceEnumResult, comEnumerateDevice, osReadObject, response);

      // set raw fields, if verbose mode is active
      if (comEnumerateDevice.getVerbose()) {
        setVerboseData(response, deviceEnumResult);
      }
    
      if (isError) {
        // status - ERROR_INFO_MISSING
        setResponseStatus(response, SERVICE_ERROR_INFO_MISSING, "info missing");
      }
      else {
        setResponseStatus(response, SERVICE_ERROR_NOERROR, "ok");
      }

      return response;
    }

    uint8_t parseAndCheckRepeat(const int repeat) {
      if (repeat < 0) {
        TRC_WARNING("repeat cannot be less than 0. It will be set to 0.");
        return 0;
      }

      if (repeat > 0xFF) {
        TRC_WARNING("repeat exceeds maximum. It will be trimmed to maximum of: " << PAR(REPEAT_MAX));
        return REPEAT_MAX;
      }

      return repeat;
    }

    uint16_t parseAndCheckDeviceAddr(int nadr) {
      if ((nadr < 0) || (nadr > 0xEF)) {
        THROW_EXC(
          std::out_of_range, "Device address outside of valid range. " << NAME_PAR_HEX("Address", nadr)
        );
      }
      return nadr;
    }

    // gets manufacturer and product
    void getManufacturerAndProduct(DeviceEnumerateResult& deviceEnumerateResult)
    {
      // manufacturer name and product name
      const IJsCacheService::Manufacturer* manufacturer = m_iJsCacheService->getManufacturer(deviceEnumerateResult.getEnumeratedNodeHwpId());
      if (manufacturer != nullptr) {
        deviceEnumerateResult.setManufacturer(manufacturer->m_name);
      }

      const IJsCacheService::Product* product = m_iJsCacheService->getProduct(deviceEnumerateResult.getEnumeratedNodeHwpId());
      if (product != nullptr) {
        deviceEnumerateResult.setProduct(product->m_name);
      }
    }

    void handleMsg(
      const std::string& messagingId,
      const IMessagingSplitterService::MsgType& msgType,
      rapidjson::Document doc
    )
    {
      TRC_FUNCTION_ENTER(
        PAR(messagingId) <<
        NAME_PAR(mType, msgType.m_type) <<
        NAME_PAR(major, msgType.m_major) <<
        NAME_PAR(minor, msgType.m_minor) <<
        NAME_PAR(micro, msgType.m_micro)
      );

      // unsupported type of request
      if (msgType.m_type != m_mTypeName_iqmeshNetworkEnumerateDevice) {
        THROW_EXC(std::logic_error, "Unsupported message type: " << PAR(msgType.m_type));
      }

      // creating representation object
      ComIqmeshNetworkEnumerateDevice comEnumerateDevice(doc);

      // service input parameters
      uint16_t deviceAddr;

      // parsing and checking service parameters
      try {
        m_repeat = parseAndCheckRepeat(comEnumerateDevice.getRepeat());
       
        if (!comEnumerateDevice.isSetDeviceAddr()) {
          THROW_EXC(std::logic_error, "deviceAddr not set");
        }
        deviceAddr = parseAndCheckDeviceAddr(comEnumerateDevice.getDeviceAddr());

        m_morePeripheralsInfo = comEnumerateDevice.getMorePeripheralsInfo();

        m_returnVerbose = comEnumerateDevice.getVerbose();
      }
      // parsing and checking service parameters failed 
      catch (std::exception& ex) {
        Document failResponse = getCheckParamsFailedResponse(comEnumerateDevice.getMsgId(), msgType, ex.what());
        m_iMessagingSplitterService->sendMessage(messagingId, std::move(failResponse));

        TRC_FUNCTION_LEAVE("");
        return;
      }
      
      // result
      DeviceEnumerateResult deviceEnumerateResult;
      deviceEnumerateResult.setDeviceAddr(deviceAddr);

      // check, if the address is bonded
      if (deviceAddr != COORDINATOR_ADDRESS) {
        checkBond(deviceEnumerateResult, deviceAddr);
      }

      if (deviceEnumerateResult.getBondedError().getType() != DeviceEnumerateError::Type::NoError) 
      {
        Document nodeNotBondedResponse = getNodeNotBondedResponse(
          comEnumerateDevice.getMsgId(),
          msgType,
          deviceEnumerateResult,
          comEnumerateDevice,
          deviceEnumerateResult.getBondedError().getMessage()
        );
        m_iMessagingSplitterService->sendMessage(messagingId, std::move(nodeNotBondedResponse));

        TRC_FUNCTION_LEAVE("");
        return;
      }

      // try to establish exclusive access
      try {
        m_exclusiveAccess = m_iIqrfDpaService->getExclusiveAccess();
      }
      catch (std::exception &e) {
        const char* errorStr = e.what();
        TRC_WARNING("Error while establishing exclusive DPA access: " << PAR(errorStr));
       
        Document failResponse = getExclusiveAccessFailedResponse(comEnumerateDevice.getMsgId(), msgType, errorStr);
        m_iMessagingSplitterService->sendMessage(messagingId, std::move(failResponse));

        TRC_FUNCTION_LEAVE("");
        return;
      }

      // discovery data
      discoveryData(deviceEnumerateResult);
      
      // OS read info
      osRead(deviceEnumerateResult);
      
      // AFTER OS READ - obtains hwpId, which in turn is needed to get manufacturer and product
      getManufacturerAndProduct(deviceEnumerateResult);

      // peripheral enumeration
      peripheralEnumeration(deviceEnumerateResult);

      //uint8_t osVersion = ...Result.getOsRead()[4];
      //std::string osVersionStr = std::to_string((osVersion >> 4) & 0xFF) + "." + std::to_string(osVersion & 0x0F);
      std::string osBuildStr;
      {
        std::ostringstream os;
        os.fill('0');
        os << std::hex << std::uppercase << std::setw(4) << (int)deviceEnumerateResult.getOsBuild();
        osBuildStr = os.str();
      }

      // fill standards
      const IJsCacheService::Package* package = m_iJsCacheService->getPackage(
        deviceEnumerateResult.getEnumeratedNodeHwpId(),
        deviceEnumerateResult.getEnumeratedNodeHwpIdVer(),
        osBuildStr, //TODO m_iIqrfDpaService->getCoordinatorParameters().osBuild ?
        m_iIqrfDpaService->getCoordinatorParameters().dpaVerWordAsStr
      );
      if (package != nullptr) {
        std::list<std::string> standards;
        for (const IJsCacheService::StdDriver* driver : package->m_stdDriverVect) {
          standards.push_back(driver->getName());
        }
        deviceEnumerateResult.setStandards(standards);
      }
      else {
        TRC_INFORMATION("Package not found");
      }

      // read Hwp configuration
      readHwpConfiguration(deviceEnumerateResult);

      // get info for more peripherals
      if (m_morePeripheralsInfo) {
        getInfoForMorePeripherals(deviceEnumerateResult);
      }

      // release exclusive access
      m_exclusiveAccess.reset();

      // create and send FINAL response
      Document responseDoc = createResponse(comEnumerateDevice.getMsgId(), msgType, deviceEnumerateResult, comEnumerateDevice);
      m_iMessagingSplitterService->sendMessage(messagingId, std::move(responseDoc));
    
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

  void EnumerateDeviceService::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
  }

}