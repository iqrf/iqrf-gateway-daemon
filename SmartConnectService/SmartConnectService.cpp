#define ISmartConnectService_EXPORTS

#include "DpaTransactionTask.h"
#include "SmartConnectService.h"
#include "Trace.h"
#include "ComIqmeshNetworkSmartConnect.h"
#include "IqrfCodeDecoder.h"
#include "ObjectFactory.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"

#include "iqrf__SmartConnectService.hxx"

#include <list>
#include <memory>
#include <math.h>
#include <bitset>

TRC_INIT_MODULE(iqrf::SmartConnectService);


using namespace rapidjson;

namespace {

  // maximum number of repeats
  static const uint8_t REPEAT_MAX = 3;

  // length of Individual Bonding key [in bytes]
  static const uint8_t IBK_LEN = 16;

  // length of MID
  static const uint8_t MID_LEN = 4;

  // length of user data
  static const uint8_t USER_DATA_LEN = 4;

  // minimal required DPA version
  static const uint16_t DPA_MIN_REQ_VERSION = 0x0303;



  // service general fail code - may and probably will be changed later in the future
  static const int SERVICE_ERROR = 1000;

  static const int SERVICE_ERROR_MIN_DPA_VER = SERVICE_ERROR + 1;
  static const int SERVICE_ERROR_SMART_CONNECT = SERVICE_ERROR + 2;
  static const int SERVICE_ERROR_OS_READ = SERVICE_ERROR + 3;
};


namespace iqrf {

  // Holds information about errors, which encounter during smart connect service run
  class SmartConnectError {
  public:
    // Type of error
    enum class Type {
      NoError,
      MinDpaVerUsed,
      SmartConnect,
      OsRead
    };

    SmartConnectError() : m_type(Type::NoError), m_message("") {};
    SmartConnectError(Type errorType) : m_type(errorType), m_message("") {};
    SmartConnectError(Type errorType, const std::string& message) : m_type(errorType), m_message(message) {};

    Type getType() const { return m_type; };
    std::string getMessage() const { return m_message; };

