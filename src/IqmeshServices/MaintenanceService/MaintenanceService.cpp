#define IMaintenanceService_EXPORTS

#include "MaintenanceService.h"
#include "Trace.h"
#include "ComIqmeshNetworkMaintenance.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "iqrf__MaintenanceService.hxx"
#include <list>
#include <memory>
#include <math.h>
#include <bitset>

TRC_INIT_MODULE(iqrf::MaintenanceService);

using namespace rapidjson;

namespace
{
  static const int serviceError = 1000;
  static const int parsingRequestError = 1001;
  static const int exclusiveAccessError = 1002;
};

namespace iqrf {

  // Holds information about result of read Tr configuration
  class MaintenanceResult
  {
  private:
    // Status
    int m_status = 0;
    std::string m_statusStr = "ok";

    // Bonded nodes
    std::basic_string<uint8_t> m_bondedNodes;

    // Maintenance test RF result
    std::map<uint16_t, uint8_t> m_testRfResult;

    // Inaccessible nodes count
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

    const std::basic_string<uint8_t> &getBondedNodes() const
    {
      return m_bondedNodes;
    }

    void setBondedNodes(const std::basic_string<uint8_t> &nodesList)
    {
      m_bondedNodes = nodesList;
    }

    const std::map<uint16_t, uint8_t> &getTestRfResult() const
    {
      return m_testRfResult;
    }

