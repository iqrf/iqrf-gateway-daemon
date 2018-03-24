#define ILocalBondService_EXPORTS


#include "DpaTransactionTask.h"
#include "LocalBondService.h"
#include "Trace.h"
#include "PrfOs.h"
#include "DpaRaw.h"
#include "PrfFrc.h"
#include "ComMngIqmeshBondNodeLocal.h"
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

  // Default bonding mask. No masking effect.
  static const uint8_t DEFAULT_BONDING_MASK = 0;

  // values of result error codes
  static const int BOND_RESULT_TYPE_NO_ERROR = 0;
  static const int BOND_RESULT_TYPE_ERROR_BASE = 1000;

  static const int BOND_RESULT_TYPE_ALREADY_USED = BOND_RESULT_TYPE_ERROR_BASE + 1;
  static const int BOND_RESULT_TYPE_NO_FREE_SPACE = BOND_RESULT_TYPE_ERROR_BASE + 2;
  static const int BOND_RESULT_TYPE_ABOVE_ADDRESS_LIMIT = BOND_RESULT_TYPE_ERROR_BASE + 3;
  static const int BOND_RESULT_TYPE_PING_FAILED = BOND_RESULT_TYPE_ERROR_BASE + 4;
  static const int BOND_RESULT_TYPE_PING_INTERNAL_ERROR = BOND_RESULT_TYPE_ERROR_BASE + 5;
};


namespace iqrf {

  /// \class BondResult
  /// \brief Result of bonding of a node.
  class BondResult {
  public:
    /// Type of result
    enum class Type {
      NoError = BOND_RESULT_TYPE_NO_ERROR,
      AlreadyUsed = BOND_RESULT_TYPE_ALREADY_USED,
      NoFreeSpace = BOND_RESULT_TYPE_NO_FREE_SPACE,
      AboveAddressLimit = BOND_RESULT_TYPE_ABOVE_ADDRESS_LIMIT,
      PingFailed = BOND_RESULT_TYPE_PING_FAILED,
      InternalError = BOND_RESULT_TYPE_PING_INTERNAL_ERROR
    };

    BondResult() {
      this->m_type = Type::NoError;
      this->m_bondedAddr = 0;
      this->m_bondedNodesNum = 0;
    }

    BondResult(Type type) {
      this->m_type = type;
      this->m_bondedAddr = 0;
      this->m_bondedNodesNum = 0;
    }

    /// \brief Constructor
    /// \param [in] type              type of result
    /// \param [in] bondedNodeAddr    address of newly bonded node
    /// \param [in] bondedNodesNum    number of bonded nodes
    BondResult(Type type, uint16_t bondedNodeAddr, uint16_t bondedNodesNum) {
      this->m_type = type;
      this->m_bondedAddr = bondedNodeAddr;
      this->m_bondedNodesNum = bondedNodesNum;

      if (type != Type::NoError) {
        this->m_bondedAddr = 0;
        this->m_bondedNodesNum = 0;
      }
    }

    /// \brief Returns type of bonding result.
    /// \return Type of bonding result
    Type getType() const { return m_type; };

    /// \brief Returns address of the newly bonded node.
    /// \return address of the newly bonded node
    /// \details
    /// Returned value is valid only if bonding was successfully processed,
    /// i.e. the result type is NoError. Otherwise the returned value is 0.
    uint16_t getBondedAddr() const { return m_bondedAddr; };

    /// \brief Returns number of bonded network nodes.
    /// \return number of bonded network nodes
    /// \details
    /// Returned value is valid only if bonding was successfully processed,
    /// i.e. the result type is NoError. Otherwise the returned value is 0.
    uint16_t getBondedNodesNum() const { return m_bondedNodesNum; };

    // sets info about device
    void setReadInfo(const TPerOSRead_Response readInfo) {
      m_readInfo = readInfo;
    }

    // returns info about device
    const TPerOSRead_Response getReadInfo() const {
      return m_readInfo;
    }

