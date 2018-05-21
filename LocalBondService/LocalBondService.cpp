#define ILocalBondService_EXPORTS


#include "DpaTransactionTask.h"
#include "LocalBondService.h"
#include "Trace.h"
#include "DpaRaw.h"
#include "ComIqmeshNetworkBondNodeLocal.h"
#include "ObjectFactory.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include <list>
#include <cmath>

#include "iqrf__LocalBondService.hxx"

TRC_INIT_MODULE(iqrf::LocalBondService);

using namespace rapidjson;

namespace {

  // maximum number of repeats
  static const uint8_t REPEAT_MAX = 3;

  // Default bonding mask. No masking effect.
  static const uint8_t DEFAULT_BONDING_MASK = 0;

  // values of result error codes
  // service general fail code - may and probably will be changed later in the future
  static const int SERVICE_ERROR = 1000;

  static const int SERVICE_ERROR_GET_BONDED_NODES = SERVICE_ERROR + 1;
  static const int SERVICE_ERROR_ALREADY_BONDED = SERVICE_ERROR + 2;
  static const int SERVICE_ERROR_NO_FREE_SPACE = SERVICE_ERROR + 3;
  static const int SERVICE_ERROR_BOND_FAILED = SERVICE_ERROR + 4;
  static const int SERVICE_ERROR_ABOVE_ADDRESS_LIMIT = SERVICE_ERROR + 5;
  static const int SERVICE_ERROR_PING_FAILED = SERVICE_ERROR + 6;
  static const int SERVICE_ERROR_PING_INTERNAL_ERROR = SERVICE_ERROR + 7;
};


namespace iqrf {

  // Holds information about errors, which encounter during local bond service run
  class BondError {
  public:
    // Type of error
    enum class Type {
      NoError,
      GetBondedNodes,
      AlreadyBonded,
      NoFreeSpace,
      BondError,
      AboveAddressLimit,
      PingFailed,
      InternalError
    };

    BondError() : m_type(Type::NoError), m_message("") {};
    BondError(Type errorType) : m_type(errorType), m_message("") {};
    BondError(Type errorType, const std::string& message) : m_type(errorType), m_message(message) {};

    Type getType() const { return m_type; };
    std::string getMessage() const { return m_message; };

