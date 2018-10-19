#define ISmartConnectService_EXPORTS

#include "SmartConnectService.h"
#include "Trace.h"
#include "ComIqmeshNetworkSmartConnect.h"
#include "IqrfCodeDecoder.h"
#include "iqrf__SmartConnectService.hxx"
#include <list>
#include <math.h>
#include <thread>

TRC_INIT_MODULE(iqrf::SmartConnectService);


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

  static const int SERVICE_ERROR_INTERNAL = SERVICE_ERROR + 1;

  static const int SERVICE_ERROR_MIN_DPA_VER = SERVICE_ERROR + 2;
  static const int SERVICE_ERROR_SMART_CONNECT = SERVICE_ERROR + 3;
  static const int SERVICE_ERROR_OS_READ = SERVICE_ERROR + 4;
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
    std::list<std::string> m_standards = { "" };

    // OS read response data
    std::vector<uns8> m_osRead;

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

    const std::vector<uns8> getOsRead() {
      return m_osRead;
    }

    void setOsRead(const uns8* readInfo) {
      m_osRead.insert(m_osRead.begin(), readInfo, readInfo + DPA_MAX_DATA_LENGTH);
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

    IJsCacheService* m_iJsCacheService = nullptr;
    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    IIqrfDpaService* m_iIqrfDpaService = nullptr;
    std::unique_ptr<IIqrfDpaService::ExclusiveAccess> m_exclusiveAccess;

    // number of repeats
    uint8_t m_repeat;

    // if is set Verbose mode
    bool m_returnVerbose = false;


  public:
    Imp(SmartConnectService& parent) : m_parent(parent)
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
   
    void _smartConnect(
      SmartConnectResult& smartConnectResult,
      const uint16_t hwpId,
      const uint8_t reqAddr,
      const uint8_t bondingTestRetries,
      const std::basic_string<uint8_t>& ibk,
      const std::basic_string<uint8_t>& mid,
      const uint8_t virtualDeviceAddress,
      const std::basic_string<uint8_t>& userData
    ) 
    {
      TRC_FUNCTION_ENTER("");
      
      DpaMessage smartConnectRequest;
      DpaMessage::DpaPacket_t smartConnectPacket;
      smartConnectPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      smartConnectPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
      smartConnectPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_SMART_CONNECT;
      smartConnectPacket.DpaRequestPacket_t.HWPID = hwpId;
      // Set pData fields
      uns8* pData = smartConnectPacket.DpaRequestPacket_t.DpaMessage.Request.PData;

      // Copy ReqAddr, BondingTestRetries
      pData[0] = reqAddr;
      pData[1] = bondingTestRetries;
      // Copy IBK
      std::copy( ibk.begin(), ibk.end(), pData + 2 );
      // Copy MID
      std::basic_string<uint8_t> reversed_mid = mid;
      std::reverse( reversed_mid.begin(), reversed_mid.end() );
      std::copy( reversed_mid.begin(), reversed_mid.end(), pData + 18 );
      // Set res0 to zero
      pData[22] = 0x00;
      pData[23] = 0x00;
      // Copy VirtualDeviceAddress
      pData[24] = virtualDeviceAddress;
      // Fill res1 with zeros
      for ( int i = 0; i < 9; i++ ) {
        pData[i + 25] = 0;
      }
      // Copy UserData
      std::copy( userData.begin(), userData.end(), pData + 34 );
      // Data to buffer
      smartConnectRequest.DataToBuffer(smartConnectPacket.Buffer, sizeof(TDpaIFaceHeader) + 38);

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> smartConnectTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      for (int rep = 0; rep <= m_repeat; rep++) {
        try {
          //smartConnectTransaction = m_iIqrfDpaService->executeDpaTransaction(smartConnectRequest, 11000);
          smartConnectTransaction = m_exclusiveAccess->executeDpaTransaction(smartConnectRequest, 0);
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

          // parsing response pdata
          smartConnectResult.setHwpId( dpaResponse.DpaPacket().DpaResponsePacket_t.HWPID );
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
          //perEnumTransaction = m_iIqrfDpaService->executeDpaTransaction(perEnumRequest);
          perEnumTransaction = m_exclusiveAccess->executeDpaTransaction(perEnumRequest);
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
    void osRead( SmartConnectResult& smartConnectResult ) {
      TRC_FUNCTION_ENTER( "" );

      DpaMessage osReadRequest;
      DpaMessage::DpaPacket_t osReadPacket;
      osReadPacket.DpaRequestPacket_t.NADR = smartConnectResult.getBondedAddr();
      osReadPacket.DpaRequestPacket_t.PNUM = PNUM_OS;
      osReadPacket.DpaRequestPacket_t.PCMD = CMD_OS_READ;
      osReadPacket.DpaRequestPacket_t.HWPID = smartConnectResult.getHwpId();
      osReadRequest.DataToBuffer( osReadPacket.Buffer, sizeof( TDpaIFaceHeader ) );

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> osReadTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      for ( int rep = 0; rep <= m_repeat; rep++ ) {
        try {
          //osReadTransaction = m_iIqrfDpaService->executeDpaTransaction( osReadRequest );
          osReadTransaction = m_exclusiveAccess->executeDpaTransaction(osReadRequest);
          transResult = osReadTransaction->get();
        }
        catch ( std::exception& e ) {
          TRC_DEBUG( "DPA transaction error : " << e.what() );

          if ( rep < m_repeat ) {
            continue;
          }

          SmartConnectError error( SmartConnectError::Type::OsRead, e.what() );
          smartConnectResult.setError( error );

          TRC_FUNCTION_LEAVE( "" );
          return;
        }

        TRC_DEBUG( "Result from OS read transaction as string:" << PAR( transResult->getErrorString() ) );

        IDpaTransactionResult2::ErrorCode errorCode = ( IDpaTransactionResult2::ErrorCode )transResult->getErrorCode();

        // because of the move-semantics
        DpaMessage dpaResponse = transResult->getResponse();
        smartConnectResult.addTransactionResult( transResult );

        if ( errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK ) {
          TRC_INFORMATION( "OS read successful!" );
          TRC_DEBUG(
            "DPA transaction: "
            << NAME_PAR( osReadRequest.PeripheralType(), osReadRequest.NodeAddress() )
            << PAR( osReadRequest.PeripheralCommand() )
          );

          // get OS data
          uns8* osData = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData;
          smartConnectResult.setOsRead( osData );

          TRC_FUNCTION_LEAVE( "" );
          return;
        }

        // transaction error
        if ( errorCode < 0 ) {
          TRC_DEBUG( "Transaction error. " << NAME_PAR_HEX( "Error code", errorCode ) );

          if ( rep < m_repeat ) {
            continue;
          }

          SmartConnectError error( SmartConnectError::Type::OsRead, "Transaction error." );
          smartConnectResult.setError( error );

          TRC_FUNCTION_LEAVE( "" );
          return;
        }

        // DPA error
        TRC_DEBUG( "DPA error. " << NAME_PAR_HEX( "Error code", errorCode ) );

        if ( rep < m_repeat ) {
          continue;
        }

        SmartConnectError error( SmartConnectError::Type::OsRead, "Dpa error." );
        smartConnectResult.setError( error );

        TRC_FUNCTION_LEAVE( "" );
      }
    }

    SmartConnectResult smartConnect(
      const uint16_t hwpId,
      const uint8_t reqAddr,
      const uint8_t bondingTestRetries,
      const std::basic_string<uint8_t>& ibk,
      const std::basic_string<uint8_t>& mid,
      const uint8_t virtualDeviceAddress,
      const std::basic_string<uint8_t>& userData
    )
    {
      TRC_FUNCTION_ENTER( "" );

      // result
      SmartConnectResult smartConnectResult;

      // check, if there is at minimal DPA ver. 3.03 at the coordinator
      uint16_t dpaVersion = 0;
      try {
        dpaVersion = getDpaVersion( HWPID_DoNotCheck );
        if ( dpaVersion < DPA_MIN_REQ_VERSION ) {
          THROW_EXC( std::logic_error, "Old version of DPA: " << PAR( dpaVersion ) );
        }
      }
      catch ( std::exception& ex ) {
        SmartConnectError error( SmartConnectError::Type::MinDpaVerUsed, ex.what() );
        smartConnectResult.setError( error );
        return smartConnectResult;
      }

      // do smart connect
      _smartConnect( smartConnectResult, HWPID_DoNotCheck, reqAddr, bondingTestRetries, ibk, mid, virtualDeviceAddress, userData );

      // if there was an error, return
      if ( smartConnectResult.getError().getType() != SmartConnectError::Type::NoError ) {
        return smartConnectResult;
      }

      // Delay after successful bonding
      std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );

      // get OS read data
      smartConnectResult.setHwpId( hwpId );
      osRead( smartConnectResult );

      // if there was an error, return
      if ( smartConnectResult.getError().getType() != SmartConnectError::Type::NoError ) {
        return smartConnectResult;
      }

      const IJsCacheService::Manufacturer* manufacturer = m_iJsCacheService->getManufacturer( hwpId );
      if ( manufacturer != nullptr ) {
        smartConnectResult.setManufacturer( manufacturer->m_name );
      }

      const IJsCacheService::Product* product = m_iJsCacheService->getProduct( hwpId );
      if ( product != nullptr ) {
        smartConnectResult.setProduct( product->m_name );
      }

      uint8_t osVersion = smartConnectResult.getOsRead()[4];
      std::string osVersionStr = std::to_string( ( osVersion >> 4 ) & 0xFF ) + "." + std::to_string( osVersion & 0x0F );
      std::string dpaVersionStr = std::to_string( ( dpaVersion >> 8 ) & 0xFF ) + "." + std::to_string( dpaVersion & 0xFF );
      const IJsCacheService::Package* package = m_iJsCacheService->getPackage( hwpId, osVersionStr, dpaVersionStr );
      if ( package != nullptr ) {
        std::list<std::string> standards;
        for ( const IJsCacheService::StdDriver* driver : package->m_stdDriverVect ) {
          standards.push_back( driver->getName() );
        }
        smartConnectResult.setStandards( standards );
      }

      TRC_FUNCTION_LEAVE( "" );
      return smartConnectResult;
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
      Pointer( "/mType" ).Set( response, msgType.m_type );
      Pointer( "/data/msgId" ).Set( response, messagingId );

      // checking of error
      SmartConnectError error = smartConnectResult.getError();
      if ( error.getType() != SmartConnectError::Type::NoError ) {
        Pointer( "/data/statusStr" ).Set( response, error.getMessage() );

        switch ( error.getType() ) {
          case SmartConnectError::Type::MinDpaVerUsed:
            Pointer( "/data/status" ).Set( response, SERVICE_ERROR_MIN_DPA_VER );
            break;
          case SmartConnectError::Type::SmartConnect:
            Pointer( "/data/status" ).Set( response, SERVICE_ERROR_SMART_CONNECT );
            break;
          case SmartConnectError::Type::OsRead:
            Pointer( "/data/status" ).Set( response, SERVICE_ERROR_OS_READ );
            break;
          default:
            // some other unsupported error
            Pointer( "/data/status" ).Set( response, SERVICE_ERROR );
            break;
        }

        // set raw fields, if verbose mode is active
        if ( comSmartConnect.getVerbose() ) {
          setVerboseData( response, smartConnectResult );
        }

        return response;
      }

      // all is ok

      // rsp object
      rapidjson::Pointer( "/data/rsp/assignedAddr" ).Set( response, smartConnectResult.getBondedAddr() );
      rapidjson::Pointer( "/data/rsp/nodesNr" ).Set( response, smartConnectResult.getBondedNodesNum() );
      rapidjson::Pointer( "/data/rsp/hwpId" ).Set( response, smartConnectResult.getHwpId() );

      // manufacturer name and product name
      rapidjson::Pointer( "/data/rsp/manufacturer" ).Set( response, smartConnectResult.getManufacturer() );
      rapidjson::Pointer( "/data/rsp/product" ).Set( response, smartConnectResult.getProduct() );
      // standards - array of strings
      rapidjson::Value standardsJsonArray( kArrayType );
      Document::AllocatorType& allocator = response.GetAllocator();
      for ( std::string standard : smartConnectResult.getStandards() ) {
        rapidjson::Value standardJsonString;
        standardJsonString.SetString( standard.c_str(), standard.length(), allocator );
        standardsJsonArray.PushBack( standardJsonString, allocator );
      }
      Pointer( "/data/rsp/standards" ).Set( response, standardsJsonArray );

      // osRead object
      const std::vector<uns8> readInfo = smartConnectResult.getOsRead();

      // MID - hex string without separator
      std::ostringstream moduleId;
      moduleId.fill('0');
      moduleId << std::hex << std::uppercase << 
        std::setw(2) << (int)readInfo[3] <<
        std::setw(2) << (int)readInfo[2] <<
        std::setw(2) << (int)readInfo[1] <<
        std::setw(2) << (int)readInfo[0];
      rapidjson::Pointer("/data/rsp/osRead/mid").Set(response, moduleId.str());

      // OS version - string
      std::ostringstream osVer;
      osVer << std::hex << (int)(readInfo[4] >> 4) << '.';
      osVer.fill('0');
      osVer << std::setw(2) << (int)(readInfo[4] & 0xf) << 'D';
      rapidjson::Pointer("/data/rsp/osRead/osVersion").Set(response, osVer.str());

      // trMcuType
      uns8 trMcuType = readInfo[5];

      rapidjson::Pointer("/data/rsp/osRead/trMcuType/value").Set(response, trMcuType);
      std::string trTypeStr = "(DC)TR-";
      switch (trMcuType >> 4) {
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
      rapidjson::Pointer("/data/rsp/osRead/trMcuType/trType").Set(response, trTypeStr);
      bool fccCertified = ((trMcuType & 0x08) == 0x08) ? true : false;
      rapidjson::Pointer("/data/rsp/osRead/trMcuType/fccCertified").Set(response, fccCertified);
      std::string mcuTypeStr = ((trMcuType & 0x07) == 0x04) ? "PIC16LF1938" : "UNKNOWN";
      rapidjson::Pointer("/data/rsp/osRead/trMcuType/mcuType").Set(response, mcuTypeStr);

      // OS build - string
      uint16_t osBuild = (readInfo[7] << 8) + readInfo[6];
      rapidjson::Pointer("/data/rsp/osRead/osBuild").Set(response, encodeHexaNum_CapitalLetters(osBuild));

      // RSSI [dBm]
      int8_t rssi = readInfo[8] - 130;
      std::string rssiStr = std::to_string(rssi) + " dBm";
      rapidjson::Pointer("/data/rsp/osRead/rssi").Set(response, rssiStr);

      // Supply voltage [V]
      float supplyVoltage = 261.12f / (float)(127 - readInfo[9]);
      char supplyVoltageStr[8];
      std::sprintf(supplyVoltageStr, "%1.2f V", supplyVoltage);
      rapidjson::Pointer("/data/rsp/osRead/supplyVoltage").Set(response, supplyVoltageStr);

      // Flags
      uns8 flags = readInfo[10];

      rapidjson::Pointer("/data/rsp/osRead/flags/value").Set(response, flags);
      bool insufficientOsBuild = ((flags & 0x01) == 0x01) ? true : false;
      rapidjson::Pointer("/data/rsp/osRead/flags/insufficientOsBuild").Set(response, insufficientOsBuild);
      std::string iface = ((flags & 0x02) == 0x02) ? "UART" : "SPI";
      rapidjson::Pointer("/data/rsp/osRead/flags/interfaceType").Set(response, iface);
      bool dpaHandlerDetected = ((flags & 0x04) == 0x04) ? true : false;
      rapidjson::Pointer("/data/rsp/osRead/flags/dpaHandlerDetected").Set(response, dpaHandlerDetected);
      bool dpaHandlerNotDetectedButEnabled = ((flags & 0x08) == 0x08) ? true : false;
      rapidjson::Pointer("/data/rsp/osRead/flags/dpaHandlerNotDetectedButEnabled").Set(response, dpaHandlerNotDetectedButEnabled);
      bool noInterfaceSupported = ((flags & 0x10) == 0x10) ? true : false;
      rapidjson::Pointer("/data/rsp/osRead/flags/noInterfaceSupported").Set(response, noInterfaceSupported);

      // SlotLimits
      uns8 slotLimits = readInfo[11];

      rapidjson::Pointer("/data/rsp/osRead/slotLimits/value").Set(response, slotLimits);
      uint8_t shortestTimeSlot = ((slotLimits & 0x0f) + 3) * 10;
      uint8_t longestTimeSlot = (((slotLimits >> 0x04) & 0x0f) + 3) * 10;
      rapidjson::Pointer("/data/rsp/osRead/slotLimits/shortestTimeslot").Set(response, std::to_string(shortestTimeSlot) + " ms");
      rapidjson::Pointer("/data/rsp/osRead/slotLimits/longestTimeslot").Set(response, std::to_string(longestTimeSlot) + " ms");

      // ibk - only if DPA version is >= 3.03
      // getting DPA version
      IIqrfDpaService::CoordinatorParameters coordParams = m_iIqrfDpaService->getCoordinatorParameters();
      uint16_t dpaVer = (coordParams.dpaVerMajor << 8) + coordParams.dpaVerMinor;

      if (dpaVer >= 0x0303) {
        Document::AllocatorType& allocator = response.GetAllocator();
        rapidjson::Value ibkBitsJsonArray(kArrayType);
        for (int i = 0; i < 16; i++) {
          ibkBitsJsonArray.PushBack(readInfo[12 + i], allocator);
        }
        Pointer("/data/rsp/osRead/ibk").Set(response, ibkBitsJsonArray);
      }

      // set raw fields, if verbose mode is active
      if ( comSmartConnect.getVerbose() ) {
        setVerboseData( response, smartConnectResult );
      }

      // status - ok
      Pointer( "/data/status" ).Set( response, 0 );
      Pointer( "/data/statusStr" ).Set( response, "ok" );

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

    // creates string of HEX characters for specified byte stream
    std::string getHexaString(const std::basic_string<uint8_t>& byteStream)
    {
      std::ostringstream os;
      
      for ( const uint8_t byte : byteStream ) {
        os << std::setfill('0') << std::setw(2) << std::hex << (int)byte;
        os << " ";
      }
      return os.str();
    }

    // creates string of HEX characters for specified byte stream
    std::string getHexaString(const uint16_t val)
    {
      std::ostringstream os;

      os << std::setfill('0') << std::setw(2) << std::hex << (int)((val >> 8) & 0xFF);
      os << " ";
      os << std::setfill('0') << std::setw(2) << std::hex << (int)(val & 0xFF);

      return os.str();
    }

    // returns reversed byte sequence to the one in the parameter
    std::basic_string<uint8_t> getReversedBytes(const std::basic_string<uint8_t>& bytes) 
    {
      std::basic_string<uint8_t> reversedBytes(bytes);
      std::reverse(reversedBytes.begin(), reversedBytes.end());
      return reversedBytes;
    }

    void logDecodedValues(
      const std::basic_string<uint8_t>& mid, 
      const std::basic_string<uint8_t>& ibk,
      uint16_t hwpId
    ) 
    {
      TRC_INFORMATION("IQRFCode decoded values: ");
      
      TRC_INFORMATION("MID: " << PAR(getHexaString(mid)));
      TRC_INFORMATION("IBK: " << PAR(getHexaString(ibk)));
      TRC_INFORMATION("HWP ID: " << PAR(getHexaString(hwpId)));
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

        // logs decoded values
        logDecodedValues(mid, ibk, hwpId );
       
        m_returnVerbose = comSmartConnect.getVerbose();
      }
      // parsing and checking service parameters failed 
      catch (std::exception& ex) {
        Document failResponse = createCheckParamsFailedResponse(comSmartConnect.getMsgId(), msgType, ex.what());
        m_iMessagingSplitterService->sendMessage(messagingId, std::move(failResponse));

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

        Document failResponse = getExclusiveAccessFailedResponse(comSmartConnect.getMsgId(), msgType, errorStr);
        m_iMessagingSplitterService->sendMessage(messagingId, std::move(failResponse));

        TRC_FUNCTION_LEAVE("");
        return;
      }

      // call service with checked params
      SmartConnectResult smartConnectResult = smartConnect(
        hwpId, deviceAddr, bondingTestRetries, ibk, mid, virtualDeviceAddress, userData
      );

      // release exclusive access
      m_exclusiveAccess.reset();

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