    void setTestRfResult(const uint16_t address, const uint8_t counter)
    {
      m_testRfResult[address] = counter;
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
  class MaintenanceService::Imp {
  private:
    // Parent object
    MaintenanceService& m_parent;

    // Message type: IQMESH Network Read TR Configuration
    const std::string m_mTypeName_iqmeshNetwork_MaintenanceTestRF = "iqmeshNetwork_MaintenanceTestRF";
    const std::string m_mTypeName_iqmeshNetwork_MaintenanceUselessPrebondedNodes = "iqmeshNetwork_MaintenanceUselessPrebondedNodes";
    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    IIqrfDpaService* m_iIqrfDpaService = nullptr;
    std::unique_ptr<IIqrfDpaService::ExclusiveAccess> m_exclusiveAccess;
    const std::string* m_messagingId = nullptr;
    const IMessagingSplitterService::MsgType* m_msgType = nullptr;
    const ComIqmeshNetworkMaintenance* m_comMaintenance = nullptr;

    // Service input parameters
    TMaintenanceInputParams m_maintenanceParams;

  public:
    explicit Imp(MaintenanceService& parent) : m_parent(parent)
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
    IDpaTransaction2::FrcResponseTime setFrcReponseTime(MaintenanceResult& maintenanceResult, IDpaTransaction2::FrcResponseTime FRCresponseTime)
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
        m_exclusiveAccess->executeDpaTransactionRepeat(setFrcParamRequest, transResult, m_maintenanceParams.repeat);
        TRC_DEBUG("Result from Set Hops transaction as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("Set Hops successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, setFrcParamRequest.PeripheralType())
          << NAME_PAR(Node address, setFrcParamRequest.NodeAddress())
          << NAME_PAR(Command, (int)setFrcParamRequest.PeripheralCommand())
        );
        maintenanceResult.addTransactionResult(transResult);
        TRC_FUNCTION_LEAVE("");
        return((IDpaTransaction2::FrcResponseTime)dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSetParams_RequestResponse.FrcParams);
      }
      catch (const std::exception& e)
      {
        maintenanceResult.setStatus(transResult->getErrorCode(), e.what());
        maintenanceResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //-----------------------------
    // Returns list of bonded nodes
    //-----------------------------
    std::basic_string<uint8_t> getBondedNodes(MaintenanceResult& maintenanceResult)
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
        m_exclusiveAccess->executeDpaTransactionRepeat(getBondedNodesRequest, transResult, m_maintenanceParams.repeat);
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
        maintenanceResult.addTransactionResult(transResult);
        std::basic_string<uint8_t> bondedNodes = bitmapToNodes(dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData);
        maintenanceResult.setBondedNodes(bondedNodes);
        TRC_FUNCTION_LEAVE("");
        return(bondedNodes);
      }
      catch (const std::exception& e)
      {
        maintenanceResult.setStatus(transResult->getErrorCode(), e.what());
        maintenanceResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //---------------------
    // Get FRC extra result
    //---------------------
    DpaMessage getFrcExtraResult(MaintenanceResult& maintenanceResult)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Read FRC extra results
        DpaMessage extraResultRequest;
        DpaMessage::DpaPacket_t extraResultPacket;
        extraResultPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        extraResultPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
        extraResultPacket.DpaRequestPacket_t.PCMD = CMD_FRC_EXTRARESULT;
        extraResultPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        extraResultRequest.DataToBuffer(extraResultPacket.Buffer, sizeof(TDpaIFaceHeader));
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(extraResultRequest, transResult, m_maintenanceParams.repeat);
        TRC_DEBUG("Result from FRC CMD_FRC_EXTRARESULT as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("FRC CMD_FRC_EXTRARESULT successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, extraResultRequest.PeripheralType())
          << NAME_PAR(Node address, extraResultRequest.NodeAddress())
          << NAME_PAR(Command, (int)extraResultRequest.PeripheralCommand())
        );
        TRC_FUNCTION_LEAVE("");
        return(dpaResponse);
      }
      catch (const std::exception& e)
      {
        maintenanceResult.setStatus(transResult->getErrorCode(), e.what());
        maintenanceResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //----------------------------
    // Test RF signal at all Nodes
    //----------------------------
    void testRfSignalAtNodes(MaintenanceResult& maintenanceResult)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        std::basic_string<uint8_t> nodesList;
        nodesList.clear();
        nodesList = maintenanceResult.getBondedNodes();
        DpaMessage frcSendSelectiveRequest;
        DpaMessage::DpaPacket_t frcSendSelectivePacket;

        // Test RF signal at all nodes
        while (nodesList.size() > 0)
        {
          frcSendSelectivePacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
          frcSendSelectivePacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
          frcSendSelectivePacket.DpaRequestPacket_t.PCMD = CMD_FRC_SEND_SELECTIVE;
          frcSendSelectivePacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
          // Initialize command to FRC_MemoryReadPlus1
          frcSendSelectivePacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.FrcCommand = FRC_TestRFsignal;
          // Initialize SelectedNodes
          memset(frcSendSelectivePacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.SelectedNodes, 0, 30 * sizeof(uint8_t));
          std::list<uint16_t> selectedNodes;
          selectedNodes.clear();
          do
          {
            uint8_t addr = nodesList.front();
            selectedNodes.push_back(addr);
            nodesList.erase(std::find(nodesList.begin(), nodesList.end(), addr));
            uint8_t byteIndex = (uint8_t)(addr / 8);
            uint8_t bitIndex = addr % 8;
            frcSendSelectivePacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.SelectedNodes[byteIndex] |= (0x01 << bitIndex);
          } while (nodesList.size() != 0 && selectedNodes.size() < 63);
          // Initialize user data to zero
          memset(frcSendSelectivePacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData, 0, 25 * sizeof(uint8_t));
          // Put RF channel and RF filter
          frcSendSelectivePacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0] = m_maintenanceParams.RFchannel;
          frcSendSelectivePacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[1] = m_maintenanceParams.RFfilter;
          frcSendSelectiveRequest.DataToBuffer(frcSendSelectivePacket.Buffer, sizeof(TDpaIFaceHeader) + 1 + 30 + 2);
          // Execute the DPA request
          m_exclusiveAccess->executeDpaTransactionRepeat(frcSendSelectiveRequest, transResult, m_maintenanceParams.repeat);
          TRC_DEBUG("Result from CMD_FRC_SEND_SELECTIVE as string:" << PAR(transResult->getErrorString()));
          DpaMessage dpaResponse = transResult->getResponse();
          TRC_INFORMATION("CMD_FRC_SEND_SELECTIVE successful!");
          TRC_DEBUG(
            "DPA transaction: "
            << NAME_PAR(Peripheral type, frcSendSelectiveRequest.PeripheralType())
            << NAME_PAR(Node address, frcSendSelectiveRequest.NodeAddress())
            << NAME_PAR(Command, (int)frcSendSelectiveRequest.PeripheralCommand())
          );
          maintenanceResult.addTransactionResult(transResult);
          // Check FRC status
          uint8_t frcStatus = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.Status;
          if (frcStatus > 0xef)
          {
            TRC_WARNING("Selective FRC Verify code failed." << NAME_PAR_HEX("Status", (int)frcStatus));
            THROW_EXC(std::logic_error, "Bad FRC status: " << PAR((int)frcStatus));
          }
          // Process FRC data
          std::basic_string<uint8_t> frcData;
          frcData.append(&dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData[2], 54);
          // Get extra result
          if (selectedNodes.size() > 54)
          {
            DpaMessage frcExtraResult = getFrcExtraResult(maintenanceResult);
            frcData.append(frcExtraResult.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData, 9);
          }
          // Set counter of test RF signal
          uint8_t frcPDataIndex = 0;
          uint8_t inaccessibleNodes = 0;
          for (uint16_t addr : selectedNodes)
          {
            uint8_t counter = frcData[frcPDataIndex++];
            maintenanceResult.setTestRfResult(addr, counter);
            if (counter == 0)
              inaccessibleNodes++;
          }
          maintenanceResult.setInaccessibleNodes(inaccessibleNodes);
        }
        TRC_FUNCTION_LEAVE("");
      }
      catch (const std::exception& e)
      {
        maintenanceResult.setStatus(transResult->getErrorCode(), e.what());
        maintenanceResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //------------------------------
    // Test RF signal at Coordinator
    //------------------------------
    void testRfSignalAtCoord(MaintenanceResult& maintenanceResult)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage testRfSignalRequest;
        DpaMessage::DpaPacket_t testRfSignalPacket;
        testRfSignalPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        testRfSignalPacket.DpaRequestPacket_t.PNUM = PNUM_OS;
        testRfSignalPacket.DpaRequestPacket_t.PCMD = CMD_OS_TEST_RF_SIGNAL;
        testRfSignalPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        // Put Channel, RXfilter and Time
        testRfSignalPacket.DpaRequestPacket_t.DpaMessage.PerOSTestRfSignal_Request.Channel = m_maintenanceParams.RFchannel;
        testRfSignalPacket.DpaRequestPacket_t.DpaMessage.PerOSTestRfSignal_Request.RXfilter = m_maintenanceParams.RFfilter;
        testRfSignalPacket.DpaRequestPacket_t.DpaMessage.PerOSTestRfSignal_Request.Time = (uint16_t)(m_maintenanceParams.measurementTimeMS / 10);
        testRfSignalRequest.DataToBuffer(testRfSignalPacket.Buffer, sizeof(TDpaIFaceHeader) + sizeof(TPerOSTestRfSignal_Request));
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(testRfSignalRequest, transResult, m_maintenanceParams.repeat, m_maintenanceParams.measurementTimeMS + 100);
        TRC_DEBUG("Result from Set Hops transaction as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("Set Hops successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, testRfSignalRequest.PeripheralType())
          << NAME_PAR(Node address, testRfSignalRequest.NodeAddress())
          << NAME_PAR(Command, (int)testRfSignalRequest.PeripheralCommand())
        );
        maintenanceResult.setTestRfResult(COORDINATOR_ADDRESS, dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerOSTestRfSignal_Response.Counter);
        maintenanceResult.addTransactionResult(transResult);
        TRC_FUNCTION_LEAVE("");
      }
      catch (const std::exception& e)
      {
        maintenanceResult.setStatus(transResult->getErrorCode(), e.what());
        maintenanceResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //------------------------------------
    // Unbond nodes with temporary address
    //------------------------------------
    void unbondTemporaryAddress(MaintenanceResult& maintenanceResult)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        DpaMessage validateBondRequest;
        DpaMessage::DpaPacket_t validateBondPacket;
        validateBondPacket.DpaRequestPacket_t.NADR = BROADCAST_ADDRESS;
        validateBondPacket.DpaRequestPacket_t.PNUM = PNUM_NODE;
        validateBondPacket.DpaRequestPacket_t.PCMD = CMD_NODE_VALIDATE_BONDS;
        validateBondPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        validateBondPacket.DpaRequestPacket_t.DpaMessage.PerNodeValidateBonds_Request.Bonds[0x00].Address = TEMPORARY_ADDRESS;
        validateBondPacket.DpaRequestPacket_t.DpaMessage.PerNodeValidateBonds_Request.Bonds[0x00].MID[0x00] = 0x00;
        validateBondPacket.DpaRequestPacket_t.DpaMessage.PerNodeValidateBonds_Request.Bonds[0x00].MID[0x01] = 0x00;
        validateBondPacket.DpaRequestPacket_t.DpaMessage.PerNodeValidateBonds_Request.Bonds[0x00].MID[0x02] = 0x00;
        validateBondPacket.DpaRequestPacket_t.DpaMessage.PerNodeValidateBonds_Request.Bonds[0x00].MID[0x03] = 0x00;
        // Data to buffer
        validateBondRequest.DataToBuffer(validateBondPacket.Buffer, sizeof(TDpaIFaceHeader) + sizeof(TPerNodeValidateBondsItem));
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(validateBondRequest, transResult, m_maintenanceParams.repeat);
        TRC_INFORMATION("CMD_NODE_VALIDATE_BONDS ok!");
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, validateBondRequest.PeripheralType())
          << NAME_PAR(Node address, validateBondRequest.NodeAddress())
          << NAME_PAR(Command, (int)validateBondRequest.PeripheralCommand())
        );
        maintenanceResult.addTransactionResult(transResult);
        TRC_FUNCTION_LEAVE("");
      }
      catch (const std::exception& e)
      {
        maintenanceResult.setStatus(transResult->getErrorCode(), e.what());
        maintenanceResult.addTransactionResult(transResult);
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
      Pointer("/data/msgId").Set(response, m_comMaintenance->getMsgId());

      // Set status
      Pointer("/data/status").Set(response, status);
      Pointer("/data/statusStr").Set(response, statusStr);

      // Send message      
      m_iMessagingSplitterService->sendMessage(*m_messagingId, std::move(response));
    }

    //--------------------------
    // Creates and send response
    //--------------------------
    void createResponse(MaintenanceResult& maintenanceResult)
    {
      Document response;

      // Set common parameters
      Pointer("/mType").Set(response, m_msgType->m_type);
      Pointer("/data/msgId").Set(response, m_comMaintenance->getMsgId());

      // nodesNr
      Pointer("/data/rsp/nodesNr").Set(response, maintenanceResult.getBondedNodes().length());

      // Set status
      int status = maintenanceResult.getStatus();
      if (status == 0)
      {
        // iqmeshNetwork_MaintenanceTestRF ?
        if (m_msgType->m_type == m_mTypeName_iqmeshNetwork_MaintenanceTestRF)
        {
          if (m_maintenanceParams.deviceAddr == BROADCAST_ADDRESS)
          {
            // inaccessibleNodesNr
            Pointer("/data/rsp/inaccessibleNodesNr").Set(response, maintenanceResult.getInaccessibleNodes());
          }

          // Array of objects
          Document::AllocatorType &allocator = response.GetAllocator();
          rapidjson::Value frcRestartResult(kArrayType);
          std::map<uint16_t, uint8_t> result = maintenanceResult.getTestRfResult();
          for (std::map<uint16_t, uint8_t>::iterator i = result.begin(); i != result.end(); ++i)
          {
            rapidjson::Value frcRestartResultItem(kObjectType);
            frcRestartResultItem.AddMember("deviceAddr", i->first, allocator);
            bool online = i->second != 0;
            frcRestartResultItem.AddMember("online", online, allocator);
            if (online == true)
              frcRestartResultItem.AddMember("counter", i->second - 1, allocator);
            frcRestartResult.PushBack(frcRestartResultItem, allocator);
          }
          Pointer("/data/rsp/testRfResult").Set(response, frcRestartResult);
        }
      }
  
      // Set raw fields, if verbose mode is active
      if (m_comMaintenance->getVerbose())
      {
        rapidjson::Value rawArray(kArrayType);
        Document::AllocatorType& allocator = response.GetAllocator();
        while (maintenanceResult.isNextTransactionResult())
        {
          std::unique_ptr<IDpaTransactionResult2> transResult = maintenanceResult.consumeNextTransactionResult();
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
      Pointer("/data/statusStr").Set(response, maintenanceResult.getStatusStr());

      // Send message      
      m_iMessagingSplitterService->sendMessage(*m_messagingId, std::move(response));
    }

    //---------------
    // Test RF signal
    //---------------
    void testRfSignal(MaintenanceResult& maintenanceResult)
    {
      TRC_FUNCTION_ENTER("");
      try
      {
        // Get bonded nodes
        getBondedNodes(maintenanceResult);

        // Test RF signal at [C] ?
        if (m_maintenanceParams.deviceAddr == COORDINATOR_ADDRESS)
        {
          testRfSignalAtCoord(maintenanceResult);
        }
        else
        {
          // Set calculated FRC response time
          m_iIqrfDpaService->setFrcResponseTime(m_maintenanceParams.measurementTime);
          IDpaTransaction2::FrcResponseTime FRCresponseTime = setFrcReponseTime(maintenanceResult, m_maintenanceParams.measurementTime);

          // Test RF signal
          testRfSignalAtNodes(maintenanceResult);

          // Finally set FRC param back to initial value
          m_iIqrfDpaService->setFrcResponseTime(FRCresponseTime);
          setFrcReponseTime(maintenanceResult, FRCresponseTime);
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
      TRC_FUNCTION_ENTER( PAR(messagingId) <<
        NAME_PAR(mType, msgType.m_type) <<
        NAME_PAR(major, msgType.m_major) <<
        NAME_PAR(minor, msgType.m_minor) <<
        NAME_PAR(micro, msgType.m_micro)
      );

      // Unsupported type of request
      if (msgType.m_type != m_mTypeName_iqmeshNetwork_MaintenanceTestRF &&
        msgType.m_type != m_mTypeName_iqmeshNetwork_MaintenanceUselessPrebondedNodes)
      {
        THROW_EXC(std::logic_error, "Unsupported message type: " << PAR(msgType.m_type));
      }

      // Creating representation object
      ComIqmeshNetworkMaintenance comMaintenance(doc);
      m_msgType = &msgType;
      m_messagingId = &messagingId;
      m_comMaintenance = &comMaintenance;

      // Parsing and checking service parameters
      try
      {
        m_maintenanceParams = comMaintenance.getMaintenanceInputParams();
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

      // MaintenanceResult
      MaintenanceResult maintenanceResult;

      try
      {
        // Get bonded nodes
        getBondedNodes(maintenanceResult);

        // iqmeshNetwork_MaintenanceTestRF ?
        if (msgType.m_type == m_mTypeName_iqmeshNetwork_MaintenanceTestRF)
        {
          // Test RF signal at [C] ?
          if (m_maintenanceParams.deviceAddr == COORDINATOR_ADDRESS)
          {
            // Test RF signal at Coordinator
            testRfSignalAtCoord(maintenanceResult);
          }
          else
          {
            // Set requested FRC response time
            m_iIqrfDpaService->setFrcResponseTime(m_maintenanceParams.measurementTime);
            IDpaTransaction2::FrcResponseTime FRCresponseTime = setFrcReponseTime(maintenanceResult, m_maintenanceParams.measurementTime);

            // Test RF signal at all Nodes
            testRfSignalAtNodes(maintenanceResult);

            // Finally set FRC param back to initial value
            m_iIqrfDpaService->setFrcResponseTime(FRCresponseTime);
            setFrcReponseTime(maintenanceResult, FRCresponseTime);
          }
        }

        // m_mTypeName_iqmeshNetwork_MaintenanceUselessPrebondedNodes ?
        if (msgType.m_type == m_mTypeName_iqmeshNetwork_MaintenanceUselessPrebondedNodes)
        {
          // Resolve useless Prebonded Nodes
          unbondTemporaryAddress(maintenanceResult);
        }
      }
      catch (std::exception& e)
      {
        CATCH_EXC_TRC_WAR(std::exception, e, e.what());
      }

      // Release exclusive access
      m_exclusiveAccess.reset();

      // Create and send response
      createResponse(maintenanceResult);

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
        m_mTypeName_iqmeshNetwork_MaintenanceTestRF,
        m_mTypeName_iqmeshNetwork_MaintenanceUselessPrebondedNodes
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
        m_mTypeName_iqmeshNetwork_MaintenanceTestRF,
        m_mTypeName_iqmeshNetwork_MaintenanceUselessPrebondedNodes
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

  MaintenanceService::MaintenanceService()
  {
    m_imp = shape_new Imp(*this);
  }

  MaintenanceService::~MaintenanceService()
  {
    delete m_imp;
  }

  void MaintenanceService::attachInterface(iqrf::IIqrfDpaService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void MaintenanceService::detachInterface(iqrf::IIqrfDpaService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void MaintenanceService::attachInterface(iqrf::IMessagingSplitterService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void MaintenanceService::detachInterface(iqrf::IMessagingSplitterService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void MaintenanceService::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void MaintenanceService::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

  void MaintenanceService::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void MaintenanceService::deactivate()
  {
    m_imp->deactivate();
  }

  void MaintenanceService::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
  }
}