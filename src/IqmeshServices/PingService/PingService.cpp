#define IPingService_EXPORTS

#include "PingService.h"
#include "Trace.h"
#include "ComIqmeshNetworkPing.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "iqrf__PingService.hxx"
#include <list>
#include <memory>
#include <math.h>
#include <bitset>

TRC_INIT_MODULE(iqrf::PingService);

using namespace rapidjson;

namespace
{
  static const int serviceError = 1000;
  static const int parsingRequestError = 1001;
  static const int exclusiveAccessError = 1002;
};

namespace iqrf {

  // Holds information about result of read Tr configuration
  class PingResult
  {
  private:
    // Status
    int m_status = 0;
    std::string m_statusStr = "ok";

    // Bonded nodes
    std::basic_string<uint8_t> m_bondedNodes;

    // Ping result
    std::map<uint16_t, bool> m_pingResult;

    // Inaccessible nodes coubt
    uint8_t m_inaccessibleNodes;

    // Transaction results
    std::list<std::unique_ptr<IDpaTransactionResult2>> m_transResults;

  public:
    // Status
    int getStatus() const { return m_status; };
    std::string getStatusStr() const { return m_statusStr; };
    void setStatus(const int status) {
      m_status = status;
    }
    void setStatus(const int status, const std::string statusStr) {
      m_status = status;
      m_statusStr = statusStr;
    }
    void setStatusStr(const std::string statusStr) {
      m_statusStr = statusStr;
    }

    const uint8_t &getInaccessibleNodes() const
    {
      return m_inaccessibleNodes;
    }

    void setInaccessibleNodes(const uint8_t inaccessibleNodes)
    {
      m_inaccessibleNodes = inaccessibleNodes;
    }

    const std::basic_string<uint8_t> &getNodesList() const
    {
      return m_bondedNodes;
    }

    void setNodesList(const std::basic_string<uint8_t> &nodesList)
    {
      m_bondedNodes = nodesList;
    }

    const std::map<uint16_t, bool> &getPingResult() const
    {
      return m_pingResult;
    }

    void setPingResult(const uint16_t address, const bool result)
    {
      m_pingResult[address] = result;
    }

    // Adds transaction result into the list of results
    void addTransactionResult(std::unique_ptr<IDpaTransactionResult2>& transResult) {
      if (transResult != nullptr)
        m_transResults.push_back(std::move(transResult));
    }

    bool isNextTransactionResult() {
      return (m_transResults.size() > 0);
    }

    // Consumes the first element in the transaction results list
    std::unique_ptr<IDpaTransactionResult2> consumeNextTransactionResult() {
      std::list<std::unique_ptr<IDpaTransactionResult2>>::iterator iter = m_transResults.begin();
      std::unique_ptr<IDpaTransactionResult2> tranResult = std::move(*iter);
      m_transResults.pop_front();
      return tranResult;
    }
  };

  // Implementation class
  class PingService::Imp {
  private:
    // Parent object
    PingService& m_parent;

    // Message type: IQMESH Network Read TR Configuration
    const std::string m_mTypeName_iqmeshNetworkPing = "iqmeshNetwork_Ping";
    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    IIqrfDpaService* m_iIqrfDpaService = nullptr;
    std::unique_ptr<IIqrfDpaService::ExclusiveAccess> m_exclusiveAccess;
    const std::string* m_messagingId = nullptr;
    const IMessagingSplitterService::MsgType* m_msgType = nullptr;
    const ComIqmeshNetworkPing* m_comPing = nullptr;

    // Service input parameters
    TPingInputParams m_pingParams;

  public:
    explicit Imp(PingService& parent) : m_parent(parent)
    {
    }

    ~Imp()
    {
    }

    //-------------------------------------------
    // Convert nodes bitmap to Node address array
    //-------------------------------------------
    std::basic_string<uint8_t> bitmapToNodes(const uint8_t *nodesBitMap)
    {
      std::basic_string<uint8_t> nodesList;
      nodesList.clear();
      for (uint8_t i = 0; i <= MAX_ADDRESS; i++)
        if (nodesBitMap[i / 8] & (1 << (i % 8)))
          nodesList.push_back(i);
      return (nodesList);
    }