    // sets transaction result
    void setTransactionResult(
      std::unique_ptr<IDpaTransactionResult2>& transResult
    ) 
    {
      m_transResult = std::move(transResult);
    }

    // returns transaction result
    std::unique_ptr<IDpaTransactionResult2> getTransactionResult() {
      return std::move(m_transResult);
    }

  private:
    Type m_type;
    uint16_t m_bondedAddr;
    uint16_t m_bondedNodesNum;
    TPerOSRead_Response m_readInfo;
    std::unique_ptr<IDpaTransactionResult2> m_transResult;
  };



  // implementation class
  class LocalBondService::Imp {
  private:
    // parent object
    LocalBondService& m_parent;

    // message type: network management bond node local
    // for temporal reasons
    const std::string m_mTypeName_mngIqmeshBondNodeLocal = "mngIqmeshBondNodeLocal";
    IMessagingSplitterService::MsgType* m_msgType_mngIqmeshBondNodeLocal;

    //iqrf::IJsCacheService* m_iJsCacheService = nullptr;
    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    IIqrfDpaService* m_iIqrfDpaService = nullptr;

    // number of repeats
    uint8_t m_repeat = 1;


  public:
    Imp(LocalBondService& parent) : m_parent(parent)
    {
      m_msgType_mngIqmeshBondNodeLocal 
        = new IMessagingSplitterService::MsgType(m_mTypeName_mngIqmeshBondNodeLocal, 1, 0, 0);
    }

    ~Imp()
    {
    }

    void checkNodeAddr(const uint16_t nodeAddr)
    {
      if ((nodeAddr < 0) || (nodeAddr > 0xEF)) {
        THROW_EXC(
          std::exception, "Node address outside of valid range. " << NAME_PAR_HEX("Address", nodeAddr)
        );
      }
    }

    // trys to bonds node and returns result
    BondResult _bondNode(const uint16_t nodeAddr) {
      TRC_FUNCTION_ENTER("");

      DpaMessage bondNodeRequest;
      DpaMessage::DpaPacket_t bondNodePacket;
      bondNodePacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      bondNodePacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
      bondNodePacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_BOND_NODE;
      bondNodePacket.DpaRequestPacket_t.HWPID = HWPID_Default;

      TPerCoordinatorBondNode_Request* tCoordBondNodeRequest = &bondNodeRequest.DpaPacket().DpaRequestPacket_t.DpaMessage.PerCoordinatorBondNode_Request;
      tCoordBondNodeRequest->ReqAddr = uint8_t(nodeAddr & 0xFF);
      tCoordBondNodeRequest->BondingMask = DEFAULT_BONDING_MASK;

      bondNodeRequest.DataToBuffer(bondNodePacket.Buffer, sizeof(TDpaIFaceHeader) + 2);

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> bondNodeTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      try {
        bondNodeTransaction = m_iIqrfDpaService->executeDpaTransaction(bondNodeRequest);
        transResult = bondNodeTransaction->get();
      }
      catch (std::exception& e) {
        TRC_DEBUG("DPA transaction error : " << e.what());
        THROW_EXC(std::exception, "Could not bond node.");
      }

      TRC_DEBUG("Result from bond node transaction as string:" << PAR(transResult->getErrorString()));

      IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

      if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
        TRC_INFORMATION("Bond node successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(bondNodeRequest.PeripheralType(), bondNodeRequest.NodeAddress())
          << PAR(bondNodeRequest.PeripheralCommand())
        );

        // getting response data
        DpaMessage dpaResponse = transResult->getResponse();

        // getting bond data
        TPerCoordinatorBondNode_Response bondNodeResponse
          = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerCoordinatorBondNode_Response;

        TRC_FUNCTION_LEAVE("");
        return BondResult(BondResult::Type::NoError, bondNodeResponse.BondAddr, bondNodeResponse.DevNr);
      }

      // transaction error
      if (errorCode < 0) {
        TRC_DEBUG("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));
      } // DPA error
      else {
        TRC_DEBUG("DPA error. " << NAME_PAR_HEX("Error code", errorCode));
      }

      THROW_EXC(std::exception, "Could not bond a node.");
    }