    SmartConnectError& operator=(const SmartConnectError& error) {
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


  // holds information about result of smart connect
  class SmartConnectResult {
  private:
    SmartConnectError m_error;

    uint16_t m_hwpId;
    uint8_t m_bondedAddr;
    uint8_t m_bondedNodesNum;
    std::string m_manufacturer = "";
    std::string m_product = "";
    std::list<std::string> m_standards;

    // OS read response data
    TPerOSRead_Response m_osRead;

    // transaction results
    std::list<std::unique_ptr<IDpaTransactionResult2>> m_transResults;

  public:
    SmartConnectError getError() const { return m_error; };

    void setError(const SmartConnectError& error) {
      m_error = error;
    }

    uint16_t getHwpId() const {
      return m_hwpId;
    }

    void setHwpId(uint16_t m_hwpId) {
      this->m_hwpId = m_hwpId;
    }

    uint8_t getBondedAddr() const {
      return m_bondedAddr;
    }

    void setBondedAddr(uint8_t m_bondedAddr) {
      this->m_bondedAddr = m_bondedAddr;
    }

    uint8_t getBondedNodesNum() const {
      return m_bondedNodesNum;
    }

    void setBondedNodesNum(uint8_t m_bondedNodesNum) {
      this->m_bondedNodesNum = m_bondedNodesNum;
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

    std::list<std::string> getStandards() {
      return m_standards;
    }

    void setStandards(std::list<std::string> standards) {
      m_standards = standards;
    }

    TPerOSRead_Response getOsRead() {
      return m_osRead;
    }

    void setOsRead(TPerOSRead_Response osRead) {
      m_osRead = osRead;
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
  class SmartConnectService::Imp {
  private:
    // parent object
    SmartConnectService& m_parent;

    // message type: IQMESH Network Smart connect
    // for temporal reasons
    const std::string m_mTypeName_iqmeshNetworkSmartConnect = "iqmeshNetwork_SmartConnect";
    //IMessagingSplitterService::MsgType* m_msgType_mngIqmeshWriteConfig;

    IJsCacheService* m_iJsCacheService = nullptr;
    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    IIqrfDpaService* m_iIqrfDpaService = nullptr;

    // number of repeats
    uint8_t m_repeat;

    // if is set Verbose mode
    bool m_returnVerbose = false;


  public:
    Imp(SmartConnectService& parent) : m_parent(parent)
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
    
    // sets PData for Smart connect request
    void setSmartConnectRequestData(
      uns8* pData, 
      const uint8_t reqAddr, 
      const uint8_t bondingTestRetries, 
      const std::basic_string<uint8_t>& ibk, 
      const std::basic_string<uint8_t>& mid, 
      const uint8_t bondingChannel, 
      const uint8_t virtualDeviceAddress, 
      const std::basic_string<uint8_t>& userData
    )
    {
      pData[0] = reqAddr;
      pData[1] = bondingTestRetries;

      std::copy(ibk.begin(), ibk.end(), pData + 2);
      std::copy(mid.begin(), mid.end(), pData + 18);

      pData[23] = bondingChannel;
      pData[24] = virtualDeviceAddress;

      // must be filled with zeroes
      for (int i = 0; i < 9; i++) {
        pData[i + 25] = 0;
      }

      std::copy(userData.begin(), userData.end(), pData + 34);
    }

    void _smartConnect(
      SmartConnectResult& smartConnectResult,
      const uint16_t hwpId,
      const uint8_t reqAddr,
      const uint8_t bondingTestRetries,
      const std::basic_string<uint8_t>& ibk,
      const std::basic_string<uint8_t>& mid,
      const uint8_t bondingChannel,
      const uint8_t virtualDeviceAddress,
      const std::basic_string<uint8_t>& userData
    ) 
    {
      TRC_FUNCTION_ENTER("");
      
      DpaMessage smartConnectRequest;
      DpaMessage::DpaPacket_t smartConnectPacket;
      smartConnectPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      smartConnectPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
      smartConnectPacket.DpaRequestPacket_t.PCMD = 0x12;
      smartConnectPacket.DpaRequestPacket_t.HWPID = hwpId;

      uns8* pData = smartConnectPacket.DpaRequestPacket_t.DpaMessage.Request.PData;
      setSmartConnectRequestData(
        pData, reqAddr, bondingTestRetries, ibk, mid, bondingChannel, virtualDeviceAddress, userData
      );

      smartConnectRequest.DataToBuffer(smartConnectPacket.Buffer, sizeof(TDpaIFaceHeader) + 38);

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> smartConnectTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      for (int rep = 0; rep <= m_repeat; rep++) {
        try {
          smartConnectTransaction = m_iIqrfDpaService->executeDpaTransaction(smartConnectRequest, 10000);
          transResult = smartConnectTransaction->get();
        }
        catch (std::exception& e) {
          TRC_DEBUG("DPA transaction error : " << e.what());

          if (rep < m_repeat) {
            continue;
          }

          SmartConnectError error(SmartConnectError::Type::SmartConnect, e.what());
          smartConnectResult.setError(error);

          TRC_FUNCTION_LEAVE("");
          return;
        }

        TRC_DEBUG("Result from smart connect transaction as string:" << PAR(transResult->getErrorString()));

        IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

        // because of the move-semantics
        DpaMessage dpaResponse = transResult->getResponse();
        smartConnectResult.addTransactionResult(transResult);

        if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
          TRC_INFORMATION("Smart connect successful!");
          TRC_DEBUG(
            "DPA transaction: "
            << NAME_PAR(smartConnectRequest.PeripheralType(), smartConnectRequest.NodeAddress())
            << PAR(smartConnectRequest.PeripheralCommand())
          );

          smartConnectResult.setHwpId(dpaResponse.DpaPacket().DpaResponsePacket_t.HWPID);

          // parsing response pdata
          uns8* respData = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData;
          smartConnectResult.setBondedAddr(respData[0]);
          smartConnectResult.setBondedNodesNum(respData[1]);

          TRC_FUNCTION_LEAVE("");
          return;
        }

        // transaction error
        if (errorCode < 0) {
          TRC_DEBUG("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

          if (rep < m_repeat) {
            continue;
          }

          SmartConnectError error(SmartConnectError::Type::SmartConnect, "Transaction error.");
          smartConnectResult.setError(error);

          TRC_FUNCTION_LEAVE("");
          return;
        }

        // DPA error
        TRC_DEBUG("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

        if (rep < m_repeat) {
          continue;
        }

        smartConnectResult.setHwpId(dpaResponse.DpaPacket().DpaResponsePacket_t.HWPID);
        SmartConnectError error(SmartConnectError::Type::SmartConnect, "Dpa error.");
        smartConnectResult.setError(error);

        TRC_FUNCTION_LEAVE("");
      }
    }

    // returns current used DPA version
    uint16_t getDpaVersion(uint16_t hwpId) {
      TRC_FUNCTION_ENTER("");

      DpaMessage perEnumRequest;
      DpaMessage::DpaPacket_t perEnumPacket;
      perEnumPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      perEnumPacket.DpaRequestPacket_t.PNUM = 0xFF;
      perEnumPacket.DpaRequestPacket_t.PCMD = 0x3F;
      perEnumPacket.DpaRequestPacket_t.HWPID = hwpId;
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

          THROW_EXC(
            std::logic_error,
            "Could not information about individual devices and their implemented peripherals."
          );
        }

        TRC_DEBUG("Result from smart connect transaction as string:" << PAR(transResult->getErrorString()));

        IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

        // because of the move-semantics
        DpaMessage dpaResponse = transResult->getResponse();

        if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
          TRC_INFORMATION("Device exploration successful!");
          TRC_DEBUG(
            "DPA transaction: "
            << NAME_PAR(perEnumRequest.PeripheralType(), perEnumRequest.NodeAddress())
            << PAR(perEnumRequest.PeripheralCommand())
          );

          // parsing response pdata
          uns8* respData = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData;
          uint8_t minorDpaVer = respData[0];
          uint8_t majorDpaVer = respData[1];

          TRC_FUNCTION_LEAVE("");
          return minorDpaVer + (majorDpaVer << 8);
        }

        // transaction error
        if (errorCode < 0) {
          TRC_DEBUG("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

          if (rep < m_repeat) {
            continue;
          }

          THROW_EXC(std::logic_error, "Transaction error.");
        }

        // DPA error
        TRC_DEBUG("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

        if (rep < m_repeat) {
          continue;
        }

        THROW_EXC(std::logic_error, "DPA error.");
      }

      // should never reach this point
      THROW_EXC(std::logic_error, "Internal service error.");
    }


    // reads OS info about smart connected node
    void osRead(SmartConnectResult& smartConnectResult) {
      TRC_FUNCTION_ENTER("");

      DpaMessage osReadRequest;
      DpaMessage::DpaPacket_t osReadPacket;
      osReadPacket.DpaRequestPacket_t.NADR = smartConnectResult.getBondedAddr();
      osReadPacket.DpaRequestPacket_t.PNUM = PNUM_OS;
      osReadPacket.DpaRequestPacket_t.PCMD = CMD_OS_READ;
      osReadPacket.DpaRequestPacket_t.HWPID = smartConnectResult.getHwpId();
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

          SmartConnectError error(SmartConnectError::Type::OsRead, e.what());
          smartConnectResult.setError(error);

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
          smartConnectResult.setOsRead(osData);

          TRC_FUNCTION_LEAVE("");
          return;
        }

        // transaction error
        if (errorCode < 0) {
          TRC_DEBUG("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

          if (rep < m_repeat) {
            continue;
          }

          SmartConnectError error(SmartConnectError::Type::OsRead, "Transaction error.");
          smartConnectResult.setError(error);

          TRC_FUNCTION_LEAVE("");
          return;
        }

        // DPA error
        TRC_DEBUG("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

        if (rep < m_repeat) {
          continue;
        }

        SmartConnectError error(SmartConnectError::Type::OsRead, "Dpa error.");
        smartConnectResult.setError(error);

        TRC_FUNCTION_LEAVE("");
      }
    }

    SmartConnectResult smartConnect(
      const uint16_t hwpId,
      const uint8_t reqAddr,
      const uint8_t bondingTestRetries,
      const std::basic_string<uint8_t>& ibk,
      const std::basic_string<uint8_t>& mid,
      const uint8_t bondingChannel,
      const uint8_t virtualDeviceAddress,
      const std::basic_string<uint8_t>& userData
    )
    {
      TRC_FUNCTION_ENTER("");

      // result
      SmartConnectResult smartConnectResult;

      // check, if there is at minimal DPA ver. 3.03 at the coordinator
      uint16_t dpaVersion = 0;
      try {
        dpaVersion = getDpaVersion(hwpId);
        if (dpaVersion < DPA_MIN_REQ_VERSION) {
          THROW_EXC(std::logic_error, "Old version of DPA: " << PAR(dpaVersion));
        }
      }
      catch (std::exception& ex) {
        SmartConnectError error(SmartConnectError::Type::MinDpaVerUsed, ex.what());
        smartConnectResult.setError(error);
        return smartConnectResult;
      }
      
      // do smart connect
      _smartConnect(
        smartConnectResult, hwpId, reqAddr, bondingTestRetries, ibk, mid, bondingChannel, virtualDeviceAddress, userData
      );

      // if there was an error, return
      if (smartConnectResult.getError().getType() != SmartConnectError::Type::NoError) {
        return smartConnectResult;
      }

      // get OS read data
      osRead(smartConnectResult);

      // if there was an error, return
      if (smartConnectResult.getError().getType() != SmartConnectError::Type::NoError) {
        return smartConnectResult;
      }
      
      const IJsCacheService::Manufacturer* manufacturer = m_iJsCacheService->getManufacturer(hwpId);
      if (manufacturer != nullptr) {
        smartConnectResult.setManufacturer(manufacturer->m_name);
      }
      
      const IJsCacheService::Product* product = m_iJsCacheService->getProduct(hwpId);
      if (product != nullptr) {
        smartConnectResult.setProduct(product->m_name);
      }

      uint8_t osVersion = smartConnectResult.getOsRead().OsVersion;
      std::string osVersionStr = std::to_string((osVersion << 4) & 0xFF) + "." + std::to_string(osVersion & 0x0F);
      std::string dpaVersionStr = std::to_string((dpaVersion << 8) & 0xFF) + "." + std::to_string(dpaVersion & 0xFF);

      const IJsCacheService::Package* package = m_iJsCacheService->getPackage(hwpId, osVersionStr, dpaVersionStr);
      if (package != nullptr) {
        std::list<std::string> standards;
        for (const IJsCacheService::StdDriver* driver : package->m_stdDriverVect) {
          standards.push_back(driver->getName());
        }
        smartConnectResult.setStandards(standards);
      }
      
      return smartConnectResult;
      TRC_FUNCTION_LEAVE("");
    }


    // creates error response about service general fail
    Document createCheckParamsFailedResponse(
      const std::string& messagingId,
      const IMessagingSplitterService::MsgType& msgType,
      const std::string& errorMsg
    )
    {
      Document response;
      
      // set common parameters
      Pointer("/mType").Set(response, msgType.m_type);
      Pointer("/data/msgId").Set(response, messagingId);

      // set result
      Pointer("/status").Set(response, SERVICE_ERROR);
      Pointer("/statusStr").Set(response, errorMsg);

      return response;
    }
   
    // sets response VERBOSE data
    void setVerboseData(rapidjson::Document& response, SmartConnectResult& smartConnectResult) 
    {
      // set raw fields, if verbose mode is active
      rapidjson::Value rawArray(kArrayType);
      Document::AllocatorType& allocator = response.GetAllocator();

      while (smartConnectResult.isNextTransactionResult()) {
        std::unique_ptr<IDpaTransactionResult2> transResult = smartConnectResult.consumeNextTransactionResult();
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

    // creates response on the basis of smart connect result
    Document createResponse(
      const std::string& messagingId,
      const IMessagingSplitterService::MsgType& msgType,
      SmartConnectResult& smartConnectResult,
      const ComIqmeshNetworkSmartConnect& comSmartConnect
    )
    {
      Document response;

      // set common parameters
      Pointer("/mType").Set(response, msgType.m_type);
      Pointer("/data/msgId").Set(response, messagingId);

      // checking of error
      SmartConnectError error = smartConnectResult.getError();
      if (error.getType() != SmartConnectError::Type::NoError) {
        Pointer("/data/statusStr").Set(response, error.getMessage());

        switch (error.getType()) {
          case SmartConnectError::Type::MinDpaVerUsed:
            Pointer("/data/status").Set(response, SERVICE_ERROR_MIN_DPA_VER);
            break;
          case SmartConnectError::Type::SmartConnect:
            Pointer("/data/status").Set(response, SERVICE_ERROR_SMART_CONNECT);
            break;
          case SmartConnectError::Type::OsRead:
            Pointer("/data/status").Set(response, SERVICE_ERROR_OS_READ);
            break;
          default:
            // some other unsupported error
            Pointer("/data/status").Set(response, SERVICE_ERROR);
            break;
        }

        // set raw fields, if verbose mode is active
        if (comSmartConnect.getVerbose()) {
          setVerboseData(response, smartConnectResult);
        }

        return response;
      }

      // all is ok

      // data/rsp
      Pointer("/data/rsp/hwpId").Set(response, smartConnectResult.getHwpId());
      Pointer("/data/rsp/assignedAddr").Set(response, smartConnectResult.getBondedAddr());
      Pointer("/data/rsp/nodesNr").Set(response, smartConnectResult.getBondedNodesNum());
      Pointer("/data/rsp/manufacturer").Set(response, smartConnectResult.getManufacturer());
      Pointer("/data/rsp/product").Set(response, smartConnectResult.getProduct());

      // standards - array of strings
      rapidjson::Value standardsJsonArray(kArrayType);
      Document::AllocatorType& allocator = response.GetAllocator();
      for (std::string standard : smartConnectResult.getStandards()) {
        rapidjson::Value standardJsonString;
        standardJsonString.SetString(standard.c_str(), standard.length(), allocator);
        standardsJsonArray.PushBack(standardJsonString, allocator);
      }
      Pointer("/data/rsp/standards").Set(response, standardsJsonArray);

      // osRead object
      TPerOSRead_Response osInfo = smartConnectResult.getOsRead();

      // MID - little endian
      uint32_t mid = 0;
      for (int i = 0; i < MID_LEN; i++) {
        mid <<= 8;
        mid |= osInfo.ModuleId[i] & 0xFF;
      }
      Pointer("/data/rsp/osRead/mid").Set(response, mid);

      Pointer("/data/rsp/osRead/osVersion").Set(response, osInfo.OsVersion);
      Pointer("/data/rsp/osRead/trMcuType").Set(response, osInfo.McuType);
      Pointer("/data/rsp/osRead/osBuild").Set(response, osInfo.OsBuild);
      Pointer("/data/rsp/osRead/rssi").Set(response, osInfo.Rssi);
      Pointer("/data/rsp/osRead/supplyVoltage").Set(response, osInfo.SupplyVoltage);
      Pointer("/data/rsp/osRead/flags").Set(response, osInfo.Flags);
      Pointer("/data/rsp/osRead/slotLimits").Set(response, osInfo.SlotLimits);

      // IBK was added later in the version of DPA 3.03 
      rapidjson::Value ibkJsonArray(kArrayType);
      for (int i = 0; i < IBK_LEN; i++) {
        ibkJsonArray.PushBack(osInfo.IBK[i], allocator);
      }
      Pointer("/data/rsp/osRead/ibk").Set(response, ibkJsonArray);
      
      // status - ok
      Pointer("/status").Set(response, 0);
      Pointer("/statusStr").Set(response, "ok");

      // set raw fields, if verbose mode is active
      if (comSmartConnect.getVerbose()) {
        setVerboseData(response, smartConnectResult);
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

    uint8_t parseAndCheckDeviceAddr(int nadr) {
      if ((nadr < 0) || (nadr > 0xEF)) {
        THROW_EXC(
          std::out_of_range, "Device address outside of valid range. " << NAME_PAR_HEX("Address", nadr)
        );
      }
      return nadr;
    }

    uint16_t checkHwpId(int hwpId) {
      if ((hwpId < 0) || (hwpId > 0xFFFF)) {
        THROW_EXC(
          std::out_of_range, "HWP ID outside of valid range. " << NAME_PAR_HEX("HWP ID", hwpId)
        );
      }
      return hwpId;
    }

    uint8_t parseAndCheckBondingTestRetries(int bondingTestRetries) {
      if ((bondingTestRetries < 0) || (bondingTestRetries > 0xFF)) {
        THROW_EXC(
          std::out_of_range, "bondingTestRetries outside of valid range. " << NAME_PAR_HEX("bondingTestRetries", bondingTestRetries)
        );
      }
      return bondingTestRetries;
    }

    std::basic_string<uint8_t> checkIbk(const std::basic_string<uint8_t>& ibk) {
      if (ibk.length() != IBK_LEN) {
        THROW_EXC(
          std::out_of_range, "ibk length invalid. " << NAME_PAR_HEX("ibk length", ibk.length())
        );
      }
      return ibk;
    }

    std::basic_string<uint8_t> checkMid(const std::basic_string<uint8_t>& mid) {
      if (mid.length() != MID_LEN) {
        THROW_EXC(
          std::out_of_range, "mid length invalid. " << NAME_PAR_HEX("mid length", mid.length())
        );
      }
      return mid;
    }

    uint8_t checkBondingChannel(int bondingChannel) {
      if ((bondingChannel < 0) || (bondingChannel > 0xFF)) {
        THROW_EXC(
          std::out_of_range, "bondingChannel outside of valid range. " << NAME_PAR_HEX("bondingChannel", bondingChannel)
        );
      }
      return bondingChannel;
    }

    uint8_t parseAndCheckVirtualDeviceAddress(int virtualDeviceAddress) {
      if ((virtualDeviceAddress < 0) || (virtualDeviceAddress > 0xFF)) {
        THROW_EXC(
          std::out_of_range, "virtualDeviceAddress outside of valid range. " << NAME_PAR_HEX("virtualDeviceAddress", virtualDeviceAddress)
        );
      }
      return virtualDeviceAddress;
    }

    std::basic_string<uint8_t> parseAndCheckUserData(const std::basic_string<uint8_t>& userData) {
      if (userData.length() != USER_DATA_LEN) {
        THROW_EXC(
          std::out_of_range, "userData length invalid. " << NAME_PAR_HEX("userData length", userData.length())
        );
      }
      return userData;
    }

    // creates and return default user data
    std::basic_string<uint8_t> createDefaultUserData() {
      std::basic_string<uint8_t> defaultUserData;

      for (int i = 0; i < USER_DATA_LEN; i++) {
        defaultUserData.push_back(0);
      }
      return defaultUserData;
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
      if (msgType.m_type != m_mTypeName_iqmeshNetworkSmartConnect) {
        THROW_EXC(std::logic_error, "Unsupported message type: " << PAR(msgType.m_type));
      }

      // creating representation object
      ComIqmeshNetworkSmartConnect comSmartConnect(doc);

      // service input parameters
      uint8_t deviceAddr;
      uint8_t bondingTestRetries;
      std::string smartConnectCode;
      std::basic_string<uint8_t> userData;

      // stores values decoded from IQRF Code
      std::basic_string<uint8_t> mid;
      std::basic_string<uint8_t> ibk;
      uint16_t hwpId;
      uint8_t bondingChannel;

      // default values - will be specified later in the request json message
      uint8_t virtualDeviceAddress = 0xFF;


      // parsing and checking service parameters
      try {
        m_repeat = parseAndCheckRepeat(comSmartConnect.getRepeat());
       
        if (!comSmartConnect.isSetDeviceAddr()) {
          THROW_EXC(std::logic_error, "deviceAddr not set");
        }
        deviceAddr = parseAndCheckDeviceAddr(comSmartConnect.getDeviceAddr());

        bondingTestRetries = parseAndCheckBondingTestRetries(comSmartConnect.getBondingTestRetries());

        if (comSmartConnect.isSetUserData()) {
          userData = parseAndCheckUserData(comSmartConnect.getUserData());
        }
        else {
          userData = createDefaultUserData();
        }

        if (!comSmartConnect.isSetSmartConnectCode()) {
          THROW_EXC(std::logic_error, "smartConnectCode not set");
        }
        smartConnectCode = comSmartConnect.getSmartConnectCode();

        // decode IQRF Code
        IqrfCodeDecoder::decode(smartConnectCode);

        // get decoded values and check them
        mid = checkMid(IqrfCodeDecoder::getMid());
        ibk = checkIbk(IqrfCodeDecoder::getIbk());
        hwpId = checkHwpId(IqrfCodeDecoder::getHwpId());
        bondingChannel = checkBondingChannel(IqrfCodeDecoder::getBondingChannel());

        m_returnVerbose = comSmartConnect.getVerbose();
      }
      // parsing and checking service parameters failed 
      catch (std::exception& ex) {
        Document failResponse = createCheckParamsFailedResponse(comSmartConnect.getMsgId(), msgType, ex.what());
        m_iMessagingSplitterService->sendMessage(messagingId, std::move(failResponse));

        TRC_FUNCTION_LEAVE("");
        return;
      }
      
      // call service with checked params
      SmartConnectResult smartConnectResult = smartConnect(
        hwpId, deviceAddr, bondingTestRetries, ibk, mid, bondingChannel, virtualDeviceAddress, userData
      );

      // create and send response
      Document responseDoc = createResponse(comSmartConnect.getMsgId(), msgType, smartConnectResult, comSmartConnect);
      m_iMessagingSplitterService->sendMessage(messagingId, std::move(responseDoc));

      TRC_FUNCTION_LEAVE("");
    }
    

  public:
    void activate(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "************************************" << std::endl <<
        "SmartConnectService instance activate" << std::endl <<
        "************************************"
      );

      // for the sake of register function parameters 
      std::vector<std::string> m_filters =
      {
        "iqmeshNetwork_SmartConnect"
      };

      m_iMessagingSplitterService->registerFilteredMsgHandler(
        m_filters,
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
        "SmartConnectService instance deactivate" << std::endl <<
        "**************************************"
      );

      // for the sake of unregister function parameters 
      std::vector<std::string> m_filters =
      {
        m_mTypeName_iqmeshNetworkSmartConnect
      };

      m_iMessagingSplitterService->unregisterFilteredMsgHandler(m_filters);

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



  SmartConnectService::SmartConnectService()
  {
    m_imp = shape_new Imp(*this);
  }

  SmartConnectService::~SmartConnectService()
  {
    delete m_imp;
  }


  void SmartConnectService::attachInterface(iqrf::IIqrfDpaService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void SmartConnectService::detachInterface(iqrf::IIqrfDpaService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void SmartConnectService::attachInterface(iqrf::IMessagingSplitterService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void SmartConnectService::detachInterface(iqrf::IMessagingSplitterService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void SmartConnectService::attachInterface(iqrf::IJsCacheService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void SmartConnectService::detachInterface(iqrf::IJsCacheService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void SmartConnectService::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void SmartConnectService::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }


  void SmartConnectService::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void SmartConnectService::deactivate()
  {
    m_imp->deactivate();
  }

  void SmartConnectService::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
  }

}