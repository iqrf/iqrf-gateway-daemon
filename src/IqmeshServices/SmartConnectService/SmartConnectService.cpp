#define ISmartConnectService_EXPORTS

#include "SmartConnectService.h"
#include "RawDpaEmbedOS.h"
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
  static const int SERVICE_ERROR_HWPID_VERSION = SERVICE_ERROR + 5;
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
      OsRead,
      HwpIdVersion
    };

    SmartConnectError() : m_type(Type::NoError), m_message("") {};
    explicit SmartConnectError(Type errorType) : m_type(errorType), m_message("") {};
    SmartConnectError(Type errorType, const std::string& message) : m_type(errorType), m_message(message) {};

    Type getType() const { return m_type; };
    std::string getMessage() const { return m_message; };

    SmartConnectError(const SmartConnectError& other) {
      m_type = other.getType();
      m_message = other.getMessage();
    }

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
    uint16_t m_hwpIdVer;
    uint8_t m_bondedAddr;
    uint8_t m_bondedNodesNum;
    std::string m_manufacturer = "";
    std::string m_product = "";
    std::list<std::string> m_standards = { "" };

    // OS read response data
    embed::os::RawDpaReadPtr m_osRead;
    uint16_t m_osBuild;

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

    void setHwpId(uint16_t hwpId) {
      this->m_hwpId = hwpId;
    }

    uint16_t getHwpIdVersion() const {
      return m_hwpIdVer;
    }

    void setHwpIdVersion(uint16_t hwpIdVer) {
      this->m_hwpIdVer = hwpIdVer;
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

    // adds transaction result into the list of results
    void addTransactionResult(std::unique_ptr<IDpaTransactionResult2> transResult) {
      if( transResult != nullptr )
        m_transResults.push_back(std::move(transResult));
    }

    // adds transaction result into the list of results
    void addTransactionResultRef(std::unique_ptr<IDpaTransactionResult2> &transResult)
    {
      if( transResult != nullptr )
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
      return tranResult;
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
        catch (const std::exception& e) {
          TRC_WARNING("DPA transaction error : " << e.what());

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
        smartConnectResult.addTransactionResultRef( transResult );

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
          TRC_WARNING("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

          if (rep < m_repeat) {
            continue;
          }

          SmartConnectError error(SmartConnectError::Type::SmartConnect, "Transaction error.");
          smartConnectResult.setError(error);

          TRC_FUNCTION_LEAVE("");
          return;
        }

        // DPA error
        TRC_WARNING("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

        if (rep < m_repeat) {
          continue;
        }

        smartConnectResult.setHwpId(dpaResponse.DpaPacket().DpaResponsePacket_t.HWPID);
        SmartConnectError error(SmartConnectError::Type::SmartConnect, "Dpa error.");
        smartConnectResult.setError(error);

        TRC_FUNCTION_LEAVE("");
      }
    }

    // gets HWP ID version of specified node address and sets it into the result
    void getHwpIdVersion(
      SmartConnectResult& smartConnectResult, 
      const uint16_t nodeAddr
    ) 
    {
      TRC_FUNCTION_ENTER("");

      DpaMessage perEnumRequest;
      DpaMessage::DpaPacket_t perEnumPacket;
      perEnumPacket.DpaRequestPacket_t.NADR = nodeAddr;
      perEnumPacket.DpaRequestPacket_t.PNUM = 0xFF;
      perEnumPacket.DpaRequestPacket_t.PCMD = 0x3F;
      perEnumPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
      perEnumRequest.DataToBuffer(perEnumPacket.Buffer, sizeof(TDpaIFaceHeader));

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> perEnumTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      for (int rep = 0; rep <= m_repeat; rep++) {
        try {
          perEnumTransaction = m_exclusiveAccess->executeDpaTransaction(perEnumRequest);
          transResult = perEnumTransaction->get();
        }
        catch (const std::exception& e) {
          TRC_WARNING("DPA transaction error : " << e.what());

          if (rep < m_repeat) {
            continue;
          }

          SmartConnectError error(SmartConnectError::Type::HwpIdVersion, e.what());
          smartConnectResult.setError(error);

          TRC_FUNCTION_LEAVE("");
          return;
        }

        TRC_DEBUG("Result from smart connect transaction as string:" << PAR(transResult->getErrorString()));

        IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

        // because of the move-semantics
        DpaMessage dpaResponse = transResult->getResponse();
        smartConnectResult.addTransactionResultRef( transResult );

        if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
          TRC_INFORMATION("Device exploration successful!");
          TRC_DEBUG(
            "DPA transaction: "
            << NAME_PAR(perEnumRequest.PeripheralType(), perEnumRequest.NodeAddress())
            << PAR(perEnumRequest.PeripheralCommand())
          );

          // parsing response pdata
          uns8* respData = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData;
          uint8_t minorHwpIdVer = respData[9];
          uint8_t majorHwpIdVer = respData[10];

          smartConnectResult.setHwpIdVersion(minorHwpIdVer + (majorHwpIdVer << 8));

          TRC_FUNCTION_LEAVE("");
          return;
        }

        // transaction error
        if (errorCode < 0) {
          TRC_WARNING("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

          if (rep < m_repeat) {
            continue;
          }

          SmartConnectError error(SmartConnectError::Type::HwpIdVersion, "Transaction error.");
          smartConnectResult.setError(error);

          TRC_FUNCTION_LEAVE("");
          return;
        }

        // DPA error
        TRC_WARNING("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

        if (rep < m_repeat) {
          continue;
        }

        SmartConnectError error(SmartConnectError::Type::HwpIdVersion, "Dpa error.");
        smartConnectResult.setError(error);

        TRC_FUNCTION_LEAVE("");
      }
    }

    // reads OS info about smart connected node
    void osRead( SmartConnectResult& smartConnectResult ) {
      TRC_FUNCTION_ENTER( "" );

      std::unique_ptr<embed::os::RawDpaRead> osReadPtr( shape_new embed::os::RawDpaRead(smartConnectResult.getBondedAddr()) );
      std::unique_ptr<IDpaTransactionResult2> transResultPtr;

      try
      {
        m_exclusiveAccess->executeDpaTransactionRepeat( osReadPtr->getRequest(), transResultPtr, m_repeat );
        osReadPtr->processDpaTransactionResult( std::move(transResultPtr) );
        TRC_DEBUG("Result from OS read transaction as string:" << PAR( osReadPtr->getResult()->getErrorString()) );
        smartConnectResult.setOsBuild( osReadPtr->getOsBuild() );
        smartConnectResult.setHwpId( osReadPtr->getHwpid() );
        smartConnectResult.addTransactionResult( osReadPtr->getResultMove() );
        smartConnectResult.setOsRead( osReadPtr );
        TRC_INFORMATION( "OS read successful!" );
      }
      catch (const std::exception &e) {
        SmartConnectError error(SmartConnectError::Type::OsRead, e.what());
        smartConnectResult.setError(error);
      }

      TRC_FUNCTION_LEAVE("");
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
        IIqrfDpaService::CoordinatorParameters coordParams = m_iIqrfDpaService->getCoordinatorParameters();
        dpaVersion = (coordParams.dpaVerMajor << 8) + coordParams.dpaVerMinor;

        if ( dpaVersion < DPA_MIN_REQ_VERSION ) {
          THROW_EXC( std::logic_error, "Old version of DPA: " << PAR( dpaVersion ) );
        }
      }
      catch ( const std::exception& ex ) {
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
      std::this_thread::sleep_for( std::chrono::milliseconds( 250 ) );

      // get OS read data
      smartConnectResult.setHwpId( hwpId );

      // if successfull, it sets hwp ID to the newly connected node
      osRead( smartConnectResult );

      // if there was an error, return
      if ( smartConnectResult.getError().getType() != SmartConnectError::Type::NoError ) {
        return smartConnectResult;
      }

      const IJsCacheService::Manufacturer* manufacturer = m_iJsCacheService->getManufacturer(smartConnectResult.getHwpId() );
      if ( manufacturer != nullptr ) {
        smartConnectResult.setManufacturer( manufacturer->m_name );
      }

      const IJsCacheService::Product* product = m_iJsCacheService->getProduct(smartConnectResult.getHwpId());
      if ( product != nullptr ) {
        smartConnectResult.setProduct( product->m_name );
      }

      //uint8_t osVersion = smartConnectResult.getOsRead()[4];
      //std::string osVersionStr = std::to_string( ( osVersion >> 4 ) & 0xFF ) + "." + std::to_string( osVersion & 0x0F );
      std::string osBuildStr;
      {
        std::ostringstream os;
        os.fill('0');
        os << std::hex << std::uppercase << std::setw(4) << (int)smartConnectResult.getOsBuild();
        osBuildStr = os.str();
      }
      
      // getting hwpId version
      getHwpIdVersion(smartConnectResult, smartConnectResult.getBondedAddr());

      // if there was an error, return
      if (smartConnectResult.getError().getType() != SmartConnectError::Type::NoError) {
        return smartConnectResult;
      }

      const IJsCacheService::Package* package = m_iJsCacheService->getPackage(
        smartConnectResult.getHwpId(),
        smartConnectResult.getHwpIdVersion(),
        osBuildStr,
        m_iIqrfDpaService->getCoordinatorParameters().dpaVerWordAsStr
      );
      if ( package != nullptr ) {
        std::list<std::string> standards;
        for ( const IJsCacheService::StdDriver* driver : package->m_stdDriverVect ) {
          standards.push_back( driver->getName() );
        }
        smartConnectResult.setStandards( standards );
      }
      else {
        TRC_INFORMATION("Package not found");
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

    // sets OS Read data to specified JSON message
    void setOsReadSection( const iqrf::embed::os::RawDpaReadPtr& osReadObject, Document& response ) 
    {
      rapidjson::Pointer( "/data/rsp/osRead/mid" ).Set( response, osReadObject->getMidAsString() );
      rapidjson::Pointer( "/data/rsp/osRead/osVersion" ).Set( response, osReadObject->getOsVersionAsString() );

      rapidjson::Pointer( "/data/rsp/osRead/trMcuType/value" ).Set( response, osReadObject->getTrMcuType() );
      rapidjson::Pointer( "/data/rsp/osRead/trMcuType/trType" ).Set( response, osReadObject->getTrTypeAsString() );
      rapidjson::Pointer( "/data/rsp/osRead/trMcuType/fccCertified" ).Set( response, osReadObject->isFccCertified() );
      rapidjson::Pointer( "/data/rsp/osRead/trMcuType/mcuType" ).Set( response, osReadObject->getTrMcuTypeAsString() );

      rapidjson::Pointer( "/data/rsp/osRead/osBuild" ).Set( response, osReadObject->getOsBuildAsString() );

      // RSSI [dBm]
      rapidjson::Pointer( "/data/rsp/osRead/rssi" ).Set( response, osReadObject->getRssiAsString() );

      // Supply voltage [V]
      rapidjson::Pointer( "/data/rsp/osRead/supplyVoltage" ).Set( response, osReadObject->getSupplyVoltageAsString() );

      // Flags
      rapidjson::Pointer( "/data/rsp/osRead/flags/value" ).Set( response, osReadObject->getFlags() );
      rapidjson::Pointer( "/data/rsp/osRead/flags/insufficientOsBuild" ).Set( response, osReadObject->isInsufficientOsBuild() );
      rapidjson::Pointer( "/data/rsp/osRead/flags/interfaceType" ).Set( response, osReadObject->getInterfaceAsString() );
      rapidjson::Pointer( "/data/rsp/osRead/flags/dpaHandlerDetected" ).Set( response, osReadObject->isDpaHandlerDetected() );
      rapidjson::Pointer( "/data/rsp/osRead/flags/dpaHandlerNotDetectedButEnabled" ).Set( response, osReadObject->isDpaHandlerNotDetectedButEnabled() );
      rapidjson::Pointer( "/data/rsp/osRead/flags/noInterfaceSupported" ).Set( response, osReadObject->isNoInterfaceSupported() );
      if ( m_iIqrfDpaService->getCoordinatorParameters().dpaVerWord >= 0x0413 )
        rapidjson::Pointer( "/data/rsp/osRead/flags/iqrfOsChanged" ).Set( response, osReadObject->isIqrfOsChanges() );

      // Slot limits
      rapidjson::Pointer( "/data/rsp/osRead/slotLimits/value" ).Set( response, osReadObject->getSlotLimits() );
      rapidjson::Pointer( "/data/rsp/osRead/slotLimits/shortestTimeslot" ).Set( response, osReadObject->getShortestTimeSlotAsString() );
      rapidjson::Pointer( "/data/rsp/osRead/slotLimits/longestTimeslot" ).Set( response, osReadObject->getLongestTimeSlotAsString() );

      if ( osReadObject->is410Compliant() )
      {
        // dpaVer, perNr
        Pointer("/data/rsp/osRead/dpaVer").Set(response, osReadObject->getDpaVerAsString());
        Pointer("/data/rsp/osRead/perNr").Set(response, osReadObject->getPerNr());

        Document::AllocatorType& allocator = response.GetAllocator();

        // embPers
        rapidjson::Value embPersJsonArray(kArrayType);
        for (std::set<int>::iterator it = osReadObject->getEmbedPer().begin(); it != osReadObject->getEmbedPer().end(); ++it)
        {
          embPersJsonArray.PushBack(*it, allocator);
        }
        Pointer("/data/rsp/osRead/embPers").Set(response, embPersJsonArray);

        // hwpId
        Pointer("/data/rsp/osRead/hwpId").Set(response, osReadObject->getHwpid());

        // hwpIdVer
        Pointer("/data/rsp/osRead/hwpIdVer").Set(response, osReadObject->getHwpidVer());

        // flags - int value
        Pointer("/data/rsp/osRead/enumFlags/value").Set(response, osReadObject->getFlags());

        // flags - parsed
        bool stdModeSupported = (osReadObject->getFlags() & 0b1) == 0b1;
        if (stdModeSupported)
        {
          Pointer("/data/rsp/osRead/enumFlags/rfModeStd").Set(response, true);
          Pointer("/data/rsp/osRead/enumFlags/rfModeLp").Set(response, false);
        }
        else
        {
          Pointer("/data/rsp/osRead/enumFlags/rfModeStd").Set(response, false);
          Pointer("/data/rsp/osRead/enumFlags/rfModeLp").Set(response, true);
        }

        // STD+LP network is running, otherwise STD network.
        if (osReadObject->getDpaVer() >= 0x0400)
        {
          bool stdAndLpModeNetwork = (osReadObject->getFlags() & 0b100) == 0b100;
          if (stdAndLpModeNetwork)
          {
            Pointer("/data/rsp/osRead/enumFlags/stdAndLpNetwork").Set(response, true);
          }
          else
          {
            Pointer("/data/rsp/osRead/enumFlags/stdAndLpNetwork").Set(response, false);
          }
        }

        // UserPers
        rapidjson::Value userPerJsonArray(kArrayType);
        for (std::set<int>::iterator it = osReadObject->getUserPer().begin(); it != osReadObject->getUserPer().end(); ++it)
        {
          userPerJsonArray.PushBack(*it, allocator);
        }
        Pointer("/data/rsp/osRead/userPers").Set(response, userPerJsonArray);
      }
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

      if ( error.getType() != SmartConnectError::Type::NoError ) 
      {
        int status = SERVICE_ERROR;

        switch ( error.getType() ) {
          case SmartConnectError::Type::MinDpaVerUsed:
            status = SERVICE_ERROR_MIN_DPA_VER;
            break;

          case SmartConnectError::Type::SmartConnect:
            status = SERVICE_ERROR_SMART_CONNECT;
            break;

          case SmartConnectError::Type::OsRead:
            Pointer("/data/rsp/assignedAddr").Set(response, smartConnectResult.getBondedAddr());
            Pointer("/data/rsp/nodesNr").Set(response, smartConnectResult.getBondedNodesNum());
            status = SERVICE_ERROR_OS_READ;
            break;

          case SmartConnectError::Type::HwpIdVersion:
            Pointer("/data/rsp/assignedAddr").Set(response, smartConnectResult.getBondedAddr());
            Pointer("/data/rsp/nodesNr").Set(response, smartConnectResult.getBondedNodesNum());
            Pointer("/data/rsp/hwpId").Set(response, smartConnectResult.getHwpId());
            Pointer("/data/rsp/manufacturer").Set(response, smartConnectResult.getManufacturer());
            Pointer("/data/rsp/product").Set(response, smartConnectResult.getProduct());
            status = SERVICE_ERROR_HWPID_VERSION;
            break;

          default:
            // some other unsupported error
            status = SERVICE_ERROR;
            break;
        }

        // set raw fields, if verbose mode is active
        if (comSmartConnect.getVerbose()) {
          setVerboseData(response, smartConnectResult);
        }

        Pointer("/data/status").Set(response, status);
        Pointer("/data/statusStr").Set(response, error.getMessage());

        return response;
      }

      // no errors

      // rsp object
      Pointer( "/data/rsp/assignedAddr" ).Set( response, smartConnectResult.getBondedAddr() );
      Pointer( "/data/rsp/nodesNr" ).Set( response, smartConnectResult.getBondedNodesNum() );
      
      rapidjson::Pointer("/data/rsp/hwpId").Set(response, smartConnectResult.getHwpId());
      rapidjson::Pointer("/data/rsp/manufacturer").Set(response, smartConnectResult.getManufacturer());
      rapidjson::Pointer("/data/rsp/product").Set(response, smartConnectResult.getProduct());

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
      setOsReadSection(smartConnectResult.getOsRead(), response);
      
      // set raw fields, if verbose mode is active
      if ( comSmartConnect.getVerbose() ) {
        setVerboseData( response, smartConnectResult );
      }

      // status - ok
      Pointer("/data/status").Set(response, 0);
      Pointer("/data/statusStr").Set(response, "ok");

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
        logDecodedValues(mid, ibk, hwpId);
       
        m_returnVerbose = comSmartConnect.getVerbose();
      }
      // parsing and checking service parameters failed 
      catch (const std::exception& ex) {
        Document failResponse = createCheckParamsFailedResponse(comSmartConnect.getMsgId(), msgType, ex.what());
        m_iMessagingSplitterService->sendMessage(messagingId, std::move(failResponse));

        TRC_FUNCTION_LEAVE("");
        return;
      }
      
      // try to establish exclusive access
      try {
        m_exclusiveAccess = m_iIqrfDpaService->getExclusiveAccess();
      }
      catch (const std::exception &e) {
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

      (void)props;

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