    // pings specified node using OS::Read command
    TPerOSRead_Response pingNode(const uint16_t nodeAddr)
    {
      TRC_FUNCTION_ENTER("");

      DpaMessage readInfoRequest;
      DpaMessage::DpaPacket_t readInfoPacket;
      readInfoPacket.DpaRequestPacket_t.NADR = nodeAddr;
      readInfoPacket.DpaRequestPacket_t.PNUM = PNUM_OS;
      readInfoPacket.DpaRequestPacket_t.PCMD = CMD_OS_READ;
      readInfoPacket.DpaRequestPacket_t.HWPID = HWPID_Default;

      readInfoRequest.DataToBuffer(readInfoPacket.Buffer, sizeof(TDpaIFaceHeader) + 2);

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> readInfoTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      try {
        readInfoTransaction = m_iIqrfDpaService->executeDpaTransaction(readInfoRequest);
        transResult = readInfoTransaction->get();
      }
      catch (std::exception& e) {
        TRC_DEBUG("DPA transaction error : " << e.what());
        THROW_EXC(std::exception, "Could not read info.");
      }

      TRC_DEBUG("Result from read node's info transaction as string:" << PAR(transResult->getErrorString()));

      IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

      if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
        TRC_INFORMATION("Read node's info successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(readInfoRequest.PeripheralType(), readInfoRequest.NodeAddress())
          << PAR(readInfoRequest.PeripheralCommand())
        );

        // getting response data
        DpaMessage dpaResponse = transResult->getResponse();

        TRC_FUNCTION_LEAVE("");
        return dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerOSRead_Response;
      }

      // transaction error
      if (errorCode < 0) {
        TRC_DEBUG("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));
      } // DPA error
      else {
        TRC_DEBUG("DPA error. " << NAME_PAR_HEX("Error code", errorCode));
      }

      THROW_EXC(std::exception, "Could not read node's info.");
    }

    // removes specified address from coordinator's list of bonded addresses
    void removeBondedNode(const uint16_t nodeAddr) {
      TRC_FUNCTION_ENTER("");

      DpaMessage removeBondRequest;
      DpaMessage::DpaPacket_t removeBondPacket;
      removeBondPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      removeBondPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
      removeBondPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_REMOVE_BOND;
      removeBondPacket.DpaRequestPacket_t.HWPID = HWPID_Default;

      TPerCoordinatorRemoveRebondBond_Request* tCoordRemoveBondRequest
        = &removeBondRequest.DpaPacket().DpaRequestPacket_t.DpaMessage.PerCoordinatorRemoveRebondBond_Request;
      tCoordRemoveBondRequest->BondAddr = uint8_t(nodeAddr & 0xFF);

      removeBondRequest.DataToBuffer(removeBondPacket.Buffer, sizeof(TDpaIFaceHeader) + 2);

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> removeBondTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      try {
        removeBondTransaction = m_iIqrfDpaService->executeDpaTransaction(removeBondRequest);
        transResult = removeBondTransaction->get();
      }
      catch (std::exception& e) {
        TRC_DEBUG("DPA transaction error : " << e.what());
        THROW_EXC(std::exception, "Could not remove bond.");
      }

      TRC_DEBUG("Result from remove bond transaction as string:" << PAR(transResult->getErrorString()));

      IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

      if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
        TRC_INFORMATION("Remove node bond successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(removeBondRequest.PeripheralType(), removeBondRequest.NodeAddress())
          << PAR(removeBondRequest.PeripheralCommand())
        );

        TRC_FUNCTION_LEAVE("");
      }

      // transaction error
      if (errorCode < 0) {
        TRC_DEBUG("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));
      } // DPA error
      else {
        TRC_DEBUG("DPA error. " << NAME_PAR_HEX("Error code", errorCode));
      }

      THROW_EXC(std::exception, "Could not remove bond.");
    }

