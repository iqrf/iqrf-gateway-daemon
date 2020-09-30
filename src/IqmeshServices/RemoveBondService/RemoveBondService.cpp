#define IRemoveBondService_EXPORTS

#include "RemoveBondService.h"
#include "Trace.h"
#include "ComIqmeshNetworkRemoveBond.h"
#include "iqrf__RemoveBondService.hxx"
#include <list>
#include <cmath>
#include <thread> 

TRC_INIT_MODULE(iqrf::RemoveBondService);

using namespace rapidjson;

namespace {
  // Service general fail code
  static const int SERVICE_ERROR = 1000;
};

namespace iqrf {
  // Holds information about errors, which encounter during local bond service run
  class RemoveBondError {
  public:
    // Type of error
    enum class Type {
      NoError,
      RemoveBondAtNodeSideError,
      RemoveBondAtCoordinatorSideError,
      ClearAllBondsError,
      BatchRemoveBondAndRestartNodeError,
      BatchRemoveBondAtCoordinatorSideError,
      GetBondedNodesError,
      FrcRemoveBondError,
      FrcSetParamsError,
      GetAddressingInfoError
    };

    RemoveBondError() : m_type(Type::NoError), m_message("ok") {};
    explicit RemoveBondError(Type errorType) : m_type(errorType), m_message("") {};
    RemoveBondError(Type errorType, const std::string& message) : m_type(errorType), m_message(message) {};

    Type getType() const { return m_type; };
    std::string getMessage() const { return m_message; };

    RemoveBondError(const RemoveBondError& other) {
      m_type = other.getType();
      m_message = other.getMessage();
    }

