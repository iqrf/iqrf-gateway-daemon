#define IEnumerateDeviceService_EXPORTS

#include "EnumerateDeviceService.h"
#include "Trace.h"
#include "ComIqmeshNetworkEnumerateDevice.h"
#include "ObjectFactory.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"

#include "iqrf__EnumerateDeviceService.hxx"

#include <list>
#include <memory>
#include <math.h>
#include <bitset>

TRC_INIT_MODULE(iqrf::EnumerateDeviceService);


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


  // service general fail code - may and probably will be changed later in the future
  static const int SERVICE_ERROR = 1000;

  static const int SERVICE_ERROR_NODE_NOT_BONDED = SERVICE_ERROR + 1;
  static const int SERVICE_ERROR_INFO_MISSING = SERVICE_ERROR + 2;
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
    TPerOSRead_Response m_osRead;
    TEnumPeripheralsAnswer m_perEnum;
    TPerOSReadCfg_Response m_hwpConfig;
    std::vector<TPeripheralInfoAnswer> m_morePersInfo;

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


    TPerOSRead_Response getOsRead() {
      return m_osRead;
    }

    void setOsRead(TPerOSRead_Response osRead) {
      m_osRead = osRead;
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

    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    IIqrfDpaService* m_iIqrfDpaService = nullptr;

    // number of repeats
    uint8_t m_repeat;

    // if is set Verbose mode
    bool m_returnVerbose = false;


  public:
    Imp(EnumerateDeviceService& parent) : m_parent(parent)
    {
      /*
      m_msgType_mngIqmeshWriteConfig
        = new IMessagingSplitterService::MsgType(m_mTypeName_mngIqmeshWriteConfig, 1, 0, 0);
        */
    }

    ~Imp()
    {
    }


  private:
    
    // returns discovery data read from the coordinator private external EEPROM storage
    std::basic_string<uint8_t> discoveryData(
      DeviceEnumerateResult& deviceEnumerateResult, 
      uint16_t address
    ) {
      TRC_FUNCTION_ENTER("");

      DpaMessage discoveryDataRequest;
      DpaMessage::DpaPacket_t discoveryDataPacket;
      discoveryDataPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      discoveryDataPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
      discoveryDataPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_DISCOVERY_DATA;
      discoveryDataPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;

      // read data from specified address
      uns8* pData = discoveryDataPacket.DpaRequestPacket_t.DpaMessage.Request.PData;
      pData[0] = address & 0xFF;
      pData[1] = (address >> 8) & 0xFF;

      discoveryDataRequest.DataToBuffer(discoveryDataPacket.Buffer, sizeof(TDpaIFaceHeader) + 2);

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> discoveryDataTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      for (int rep = 0; rep <= m_repeat; rep++) {
        try {
          discoveryDataTransaction = m_iIqrfDpaService->executeDpaTransaction(discoveryDataRequest);
          transResult = discoveryDataTransaction->get();
        }
        catch (std::exception& e) {
          TRC_DEBUG("DPA transaction error : " << e.what());

          if (rep < m_repeat) {
            continue;
          }

          THROW_EXC(std::logic_error, "DPA transaction error : " << e.what());
        }

        TRC_DEBUG("Result from discovery data transaction as string:" << PAR(transResult->getErrorString()));

        IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

        // because of the move-semantics
        DpaMessage dpaResponse = transResult->getResponse();

        if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
          TRC_INFORMATION("Discovery data successful!");
          TRC_DEBUG(
            "DPA transaction: "
            << NAME_PAR(discoveryDataRequest.PeripheralType(), discoveryDataRequest.NodeAddress())
            << PAR(discoveryDataRequest.PeripheralCommand())
          );

          // is device discovered?
          TPerCoordinatorDiscoveryData_Response responseData
            = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerCoordinatorDiscoveryData_Response;

          std::basic_string<uint8_t> discoveredDevicesBitmap;
          for (int i = 0; i < DISCOVERED_DEVICES_BITMAP_SIZE; i++) {
            discoveredDevicesBitmap.push_back(responseData.DiscoveryData[i]);
          }

          TRC_FUNCTION_LEAVE("");
          return discoveredDevicesBitmap;
        }

        // transaction error
        if (errorCode < 0) {
          TRC_DEBUG("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

          if (rep < m_repeat) {
            continue;
          }

          THROW_EXC(std::logic_error, "Transaction error. " << NAME_PAR_HEX("Error code", errorCode));
        }

        // DPA error
        TRC_DEBUG("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

        if (rep < m_repeat) {
          continue;
        }

        THROW_EXC(std::logic_error, "DPA error. " << NAME_PAR_HEX("Error code", errorCode));
      }
    }

    // read discovery data
    void discoveryData(DeviceEnumerateResult& deviceEnumerateResult) {
      // get discovered indicator
      try {
        std::basic_string<uint8_t> discoveredDevicesBitmap = discoveryData(deviceEnumerateResult, 0x20);
        
        uint8_t byteIndex = deviceEnumerateResult.getDeviceAddr() / 8;
        uint8_t bitIndex = deviceEnumerateResult.getDeviceAddr() % 8;
        uint8_t compareByte = uint8_t(pow(2, bitIndex));

        deviceEnumerateResult.setDiscovered(
          (discoveredDevicesBitmap[byteIndex] & compareByte) == compareByte
        );
      }
      catch (std::exception& ex) {
        DeviceEnumerateError error(DeviceEnumerateError::Type::InfoMissing, ex.what());
        deviceEnumerateResult.setDiscoveredError(error);
      }

      // VRN
      try {
        uint16_t address = 0x5000 + deviceEnumerateResult.getDeviceAddr();

        std::basic_string<uint8_t> vrnArray = discoveryData(deviceEnumerateResult, address);
        deviceEnumerateResult.setVrn(vrnArray[0]);
      }
      catch (std::exception& ex) {
        DeviceEnumerateError error(DeviceEnumerateError::Type::InfoMissing, ex.what());
        deviceEnumerateResult.setVrnError(error);
      }

      // zone
      try {
        uint16_t address = 0x5200 + deviceEnumerateResult.getDeviceAddr();

        std::basic_string<uint8_t> zoneArray = discoveryData(deviceEnumerateResult, address);
        deviceEnumerateResult.setZone(zoneArray[0]);
      }
      catch (std::exception& ex) {
        DeviceEnumerateError error(DeviceEnumerateError::Type::InfoMissing, ex.what());
        deviceEnumerateResult.setZoneError(error);
      }

      // parent
      try {
        uint16_t address = 0x5300 + deviceEnumerateResult.getDeviceAddr();

        std::basic_string<uint8_t> parentArray = discoveryData(deviceEnumerateResult, address);
        deviceEnumerateResult.setParent(parentArray[0]);
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
          osReadTransaction = m_iIqrfDpaService->executeDpaTransaction(osReadRequest);
          transResult = osReadTransaction->get();
        }
        catch (std::exception& e) {
          TRC_DEBUG("DPA transaction error : " << e.what());

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

        if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
          TRC_INFORMATION("OS read successful!");
          TRC_DEBUG(
            "DPA transaction: "
            << NAME_PAR(osReadRequest.PeripheralType(), osReadRequest.NodeAddress())
            << PAR(osReadRequest.PeripheralCommand())
          );

          // get OS data
          TPerOSRead_Response osData = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerOSRead_Response;
          deviceEnumerateResult.setOsRead(osData);

          TRC_FUNCTION_LEAVE("");
          return;
        }

        // transaction error
        if (errorCode < 0) {
          TRC_DEBUG("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

          if (rep < m_repeat) {
            continue;
          }

          DeviceEnumerateError error(DeviceEnumerateError::Type::OsRead, "Transaction error.");
          deviceEnumerateResult.setOsReadError(error);

          TRC_FUNCTION_LEAVE("");
          return;
        }

        // DPA error
        TRC_DEBUG("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

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
          perEnumTransaction = m_iIqrfDpaService->executeDpaTransaction(perEnumRequest);
          transResult = perEnumTransaction->get();
        }
        catch (std::exception& e) {
          TRC_DEBUG("DPA transaction error : " << e.what());

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

          TRC_FUNCTION_LEAVE("");
          return;
        }

        // transaction error
        if (errorCode < 0) {
          TRC_DEBUG("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

          if (rep < m_repeat) {
            continue;
          }

          DeviceEnumerateError error(DeviceEnumerateError::Type::PerEnum, "Transaction error.");
          deviceEnumerateResult.setPerEnumError(error);

          TRC_FUNCTION_LEAVE("");
          return;
        }

        // DPA error
        TRC_DEBUG("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

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
          readHwpTransaction = m_iIqrfDpaService->executeDpaTransaction(readHwpRequest);
          transResult = readHwpTransaction->get();
        }
        catch (std::exception& e) {
          TRC_DEBUG("DPA transaction error : " << e.what());

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

        if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
          TRC_INFORMATION("Read HWP configuration successful!");
          TRC_DEBUG(
            "DPA transaction: "
            << NAME_PAR(readHwpRequest.PeripheralType(), readHwpRequest.NodeAddress())
            << PAR(readHwpRequest.PeripheralCommand())
          );

          // get HWP configuration 
          TPerOSReadCfg_Response readHwpConfig = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerOSReadCfg_Response;
          deviceEnumerateResult.setHwpConfig(readHwpConfig);

          TRC_FUNCTION_LEAVE("");
          return;
        }

        // transaction error
        if (errorCode < 0) {
          TRC_DEBUG("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

          if (rep < m_repeat) {
            continue;
          }

          DeviceEnumerateError error(DeviceEnumerateError::Type::ReadHwp, "Transaction error.");
          deviceEnumerateResult.setReadHwpConfigError(error);

          TRC_FUNCTION_LEAVE("");
          return;
        }

        // DPA error
        TRC_DEBUG("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

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
          morePersInfoTransaction = m_iIqrfDpaService->executeDpaTransaction(morePersInfoRequest);
          transResult = morePersInfoTransaction->get();
        }
        catch (std::exception& e) {
          TRC_DEBUG("DPA transaction error : " << e.what());

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
          TRC_DEBUG("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

          if (rep < m_repeat) {
            continue;
          }

          DeviceEnumerateError error(DeviceEnumerateError::Type::MorePersInfo, "Transaction error.");
          deviceEnumerateResult.setMorePersInfoError(error);

          TRC_FUNCTION_LEAVE("");
          return;
        }

        // DPA error
        TRC_DEBUG("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

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
          TRC_DEBUG("DPA transaction error : " << e.what());

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

        if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
          TRC_INFORMATION("Get bonded nodes successful!");
          TRC_DEBUG(
            "DPA transaction: "
            << NAME_PAR(bondedNodesRequest.PeripheralType(), bondedNodesRequest.NodeAddress())
            << PAR(bondedNodesRequest.PeripheralCommand())
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
          TRC_DEBUG("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

          if (rep < m_repeat) {
            continue;
          }

          DeviceEnumerateError error(DeviceEnumerateError::Type::NotBonded, "Transaction error.");
          deviceEnumerateResult.setBondedError(error);

          TRC_FUNCTION_LEAVE("");
          return;
        }

        // DPA error
        TRC_DEBUG("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

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

    // creates and sends discovery message
    void sendDiscoveryDataResponse(
      const std::string& messagingId,
      const IMessagingSplitterService::MsgType& msgType,
      DeviceEnumerateResult& deviceEnumerateResult,
      const ComIqmeshNetworkEnumerateDevice& comDeviceEnumerate
    )
    {
      rapidjson::Document response;

      Pointer("/mType").Set(response, msgType.m_type);
      Pointer("/data/msgId").Set(response, comDeviceEnumerate.getMsgId());

      // if no data was successfully read, return empty message
      if (
        deviceEnumerateResult.getDiscoveredError().getType() != DeviceEnumerateError::Type::NoError
        && deviceEnumerateResult.getVrnError().getType() != DeviceEnumerateError::Type::NoError
        && deviceEnumerateResult.getZoneError().getType() != DeviceEnumerateError::Type::NoError
        && deviceEnumerateResult.getParentError().getType() != DeviceEnumerateError::Type::NoError
      ) {
        // status - ERROR_INFO_MISSING
        setResponseStatus(response, SERVICE_ERROR_INFO_MISSING, "info missing");
      }
      else {
        Pointer("/data/rsp/deviceAddr").Set(response, deviceEnumerateResult.getDeviceAddr());

        Pointer("/data/rsp/result/discovered").Set(response, deviceEnumerateResult.isDiscovered());
        Pointer("/data/rsp/result/vrn").Set(response, deviceEnumerateResult.getVrn());
        Pointer("/data/rsp/result/zone").Set(response, deviceEnumerateResult.getZone());
        Pointer("/data/rsp/result/parent").Set(response, deviceEnumerateResult.getParent());

        // status - ok
        setResponseStatus(response, 0, "ok");
      }

      // set raw fields, if verbose mode is active
      if (comDeviceEnumerate.getVerbose()) {
        setVerboseData(response, deviceEnumerateResult);
      }

      // send response
      m_iMessagingSplitterService->sendMessage(messagingId, std::move(response));
    }

    // creates and sends OS read message
    void sendOsReadResponse(
      const std::string& messagingId,
      const IMessagingSplitterService::MsgType& msgType,
      DeviceEnumerateResult& deviceEnumerateResult,
      const ComIqmeshNetworkEnumerateDevice& comDeviceEnumerate
    )
    {
      rapidjson::Document response;

      Pointer("/mType").Set(response, msgType.m_type);
      Pointer("/data/msgId").Set(response, comDeviceEnumerate.getMsgId());
 
      // if data was not successfully read, send empty message
      if (
        deviceEnumerateResult.getOsReadError().getType() != DeviceEnumerateError::Type::NoError
        ) {
        // status - ERROR_INFO_MISSING
        setResponseStatus(response, SERVICE_ERROR_INFO_MISSING, "info missing");
      }
      else {
        Pointer("/data/rsp/deviceAddr").Set(response, deviceEnumerateResult.getDeviceAddr());

        // osRead object
        TPerOSRead_Response osInfo = deviceEnumerateResult.getOsRead();

        rapidjson::Pointer("/data/rsp/result/mid").Set(response, encodeBinary(osInfo.ModuleId, 4));
        rapidjson::Pointer("/data/rsp/result/osVersion").Set(response, encodeHexaNum(osInfo.OsVersion));
        rapidjson::Pointer("/data/rsp/result/trMcuType").Set(response, encodeHexaNum(osInfo.McuType));
        rapidjson::Pointer("/data/rsp/result/osBuild").Set(response, encodeHexaNum(osInfo.OsBuild));
        rapidjson::Pointer("/data/rsp/result/rssi").Set(response, osInfo.Rssi);
        rapidjson::Pointer("/data/rsp/result/supplyVoltage").Set(response, encodeHexaNum(osInfo.SupplyVoltage));
        rapidjson::Pointer("/data/rsp/result/flags").Set(response, osInfo.Flags);
        rapidjson::Pointer("/data/rsp/result/slotLimits").Set(response, osInfo.SlotLimits);
        
        // status - ok
        setResponseStatus(response, 0, "ok");
      }
      
      // set raw fields, if verbose mode is active
      if (comDeviceEnumerate.getVerbose()) {
        setVerboseData(response, deviceEnumerateResult);
      }

      // send response
      m_iMessagingSplitterService->sendMessage(messagingId, std::move(response));
    }

    void sendPeripheralEnumerationResponse(
      const std::string& messagingId,
      const IMessagingSplitterService::MsgType& msgType,
      DeviceEnumerateResult& deviceEnumerateResult,
      const ComIqmeshNetworkEnumerateDevice& comDeviceEnumerate
    ) 
    {
      rapidjson::Document response;

      Pointer("/mType").Set(response, msgType.m_type);
      Pointer("/data/msgId").Set(response, comDeviceEnumerate.getMsgId());

      // if data was not successfully read, send empty message
      if (
        deviceEnumerateResult.getPerEnumError().getType() != DeviceEnumerateError::Type::NoError
        ) {
        // status - ERROR_INFO_MISSING
        setResponseStatus(response, SERVICE_ERROR_INFO_MISSING, "info missing");
      }
      else {
        Pointer("/data/rsp/deviceAddr").Set(response, deviceEnumerateResult.getDeviceAddr());

        // per enum object
        TEnumPeripheralsAnswer perEnum = deviceEnumerateResult.getPerEnum();

        // dpa version - string
        std::string dpaVer = std::to_string((perEnum.DpaVersion & 0xFF) >> 8)
          + "." + std::to_string(perEnum.DpaVersion & 0xFF);
        Pointer("/data/rsp/result/dpaVer").Set(response, dpaVer);

        // perNr
        Pointer("/data/rsp/result/perNr").Set(response, perEnum.UserPerNr);

        Document::AllocatorType& allocator = response.GetAllocator();

        // embPers
        rapidjson::Value embPersJsonArray(kArrayType);
        for (int i = 0; i < EMBEDDED_PERS_LEN; i++) {
          embPersJsonArray.PushBack(perEnum.EmbeddedPers[i], allocator);
        }
        Pointer("/data/rsp/result/embPers").Set(response, embPersJsonArray);

        // hwpId
        Pointer("/data/rsp/result/hwpId").Set(response, encodeHexaNum(perEnum.HWPID));

        // hwpIdVer
        Pointer("/data/rsp/result/hwpIdVer").Set(response, perEnum.HWPIDver);

        // flags
        Pointer("/data/rsp/result/flags").Set(response, perEnum.Flags);

        // userPers
        rapidjson::Value userPerJsonArray(kArrayType);
        for (int i = 0; i < USER_PER_LEN; i++) {
          userPerJsonArray.PushBack(perEnum.UserPer[i], allocator);
        }
        Pointer("/data/rsp/result/userPers").Set(response, userPerJsonArray);

        // status - ok
        setResponseStatus(response, 0, "ok");
      }
      
      // set raw fields, if verbose mode is active
      if (comDeviceEnumerate.getVerbose()) {
        setVerboseData(response, deviceEnumerateResult);
      }

      // send response
      m_iMessagingSplitterService->sendMessage(messagingId, std::move(response));
    }
    
    void sendReadHwpConfigurationResponse(
      const std::string& messagingId,
      const IMessagingSplitterService::MsgType& msgType,
      DeviceEnumerateResult& deviceEnumerateResult,
      const ComIqmeshNetworkEnumerateDevice& comDeviceEnumerate
    )
    {
      rapidjson::Document response;

      Pointer("/mType").Set(response, msgType.m_type);
      Pointer("/data/msgId").Set(response, comDeviceEnumerate.getMsgId());

      // if data was not successfully read, send empty message
      if (
        deviceEnumerateResult.getPerEnumError().getType() != DeviceEnumerateError::Type::NoError
        ) {
        // status - ERROR_INFO_MISSING
        setResponseStatus(response, SERVICE_ERROR_INFO_MISSING, "info missing");
      }
      else {
        Pointer("/data/rsp/deviceAddr").Set(response, deviceEnumerateResult.getDeviceAddr());

        // per enum object
        TPerOSReadCfg_Response hwpConfig = deviceEnumerateResult.getHwpConfig();
        
        // checkSum
        Pointer("/data/rsp/result/checkSum").Set(response, hwpConfig.Checksum);

        // cfgBytes
        Document::AllocatorType& allocator = response.GetAllocator();
        rapidjson::Value cfgBytesJsonArray(kArrayType);
        for (int i = 0; i < CONFIGURATION_LEN; i++) {
          cfgBytesJsonArray.PushBack(hwpConfig.Configuration[i], allocator);
        }
        Pointer("/data/rsp/result/cfgBytes").Set(response, cfgBytesJsonArray);

        // rfPgm
        Pointer("/data/rsp/result/rfPgm").Set(response, hwpConfig.RFPGM);

        // undocumented
        Pointer("/data/rsp/result/undocumented").Set(response, hwpConfig.Undocumented[1]);

        // status - ok
        setResponseStatus(response, 0, "ok");
      }

      // set raw fields, if verbose mode is active
      if (comDeviceEnumerate.getVerbose()) {
        setVerboseData(response, deviceEnumerateResult);
      }

      // send response
      m_iMessagingSplitterService->sendMessage(messagingId, std::move(response));
    }

    void sendInfoForMorePeripheralsResponse(
      const std::string& messagingId,
      const IMessagingSplitterService::MsgType& msgType,
      DeviceEnumerateResult& deviceEnumerateResult,
      const ComIqmeshNetworkEnumerateDevice& comDeviceEnumerate
    ) 
    {
      rapidjson::Document response;

      Pointer("/mType").Set(response, msgType.m_type);
      Pointer("/data/msgId").Set(response, comDeviceEnumerate.getMsgId());

      // if data was not successfully read, send empty message
      if (
        deviceEnumerateResult.getPerEnumError().getType() != DeviceEnumerateError::Type::NoError
        ) {
        // status - ERROR_INFO_MISSING
        setResponseStatus(response, SERVICE_ERROR_INFO_MISSING, "Info missing");
      }
      else {
        Pointer("/data/rsp/deviceAddr").Set(response, deviceEnumerateResult.getDeviceAddr());

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
        Pointer("/data/rsp/result").Set(response, perInfoJsonArray);

        // status - ok
        setResponseStatus(response, 0, "ok");
      }

      // set raw fields, if verbose mode is active
      if (comDeviceEnumerate.getVerbose()) {
        setVerboseData(response, deviceEnumerateResult);
      }

      // send response
      m_iMessagingSplitterService->sendMessage(messagingId, std::move(response));
    }


    // field "valid" is set to "true" for now
    // ma se poslat vsechno, co bylo v predchozich DPA responses dostupne anebo
    // pokud je neco nedostupneho, tak hned SERVICE_ERROR_INFO_MISSING?
    void sendValidationAndUpdatesResponse(
      const std::string& messagingId,
      const IMessagingSplitterService::MsgType& msgType,
      DeviceEnumerateResult& deviceEnumerateResult,
      const ComIqmeshNetworkEnumerateDevice& comDeviceEnumerate
    ) 
    {
      rapidjson::Document response;

      Pointer("/mType").Set(response, msgType.m_type);
      Pointer("/data/msgId").Set(response, comDeviceEnumerate.getMsgId());

      Pointer("/data/rsp/deviceAddr").Set(response, deviceEnumerateResult.getDeviceAddr());

      // valid for now always true
      Pointer("/data/rsp/result/validation/valid").Set(response, true);
      
      // OS read
      if (deviceEnumerateResult.getOsReadError().getType() == DeviceEnumerateError::Type::NoError) 
      {
        // OS version
        TPerOSRead_Response osRead = deviceEnumerateResult.getOsRead();
        Pointer("/data/rsp/result/validation/osVer").Set(response, encodeHexaNum(osRead.OsVersion));

        // OS build
        Pointer("/data/rsp/result/validation/osBuild").Set(response, encodeHexaNum(osRead.OsBuild));
      }

      if (
        deviceEnumerateResult.getPerEnumError().getType() == DeviceEnumerateError::Type::NoError
        ) 
      {
        // DPA ver
        TEnumPeripheralsAnswer perEnum = deviceEnumerateResult.getPerEnum();
        Pointer("/data/rsp/result/validation/dpaVer").Set(response, encodeHexaNum(perEnum.DpaVersion));
      }

      if (deviceEnumerateResult.getReadHwpConfigError().getType() == DeviceEnumerateError::Type::NoError)
      {
        // configuration
        TPerOSReadCfg_Response hwpConfig = deviceEnumerateResult.getHwpConfig();

        // txPower
        Pointer("/data/rsp/result/validation/txPower").Set(response, hwpConfig.Configuration[0x08]);

        // rxFilter
        Pointer("/data/rsp/result/validation/rxFilter").Set(response, hwpConfig.Configuration[0x09]);
      }

      // status - ok
      setResponseStatus(response, 0, "ok");

      // set raw fields, if verbose mode is active
      if (comDeviceEnumerate.getVerbose()) {
        setVerboseData(response, deviceEnumerateResult);
      }

      // send response
      m_iMessagingSplitterService->sendMessage(messagingId, std::move(response));
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

    uint8_t parseAndCheckDeviceAddr(int nadr) {
      if ((nadr < 0) || (nadr > 0xEF)) {
        THROW_EXC(
          std::out_of_range, "Device address outside of valid range. " << NAME_PAR_HEX("Address", nadr)
        );
      }
      return nadr;
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
      uint8_t deviceAddr;

      // parsing and checking service parameters
      try {
        m_repeat = parseAndCheckRepeat(comEnumerateDevice.getRepeat());
       
        if (!comEnumerateDevice.isSetDeviceAddr()) {
          THROW_EXC(std::logic_error, "deviceAddr not set");
        }
        deviceAddr = parseAndCheckDeviceAddr(comEnumerateDevice.getDeviceAddr());

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

      if (deviceEnumerateResult.getBondedError().getType() != DeviceEnumerateError::Type::NoError) {
        Document failResponse = getCheckParamsFailedResponse(comEnumerateDevice.getMsgId(), msgType, deviceEnumerateResult.getBondedError().getMessage());
        m_iMessagingSplitterService->sendMessage(messagingId, std::move(failResponse));

        TRC_FUNCTION_LEAVE("");
        return;
      }

      // discovery data
      discoveryData(deviceEnumerateResult);
      sendDiscoveryDataResponse(messagingId, msgType, deviceEnumerateResult, comEnumerateDevice);

      // OS read info
      osRead(deviceEnumerateResult);
      sendOsReadResponse(messagingId, msgType, deviceEnumerateResult, comEnumerateDevice);

      // peripheral enumeration
      peripheralEnumeration(deviceEnumerateResult);
      sendPeripheralEnumerationResponse(messagingId, msgType, deviceEnumerateResult, comEnumerateDevice);
      
      // read Hwp configuration
      readHwpConfiguration(deviceEnumerateResult);
      sendReadHwpConfigurationResponse(messagingId, msgType, deviceEnumerateResult, comEnumerateDevice);

      // get info for more peripherals
      getInfoForMorePeripherals(deviceEnumerateResult);
      sendInfoForMorePeripheralsResponse(messagingId, msgType, deviceEnumerateResult, comEnumerateDevice);

      // validation
      // updates will be implemented later
      sendValidationAndUpdatesResponse(messagingId, msgType, deviceEnumerateResult, comEnumerateDevice);
      
      /*
      // create and send FINAL response
      Document responseDoc = createResponse(messagingId, msgType, deviceEnumerateResult, comDeviceEnumerate);
      m_iMessagingSplitterService->sendMessage(messagingId, std::move(responseDoc));
      */
      
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