    // parses bit array of bonded nodes
    std::list<uint16_t> parseBondedNodes(const unsigned char* pData) {
      std::list<uint16_t> bondedNodes;

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
    std::list<uint16_t> getBondedNodes() {
      TRC_FUNCTION_ENTER("");

      DpaMessage getBondedNodesRequest;
      DpaMessage::DpaPacket_t getBondedNodesPacket;
      getBondedNodesPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      getBondedNodesPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
      getBondedNodesPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_BONDED_DEVICES;
      getBondedNodesPacket.DpaRequestPacket_t.HWPID = HWPID_Default;

      getBondedNodesRequest.DataToBuffer(getBondedNodesPacket.Buffer, sizeof(TDpaIFaceHeader) + 2);

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> getBondedNodesTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      try {
        getBondedNodesTransaction = m_iIqrfDpaService->executeDpaTransaction(getBondedNodesRequest);
        transResult = getBondedNodesTransaction->get();
      }
      catch (std::exception& e) {
        TRC_DEBUG("DPA transaction error : " << e.what());
        THROW_EXC(std::exception, "Could not get bonded nodes.");
      }

      TRC_DEBUG("Result from get bonded nodes transaction as string:" << PAR(transResult->getErrorString()));

      IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

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
      } // DPA error
      else {
        TRC_DEBUG("DPA error. " << NAME_PAR_HEX("Error code", errorCode));
      }

      THROW_EXC(std::exception, "Could not get bonded nodes.");
    }

    // indicates, whether all nodes are bonded
    bool areAllUsed(const std::list<uint16_t>& bondedNodes) {
      return (bondedNodes.size() == (0xEF + 1));
    }

    // indicates, whether the specified node is already bonded
    bool isBonded(const std::list<uint16_t>& bondedNodes, const uint16_t nodeAddr) {
      std::list<uint16_t>::const_iterator it 
        = std::find(bondedNodes.begin(), bondedNodes.end(), nodeAddr);
      return (it != bondedNodes.end());
    }

    BondResult bondNode(const uint16_t nodeAddr)
    {
      // basic check
      checkNodeAddr(nodeAddr);

      // get bonded nodes to check it against address to bond
      std::list<uint16_t> bondedNodes = getBondedNodes();
      
      // node is already bonded
      if (isBonded(bondedNodes, nodeAddr)) {
        return BondResult(BondResult::Type::AlreadyUsed);
      }

      // all nodes are already bonded
      if (areAllUsed(bondedNodes)) {
        return BondResult(BondResult::Type::NoFreeSpace);
      }

      // bond a node
      BondResult bondResult = _bondNode(nodeAddr);

      // bonding node failed
      if (bondResult.getType() != BondResult::Type::NoError) {
        return bondResult;
      }

      // ping newly bonded node and return info
      bool pingSuccessful = false;
      TPerOSRead_Response readInfo;

      for (int i = 0; i < m_repeat; i++) {
        try {
          readInfo = pingNode(bondResult.getBondedAddr());
          bondResult.setReadInfo(readInfo);
          pingSuccessful = true;
          break;
        }
        catch (std::exception& ex) {
          TRC_ERROR("Ping attempt: " << PAR(i) << " failed.");
        } 
      }

      if (pingSuccessful) {
        return bondResult;
      }

      // if ping failed, remove bonded node from the coordinator's list
      removeBondedNode(bondResult.getBondedAddr());

      // and return Ping Failed error
      return BondResult(BondResult::Type::PingFailed, 0, 0);
    }

    
    void setOkResponse(Document& response, BondResult& bondResult) {
      Pointer("/status").Set(response, (int)BondResult::Type::NoError);
 
      // rsp object
      rapidjson::Pointer("/data/rsp/assignedAddr").Set(response, bondResult.getBondedAddr());
      rapidjson::Pointer("/data/rsp/nodesNr").Set(response, bondResult.getBondedNodesNum());

      // manufacturer name and product name
      // TODO fill from JsCache 
      rapidjson::Pointer("/data/rsp/manufacturer").Set(response, "");
      rapidjson::Pointer("/data/rsp/product").Set(response, "");

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
    }

