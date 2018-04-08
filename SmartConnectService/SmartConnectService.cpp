#define ISmartConnectService_EXPORTS

#include "DpaTransactionTask.h"
#include "SmartConnectService.h"
#include "IMessagingSplitterService.h"
#include "Trace.h"
#include "ComIqrfEmbedCoordSmartConnect.h"
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
};


namespace iqrf {

  // Holds information about errors, which encounter during smart connect service run
  class SmartConnectError {
  public:
    // Type of error
    enum class Type {
      NoError,
      MinDpaVerUsed,
      SmartConnect
    };

    SmartConnectError() : m_type(Type::NoError), m_message("") {};
    SmartConnectError(Type errorType) : m_type(errorType), m_message("") {};
    SmartConnectError(Type errorType, const std::string& message) : m_type(errorType), m_message(message) {};

    Type getType() const { return m_type; };
    std::string getMessage() const { return m_message; };

    SmartConnectError& SmartConnectError::operator=(const SmartConnectError& error) {
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
    SmartConnectError error;

    uint16_t nadr;
    uint16_t hwpId;
    uint8_t rCode;
    uint8_t dpaVal;
    uint8_t bondedAddr;
    uint8_t bondedNodesNum;

    // transaction results
    std::list<std::unique_ptr<IDpaTransactionResult2>> m_transResults;

  public:
    SmartConnectError getError() const { return error; };

    void setError(const SmartConnectError& error) {
      this->error = error;
    }

    uint16_t getNadr() const {
      return nadr;
    }

    void setNadr(uint16_t nadr) {
      this->nadr = nadr;
    }

    uint16_t getHwpId() const {
      return hwpId;
    }

    void setHwpId(uint16_t hwpId) {
      this->hwpId = hwpId;
    }

    uint8_t getRcode() const {
      return rCode;
    }

    void setRcode(uint8_t rCode) {
      this->rCode = rCode;
    }

    uint8_t getDpaVal() const {
      return dpaVal;
    }

    void setDpaVal(uint8_t dpaVal) {
      this->dpaVal = dpaVal;
    }

    uint8_t getBondedAddr() const {
      return bondedAddr;
    }

    void setBondedAddr(uint8_t bondedAddr) {
      this->bondedAddr = bondedAddr;
    }

    uint8_t getBondedNodesNum() const {
      return bondedNodesNum;
    }

    void setBondedNodesNum(uint8_t bondedNodesNum) {
      this->bondedNodesNum = bondedNodesNum;
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

    // message type: iqrf embed coordinator Smart connect
    // for temporal reasons
    const std::string m_mTypeName_iqrfEmbedCoordSmartConnect = "iqrfEmbedCoordinator_SmartConnect";
    //IMessagingSplitterService::MsgType* m_msgType_mngIqmeshWriteConfig;

    //iqrf::IJsCacheService* m_iJsCacheService = nullptr;
    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    IIqrfDpaService* m_iIqrfDpaService = nullptr;


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

      smartConnectRequest.DataToBuffer(smartConnectPacket.Buffer, sizeof(TDpaIFaceHeader) + 2);

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> smartConnectTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      try {
        smartConnectTransaction = m_iIqrfDpaService->executeDpaTransaction(smartConnectRequest);
        transResult = smartConnectTransaction->get();
      }
      catch (std::exception& e) {
        TRC_DEBUG("DPA transaction error : " << e.what());
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

        // parsing response
        smartConnectResult.setHwpId(dpaResponse.DpaPacket().DpaResponsePacket_t.HWPID);
        smartConnectResult.setRcode(dpaResponse.DpaPacket().DpaResponsePacket_t.ResponseCode);
        smartConnectResult.setDpaVal(dpaResponse.DpaPacket().DpaResponsePacket_t.DpaValue);

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
        SmartConnectError error(SmartConnectError::Type::SmartConnect, "Transaction error.");
        smartConnectResult.setError(error);

        TRC_FUNCTION_LEAVE("");
        return;
      } 
      
      // DPA error
      // parsing response
      smartConnectResult.setHwpId(dpaResponse.DpaPacket().DpaResponsePacket_t.HWPID);
      smartConnectResult.setRcode(dpaResponse.DpaPacket().DpaResponsePacket_t.ResponseCode);
      smartConnectResult.setDpaVal(dpaResponse.DpaPacket().DpaResponsePacket_t.DpaValue);

      TRC_DEBUG("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

      SmartConnectError error(SmartConnectError::Type::SmartConnect, "Dpa error.");
      smartConnectResult.setError(error);

      TRC_FUNCTION_LEAVE("");
    }

    // indicates, whether the DPA version used is >= 3.03
    void checkDpaVersion(uint16_t hwpId) {
      TRC_FUNCTION_ENTER("");

      DpaMessage perEnumRequest;
      DpaMessage::DpaPacket_t perEnumPacket;
      perEnumPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      perEnumPacket.DpaRequestPacket_t.PNUM = 0xFF;
      perEnumPacket.DpaRequestPacket_t.PCMD = 0x3F;
      perEnumPacket.DpaRequestPacket_t.HWPID = hwpId;
      perEnumRequest.DataToBuffer(perEnumPacket.Buffer, sizeof(TDpaIFaceHeader) + 2);

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> perEnumTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      try {
        perEnumTransaction = m_iIqrfDpaService->executeDpaTransaction(perEnumRequest);
        transResult = perEnumTransaction->get();
      }
      catch (std::exception& e) {
        TRC_DEBUG("DPA transaction error : " << e.what());
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

        // check the DPA version
        uint16_t dpaVersion = minorDpaVer + (majorDpaVer << 8);

        if (DPA_MIN_REQ_VERSION > dpaVersion) {
          THROW_EXC(std::logic_error, "Old version of DPA: " << PAR(dpaVersion) );
        }

        TRC_FUNCTION_LEAVE("");
        return;
      }

      // transaction error
      if (errorCode < 0) {
        TRC_DEBUG("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));
        THROW_EXC(std::logic_error, "Transaction error.");
      } 

      // DPA error
      TRC_DEBUG("DPA error. " << NAME_PAR_HEX("Error code", errorCode));
      THROW_EXC(std::logic_error, "DPA error.");
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
      try {
        checkDpaVersion(hwpId);
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

      // TODO: playing with IQRF repository

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

    // creates response on the basis of smart connect result
    Document createResponse(
      const std::string& messagingId,
      const IMessagingSplitterService::MsgType& msgType,
      SmartConnectResult& smartConnectResult,
      const ComIqrfEmbedCoordSmartConnect& comSmartConnect
    )
    {
      Document response;

      // set common parameters
      Pointer("/mType").Set(response, msgType.m_type);
      Pointer("/data/msgId").Set(response, messagingId);

      // checking of error
      SmartConnectError error = smartConnectResult.getError();
      
      if (error.getType() == SmartConnectError::Type::MinDpaVerUsed) {
        Pointer("/data/status").Set(response, SERVICE_ERROR_MIN_DPA_VER);
        Pointer("/data/statusStr").Set(response, error.getMessage());
        return response;
      }

      // data/rsp
      Pointer("/data/rsp/nadr").Set(response, smartConnectResult.getNadr());
      Pointer("/data/rsp/hwpId").Set(response, smartConnectResult.getHwpId());
      Pointer("/data/rsp/rCode").Set(response, smartConnectResult.getRcode());
      Pointer("/data/rsp/dpaVal").Set(response, smartConnectResult.getDpaVal());
      Pointer("/data/rsp/bondAddr").Set(response, smartConnectResult.getBondedAddr());
      Pointer("/data/rsp/devNr").Set(response, smartConnectResult.getBondedNodesNum());

      // status

      // set raw fields, if verbose mode is active
      if (comSmartConnect.getVerbose()) {
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

      return response;
    }

    

    uint16_t parseAndCheckNadr(int nadr) {
      if ((nadr < 0) || (nadr > 0xEF)) {
        THROW_EXC(
          std::out_of_range, "Device address outside of valid range. " << NAME_PAR_HEX("Address", nadr)
        );
      }
      return nadr;
    }

    uint16_t parseAndCheckHwpId(int hwpId) {
      if ((hwpId < 0) || (hwpId > 0xFFFF)) {
        THROW_EXC(
          std::out_of_range, "HWP ID outside of valid range. " << NAME_PAR_HEX("HWP ID", hwpId)
        );
      }
      return hwpId;
    }

    uint8_t parseAndCheckReqAddr(int reqAddr) {
      if ((reqAddr < 0) || (reqAddr > 0xFF)) {
        THROW_EXC(
          std::out_of_range, "ReqAddr outside of valid range. " << NAME_PAR_HEX("ReqAddr", reqAddr)
        );
      }
      return reqAddr;
    }

    uint8_t parseAndCheckBondingTestRetries(int bondingTestRetries) {
      if ((bondingTestRetries < 0) || (bondingTestRetries > 0xFF)) {
        THROW_EXC(
          std::out_of_range, "bondingTestRetries outside of valid range. " << NAME_PAR_HEX("bondingTestRetries", bondingTestRetries)
        );
      }
      return bondingTestRetries;
    }

    std::basic_string<uint8_t> parseAndCheckIbk(const std::basic_string<uint8_t>& ibk) {
      if (ibk.length() != IBK_LEN) {
        THROW_EXC(
          std::out_of_range, "ibk length invalid. " << NAME_PAR_HEX("ibk length", ibk.length())
        );
      }
      return ibk;
    }

    std::basic_string<uint8_t> parseAndCheckMid(const std::basic_string<uint8_t>& mid) {
      if (mid.length() != MID_LEN) {
        THROW_EXC(
          std::out_of_range, "mid length invalid. " << NAME_PAR_HEX("mid length", mid.length())
        );
      }
      return mid;
    }

    uint8_t parseAndCheckBondingChannel(int bondingChannel) {
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
      if (msgType.m_type != m_mTypeName_iqrfEmbedCoordSmartConnect) {
        THROW_EXC(std::logic_error, "Unsupported message type: " << PAR(msgType.m_type));
      }

      // creating representation object
      ComIqrfEmbedCoordSmartConnect comSmartConnect(doc);

      // service input parameters
      uint16_t nadr;
      uint16_t hwpId;
      uint8_t reqAddr;
      uint8_t bondingTestRetries;
      std::basic_string<uint8_t> ibk;
      std::basic_string<uint8_t> mid;
      uint8_t bondingChannel;
      uint8_t virtualDeviceAddress;
      std::basic_string<uint8_t> userData;

      // parsing and checking service parameters
      try {
        if (!comSmartConnect.isSetNadr()) {
          THROW_EXC(std::logic_error, "Nadr not set");
        }
        nadr = parseAndCheckNadr(comSmartConnect.getNadr());

        hwpId = HWPID_Default;
        if (comSmartConnect.isSetHwpId()) {
          hwpId = parseAndCheckHwpId(comSmartConnect.getHwpId());
        }
        
        if (!comSmartConnect.isSetReqAddr()) {
          THROW_EXC(std::logic_error, "ReqAddr not set");
        }
        reqAddr = parseAndCheckReqAddr(comSmartConnect.getReqAddr());

        if (!comSmartConnect.isSetBondingTestRetries()) {
          THROW_EXC(std::logic_error, "bondingTestRetries not set");
        }
        bondingTestRetries = parseAndCheckBondingTestRetries(comSmartConnect.getBondingTestRetries());

        if (!comSmartConnect.isSetIbk()) {
          THROW_EXC(std::logic_error, "ibk not set");
        }
        ibk = parseAndCheckIbk(comSmartConnect.getIbk());

        if (!comSmartConnect.isSetMid()) {
          THROW_EXC(std::logic_error, "mid not set");
        }
        mid = parseAndCheckMid(comSmartConnect.getMid());

        if (!comSmartConnect.isSetBondingChannel()) {
          THROW_EXC(std::logic_error, "bondingChannel not set");
        }
        bondingChannel = parseAndCheckBondingChannel(comSmartConnect.getBondingChannel());

        if (!comSmartConnect.isSetVirtualDeviceAddress()) {
          THROW_EXC(std::logic_error, "virtualDeviceAddress not set");
        }
        virtualDeviceAddress = parseAndCheckVirtualDeviceAddress(comSmartConnect.getVirtualDeviceAddress());
        
        if (!comSmartConnect.isSetUserData()) {
          THROW_EXC(std::logic_error, "userData not set");
        }
        userData = parseAndCheckUserData(comSmartConnect.getUserData());
      }
      // parsing and checking service parameters failed 
      catch (std::exception& ex) {
        Document failResponse = createCheckParamsFailedResponse(messagingId, msgType, ex.what());
        m_iMessagingSplitterService->sendMessage(messagingId, std::move(failResponse));

        TRC_FUNCTION_LEAVE("");
        return;
      }
      
      // call service with checked params
      SmartConnectResult smartConnectResult = smartConnect(
        hwpId, reqAddr, bondingTestRetries, ibk, mid, bondingChannel, virtualDeviceAddress, userData
      );

      // create and send response
      Document responseDoc = createResponse(messagingId, msgType, smartConnectResult, comSmartConnect);
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
      std::vector<std::string> supportedMsgTypes;
      supportedMsgTypes.push_back(m_mTypeName_iqrfEmbedCoordSmartConnect);

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
        "SmartConnectService instance deactivate" << std::endl <<
        "**************************************"
      );

      // for the sake of unregister function parameters 
      std::vector<std::string> supportedMsgTypes;
      supportedMsgTypes.push_back(m_mTypeName_iqrfEmbedCoordSmartConnect);

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