    //----------------------
    // Set FRC response time
    //----------------------
    IDpaTransaction2::FrcResponseTime setFrcReponseTime(PingResult& pingResult, IDpaTransaction2::FrcResponseTime FRCresponseTime)
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
        setFrcParamPacket.DpaRequestPacket_t.DpaMessage.PerFrcSetParams_RequestResponse.FrcParams = (uint8_t)FRCresponseTime;
        setFrcParamRequest.DataToBuffer(setFrcParamPacket.Buffer, sizeof(TDpaIFaceHeader) + sizeof(TPerFrcSetParams_RequestResponse));
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(setFrcParamRequest, transResult, m_pingParams.repeat);
        TRC_DEBUG("Result from Set Hops transaction as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("Set Hops successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, setFrcParamRequest.PeripheralType())
          << NAME_PAR(Node address, setFrcParamRequest.NodeAddress())
          << NAME_PAR(Command, (int)setFrcParamRequest.PeripheralCommand())
        );
        pingResult.addTransactionResult(transResult);
        TRC_FUNCTION_LEAVE("");
        return((IDpaTransaction2::FrcResponseTime)dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSetParams_RequestResponse.FrcParams);
      }
      catch (const std::exception& e)
      {
        pingResult.setStatus(transResult->getErrorCode(), e.what());
        pingResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //-----------------------------
    // Returns list of bonded nodes
    //-----------------------------
    std::basic_string<uint8_t> getBondedNodes(PingResult& pingResult)
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
        m_exclusiveAccess->executeDpaTransactionRepeat(getBondedNodesRequest, transResult, m_pingParams.repeat);
        TRC_DEBUG("Result from CMD_COORDINATOR_BONDED_DEVICES transaction as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("CMD_COORDINATOR_BONDED_DEVICES nodes successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, getBondedNodesRequest.PeripheralType())
          << NAME_PAR(Node address, getBondedNodesRequest.NodeAddress())
          << NAME_PAR(Command, (int)getBondedNodesRequest.PeripheralCommand())
        );
        // Get response data
        pingResult.addTransactionResult(transResult);
        std::basic_string<uint8_t> bondedNodes = bitmapToNodes(dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData);
        pingResult.setNodesList(bondedNodes);
        TRC_FUNCTION_LEAVE("");
        return(bondedNodes);
      }
      catch (const std::exception& e)
      {
        pingResult.setStatus(transResult->getErrorCode(), e.what());
        pingResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //---------------
    // Ping new nodes
    //---------------
    TPerFrcSend_Response FrcPingNodes(PingResult& pingResult)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage checkNewNodesRequest;
        DpaMessage::DpaPacket_t checkNewNodesPacket;
        checkNewNodesPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        checkNewNodesPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
        checkNewNodesPacket.DpaRequestPacket_t.PCMD = CMD_FRC_SEND;
        checkNewNodesPacket.DpaRequestPacket_t.HWPID = m_pingParams.hwpId;
        // FRC command - Ping
        checkNewNodesPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.FrcCommand = FRC_Ping;
        // User data
        checkNewNodesPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[0x00] = 0x00;
        checkNewNodesPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[0x01] = 0x00;
        // Data to buffer
        checkNewNodesRequest.DataToBuffer(checkNewNodesPacket.Buffer, sizeof(TDpaIFaceHeader) + 3);
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(checkNewNodesRequest, transResult, m_pingParams.repeat);
        TRC_DEBUG("Result from PNUM_FRC Ping transaction as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("Check new nodes ok!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, checkNewNodesRequest.PeripheralType())
          << NAME_PAR(Node address, checkNewNodesRequest.NodeAddress())
          << NAME_PAR(Command, (int)checkNewNodesRequest.PeripheralCommand())
        );
        pingResult.addTransactionResult(transResult);
        // Check FRC status
        uint8_t status = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.Status;
        if (status <= 0xEF)
        {
          TRC_INFORMATION("FRC_Ping: status OK." << NAME_PAR_HEX("Status", (int)status));
          TRC_FUNCTION_LEAVE("");
          return dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response;
        }
        else
        {
          TRC_WARNING("FRC_Ping: status NOK!" << NAME_PAR_HEX("Status", (int)status));
          THROW_EXC(std::logic_error, "Bad FRC status: " << PAR((int)status));
        }
      }
      catch (const std::exception& e)
      {
        pingResult.setStatus(transResult->getErrorCode(), e.what());
        pingResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //--------------------------
    // Creates and send response
    //--------------------------
    void createResponse(const int status, const std::string statusStr)
    {
      Document response;

      // Set common parameters
      Pointer("/mType").Set(response, m_msgType->m_type);
      Pointer("/data/msgId").Set(response, m_comPing->getMsgId());

      // Set status
      Pointer("/data/status").Set(response, status);
      Pointer("/data/statusStr").Set(response, statusStr);

      // Send message      
      m_iMessagingSplitterService->sendMessage(*m_messagingId, std::move(response));
    }

    //--------------------------
    // Creates and send response
    //--------------------------
    void createResponse(PingResult& pingResult)
    {
      Document response;

      // Set common parameters
      Pointer("/mType").Set(response, m_msgType->m_type);
      Pointer("/data/msgId").Set(response, m_comPing->getMsgId());

      // hwpId
      Pointer("/data/rsp/hwpId").Set(response, m_pingParams.hwpId);

      // nodesNr
      Pointer("/data/rsp/nodesNr").Set(response, pingResult.getNodesList().length());
     
      // inaccessibleNodesNr
      Pointer("/data/rsp/inaccessibleNodesNr").Set(response, pingResult.getInaccessibleNodes());

      // Set status
      int status = pingResult.getStatus();
      //if (status == 0)
      {
        // Array of objects
        Document::AllocatorType &allocator = response.GetAllocator();
        rapidjson::Value frcPingResult(kArrayType);
        std::map<uint16_t, bool> result = pingResult.getPingResult();
        for (std::map<uint16_t, bool>::iterator i = result.begin(); i != result.end(); ++i)
        {
          rapidjson::Value frcPingResultItem(kObjectType);
          frcPingResultItem.AddMember("address", i->first, allocator);
          frcPingResultItem.AddMember("result", i->second, allocator);
          frcPingResult.PushBack(frcPingResultItem, allocator);
        }
        Pointer("/data/rsp/pingResult").Set(response, frcPingResult);
      }
  
      // Set raw fields, if verbose mode is active
      if (m_comPing->getVerbose())
      {
        rapidjson::Value rawArray(kArrayType);
        Document::AllocatorType& allocator = response.GetAllocator();
        while (pingResult.isNextTransactionResult())
        {
          std::unique_ptr<IDpaTransactionResult2> transResult = pingResult.consumeNextTransactionResult();
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
          // Add object into array
          rawArray.PushBack(rawObject, allocator);
        }
        // Add array into response document
        Pointer("/data/raw").Set(response, rawArray);
      }

      // Set status
      Pointer("/data/status").Set(response, status);
      Pointer("/data/statusStr").Set(response, pingResult.getStatusStr());

      // Send message      
      m_iMessagingSplitterService->sendMessage(*m_messagingId, std::move(response));
    }

    //-------------
    // Ping service
    //-------------
    void ping(PingResult& pingResult)
    {
      TRC_FUNCTION_ENTER("");
      try
      {
        // Get bonded nodes
        getBondedNodes(pingResult);

        // Set calculated FRC response time
        m_iIqrfDpaService->setFrcResponseTime(IDpaTransaction2::FrcResponseTime::k40Ms);
        IDpaTransaction2::FrcResponseTime FRCresponseTime = setFrcReponseTime(pingResult, IDpaTransaction2::FrcResponseTime::k40Ms);

        // Ping nodes
        TPerFrcSend_Response response = FrcPingNodes(pingResult);

        // Finally set FRC param back to initial value
        m_iIqrfDpaService->setFrcResponseTime(FRCresponseTime);
        setFrcReponseTime(pingResult, FRCresponseTime);

        // Check the response   
        uint8_t inaccessibleNodes = 0;
        for (uint8_t addr : pingResult.getNodesList())
        {
          uint8_t byteIndex = addr / 8;
          uint8_t bitIndex = addr % 8;
          uint8_t bitMask = 1 << bitIndex;
          bool nodeOnline = ((response.FrcData[byteIndex] & bitMask) != 0);
          if (nodeOnline == false)
            inaccessibleNodes++;
          pingResult.setPingResult(addr, nodeOnline);
          pingResult.setInaccessibleNodes(inaccessibleNodes);
        }
        TRC_FUNCTION_LEAVE("");
      }
      catch (std::exception& e)
      {
        CATCH_EXC_TRC_WAR(std::exception, e, e.what());
        TRC_FUNCTION_LEAVE("");
      }
    }

    //-------------------
    // Handle the request
    //-------------------
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
      if (msgType.m_type != m_mTypeName_iqmeshNetworkPing)
        THROW_EXC(std::logic_error, "Unsupported message type: " << PAR(msgType.m_type));

      // Creating representation object
      ComIqmeshNetworkPing comPing(doc);
      m_msgType = &msgType;
      m_messagingId = &messagingId;
      m_comPing = &comPing;

      // Parsing and checking service parameters
      try
      {
        m_pingParams = comPing.getPingInputParams();
      }
      catch (const std::exception& e)
      {
        CATCH_EXC_TRC_WAR(std::exception, e, "Error while parsing service input parameters.");
        createResponse(parsingRequestError, e.what());
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
        CATCH_EXC_TRC_WAR(std::exception, e, "Exclusive access error.");
        createResponse(exclusiveAccessError, e.what());
        TRC_FUNCTION_LEAVE("");
        return;
      }

      try
      {
        // Read TR config result
        PingResult pingResult;

        // Ping nodes
        ping(pingResult);

        // Create and send response
        createResponse(pingResult);
      }
      catch (std::exception& e)
      {
        CATCH_EXC_TRC_WAR(std::exception, e, e.what());
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
        "ReadTrConfService instance activate" << std::endl <<
        "************************************"
      );

      (void)props;
      
      // for the sake of register function parameters 
      std::vector<std::string> supportedMsgTypes =
      {
        m_mTypeName_iqmeshNetworkPing
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
        "ReadTrConfService instance deactivate" << std::endl <<
        "**************************************"
      );

      // for the sake of unregister function parameters 
      std::vector<std::string> supportedMsgTypes =
      {
        m_mTypeName_iqmeshNetworkPing
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

  PingService::PingService()
  {
    m_imp = shape_new Imp(*this);
  }

  PingService::~PingService()
  {
    delete m_imp;
  }

  void PingService::attachInterface(iqrf::IIqrfDpaService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void PingService::detachInterface(iqrf::IIqrfDpaService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void PingService::attachInterface(iqrf::IMessagingSplitterService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void PingService::detachInterface(iqrf::IMessagingSplitterService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void PingService::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void PingService::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

  void PingService::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void PingService::deactivate()
  {
    m_imp->deactivate();
  }

  void PingService::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
  }
}