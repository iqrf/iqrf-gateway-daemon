#define IEnumerateService_EXPORTS

#include "EnumerateService.h"
#include "Trace.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/pointer.h"
//#include "HexStringCoversion.h"

#include "iqrf__EnumerateService.hxx"

#include <list>
#include <memory>
#include <math.h>
#include <bitset>

TRC_INIT_MODULE(iqrf::EnumerateService);


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
  class EnumerateService::Imp {
  private:
    // parent object
    EnumerateService& m_parent;

    // message type: IQMESH Network Enumerate Device
    // for temporal reasons
    const std::string m_mTypeName_iqmeshNetworkEnumerateDevice = "iqmeshNetwork_EnumerateDevice";
    //IMessagingSplitterService::MsgType* m_msgType_mngIqmeshWriteConfig;

    iqrf::IJsRenderService* m_iJsRenderService = nullptr;
    IIqrfDpaService* m_iIqrfDpaService = nullptr;
    std::unique_ptr<IIqrfDpaService::ExclusiveAccess> m_exclusiveAccess;

    // number of repeats
    uint8_t m_repeat;

    // if is set Verbose mode
    bool m_returnVerbose = false;

    bool m_morePeripheralsInfo = false;


  public:
    Imp(EnumerateService& parent) : m_parent(parent)
    {
    }

    ~Imp()
    {
    }

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

    std::unique_ptr<IDpaTransactionResult2> dpaRepeat(std::unique_ptr<IIqrfDpaService::ExclusiveAccess> & exclusiveAccess, DpaMessage & request, int repeat)
    {
      TRC_FUNCTION_ENTER("");

      std::unique_ptr<IDpaTransactionResult2> transResult;

      for (int rep = 0; rep <= repeat; rep++) {
        std::shared_ptr<IDpaTransaction2> transaction;
        try {
          transaction = exclusiveAccess->executeDpaTransaction(request);
          transResult = transaction->get();

          TRC_DEBUG("Result from read transaction as string:" << PAR(transResult->getErrorString()));
          IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

          if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
            break;
          }
          else if (errorCode < 0) {
            TRC_WARNING("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));
          }
          else {
            TRC_WARNING("DPA error. " << NAME_PAR_HEX("Error code", errorCode));
          }
        }
        catch (std::exception& e) {
          CATCH_EXC_TRC_WAR(std::logic_error, e, "DPA transaction error : " << e.what());
          if (rep == m_repeat) {
            THROW_EXC_TRC_WAR(std::logic_error, "DPA transaction error : " << e.what())
          }
        }
      }

      TRC_FUNCTION_LEAVE("");
      return transResult;
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

      std::unique_ptr<IDpaTransactionResult2> transResult;

      try {

        // make DPA transaction
        transResult = dpaRepeat(m_exclusiveAccess, osReadRequest, m_repeat);

        TRC_DEBUG("Result from OS read transaction as string:" << PAR(transResult->getErrorString()));
        IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

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

        }
        else if (errorCode < 0) {
          TRC_WARNING("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));
          DeviceEnumerateError error(DeviceEnumerateError::Type::OsRead, "Transaction error.");
          deviceEnumerateResult.setOsReadError(error);
        }
        else {
          TRC_WARNING("DPA error. " << NAME_PAR_HEX("Error code", errorCode));
          DeviceEnumerateError error(DeviceEnumerateError::Type::OsRead, "Dpa error.");
          deviceEnumerateResult.setOsReadError(error);
        }

      }
      catch (std::exception& e) {
        CATCH_EXC_TRC_WAR(std::logic_error, e, "Cannot read OS: " << e.what());
        DeviceEnumerateError error(DeviceEnumerateError::Type::OsRead, e.what());
        deviceEnumerateResult.setOsReadError(error);
      }

      TRC_FUNCTION_LEAVE("");
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

      std::unique_ptr<IDpaTransactionResult2> transResult;

      try {

        // make DPA transaction
        transResult = dpaRepeat(m_exclusiveAccess, perEnumRequest, m_repeat);

        TRC_DEBUG("Result from peripheral enumeration transaction as string:" << PAR(transResult->getErrorString()));
        IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

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

        }
        else if (errorCode < 0) {
          TRC_WARNING("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));
          DeviceEnumerateError error(DeviceEnumerateError::Type::PerEnum, "Transaction error.");
          deviceEnumerateResult.setPerEnumError(error);
        }
        else {
          TRC_WARNING("DPA error. " << NAME_PAR_HEX("Error code", errorCode));
          DeviceEnumerateError error(DeviceEnumerateError::Type::PerEnum, "Dpa error.");
          deviceEnumerateResult.setOsReadError(error);
        }

      }
      catch (std::exception& e) {
        CATCH_EXC_TRC_WAR(std::logic_error, e, "Cannot Enum: " << e.what());
        DeviceEnumerateError error(DeviceEnumerateError::Type::PerEnum, "Dpa error.");
        deviceEnumerateResult.setPerEnumError(error);
      }

      TRC_FUNCTION_LEAVE("");
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

      std::unique_ptr<IDpaTransactionResult2> transResult;

      try {

        // make DPA transaction
        transResult = dpaRepeat(m_exclusiveAccess, readHwpRequest, m_repeat);

        TRC_DEBUG("Result from read HWP transaction as string:" << PAR(transResult->getErrorString()));
        IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

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


        }
        else if (errorCode < 0) {
          TRC_WARNING("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));
          DeviceEnumerateError error(DeviceEnumerateError::Type::ReadHwp, "Transaction error.");
          deviceEnumerateResult.setReadHwpConfigError(error);
        }
        else {
          TRC_WARNING("DPA error. " << NAME_PAR_HEX("Error code", errorCode));
          DeviceEnumerateError error(DeviceEnumerateError::Type::ReadHwp, "Dpa error.");
          deviceEnumerateResult.setReadHwpConfigError(error);
        }

      }
      catch (std::exception& e) {
        CATCH_EXC_TRC_WAR(std::logic_error, e, "Cannot read HWP cfg: " << e.what());
        DeviceEnumerateError error(DeviceEnumerateError::Type::ReadHwp, e.what());
        deviceEnumerateResult.setReadHwpConfigError(error);
      }

      TRC_FUNCTION_LEAVE("");
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

      std::unique_ptr<IDpaTransactionResult2> transResult;

      try {

        // make DPA transaction
        transResult = dpaRepeat(m_exclusiveAccess, morePersInfoRequest, m_repeat);

        TRC_DEBUG("Result from get info for more peripherals transaction as string:" << PAR(transResult->getErrorString()));
        IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

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
        }
        else if (errorCode < 0) {
          TRC_WARNING("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));
          DeviceEnumerateError error(DeviceEnumerateError::Type::MorePersInfo, "Transaction error.");
          deviceEnumerateResult.setMorePersInfoError(error);
        }
        else {
          TRC_WARNING("DPA error. " << NAME_PAR_HEX("Error code", errorCode));
          DeviceEnumerateError error(DeviceEnumerateError::Type::MorePersInfo, "Transaction error.");
          deviceEnumerateResult.setMorePersInfoError(error);
        }

      }
      catch (std::exception& e) {
        CATCH_EXC_TRC_WAR(std::logic_error, e, "Cannot read more peripherals: " << e.what());
        DeviceEnumerateError error(DeviceEnumerateError::Type::MorePersInfo, e.what());
        deviceEnumerateResult.setReadHwpConfigError(error);
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

      std::unique_ptr<IDpaTransactionResult2> transResult;

      try {

        // make DPA transaction
        transResult = dpaRepeat(m_exclusiveAccess, bondedNodesRequest, m_repeat);

        TRC_DEBUG("Result from get bonded nodes transaction as string:" << PAR(transResult->getErrorString()));
        IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

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
        }
        else if (errorCode < 0) {
          TRC_WARNING("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));
          DeviceEnumerateError error(DeviceEnumerateError::Type::NotBonded, "Transaction error.");
          deviceEnumerateResult.setBondedError(error);
        }
        else {
          TRC_WARNING("DPA error. " << NAME_PAR_HEX("Error code", errorCode));
          DeviceEnumerateError error(DeviceEnumerateError::Type::NotBonded, "Dpa error.");
          deviceEnumerateResult.setBondedError(error);
        }

      }
      catch (std::exception& e) {
        CATCH_EXC_TRC_WAR(std::logic_error, e, "Cannot read bonded nodes: " << e.what());
        DeviceEnumerateError error(DeviceEnumerateError::Type::NotBonded, e.what());
        deviceEnumerateResult.setReadHwpConfigError(error);
      }

      TRC_FUNCTION_LEAVE("");

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

    NodeEnumeration getEnumerateResult(uint16_t deviceAddr)
    {
      DeviceEnumerateResult deviceEnumerateResult;
      try {
        return getEnumerateResult(deviceAddr, deviceEnumerateResult);
      }
      catch (std::exception &e) {
        CATCH_EXC_TRC_WAR(std::exception, e, "Error while establishing exclusive DPA access ");
        return NodeEnumeration();
      }
    }

    NodeEnumeration getEnumerateResult(uint16_t deviceAddr, DeviceEnumerateResult & deviceEnumerateResult)
    {
      TRC_FUNCTION_ENTER(deviceAddr);

      NodeEnumeration nde;

      deviceEnumerateResult.setDeviceAddr(deviceAddr);

      try {
        // try to establish exclusive access
        m_exclusiveAccess = m_iIqrfDpaService->getExclusiveAccess();

        if (m_exclusiveAccess) {

          // check, if the address is bonded
          if (deviceAddr != COORDINATOR_ADDRESS) {
            checkBond(deviceEnumerateResult, deviceAddr);
          }

          if (deviceEnumerateResult.getBondedError().getType() == DeviceEnumerateError::Type::NoError) {

            // discovery data
            discoveryData(deviceEnumerateResult);

            // OS read info
            osRead(deviceEnumerateResult);

            // AFTER OS READ - obtains hwpId, which in turn is needed to get manufacturer and product
            //getManufacturerAndProduct(deviceEnumerateResult);

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
            //const IJsCacheService::Package* package = m_iJsCacheService->getPackage(
            //  deviceEnumerateResult.getEnumeratedNodeHwpId(),
            //  deviceEnumerateResult.getEnumeratedNodeHwpIdVer(),
            //  osBuildStr, //TODO m_iIqrfDpaService->getCoordinatorParameters().osBuild ?
            //  m_iIqrfDpaService->getCoordinatorParameters().dpaVerWordAsStr
            //);
            //if (package != nullptr) {
            //  std::list<std::string> standards;
            //  for (const IJsCacheService::StdDriver* driver : package->m_stdDriverVect) {
            //    standards.push_back(driver->getName());
            //  }
            //  deviceEnumerateResult.setStandards(standards);
            //}
            //else {
            //  TRC_INFORMATION("Package not found");
            //}

            // read Hwp configuration
            readHwpConfiguration(deviceEnumerateResult);

            // get info for more peripherals
            if (m_morePeripheralsInfo) {
              getInfoForMorePeripherals(deviceEnumerateResult);
            }

            // release exclusive access
            m_exclusiveAccess.reset();

            fillEnumeration(deviceEnumerateResult, nde);

          }
          else {
            TRC_WARNING("Not Bonded");
          }
        }
        else {
          TRC_WARNING("Cannot get exclusive DPA access");
        }
      }
      catch (std::exception & e) {
        CATCH_EXC_TRC_WAR(std::exception, e, "Cannot enumerate: ");
        if (m_exclusiveAccess) m_exclusiveAccess.reset();
      }

      TRC_FUNCTION_LEAVE("");
      return nde;
    }

    // creates response on the basis of write result
    void fillEnumeration(DeviceEnumerateResult& deviceEnumResult, NodeEnumeration& nde)
    {
      Document response;
      bool isError = false;

      nde.m_deviceAddr = deviceEnumResult.getDeviceAddr();
      nde.m_manufacturer = deviceEnumResult.getManufacturer();
      nde.m_product = deviceEnumResult.getProduct();
      nde.m_standards = deviceEnumResult.getStandards();
      nde.m_discovered = deviceEnumResult.isDiscovered();
      nde.m_vrn = deviceEnumResult.getVrn();
      nde.m_zone = deviceEnumResult.getZone();
      nde.m_parent = deviceEnumResult.getParent();

      OsReadObject osReadObject = parseOsReadResponse(deviceEnumResult.getOsRead());

      nde.m_mid = osReadObject.mid;
      nde.m_osVersion = osReadObject.osVersion;
      nde.m_trMcuTypeVal = osReadObject.trMcuType.value;
      nde.m_trType = osReadObject.trMcuType.trType;
      nde.m_fccCertified = osReadObject.trMcuType.fccCertified;
      nde.m_mcuType = osReadObject.trMcuType.mcuType;
      nde.m_osBuild = osReadObject.osBuild;
      nde.m_rssi = osReadObject.rssi;
      nde.m_supplyVoltage = osReadObject.supplyVoltage;
      nde.m_flagsVal = osReadObject.flags.value;
      nde.m_insufficientOsBuild = osReadObject.flags.insufficientOsBuild;
      nde.m_interface = osReadObject.flags.interface;
      nde.m_dpaHandlerDetected = osReadObject.flags.dpaHandlerDetected;
      nde.m_dpaHandlerNotDetectedButEnabled = osReadObject.flags.dpaHandlerNotDetectedButEnabled;
      nde.m_noInterfaceSupported = osReadObject.flags.noInterfaceSupported;
      nde.m_slotLimitsVal = osReadObject.slotLimits.value;
      nde.m_shortestTimeslot = osReadObject.slotLimits.shortestTimeslot;
      nde.m_longestTimeslot = osReadObject.slotLimits.longestTimeslot;

      // per enum object
      TEnumPeripheralsAnswer perEnum = deviceEnumResult.getPerEnum();

      // dpa version - string
      //std::string dpaVerStr = std::to_string((perEnum.DpaVersion >> 8) & 0xFF)
      //  + "." + encodeHexaNum((uint8_t)(perEnum.DpaVersion & 0xFF));

      nde.m_DpaVersion = perEnum.DpaVersion;
      nde.m_UserPerNr = perEnum.UserPerNr;

      for (int i = 0; i < EMBEDDED_PERS_LEN; i++) {
        nde.m_EmbeddedPers.push_back(perEnum.EmbeddedPers[i]);
      }

      nde.m_HWPID = perEnum.HWPID;
      nde.m_HWPIDver = perEnum.HWPIDver;
      nde.m_rfModeStd = ((perEnum.Flags & 0b1) == 0b1) ? true : false;
      nde.m_rfModeLp = !nde.m_rfModeStd;

      // getting DPA version
      IIqrfDpaService::CoordinatorParameters coordParams = m_iIqrfDpaService->getCoordinatorParameters();
      uint16_t dpaVer = (coordParams.dpaVerMajor << 8) + coordParams.dpaVerMinor;
      // STD+LP network is running, otherwise STD network.
      if (dpaVer >= 0x0400) {
        nde.m_stdAndLpNetwork = ((perEnum.Flags & 0b100) == 0b100) ? true : false;
      }
      else {
        nde.m_stdAndLpNetwork = false;
      }

      TPerOSReadCfg_Response hwpConfig = deviceEnumResult.getHwpConfig();
      uns8* configuration = hwpConfig.Configuration;

      // userPers
      for (int i = 0; i < USER_PER_LEN; i++) {
        nde.m_UserPer.push_back(perEnum.UserPer[i]);
      }

      ////////////////////////////// TODO really interesting here?
      //if (
      //  deviceEnumResult.getPerEnumError().getType() == DeviceEnumerateError::Type::NoError
      //  )
      //{
      //  setReadHwpConfigurationResponse(messagingId, msgType, deviceEnumResult, comEnumerateDevice, response);
      //}
      //else {
      //  isError = true;
      //}

      //// result of more peripherals info according to request
      //if (m_morePeripheralsInfo) {
      //  if (
      //    deviceEnumResult.getPerEnumError().getType() == DeviceEnumerateError::Type::NoError
      //    )
      //  {
      //    setInfoForMorePeripheralsResponse(messagingId, msgType, deviceEnumResult, comEnumerateDevice, response);
      //  }
      //  else {
      //    isError = true;
      //  }
      //}

      // removed also from JSON api - will be implemented next year
      //setValidationAndUpdatesResponse(messagingId, msgType, deviceEnumResult, comEnumerateDevice, osReadObject, response);

    }

  public:
    void activate(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************************" << std::endl <<
        "EnumerateService instance activate" << std::endl <<
        "******************************************"
      );

      // for the sake of register function parameters 
      std::vector<std::string> supportedMsgTypes =
      {
        m_mTypeName_iqmeshNetworkEnumerateDevice
      };

      TRC_FUNCTION_LEAVE("");
    }

    void deactivate()
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "**************************************" << std::endl <<
        "EnumerateService instance deactivate" << std::endl <<
        "**************************************"
      );

      // for the sake of unregister function parameters 
      std::vector<std::string> supportedMsgTypes =
      {
        m_mTypeName_iqmeshNetworkEnumerateDevice
      };

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

    void attachInterface(IJsRenderService* iface)
    {
      m_iJsRenderService = iface;
    }

    void detachInterface(IJsRenderService* iface)
    {
      if (m_iJsRenderService == iface) {
        m_iJsRenderService = nullptr;
      }
    }

  };

  EnumerateService::EnumerateService()
  {
    m_imp = shape_new Imp(*this);
  }

  EnumerateService::~EnumerateService()
  {
    delete m_imp;
  }

  IEnumerateService::NodeEnumeration EnumerateService::getEnumerateResult(uint16_t deviceAddr)
  {
    return m_imp->getEnumerateResult(deviceAddr);
  }

  void EnumerateService::attachInterface(iqrf::IIqrfDpaService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void EnumerateService::detachInterface(iqrf::IIqrfDpaService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void EnumerateService::attachInterface(iqrf::IJsRenderService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void EnumerateService::detachInterface(iqrf::IJsRenderService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void EnumerateService::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void EnumerateService::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }


  void EnumerateService::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void EnumerateService::deactivate()
  {
    m_imp->deactivate();
  }

  void EnumerateService::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
  }

}