    BondError& operator=(const BondError& error) {
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


  /// \class BondResult
  /// \brief Result of bonding of a node.
  class BondResult {
  private:
    BondError m_error;
    uint8_t m_bondedAddr;
    uint8_t m_bondedNodesNum;
    TPerOSRead_Response m_readInfo;

    // transaction results
    std::list<std::unique_ptr<IDpaTransactionResult2>> m_transResults;

  public:
    BondError getError() const { return m_error; };

    void setError(const BondError& error) {
      m_error = error;
    }

    void setBondedAddr(const uint8_t addr) {
      m_bondedAddr = addr;
    }

    // returns address of the newly bonded node
    uint8_t getBondedAddr() const { return m_bondedAddr; };

    void setBondedNodesNum(const uint8_t nodesNum) {
      m_bondedNodesNum = nodesNum;
    }

    // returns number of bonded network nodes.
    uint8_t getBondedNodesNum() const { return m_bondedNodesNum; };

    // sets info about device
    void setReadInfo(const TPerOSRead_Response readInfo) {
      m_readInfo = readInfo;
    }

    // returns info about device
    const TPerOSRead_Response getReadInfo() const {
      return m_readInfo;
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
  class LocalBondService::Imp {
  private:
    // parent object
    LocalBondService& m_parent;

    // message type: network management bond node local
    // for temporal reasons
    const std::string m_mTypeName_iqmeshNetworkBondNodeLocal = "iqmeshNetwork_BondNodeLocal";
    //IMessagingSplitterService::MsgType* m_msgType_mngIqmeshBondNodeLocal;

    iqrf::IJsCacheService* m_iJsCacheService = nullptr;
    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    IIqrfDpaService* m_iIqrfDpaService = nullptr;

    // number of repeats
    uint8_t m_repeat = 1;

    // if is set Verbose mode
    bool m_returnVerbose = false;


  public:
    Imp(LocalBondService& parent) : m_parent(parent)
    {
    }

    ~Imp()
    {
    }

    void checkNodeAddr(const uint16_t nodeAddr)
    {
      if ((nodeAddr < 0) || (nodeAddr > 0xEF)) {
        THROW_EXC(
          std::logic_error, "Node address outside of valid range. " << NAME_PAR_HEX("Address", nodeAddr)
        );
      }
    }

    // trys to bonds node and returns result
    void _bondNode(BondResult& bondResult, const uint8_t nodeAddr) 
    {
      TRC_FUNCTION_ENTER("");

      DpaMessage bondNodeRequest;
      DpaMessage::DpaPacket_t bondNodePacket;
      bondNodePacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      bondNodePacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
      bondNodePacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_BOND_NODE;
      bondNodePacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;

      TPerCoordinatorBondNode_Request* tCoordBondNodeRequest = &bondNodeRequest.DpaPacket().DpaRequestPacket_t.DpaMessage.PerCoordinatorBondNode_Request;
      tCoordBondNodeRequest->ReqAddr = uint8_t(nodeAddr & 0xFF);
      tCoordBondNodeRequest->BondingMask = DEFAULT_BONDING_MASK;

      bondNodeRequest.DataToBuffer(bondNodePacket.Buffer, sizeof(TDpaIFaceHeader) + 2);

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> bondNodeTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      for (int rep = 0; rep <= m_repeat; rep++) {
        try {
          bondNodeTransaction = m_iIqrfDpaService->executeDpaTransaction(
            bondNodeRequest, 10500 + rep*100
          );
          transResult = bondNodeTransaction->get();
        }
        catch (std::exception& e) {
          TRC_DEBUG("DPA transaction error : " << e.what());

          if (rep < m_repeat) {
            continue;
          }

          BondError error(BondError::Type::BondError, e.what());
          bondResult.setError(error);

          TRC_FUNCTION_LEAVE("");
          return;

        }

        TRC_DEBUG("Result from bond node transaction as string:" << PAR(transResult->getErrorString()));

        IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

        // because of the move-semantics
        DpaMessage dpaResponse = transResult->getResponse();
        bondResult.addTransactionResult(transResult);

        if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
          TRC_INFORMATION("Bond node successful!");
          TRC_DEBUG(
            "DPA transaction: "
            << NAME_PAR(bondNodeRequest.PeripheralType(), bondNodeRequest.NodeAddress())
            << PAR(bondNodeRequest.PeripheralCommand())
          );

          // getting bond data
          TPerCoordinatorBondNodeSmartConnect_Response respData
            = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerCoordinatorBondNodeSmartConnect_Response;

          bondResult.setBondedAddr(respData.BondAddr);
          bondResult.setBondedNodesNum(respData.DevNr);

          TRC_FUNCTION_LEAVE("");
          return;
        }

        // transaction error
        if (errorCode < 0) {
          TRC_DEBUG("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

          if (rep < m_repeat) {
            continue;
          }

          BondError error(BondError::Type::BondError, "Transaction error.");
          bondResult.setError(error);
        }
        else {
          // DPA error
          TRC_DEBUG("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

          if (rep < m_repeat) {
            continue;
          }

          BondError error(BondError::Type::BondError, "Dpa error.");
          bondResult.setError(error);
        }
      }

      TRC_FUNCTION_LEAVE("");
      return;
    }

    // pings specified node using OS::Read command
    void pingNode(BondResult& bondResult, const uint8_t nodeAddr)
    {
      TRC_FUNCTION_ENTER("");

      DpaMessage readInfoRequest;
      DpaMessage::DpaPacket_t readInfoPacket;
      readInfoPacket.DpaRequestPacket_t.NADR = nodeAddr;
      readInfoPacket.DpaRequestPacket_t.PNUM = PNUM_OS;
      readInfoPacket.DpaRequestPacket_t.PCMD = CMD_OS_READ;
      readInfoPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
      readInfoRequest.DataToBuffer(readInfoPacket.Buffer, sizeof(TDpaIFaceHeader));

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> readInfoTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      for (int rep = 0; rep <= m_repeat; rep++) {
        try {
          readInfoTransaction = m_iIqrfDpaService->executeDpaTransaction(readInfoRequest);
          transResult = readInfoTransaction->get();
        }
        catch (std::exception& e) {
          TRC_DEBUG("DPA transaction error : " << e.what());

          if (rep < m_repeat) {
            continue;
          }

          BondError error(BondError::Type::PingFailed, e.what());
          bondResult.setError(error);

          TRC_FUNCTION_LEAVE("");
          return;
        }

        TRC_DEBUG("Result from read node's info transaction as string:" << PAR(transResult->getErrorString()));

        IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

        // because of the move-semantics
        DpaMessage dpaResponse = transResult->getResponse();
        bondResult.addTransactionResult(transResult);

        if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
          TRC_INFORMATION("Read node's info successful!");
          TRC_DEBUG(
            "DPA transaction: "
            << NAME_PAR(readInfoRequest.PeripheralType(), readInfoRequest.NodeAddress())
            << PAR(readInfoRequest.PeripheralCommand())
          );

          bondResult.setReadInfo(
            dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerOSRead_Response
          );

          TRC_FUNCTION_LEAVE("");
          return;
        }

        // transaction error
        if (errorCode < 0) {
          TRC_DEBUG("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

          if (rep < m_repeat) {
            continue;
          }

          BondError error(BondError::Type::PingFailed, "Transaction error");
          bondResult.setError(error);
        } // DPA error
        else {
          TRC_DEBUG("Dpa error. " << NAME_PAR_HEX("Error code", errorCode));

          if (rep < m_repeat) {
            continue;
          }

          BondError error(BondError::Type::PingFailed, "Dpa error");
          bondResult.setError(error);
        }
      }
    }

    // removes specified address from coordinator's list of bonded addresses
    void removeBondedNode(BondResult& bondResult, const uint8_t nodeAddr) 
    {
      TRC_FUNCTION_ENTER("");

      DpaMessage removeBondRequest;
      DpaMessage::DpaPacket_t removeBondPacket;
      removeBondPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      removeBondPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
      removeBondPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_REMOVE_BOND;
      removeBondPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;

      TPerCoordinatorRemoveRebondBond_Request* tCoordRemoveBondRequest
        = &removeBondRequest.DpaPacket().DpaRequestPacket_t.DpaMessage.PerCoordinatorRemoveRebondBond_Request;
      tCoordRemoveBondRequest->BondAddr = uint8_t(nodeAddr & 0xFF);

      removeBondRequest.DataToBuffer(removeBondPacket.Buffer, sizeof(TDpaIFaceHeader) + 1);

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> removeBondTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      for (int rep = 0; rep <= m_repeat; rep++) {
        try {
          removeBondTransaction = m_iIqrfDpaService->executeDpaTransaction(removeBondRequest);
          transResult = removeBondTransaction->get();
        }
        catch (std::exception& e) {
          TRC_DEBUG("DPA transaction error : " << e.what());
          THROW_EXC(std::logic_error, "Could not remove bond.");
        }

        TRC_DEBUG("Result from remove bond transaction as string:" << PAR(transResult->getErrorString()));

        IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

        // because of the move-semantics
        DpaMessage dpaResponse = transResult->getResponse();
        bondResult.addTransactionResult(transResult);

        if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
          TRC_INFORMATION("Remove node bond done!");
          TRC_DEBUG(
            "DPA transaction: "
            << NAME_PAR(removeBondRequest.PeripheralType(), removeBondRequest.NodeAddress())
            << PAR(removeBondRequest.PeripheralCommand())
          );

          TRC_FUNCTION_LEAVE("");
          return;
        }

        // transaction error
        if (errorCode < 0) {
          TRC_DEBUG("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

          if (rep < m_repeat) {
            continue;
          }
        } // DPA error
        else {
          TRC_DEBUG("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

          if (rep < m_repeat) {
            continue;
          }
        }
      }

      TRC_FUNCTION_LEAVE("");
      return;
    }

    // parses bit array of bonded nodes
    std::list<uint8_t> parseBondedNodes(const unsigned char* pData) {
      std::list<uint8_t> bondedNodes;

      // maximal bonded node number
      const uint8_t MAX_BONDED_NODE_NUMBER = 0xEF;
      const uint8_t MAX_BYTES_USED = (uint8_t)ceil(MAX_BONDED_NODE_NUMBER / 8.0);

      for (int byteId = 0; byteId < 32; byteId++) {
        if (byteId >= MAX_BYTES_USED) {
          break;
        }

        if (pData[byteId] == 0) {
          continue;
        }

        int bitComp = 1;
        for (int bitId = 0; bitId < 8; bitId++) {
          if ((pData[byteId] & bitComp) == bitComp) {
            bondedNodes.push_back(byteId * 8 + bitId);
          }
          bitComp *= 2;
        }
      }

      return bondedNodes;
    }

    // returns list of bonded nodes
    std::list<uint8_t> getBondedNodes(BondResult& bondResult) {
      TRC_FUNCTION_ENTER("");

      DpaMessage getBondedNodesRequest;
      DpaMessage::DpaPacket_t getBondedNodesPacket;
      getBondedNodesPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      getBondedNodesPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
      getBondedNodesPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_BONDED_DEVICES;
      getBondedNodesPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;

      getBondedNodesRequest.DataToBuffer(getBondedNodesPacket.Buffer, sizeof(TDpaIFaceHeader));

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> getBondedNodesTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      for (int rep = 0; rep <= m_repeat; rep++) {
        try {
          getBondedNodesTransaction = m_iIqrfDpaService->executeDpaTransaction(getBondedNodesRequest);
          transResult = getBondedNodesTransaction->get();
        }
        catch (std::exception& e) {
          TRC_DEBUG("DPA transaction error : " << e.what());

          if (rep < m_repeat) {
            continue;
          }

          BondError error(BondError::Type::GetBondedNodes, e.what());
          bondResult.setError(error);

          THROW_EXC(std::logic_error, "Could not get bonded nodes.");
        }

        TRC_DEBUG("Result from get bonded nodes transaction as string:" << PAR(transResult->getErrorString()));

        IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

        // because of the move-semantics
        DpaMessage dpaResponse = transResult->getResponse();
        bondResult.addTransactionResult(transResult);

        if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
          TRC_INFORMATION("Get bonded nodes successful!");
          TRC_DEBUG(
            "DPA transaction: "
            << NAME_PAR(getBondedNodesRequest.PeripheralType(), getBondedNodesRequest.NodeAddress())
            << PAR(getBondedNodesRequest.PeripheralCommand())
          );

          // get response data
          const unsigned char* pData = transResult->getResponse().DpaPacketData();

          TRC_FUNCTION_LEAVE("");
          return parseBondedNodes(pData);
        }

        // transaction error
        if (errorCode < 0) {
          TRC_DEBUG("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

          if (rep < m_repeat) {
            continue;
          }

          BondError error(BondError::Type::GetBondedNodes, "Transaction error.");
          bondResult.setError(error);
        }
        else {
          // DPA error
          TRC_DEBUG("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

          if (rep < m_repeat) {
            continue;
          }

          BondError error(BondError::Type::GetBondedNodes, "Dpa error.");
          bondResult.setError(error);
        }
      }

      THROW_EXC(std::logic_error, "Could not get bonded nodes.");
    }

    // indicates, whether all nodes are bonded
    bool areAllUsed(const std::list<uint8_t>& bondedNodes) {
      return (bondedNodes.size() == (0xEF + 1));
    }

    // indicates, whether the specified node is already bonded
    bool isBonded(const std::list<uint8_t>& bondedNodes, const uint8_t nodeAddr) 
    {
      std::list<uint8_t>::const_iterator it 
        = std::find(bondedNodes.begin(), bondedNodes.end(), nodeAddr);
      return (it != bondedNodes.end());
    }

    BondResult bondNode(const uint8_t nodeAddr)
    {
      TRC_FUNCTION_ENTER("");

      BondResult bondResult;
      
      std::list<uint8_t> bondedNodes;
      try {
        // get bonded nodes to check it against address to bond
        std::list<uint8_t> bondedNodes = getBondedNodes(bondResult);
      }
      catch (std::exception& ex) {
        TRC_FUNCTION_LEAVE("");
        return bondResult;
      }

      // node is already bonded
      if (isBonded(bondedNodes, nodeAddr)) {
        BondError error(BondError::Type::AlreadyBonded, "Address already bonded.");
        bondResult.setError(error);

        TRC_FUNCTION_LEAVE("");
        return bondResult;
      }

      // all nodes are already bonded
      if (areAllUsed(bondedNodes)) {
        BondError error(BondError::Type::NoFreeSpace, "No free space.");
        bondResult.setError(error);

        TRC_FUNCTION_LEAVE("");
        return bondResult;
      }

      // bond a node
      _bondNode(bondResult, nodeAddr);

      // bonding node failed
      if (bondResult.getError().getType() != BondError::Type::NoError) {
        return bondResult;
      }

      // ping newly bonded node and return info
      pingNode(bondResult, nodeAddr);
      if (bondResult.getError().getType() == BondError::Type::NoError) {
        return bondResult;
      }

      // if ping failed, remove bonded node from the coordinator's list
      removeBondedNode(bondResult, nodeAddr);

      // and return Ping Failed error
      return bondResult;
    }

    
    // creates error response about service general fail
    Document createCheckParamsFailedResponse(
      const std::string& msgId,
      const IMessagingSplitterService::MsgType& msgType,
      const std::string& errorMsg
    )
    {
      Document response;

      // set common parameters
      Pointer("/mType").Set(response, msgType.m_type);
      Pointer("/data/msgId").Set(response, msgId);

      // set result
      Pointer("/status").Set(response, SERVICE_ERROR);
      Pointer("/statusStr").Set(response, errorMsg);

      return response;
    }


    // sets response VERBOSE data
    void setVerboseData(rapidjson::Document& response, BondResult& bondResult)
    {
      rapidjson::Value rawArray(kArrayType);
      Document::AllocatorType& allocator = response.GetAllocator();

      while (bondResult.isNextTransactionResult()) {
        std::unique_ptr<IDpaTransactionResult2> transResult = bondResult.consumeNextTransactionResult();
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

    // creates response on the basis of bond result
    Document createResponse(
      const std::string& msgId,
      const IMessagingSplitterService::MsgType& msgType,
      BondResult& bondResult,
      const ComIqmeshNetworkBondNodeLocal& comBondNodeLocal
    )
    {
      Document response;
      
      // set common parameters
      Pointer("/mType").Set(response, msgType.m_type);
      Pointer("/data/msgId").Set(response, msgId);

      // checking of error
      BondError error = bondResult.getError();

      if (error.getType() != BondError::Type::NoError) {
        Pointer("/data/statusStr").Set(response, error.getMessage());

        switch (error.getType()) {
          case BondError::Type::GetBondedNodes:
            Pointer("/status").Set(response, SERVICE_ERROR_GET_BONDED_NODES);
            break;
          case BondError::Type::AlreadyBonded:
            Pointer("/status").Set(response, SERVICE_ERROR_ALREADY_BONDED);
            break;
          case BondError::Type::NoFreeSpace:
            Pointer("/status").Set(response, SERVICE_ERROR_NO_FREE_SPACE);
            break;
          case BondError::Type::PingFailed:
            Pointer("/status").Set(response, SERVICE_ERROR_PING_FAILED);
            break;
          case BondError::Type::BondError:
            Pointer("/status").Set(response, SERVICE_ERROR_BOND_FAILED);
            break;
          default:
            Pointer("/status").Set(response, SERVICE_ERROR);
            break;
        }

        // set raw fields, if verbose mode is active
        if (comBondNodeLocal.getVerbose()) {
          setVerboseData(response, bondResult);
        }

        return response;
      }

      // status - ok
      Pointer("/status").Set(response, 0);
      Pointer("/statusStr").Set(response, "ok");

      // rsp object
      rapidjson::Pointer("/data/rsp/assignedAddr").Set(response, bondResult.getBondedAddr());
      rapidjson::Pointer("/data/rsp/nodesNr").Set(response, bondResult.getBondedNodesNum());

      // only for temporal reasons
      rapidjson::Pointer("/data/rsp/manufacturer").Set(response, "");
      rapidjson::Pointer("/data/rsp/product").Set(response, "");

      // manufacturer name and product name - how to obtain hwpid
      /*
      const IJsCacheService::Manufacturer* manufacturer = m_iJsCacheService->getManufacturer(hwpId);
      if (manufacturer != nullptr) {
        rapidjson::Pointer("/data/rsp/manufacturer").Set(response, manufacturer->m_name);
      }
      else {
        rapidjson::Pointer("/data/rsp/manufacturer").Set(response, "");
      }
      
      const IJsCacheService::Product* product = m_iJsCacheService->getProduct(hwpId);
      if (product != nullptr) {
        rapidjson::Pointer("/data/rsp/product").Set(response, product->m_name);
      }
      else {
        rapidjson::Pointer("/data/rsp/product").Set(response, "");
      }
      */

      // osRead object
      const TPerOSRead_Response readInfo = bondResult.getReadInfo();

      rapidjson::Pointer("/data/rsp/osRead/mid").Set(response, encodeBinary(readInfo.ModuleId, 4));
      rapidjson::Pointer("/data/rsp/osRead/osVersion").Set(response, encodeHexaNum(readInfo.OsVersion));
      rapidjson::Pointer("/data/rsp/osRead/trMcuType").Set(response, encodeHexaNum(readInfo.McuType));
      rapidjson::Pointer("/data/rsp/osRead/osBuild").Set(response, encodeHexaNum(readInfo.OsBuild));
      rapidjson::Pointer("/data/rsp/osRead/rssi").Set(response, encodeHexaNum(readInfo.Rssi));
      rapidjson::Pointer("/data/rsp/osRead/supplyVoltage").Set(response, encodeHexaNum(readInfo.SupplyVoltage));
      rapidjson::Pointer("/data/rsp/osRead/flags").Set(response, encodeHexaNum(readInfo.Flags));
      rapidjson::Pointer("/data/rsp/osRead/slotLimits").Set(response, encodeHexaNum(readInfo.SlotLimits));

      // set raw fields, if verbose mode is active
      if (comBondNodeLocal.getVerbose()) {
        setVerboseData(response, bondResult);
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

    uint8_t parseAndCheckDeviceAddr(const int devAddr) {
      if ((devAddr < 0) || (devAddr > 0xEF)) {
        THROW_EXC(
          std::out_of_range, "Device address outside of valid range. " << NAME_PAR_HEX("Address", devAddr)
        );
      }
      return devAddr;
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
      if (msgType.m_type != m_mTypeName_iqmeshNetworkBondNodeLocal) {
        THROW_EXC(std::logic_error, "Unsupported message type: " << PAR(msgType.m_type));
      }

      // creating representation object
      ComIqmeshNetworkBondNodeLocal comBondNodeLocal(doc);
      
      // service input parameters
      uint8_t deviceAddr;

      // parsing and checking service parameters
      try {
        m_repeat = parseAndCheckRepeat(comBondNodeLocal.getRepeat());

        if (!comBondNodeLocal.isSetDeviceAddr()) {
          THROW_EXC(std::logic_error, "deviceAddr not set");
        }
        deviceAddr = parseAndCheckDeviceAddr(comBondNodeLocal.getDeviceAddr());

        m_returnVerbose = comBondNodeLocal.getVerbose();
      } 
      // parsing and checking service parameters failed 
      catch (std::exception& ex) {
        Document failResponse = createCheckParamsFailedResponse(comBondNodeLocal.getMsgId(), msgType, ex.what());
        m_iMessagingSplitterService->sendMessage(messagingId, std::move(failResponse));

        TRC_FUNCTION_LEAVE("");
        return;
      }

      BondResult bondResult = bondNode(deviceAddr);

      // creating response
      Document responseDoc = createResponse(messagingId, msgType, bondResult, comBondNodeLocal);

      // send response back
      m_iMessagingSplitterService->sendMessage(messagingId, std::move(responseDoc));

      TRC_FUNCTION_LEAVE("");
    }
    
    void activate(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "************************************" << std::endl <<
        "LocalBondService instance activate" << std::endl <<
        "************************************"
      );

      // for the sake of register function parameters 
      std::vector<std::string> supportedMsgTypes =
      {
        m_mTypeName_iqmeshNetworkBondNodeLocal
      };

      /*
      m_iMessagingSplitterService->registerFilteredMsgHandler(
        supportedMsgTypes,
        [&](const std::string & messagingId, const IMessagingSplitterService::MsgType & msgType, rapidjson::Document doc)
      {
        handleMsg(messagingId, msgType, std::move(doc));
      });
      */
      TRC_FUNCTION_LEAVE("")
    }

    void deactivate()
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "************************************" << std::endl <<
        "LocalBondService instance deactivate" << std::endl <<
        "************************************"
      );
      
      std::vector<std::string> supportedMsgTypes =
      {
        m_mTypeName_iqmeshNetworkBondNodeLocal
      };

      //m_iMessagingSplitterService->unregisterFilteredMsgHandler(supportedMsgTypes);
      
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

  
  LocalBondService::LocalBondService()
  {
    m_imp = shape_new Imp(*this);
  }

  LocalBondService::~LocalBondService()
  {
    delete m_imp;
  }

  
  void LocalBondService::attachInterface(iqrf::IIqrfDpaService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void LocalBondService::detachInterface(iqrf::IIqrfDpaService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void LocalBondService::attachInterface(iqrf::IJsCacheService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void LocalBondService::detachInterface(iqrf::IJsCacheService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void LocalBondService::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void LocalBondService::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }


  void LocalBondService::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void LocalBondService::deactivate()
  {
    m_imp->deactivate();
  }

  void LocalBondService::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
  }
  
}