    // creates response on the basis of bond result
    Document createResponse(
      const std::string& messagingId,
      const IMessagingSplitterService::MsgType& msgType,
      BondResult& bondResult,
      ComMngIqmeshBondNodeLocal& comBondNodeLocal
    )
    {
      Document response;
      std::unique_ptr<IDpaTransactionResult2> transResult = bondResult.getTransactionResult();

      switch (bondResult.getType()) {
        case BondResult::Type::AlreadyUsed:
          Pointer("/status").Set(response, (int)BondResult::Type::AlreadyUsed);
          break;
        case BondResult::Type::NoFreeSpace:
          Pointer("/status").Set(response, (int)BondResult::Type::NoFreeSpace);
          break;
        case BondResult::Type::PingFailed:
          Pointer("/status").Set(response, (int)BondResult::Type::PingFailed);
          break;
        case BondResult::Type::NoError:
          setOkResponse(response, bondResult);
          rapidjson::Pointer("/data/rsp/hwpId").Set(
            response, transResult->getResponse().DpaPacket().DpaResponsePacket_t.HWPID
          );
          break;
        case BondResult::Type::InternalError:
        // unsupported type - internal error
        default:
          Pointer("/status").Set(response, (int)BondResult::Type::InternalError);
          break;
      }

      // set common parameters
      Pointer("/mType").Set(response, msgType.m_type);
      Pointer("/data/msgId").Set(response, messagingId);

      // set raw fields, if verbose mode is active
      if (comBondNodeLocal.getVerbose()) {
        rapidjson::Pointer("/data/raw/request")
          .Set(response, encodeBinary(transResult->getRequest().DpaPacket().Buffer, transResult->getRequest().GetLength()));
        rapidjson::Pointer("/data/raw/requestTs")
          .Set(response, encodeTimestamp(transResult->getRequestTs()));
        rapidjson::Pointer("/data/raw/confirmation")
          .Set(response, encodeBinary(transResult->getConfirmation().DpaPacket().Buffer, transResult->getConfirmation().GetLength()));
        rapidjson::Pointer("/data/raw/confirmationTs")
          .Set(response, encodeTimestamp(transResult->getConfirmationTs()));
        rapidjson::Pointer("/data/raw/response")
          .Set(response, encodeBinary(transResult->getResponse().DpaPacket().Buffer, transResult->getResponse().GetLength()));
        rapidjson::Pointer("/data/raw/responseTs")
          .Set(response, encodeTimestamp(transResult->getResponseTs()));
      }
     
      return response;
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
      if (msgType.m_type != m_mTypeName_mngIqmeshBondNodeLocal) {
        THROW_EXC(std::logic_error, "Unsupported message type: " << PAR(msgType.m_type));
      }

      // creating representation object
      ComMngIqmeshBondNodeLocal comBondNodeLocal(doc);
      
      // pass request data as parameters into processing method
      m_repeat = comBondNodeLocal.getRepeat();

      BondResult bondResult;
      try {
        bondResult = bondNode(comBondNodeLocal.getDeviceAddr());
      }
      // all errors are generally taken as "internal errors"  
      catch (std::exception& ex) {
        bondResult = BondResult(BondResult::Type::InternalError);
      }

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

      m_msgType_mngIqmeshBondNodeLocal->m_handlerFunc = 
        [&](const std::string & messagingId, const IMessagingSplitterService::MsgType & msgType, rapidjson::Document doc)
      {
        handleMsg(messagingId, msgType, std::move(doc));
      };

      // for the sake of register function parameters 
      std::vector<IMessagingSplitterService::MsgType> supportedMsgTypes;
      supportedMsgTypes.push_back(*m_msgType_mngIqmeshBondNodeLocal);

      m_iMessagingSplitterService->registerFilteredMsgHandler(supportedMsgTypes);
      

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
      
      // for the sake of unregister function parameters 
      std::vector<IMessagingSplitterService::MsgType> supportedMsgTypes;
      supportedMsgTypes.push_back(*m_msgType_mngIqmeshBondNodeLocal);

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