    RemoveBondError& operator=(const RemoveBondError& error) {
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
  class RemoveBondResult {
  private:
    RemoveBondError m_error;

    uint8_t m_nodesNr = 0;
    std::basic_string<uint8_t> m_notRemovedNodes;

    // transaction results
    std::list<std::unique_ptr<IDpaTransactionResult2>> m_transResults;

  public:
    RemoveBondError getError() const { return m_error; };

    void setError(const RemoveBondError& error) {
      m_error = error;
    }

    uint8_t getNodesNr() {
      return m_nodesNr;
    }

    void setNodesNr(const uint8_t nodesNr) {
      m_nodesNr = nodesNr;
    }

    void setNotRemovedNodes(const std::basic_string<uint8_t> &nodes)
    {
      m_notRemovedNodes = nodes;
    }

    std::basic_string<uint8_t> getNotRemovedNodes(void)
    {
      return m_notRemovedNodes;
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
      return tranResult;
    }

  };

  // Implementation class
  class RemoveBondService::Imp {
  private:
    // parent object
    RemoveBondService & m_parent;

    // message type: iqmesh network remove bond
    const std::string m_mTypeName_iqmeshNetworkRemoveBond = "iqmeshNetwork_RemoveBond";
    const std::string m_mTypeName_iqmeshNetworkRemoveBondOnlyInC = "iqmeshNetwork_RemoveBondOnlyInC";

    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    IIqrfDpaService* m_iIqrfDpaService = nullptr;
    std::unique_ptr<IIqrfDpaService::ExclusiveAccess> m_exclusiveAccess;
    const std::string* m_messagingId = nullptr;
    const IMessagingSplitterService::MsgType* m_msgType = nullptr;
    const ComIqmeshNetworkRemoveBond* m_comRemoveBond = nullptr;

    // Service input parameters
    TRemoveBondInputParams m_removeBondInputParams;

  public:
    explicit Imp(RemoveBondService& parent) : m_parent(parent)
    {
    }

    ~Imp()
    {
    }

    //-----------------
    // Get bonded nodes
    //-----------------
    std::basic_string<uint8_t> getBondedNodes(RemoveBondResult& removeBondResult)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage getBondedNodesRequest;
        DpaMessage::DpaPacket_t getBondedNodesPacket;
        getBondedNodesPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        getBondedNodesPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
        getBondedNodesPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_BONDED_DEVICES;
        getBondedNodesPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        getBondedNodesRequest.DataToBuffer(getBondedNodesPacket.Buffer, sizeof(TDpaIFaceHeader));
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(getBondedNodesRequest, transResult, m_removeBondInputParams.repeat);
        TRC_DEBUG("Result from CMD_COORDINATOR_BONDED_DEVICES transaction as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("CMD_COORDINATOR_BONDED_DEVICES OK.");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, getBondedNodesRequest.PeripheralType())
          << NAME_PAR(Node address, getBondedNodesRequest.NodeAddress())
          << NAME_PAR(Command, (int)getBondedNodesRequest.PeripheralCommand())
        );
        // Get response data
        std::basic_string<uint8_t> bondedNodes;
        bondedNodes.clear();
        for (uint8_t nodeAddr = 1; nodeAddr <= MAX_ADDRESS; nodeAddr++)
          if ((dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData[nodeAddr / 8] & (1 << (nodeAddr % 8))) != 0)
            bondedNodes.push_back(nodeAddr);
        removeBondResult.addTransactionResult(transResult);
        TRC_FUNCTION_LEAVE("");
        return bondedNodes;
      }
      catch (std::exception& e)
      {
        RemoveBondError error(RemoveBondError::Type::GetBondedNodesError, e.what());
        removeBondResult.setError(error);
        removeBondResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //------------------------
    // Returns addressing info
    //------------------------
    TPerCoordinatorAddrInfo_Response getAddressingInfo(RemoveBondResult& removeBondResult)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage addrInfoRequest;
        DpaMessage::DpaPacket_t addrInfoPacket;
        addrInfoPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        addrInfoPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
        addrInfoPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_ADDR_INFO;
        addrInfoPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        addrInfoRequest.DataToBuffer(addrInfoPacket.Buffer, sizeof(TDpaIFaceHeader));
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(addrInfoRequest, transResult, m_removeBondInputParams.repeat);
        TRC_DEBUG("Result from Get addressing information transaction as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("Get addressing information successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, addrInfoRequest.PeripheralType())
          << NAME_PAR(Node address, addrInfoRequest.NodeAddress())
          << NAME_PAR(Command, (int)addrInfoRequest.PeripheralCommand())
        );
        removeBondResult.addTransactionResult(transResult);
        removeBondResult.setNodesNr(dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerCoordinatorAddrInfo_Response.DevNr);
        TRC_FUNCTION_LEAVE("");
        return dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerCoordinatorAddrInfo_Response;
      }
      catch (const std::exception& e)
      {
        RemoveBondError error(RemoveBondError::Type::GetAddressingInfoError, e.what());
        removeBondResult.setError(error);
        removeBondResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //----------------------
    // Set FRC response time
    //----------------------
    uint8_t setFrcReponseTime(RemoveBondResult& removeBondResult, uint8_t FRCresponseTime)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage setFrcParamRequest;
        DpaMessage::DpaPacket_t setFrcParamPacket;
        setFrcParamPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        setFrcParamPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
        setFrcParamPacket.DpaRequestPacket_t.PCMD = CMD_FRC_SET_PARAMS;
        setFrcParamPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        setFrcParamPacket.DpaRequestPacket_t.DpaMessage.PerFrcSetParams_RequestResponse.FRCresponseTime = FRCresponseTime;
        setFrcParamRequest.DataToBuffer(setFrcParamPacket.Buffer, sizeof(TDpaIFaceHeader) + sizeof(TPerFrcSetParams_RequestResponse));
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(setFrcParamRequest, transResult, m_removeBondInputParams.repeat);
        TRC_DEBUG("Result from CMD_FRC_SET_PARAMS transaction as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("CMD_FRC_SET_PARAMS OK.");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, setFrcParamRequest.PeripheralType())
          << NAME_PAR(Node address, setFrcParamRequest.NodeAddress())
          << NAME_PAR(Command, (int)setFrcParamRequest.PeripheralCommand())
        );
        removeBondResult.addTransactionResult(transResult);
        TRC_FUNCTION_LEAVE("");
        return dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSetParams_RequestResponse.FRCresponseTime;
      }
      catch (std::exception& e)
      {
        RemoveBondError error(RemoveBondError::Type::FrcSetParamsError, e.what());
        removeBondResult.setError(error);
        removeBondResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //------------------------------
    // FRC_AcknowledgedBroadcastBits
    //------------------------------
    std::basic_string<uint8_t> FRCAcknowledgedBroadcastBits(RemoveBondResult& removeBondResult, const uint8_t PNUM, const uint8_t PCMD, const uint16_t hwpId, const std::basic_string<uint8_t>& data)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage frcAckBroadcastRequest;
        DpaMessage::DpaPacket_t frcAckBroadcastPacket;
        frcAckBroadcastPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        frcAckBroadcastPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
        frcAckBroadcastPacket.DpaRequestPacket_t.PCMD = CMD_FRC_SEND;
        frcAckBroadcastPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        // FRC - Acknowledge Broadcast - Bits
        frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.FrcCommand = FRC_AcknowledgedBroadcastBits;
        // Clear UserData
        memset((void*)frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData, 0, sizeof(frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData));
        // DPA request
        frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[0x00] = (uint8_t)(5 * sizeof(uint8_t) + data.size());
        frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[0x01] = PNUM;
        frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[0x02] = PCMD;
        frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[0x03] = hwpId & 0xff;
        frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[0x04] = hwpId >> 0x08;
        // Copy optional user data (if any)
        if (data.size() != 0)
          std::copy(data.begin(), data.end(), &frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[0x05]);
        // Data to buffer
        uint8_t requestLength = sizeof(TDpaIFaceHeader);
        requestLength += sizeof(frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.FrcCommand);
        requestLength += frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[0x00];
        frcAckBroadcastRequest.DataToBuffer(frcAckBroadcastPacket.Buffer, requestLength);
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(frcAckBroadcastRequest, transResult, m_removeBondInputParams.repeat);
        TRC_DEBUG("Result from FRC_AcknowledgedBroadcastBits transaction as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("FRC_AcknowledgedBroadcastBits OK.");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, frcAckBroadcastRequest.PeripheralType())
          << NAME_PAR(Node address, frcAckBroadcastRequest.NodeAddress())
          << NAME_PAR(Command, (int)frcAckBroadcastRequest.PeripheralCommand())
        );
        // Check FRC status
        uint8_t status = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.Status;
        if (status <= 0xfd)
        {
          TRC_INFORMATION("FRC_AcknowledgedBroadcastBits OK." << NAME_PAR_HEX("Status", (int)status));
          // Return nodes that executed DPA request (bit0 is set - the DPA Request is executed.)
          std::basic_string<uint8_t> acknowledgedNodes;
          acknowledgedNodes.clear();
          for (uint8_t nodeAddr = 1; nodeAddr <= MAX_ADDRESS; nodeAddr++)
            if ((dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.FrcData[nodeAddr / 8] & (1 << (nodeAddr % 8))) != 0)
              acknowledgedNodes.push_back(nodeAddr);
          removeBondResult.addTransactionResult(transResult);
          TRC_FUNCTION_LEAVE("");
          return acknowledgedNodes;
        }
        else
        {
          TRC_WARNING("FRC_AcknowledgedBroadcastBits NOK." << NAME_PAR_HEX("Status", (int)status));
          THROW_EXC(std::logic_error, "Bad FRC status: " << PAR((int)status));
        }
      }
      catch (std::exception& e)
      {
        RemoveBondError error(RemoveBondError::Type::FrcRemoveBondError, e.what());
        removeBondResult.setError(error);
        removeBondResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //-------------------
    // Remove bond at [c]
    //-------------------
    void coordRemoveBond(RemoveBondResult& removeBondResult, const uint8_t nodeAddr)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage removeBondRequest;
        DpaMessage::DpaPacket_t removeBondPacket;
        removeBondPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        removeBondPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
        removeBondPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_REMOVE_BOND;
        removeBondPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        removeBondPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorRemoveBond_Request.BondAddr = (uint8_t)nodeAddr;
        removeBondRequest.DataToBuffer(removeBondPacket.Buffer, sizeof(TDpaIFaceHeader) + sizeof(TPerCoordinatorRemoveBond_Request));
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(removeBondRequest, transResult, m_removeBondInputParams.repeat);
        TRC_DEBUG("Result from CMD_COORDINATOR_REMOVE_BOND transaction as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("CMD_COORDINATOR_REMOVE_BOND OK.");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, removeBondRequest.PeripheralType())
          << NAME_PAR(Node address, removeBondRequest.NodeAddress())
          << NAME_PAR(Command, (int)removeBondRequest.PeripheralCommand())
        );
        removeBondResult.addTransactionResult(transResult);
        TRC_FUNCTION_LEAVE("");
      }
      catch (std::exception& e)
      {
        RemoveBondError error(RemoveBondError::Type::RemoveBondAtCoordinatorSideError, e.what());
        removeBondResult.setError(error);
        removeBondResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //------------------
    // Remove bond batch
    //------------------
    void coordRemoveBondBatch(RemoveBondResult& removeBondResult, std::basic_string<uint8_t> &nodes)
    {
      TRC_FUNCTION_ENTER("");
      if (nodes.size() == 0)
      {
        TRC_FUNCTION_LEAVE("");
        return;
      }
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        uint8_t nodeIndex = 0x00;
        do
        {
          // Prepare DPA request
          DpaMessage removeBondRequest;
          DpaMessage::DpaPacket_t removeBondPacket;
          removeBondPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
          removeBondPacket.DpaRequestPacket_t.PNUM = PNUM_OS;
          removeBondPacket.DpaRequestPacket_t.PCMD = CMD_OS_BATCH;
          removeBondPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
          uint8_t index = 0x00;
          uint8_t numNodes = 0x00;
          do
          {
            removeBondPacket.DpaRequestPacket_t.DpaMessage.Request.PData[index++] = 0x06;
            removeBondPacket.DpaRequestPacket_t.DpaMessage.Request.PData[index++] = COORDINATOR_ADDRESS;
            removeBondPacket.DpaRequestPacket_t.DpaMessage.Request.PData[index++] = CMD_COORDINATOR_REMOVE_BOND;
            removeBondPacket.DpaRequestPacket_t.DpaMessage.Request.PData[index++] = HWPID_DoNotCheck & 0xFF;
            removeBondPacket.DpaRequestPacket_t.DpaMessage.Request.PData[index++] = (HWPID_DoNotCheck >> 0x08);
            removeBondPacket.DpaRequestPacket_t.DpaMessage.Request.PData[index++] = (uint8_t)nodes[nodeIndex++];
          } while ((++numNodes < 9) && (nodeIndex < nodes.size()));
          removeBondPacket.DpaRequestPacket_t.DpaMessage.Request.PData[index++] = 0x00;
          removeBondRequest.DataToBuffer(removeBondPacket.Buffer, sizeof(TDpaIFaceHeader) + index);
          // Execute the DPA request
          m_exclusiveAccess->executeDpaTransactionRepeat(removeBondRequest, transResult, m_removeBondInputParams.repeat);
          TRC_DEBUG("Result from CMD_OS_BATCH transaction as string:" << PAR(transResult->getErrorString()));
          DpaMessage dpaResponse = transResult->getResponse();
          TRC_INFORMATION("CMD_OS_BATCH OK.");
          TRC_DEBUG(
            "DPA transaction: "
            << NAME_PAR(Peripheral type, removeBondRequest.PeripheralType())
            << NAME_PAR(Node address, removeBondRequest.NodeAddress())
            << NAME_PAR(Command, (int)removeBondRequest.PeripheralCommand())
          );
          removeBondResult.addTransactionResult(transResult);
        } while (nodeIndex < nodes.size());
        TRC_FUNCTION_LEAVE("");
      }
      catch (std::exception& e)
      {
        RemoveBondError error(RemoveBondError::Type::BatchRemoveBondAtCoordinatorSideError, e.what());
        removeBondResult.setError(error);
        removeBondResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //----------------
    // Clear all bonds
    //----------------
    void clearAllBonds(RemoveBondResult& removeBondResult)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage clearAllBondsRequest;
        DpaMessage::DpaPacket_t clearAllBondsPacket;
        clearAllBondsPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        clearAllBondsPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
        clearAllBondsPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_CLEAR_ALL_BONDS;
        clearAllBondsPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        clearAllBondsRequest.DataToBuffer(clearAllBondsPacket.Buffer, sizeof(TDpaIFaceHeader));
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(clearAllBondsRequest, transResult, m_removeBondInputParams.repeat);
        TRC_DEBUG("Result from CMD_COORDINATOR_CLEAR_ALL_BONDS transaction as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("CMD_COORDINATOR_CLEAR_ALL_BONDS OK.");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, clearAllBondsRequest.PeripheralType())
          << NAME_PAR(Node address, clearAllBondsRequest.NodeAddress())
          << NAME_PAR(Command, (int)clearAllBondsRequest.PeripheralCommand())
        );
        removeBondResult.addTransactionResult(transResult);
        TRC_FUNCTION_LEAVE("");
      }
      catch (std::exception& e)
      {
        RemoveBondError error(RemoveBondError::Type::ClearAllBondsError, e.what());
        removeBondResult.setError(error);
        removeBondResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //------------
    // Remove bond
    //------------
    void nodeRemoveBond(RemoveBondResult& removeBondResult, const uint8_t nodeAddr, const uint16_t hwpId)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage removeBondRequest;
        DpaMessage::DpaPacket_t removeBondPacket;
        removeBondPacket.DpaRequestPacket_t.NADR = nodeAddr;
        removeBondPacket.DpaRequestPacket_t.PNUM = PNUM_NODE;
        removeBondPacket.DpaRequestPacket_t.PCMD = CMD_NODE_REMOVE_BOND;
        removeBondPacket.DpaRequestPacket_t.HWPID = hwpId;
        removeBondRequest.DataToBuffer(removeBondPacket.Buffer, sizeof(TDpaIFaceHeader));
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(removeBondRequest, transResult, m_removeBondInputParams.repeat);
        TRC_DEBUG("Result from CMD_NODE_REMOVE_BOND_ADDRESS transaction as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("CMD_NODE_REMOVE_BOND_ADDRESS OK.");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, removeBondRequest.PeripheralType())
          << NAME_PAR(Node address, removeBondRequest.NodeAddress())
          << NAME_PAR(Command, (int)removeBondRequest.PeripheralCommand())
        );
        removeBondResult.addTransactionResult(transResult);
        TRC_FUNCTION_LEAVE("");
      }
      catch (std::exception& e)
      {
        RemoveBondError error(RemoveBondError::Type::RemoveBondAtNodeSideError, e.what());
        removeBondResult.setError(error);
        removeBondResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //------------------
    // Remove bond batch
    //------------------
    void nodeRemoveBondBatch(RemoveBondResult& removeBondResult, const uint8_t nodeAddr, const uint16_t hwpId)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage removeBondRequest;
        DpaMessage::DpaPacket_t removeBondPacket;
        removeBondPacket.DpaRequestPacket_t.NADR = nodeAddr;
        removeBondPacket.DpaRequestPacket_t.PNUM = PNUM_OS;
        removeBondPacket.DpaRequestPacket_t.PCMD = CMD_OS_BATCH;
        removeBondPacket.DpaRequestPacket_t.HWPID = hwpId;
        removeBondPacket.DpaRequestPacket_t.DpaMessage.Request.PData[0x00] = 0x05;
        removeBondPacket.DpaRequestPacket_t.DpaMessage.Request.PData[0x01] = PNUM_NODE;
        removeBondPacket.DpaRequestPacket_t.DpaMessage.Request.PData[0x02] = CMD_NODE_REMOVE_BOND;
        removeBondPacket.DpaRequestPacket_t.DpaMessage.Request.PData[0x03] = hwpId & 0xFF;
        removeBondPacket.DpaRequestPacket_t.DpaMessage.Request.PData[0x04] = (hwpId >> 0x08);
        removeBondPacket.DpaRequestPacket_t.DpaMessage.Request.PData[0x05] = 0x05;
        removeBondPacket.DpaRequestPacket_t.DpaMessage.Request.PData[0x06] = PNUM_OS;
        removeBondPacket.DpaRequestPacket_t.DpaMessage.Request.PData[0x07] = CMD_OS_RESTART;
        removeBondPacket.DpaRequestPacket_t.DpaMessage.Request.PData[0x08] = hwpId & 0xFF;
        removeBondPacket.DpaRequestPacket_t.DpaMessage.Request.PData[0x09] = (hwpId >> 0x08);
        removeBondPacket.DpaRequestPacket_t.DpaMessage.Request.PData[0x0a] = 0x00;
        removeBondRequest.DataToBuffer(removeBondPacket.Buffer, sizeof(TDpaIFaceHeader));
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(removeBondRequest, transResult, m_removeBondInputParams.repeat);
        TRC_DEBUG("Result from CMD_OS_BATCH transaction as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("CMD_OS_BATCH OK.");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, removeBondRequest.PeripheralType())
          << NAME_PAR(Node address, removeBondRequest.NodeAddress())
          << NAME_PAR(Command, (int)removeBondRequest.PeripheralCommand())
        );
        removeBondResult.addTransactionResult(transResult);
        TRC_FUNCTION_LEAVE("");
      }
      catch (std::exception& e)
      {
        RemoveBondError error(RemoveBondError::Type::BatchRemoveBondAndRestartNodeError, e.what());
        removeBondResult.setError(error);
        removeBondResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //--------------------------
    // Creates and send response
    //--------------------------
    void createResponse(RemoveBondResult& removeBondResult)
    {
      Document response;

      // Set common parameters
      Pointer("/mType").Set(response, m_msgType->m_type);
      Pointer("/data/msgId").Set(response, m_comRemoveBond->getMsgId());

      // Set status
      RemoveBondError error = removeBondResult.getError();
      int status = 0;
      if (error.getType() == RemoveBondError::Type::NoError)
      {
        // Rsp object
        rapidjson::Pointer("/data/rsp/nodesNr").Set(response, removeBondResult.getNodesNr());

        // iqmeshNetwork_RemoveBond request ?
        if (m_msgType->m_type == m_mTypeName_iqmeshNetworkRemoveBond)
        {
          // Yes, add nodes that didn't set that didn't set FRC Acknowledged broadcast bit0 (remove bond failed)
          std::basic_string<uint8_t> nodes = removeBondResult.getNotRemovedNodes();
          if (nodes.size() != 0)
          {
            rapidjson::Value notRemovedNodesArray;
            notRemovedNodesArray.SetArray();
            Document::AllocatorType& allocator = response.GetAllocator();
            for (uint8_t n : nodes)
              notRemovedNodesArray.PushBack(n, allocator);
            Pointer("/data/rsp/removeBondFailedNodes").Set(response, notRemovedNodesArray);
          }
        }
      }
      else
        status = SERVICE_ERROR + (int)error.getType();

      // Set raw fields, if verbose mode is active
      if (m_comRemoveBond->getVerbose() == true)
      {
        rapidjson::Value rawArray(kArrayType);
        Document::AllocatorType& allocator = response.GetAllocator();

        while (removeBondResult.isNextTransactionResult())
        {
          std::unique_ptr<IDpaTransactionResult2> transResult = removeBondResult.consumeNextTransactionResult();
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

        // Add array into response document
        Pointer("/data/raw").Set(response, rawArray);
      }

      // Set status
      Pointer("/data/status").Set(response, status);
      Pointer("/data/statusStr").Set(response, error.getMessage());

      // Send message      
      m_iMessagingSplitterService->sendMessage(*m_messagingId, std::move(response));
    }

    //------------
    // Remove bond
    //------------
    void removeBond(RemoveBondResult& removeBondResult, const uint8_t nodeAddr, const uint16_t hwpId)
    {
      TRC_FUNCTION_ENTER("");

      try
      {
        // Get DPA version
        IIqrfDpaService::CoordinatorParameters coordParams = m_iIqrfDpaService->getCoordinatorParameters();

        // Get bonded nodes
        std::basic_string<uint8_t> bondedNodes = getBondedNodes(removeBondResult);

        // Broadcast address ?
        if (nodeAddr == BROADCAST_ADDRESS)
        {
          // Yes, remove all bonds
          if (coordParams.dpaVerWord < 0x0400)
          {
            // Remove all Nodes (brodacast batch - no check of removeBond at Nodes is performed)
            nodeRemoveBondBatch(removeBondResult, BROADCAST_ADDRESS, hwpId);
            // Clear all bonds at [C]
            clearAllBonds(removeBondResult);
          }
          else
          {
            // Set FRC response time 40 ms
            uint8_t initialFrcResponseTime = setFrcReponseTime(removeBondResult, IDpaTransaction2::FrcResponseTime::k40Ms);
            // FRCAcknowledgedBroadcastBits
            std::basic_string<uint8_t> removedNodes = FRCAcknowledgedBroadcastBits(removeBondResult, PNUM_NODE, CMD_NODE_REMOVE_BOND, hwpId, std::basic_string<uint8_t> {});
            // Switch FRC response time back to initial value
            setFrcReponseTime(removeBondResult, initialFrcResponseTime);
            // Remove the sucessfully removed nodes at C side too
            coordRemoveBondBatch(removeBondResult, removedNodes);
            // Check bonded nodes
            bondedNodes = getBondedNodes(removeBondResult);
            removeBondResult.setNotRemovedNodes(bondedNodes);
          }
          TRC_FUNCTION_LEAVE("");
        }
        else
        {
          // No, remove specified node (unicast request)
          if (coordParams.dpaVerWord < 0x0400)
            nodeRemoveBondBatch(removeBondResult, nodeAddr, hwpId);
          else
            nodeRemoveBond(removeBondResult, nodeAddr, hwpId);

          // Remove node at the coordinator's side
          coordRemoveBond(removeBondResult, nodeAddr);
        }

        // Get addressing info
        getAddressingInfo(removeBondResult);
        TRC_FUNCTION_LEAVE("");
      }
      catch (std::exception& e)
      {
        CATCH_EXC_TRC_WAR(std::exception, e, e.what());
        TRC_FUNCTION_LEAVE("");
      }
    }

    //------------
    // Remove bond
    //------------
    void removeBondOnlyInC(RemoveBondResult& removeBondResult)
    {
      TRC_FUNCTION_ENTER("");

      try
      {
        // Get DPA version
        IIqrfDpaService::CoordinatorParameters coordParams = m_iIqrfDpaService->getCoordinatorParameters();

        // Get bonded nodes
        std::basic_string<uint8_t> bondedNodes = getBondedNodes(removeBondResult);

        // Clear all bonds ?
        if (m_removeBondInputParams.clearAllBonds == true)
        {
          // Yes, clear all bonds
          clearAllBonds(removeBondResult);
        }
        else
        {
          // No, remove specified nodes only
          if (m_removeBondInputParams.deviceAddrList.size() != 0)
          {
            // One Node in the list ?
            if (m_removeBondInputParams.deviceAddrList.size() == 1)
              coordRemoveBond(removeBondResult, m_removeBondInputParams.deviceAddrList[0x00]);
            else
              coordRemoveBondBatch(removeBondResult, m_removeBondInputParams.deviceAddrList);
          }
        }

        // Get addressing info
        getAddressingInfo(removeBondResult);
        TRC_FUNCTION_LEAVE("");
      }
      catch (std::exception& e)
      {
        CATCH_EXC_TRC_WAR(std::exception, e, e.what());
        TRC_FUNCTION_LEAVE("");
      }
    }

    //---------------
    // Handle message
    //---------------
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
      if ((msgType.m_type != m_mTypeName_iqmeshNetworkRemoveBond) && (msgType.m_type != m_mTypeName_iqmeshNetworkRemoveBondOnlyInC))
      {
        THROW_EXC(std::logic_error, "Unsupported message type: " << PAR(msgType.m_type));
      }

      // Creating representation object
      ComIqmeshNetworkRemoveBond comRemoveBond(doc);

      // Parsing and checking service parameters
      try
      {
        m_removeBondInputParams = comRemoveBond.getRomveBondParams();
      }
      catch (std::exception& e)
      {
        CATCH_EXC_TRC_WAR(std::exception, e, "Error while parsing service input parameters.")
          // Create error response
          Document response;
        Pointer("/mType").Set(response, msgType.m_type);
        Pointer("/data/msgId").Set(response, comRemoveBond.getMsgId());
        // Set result
        Pointer("/data/status").Set(response, SERVICE_ERROR);
        Pointer("/data/statusStr").Set(response, e.what());
        m_iMessagingSplitterService->sendMessage(messagingId, std::move(response));
        TRC_FUNCTION_LEAVE("");
        return;
      }

      // Try to establish exclusive access
      try
      {
        m_exclusiveAccess = m_iIqrfDpaService->getExclusiveAccess();
      }
      catch (const std::exception &e)
      {
        const char* errorStr = e.what();
        TRC_WARNING("Error while establishing exclusive DPA access: " << PAR(errorStr));
        // Create error response
        Document response;
        Pointer("/mType").Set(response, msgType.m_type);
        Pointer("/data/msgId").Set(response, comRemoveBond.getMsgId());
        // Set result
        Pointer("/data/status").Set(response, SERVICE_ERROR);
        Pointer("/data/statusStr").Set(response, errorStr);
        m_iMessagingSplitterService->sendMessage(messagingId, std::move(response));
        TRC_FUNCTION_LEAVE("");
        return;
      }

      try
      {
        // Remove bond result
        RemoveBondResult removeBondResult;
        m_msgType = &msgType;
        m_messagingId = &messagingId;
        m_comRemoveBond = &comRemoveBond;

        // iqmeshNetwork_RemoveBond ?
        if (msgType.m_type == m_mTypeName_iqmeshNetworkRemoveBond)
        {
          uint8_t addr = m_removeBondInputParams.wholeNetwork == true ? BROADCAST_ADDRESS : m_removeBondInputParams.deviceAddr;
          uint16_t hwpId = m_removeBondInputParams.hwpId;
          removeBond(removeBondResult, addr, hwpId);
        }

        // iqmeshNetwork_RemoveBondOnlyInC ?
        if (msgType.m_type == m_mTypeName_iqmeshNetworkRemoveBondOnlyInC)
        {
          removeBondOnlyInC(removeBondResult);
        }

        // Create and send response
        createResponse(removeBondResult);
      }
      catch (std::exception& e)
      {
        CATCH_EXC_TRC_WAR(std::exception, e, e.what());
        TRC_FUNCTION_LEAVE("");
      }

      // Release exclusive access
      m_exclusiveAccess.reset();

      TRC_FUNCTION_LEAVE("");
    }

    void activate(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "************************************" << std::endl <<
        "RemoveBondService instance activate" << std::endl <<
        "************************************"
      );

      (void)props;

      // for the sake of register function parameters 
      std::vector<std::string> supportedMsgTypes =
      {
        m_mTypeName_iqmeshNetworkRemoveBond,
        m_mTypeName_iqmeshNetworkRemoveBondOnlyInC
      };

      m_iMessagingSplitterService->registerFilteredMsgHandler(
        supportedMsgTypes,
        [&](const std::string & messagingId, const IMessagingSplitterService::MsgType & msgType, rapidjson::Document doc)
        {
          handleMsg(messagingId, msgType, std::move(doc));
        });

      TRC_FUNCTION_LEAVE("")
    }

    void deactivate()
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "************************************" << std::endl <<
        "RemoveBondService instance deactivate" << std::endl <<
        "************************************"
      );

      std::vector<std::string> supportedMsgTypes =
      {
        m_mTypeName_iqmeshNetworkRemoveBond,
        m_mTypeName_iqmeshNetworkRemoveBondOnlyInC
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


  RemoveBondService::RemoveBondService()
  {
    m_imp = shape_new Imp(*this);
  }

  RemoveBondService::~RemoveBondService()
  {
    delete m_imp;
  }


  void RemoveBondService::attachInterface(iqrf::IIqrfDpaService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void RemoveBondService::detachInterface(iqrf::IIqrfDpaService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void RemoveBondService::attachInterface(iqrf::IMessagingSplitterService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void RemoveBondService::detachInterface(iqrf::IMessagingSplitterService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void RemoveBondService::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void RemoveBondService::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }


  void RemoveBondService::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void RemoveBondService::deactivate()
  {
    m_imp->deactivate();
  }

  void RemoveBondService::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
  }
}
