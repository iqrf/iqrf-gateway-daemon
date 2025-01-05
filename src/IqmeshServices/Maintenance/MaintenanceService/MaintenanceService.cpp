/**
 * Copyright 2015-2025 IQRF Tech s.r.o.
 * Copyright 2019-2025 MICRORISC s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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

TRC_INIT_MODULE(iqrf::MaintenanceService)

using namespace rapidjson;

namespace
{
  static const int serviceError = 1000;
  static const int parsingRequestError = 1001;
  static const int exclusiveAccessError = 1002;
  static const int noBondedNodesError = 1003;
}

namespace iqrf {

  // Holds information about result of read Tr configuration
  class MaintenanceResult
  {
  public:
    // MID union
    typedef union
    {
      uint8_t bytes[sizeof(uint32_t)];
      uint32_t value;
    }TMID;

  private:
    // Status
    int m_status = 0;
    std::string m_statusStr = "ok";

    // Bonded nodes
    std::basic_string<uint8_t> m_bondedNodes;

    // Maintenance test RF result
    std::map<uint16_t, uint8_t> m_testRfResult;

    // Nodes MID map
    std::map<uint16_t, TMID> m_nodesMidMapCoord;
    std::map<uint16_t, TMID> m_nodesMidMap;

    // Inaccessible nodes count
    uint8_t m_inaccessibleNodesNr;

    // Inaccessible nodes
    std::basic_string<uint8_t> m_inaccessibleNodes;

    // Inconsistent nodes count
    uint8_t m_inconsistentNodesNr;

    // MID Inconsistent nodes
    std::basic_string<uint8_t> m_inconsistentNodes;

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

    const uint8_t &getInaccessibleNodesNr() const
    {
      return m_inaccessibleNodesNr;
    }
    void setInaccessibleNodesNr(const uint8_t inaccessibleNodesNr)
    {
      m_inaccessibleNodesNr = inaccessibleNodesNr;
    }

    const uint8_t &getInconsistentNodesNr() const
    {
      return m_inconsistentNodesNr;
    }
    void setInconsistentNodesNr(const uint8_t inconsistentNodesNr)
    {
      m_inconsistentNodesNr = inconsistentNodesNr;
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

    const std::map<uint16_t, TMID> &getNodesMidMapCoord() const
    {
      return m_nodesMidMapCoord;
    }
    void setNodesMidMapCoord(const uint16_t address, const TMID mid)
    {
      m_nodesMidMapCoord[address].value = mid.value;
    }
    void clearNodesMidMapCoord(void)
    {
      m_nodesMidMapCoord.clear();
    }

    const std::map<uint16_t, TMID> &getNodesMidMap() const
    {
      return m_nodesMidMap;
    }
    void setNodesMidMap(const uint16_t address, const TMID mid)
    {
      m_nodesMidMap[address].value = mid.value;
    }
    void clearNodesMidMap(void)
    {
      m_nodesMidMap.clear();
    }

    const std::basic_string<uint8_t> &getInconsistentNodes() const
    {
      return m_inconsistentNodes;
    }
    void setInconsistentNodes(const std::basic_string<uint8_t> &inconsistentNodes)
    {
      m_inconsistentNodes = inconsistentNodes;
    }

    const std::basic_string<uint8_t> &getInaccessibleNodes() const
    {
      return m_inaccessibleNodes;
    }
    void setInaccessibleNodes(const std::basic_string<uint8_t> &inaccessibleNodes)
    {
      m_inaccessibleNodes = inaccessibleNodes;
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
    const std::string m_mTypeName_iqmeshNetwork_MaintenanceInconsistentMIDsInCoord = "iqmeshNetwork_MaintenanceInconsistentMIDsInCoord";
    const std::string m_mTypeName_iqmeshNetwork_MaintenanceDuplicatedAddresses = "iqmeshNetwork_MaintenanceDuplicatedAddresses";
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
          frcSendSelectivePacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0] = m_maintenanceParams.rfChannel;
          frcSendSelectivePacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[1] = m_maintenanceParams.rxFilter;
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
          // Check FRC status
          uint8_t frcStatus = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.Status;
          if (frcStatus > MAX_ADDRESS)
          {
            TRC_WARNING("Selective FRC Verify code failed." << NAME_PAR_HEX("Status", (int)frcStatus));
            THROW_EXC(std::logic_error, "Bad FRC status: " << PAR((int)frcStatus));
          }
          // Add FRC result
          maintenanceResult.addTransactionResult(transResult);
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
          uint8_t inaccessibleNodesNr = 0;
          for (uint16_t addr : selectedNodes)
          {
            uint8_t counter = frcData[frcPDataIndex++];
            maintenanceResult.setTestRfResult(addr, counter);
            if (counter == 0)
              inaccessibleNodesNr++;
          }
          maintenanceResult.setInaccessibleNodesNr(inaccessibleNodesNr);
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
        testRfSignalPacket.DpaRequestPacket_t.DpaMessage.PerOSTestRfSignal_Request.Channel = m_maintenanceParams.rfChannel;
        testRfSignalPacket.DpaRequestPacket_t.DpaMessage.PerOSTestRfSignal_Request.RXfilter = m_maintenanceParams.rxFilter;
        testRfSignalPacket.DpaRequestPacket_t.DpaMessage.PerOSTestRfSignal_Request.Time = (uint16_t)(m_maintenanceParams.measurementTimeMS / 10);
        testRfSignalRequest.DataToBuffer(testRfSignalPacket.Buffer, sizeof(TDpaIFaceHeader) + sizeof(TPerOSTestRfSignal_Request));
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(testRfSignalRequest, transResult, m_maintenanceParams.repeat, m_maintenanceParams.measurementTimeMS + 100);
        TRC_DEBUG("Result from CMD_OS_TEST_RF_SIGNAL as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("CMD_OS_TEST_RF_SIGNAL successful!");
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

    //-----------------
    // FRC_MemoryRead4B
    //-----------------
    std::basic_string<uint8_t> FRC_MemoryRead4BPlus1(MaintenanceResult& maintenanceResult, const std::basic_string<uint8_t>& selectedNodes, const uint16_t address)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage memoryReadRequest;
        DpaMessage::DpaPacket_t memoryReadPacket;
        memoryReadPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        memoryReadPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
        memoryReadPacket.DpaRequestPacket_t.PCMD = CMD_FRC_SEND_SELECTIVE;
        memoryReadPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        // FRC Command
        memoryReadPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.FrcCommand = FRC_MemoryRead4B;
        // Selected nodes - prebonded alive nodes
        memset(memoryReadPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.SelectedNodes, 0, 30 * sizeof(uint8_t));
        for (uint8_t i : selectedNodes)
          memoryReadPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.SelectedNodes[i / 0x08] |= (0x01 << (i % 8));
        // Inc, 0
        memoryReadPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x00] = 0x01;
        memoryReadPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x01] = 0x00;
        // OS Read command
        memoryReadPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x02] = address & 0xff;
        memoryReadPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x03] = address >> 0x08;
        memoryReadPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x04] = PNUM_OS;
        memoryReadPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x05] = CMD_OS_READ;
        memoryReadPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x06] = 0x00;
        memoryReadRequest.DataToBuffer(memoryReadPacket.Buffer, sizeof(TDpaIFaceHeader) + 38);
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(memoryReadRequest, transResult, m_maintenanceParams.repeat);
        TRC_DEBUG("Result from FRC_MemoryRead4B transaction as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("FRC_MemoryRead4B successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, memoryReadRequest.PeripheralType())
          << NAME_PAR(Node address, memoryReadRequest.NodeAddress())
          << NAME_PAR(Command, (int)memoryReadRequest.PeripheralCommand())
        );
        // Data from FRC
        std::basic_string<uint8_t> memoryData;
        memoryData.clear();
        // Check status
        uint8_t status = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.Status;
        if (status <= MAX_ADDRESS)
        {
          TRC_INFORMATION("FRC_MemoryRead4B status ok." << NAME_PAR_HEX("Status", (int)status));
          memoryData.append(dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.FrcData + sizeof(uint32_t), 51);
          TRC_DEBUG("Size of FRC data: " << PAR(memoryData.size()));
        }
        else
        {
          TRC_WARNING("FRC_MemoryRead4B NOT ok." << NAME_PAR_HEX("Status", (int)status));
          THROW_EXC(std::logic_error, "Bad FRC status: " << PAR((int)status));
        }
        // Add FRC result
        maintenanceResult.addTransactionResult(transResult);

        // Read FRC extra result (if needed)
        if (selectedNodes.size() > 12)
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
          TRC_DEBUG("Result from FRC CMD_FRC_EXTRARESULT transaction as string:" << PAR(transResult->getErrorString()));
          dpaResponse = transResult->getResponse();
          TRC_INFORMATION("FRC CMD_FRC_EXTRARESULT successful!");
          TRC_DEBUG(
            "DPA transaction: "
            << NAME_PAR(Peripheral type, extraResultRequest.PeripheralType())
            << NAME_PAR(Node address, extraResultRequest.NodeAddress())
            << NAME_PAR(Command, (int)extraResultRequest.PeripheralCommand())
          );
          // Append FRC data
          memoryData.append(dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData, 9);
          // Add FRC extra result
          maintenanceResult.addTransactionResult(transResult);
        }
        TRC_FUNCTION_LEAVE("");
        return memoryData;
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

    //---------------
    // Validate bonds
    //---------------
    void validateBonds(MaintenanceResult& maintenanceResult)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Get Nodes MID
        std::map<uint16_t, MaintenanceResult::TMID> nodesMID = maintenanceResult.getNodesMidMapCoord();
        std::map<uint16_t, MaintenanceResult::TMID>::iterator node = nodesMID.begin();
        do
        {
          DpaMessage validateBondRequest;
          DpaMessage::DpaPacket_t validateBondPacket;
          validateBondPacket.DpaRequestPacket_t.NADR = BROADCAST_ADDRESS;
          validateBondPacket.DpaRequestPacket_t.PNUM = PNUM_NODE;
          validateBondPacket.DpaRequestPacket_t.PCMD = CMD_NODE_VALIDATE_BONDS;
          validateBondPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
          // Put up to 11 pairs of network [N] address and [N] MID in the data part
          uint8_t index = 0x00;
          do
          {
            validateBondPacket.DpaRequestPacket_t.DpaMessage.PerNodeValidateBonds_Request.Bonds[index].Address = (uint8_t)node->first;
            validateBondPacket.DpaRequestPacket_t.DpaMessage.PerNodeValidateBonds_Request.Bonds[index].MID[0x00] = node->second.bytes[0x00];
            validateBondPacket.DpaRequestPacket_t.DpaMessage.PerNodeValidateBonds_Request.Bonds[index].MID[0x01] = node->second.bytes[0x01];
            validateBondPacket.DpaRequestPacket_t.DpaMessage.PerNodeValidateBonds_Request.Bonds[index].MID[0x02] = node->second.bytes[0x02];
            validateBondPacket.DpaRequestPacket_t.DpaMessage.PerNodeValidateBonds_Request.Bonds[index].MID[0x03] = node->second.bytes[0x03];
            node++;
          } while ((++index != 11) && (node != nodesMID.end()));
          // Data to buffer
          validateBondRequest.DataToBuffer(validateBondPacket.Buffer, sizeof(TDpaIFaceHeader) + index * sizeof(TPerNodeValidateBondsItem));
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
        } while (node != nodesMID.end());
        TRC_FUNCTION_LEAVE("");
      }
      catch (const std::exception& e)
      {
        maintenanceResult.setStatus(transResult->getErrorCode(), e.what());
        maintenanceResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //--------------------------------------
    // Read from coordinator extended eeprom
    //--------------------------------------
    std::basic_string<uint8_t> readCoordXMemory(MaintenanceResult& maintenanceResult, uint16_t address, uint8_t length)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage XMemoryReadRequest;
        DpaMessage::DpaPacket_t XMemoryReadPacket;
        XMemoryReadPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        XMemoryReadPacket.DpaRequestPacket_t.PNUM = PNUM_EEEPROM;
        XMemoryReadPacket.DpaRequestPacket_t.PCMD = CMD_EEEPROM_XREAD;
        XMemoryReadPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        // Set address and length
        XMemoryReadPacket.DpaRequestPacket_t.DpaMessage.XMemoryRequest.Address = address;
        XMemoryReadPacket.DpaRequestPacket_t.DpaMessage.XMemoryRequest.ReadWrite.Read.Length = length;
        // Data to buffer
        XMemoryReadRequest.DataToBuffer(XMemoryReadPacket.Buffer, sizeof(TDpaIFaceHeader) + sizeof(uint16_t) + sizeof(uint8_t));
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(XMemoryReadRequest, transResult, m_maintenanceParams.repeat);
        TRC_DEBUG("Result from XMemoryRequest transaction as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("Read XMemoryRequest successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, XMemoryReadRequest.PeripheralType())
          << NAME_PAR(Node address, XMemoryReadRequest.NodeAddress())
          << NAME_PAR(Command, (int)XMemoryReadRequest.PeripheralCommand())
        );
        maintenanceResult.addTransactionResult(transResult);
        // Get response data
        std::basic_string<uint8_t> XMemoryData;
        XMemoryData.append(dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData, length);
        TRC_FUNCTION_LEAVE("");
        return XMemoryData;
      }
      catch (const std::exception& e)
      {
        maintenanceResult.setStatus(transResult->getErrorCode(), e.what());
        maintenanceResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //--------
    // Set MID
    //--------
    void setMid(MaintenanceResult& maintenanceResult, const uint16_t deviceAddr, const MaintenanceResult::TMID mid)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage setMidRequest;
        DpaMessage::DpaPacket_t setMidPacket;
        setMidPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        setMidPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
        setMidPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_SET_MID;
        setMidPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        // Set address and length
        setMidPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSetMID_Request.BondAddr = (uint8_t)deviceAddr;
        setMidPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSetMID_Request.MID[0x00] = mid.bytes[0x00];
        setMidPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSetMID_Request.MID[0x01] = mid.bytes[0x01];
        setMidPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSetMID_Request.MID[0x02] = mid.bytes[0x02];
        setMidPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSetMID_Request.MID[0x03] = mid.bytes[0x03];
        // Data to buffer
        setMidRequest.DataToBuffer(setMidPacket.Buffer, sizeof(TDpaIFaceHeader) + sizeof(TPerCoordinatorSetMID_Request));
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(setMidRequest, transResult, m_maintenanceParams.repeat);
        TRC_DEBUG("Result from CMD_COORDINATOR_SET_MID transaction as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("Read CMD_COORDINATOR_SET_MID successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, setMidRequest.PeripheralType())
          << NAME_PAR(Node address, setMidRequest.NodeAddress())
          << NAME_PAR(Command, (int)setMidRequest.PeripheralCommand())
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
            Pointer("/data/rsp/inaccessibleNodesNr").Set(response, maintenanceResult.getInaccessibleNodesNr());
          }

          // Array of objects
          Document::AllocatorType &allocator = response.GetAllocator();
          rapidjson::Value frcTestRfResult(kArrayType);
          std::map<uint16_t, uint8_t> result = maintenanceResult.getTestRfResult();
          for (std::map<uint16_t, uint8_t>::iterator i = result.begin(); i != result.end(); ++i)
          {
            rapidjson::Value frcTestRfResultItem(kObjectType);
            frcTestRfResultItem.AddMember("deviceAddr", i->first, allocator);
            bool online = i->second != 0;
            frcTestRfResultItem.AddMember("online", online, allocator);
            if (online == true)
              frcTestRfResultItem.AddMember("counter", i->second - 1, allocator);
            frcTestRfResult.PushBack(frcTestRfResultItem, allocator);
          }
          Pointer("/data/rsp/testRfResult").Set(response, frcTestRfResult);
        }

        // iqmeshNetwork_MaintenanceInconsistentMIDsInCoord ?
        if (m_msgType->m_type == m_mTypeName_iqmeshNetwork_MaintenanceInconsistentMIDsInCoord)
        {
          // inaccessibleNodesNr
          Pointer("/data/rsp/inaccessibleNodesNr").Set(response, maintenanceResult.getInaccessibleNodesNr());

          // inconsistentNodesNr
          Pointer("/data/rsp/inconsistentNodesNr").Set(response, maintenanceResult.getInconsistentNodesNr());

          // inaccessibleNodes
          if (maintenanceResult.getInaccessibleNodesNr() > 0)
          {
            Document::AllocatorType &allocator = response.GetAllocator();
            rapidjson::Value inaccessibleNodesJsonArray(kArrayType);
            for (std::basic_string<uint8_t>::const_iterator it = maintenanceResult.getInaccessibleNodes().begin(); it != maintenanceResult.getInaccessibleNodes().end(); ++it)
              inaccessibleNodesJsonArray.PushBack(*it, allocator);
            Pointer("/data/rsp/inaccessibleNodes").Set(response, inaccessibleNodesJsonArray);
          }

          // inconsistentNodes
          if (maintenanceResult.getInconsistentNodesNr() > 0)
          {
            Document::AllocatorType &allocator = response.GetAllocator();
            rapidjson::Value inconsistentNodesJsonArray(kArrayType);
            for (std::basic_string<uint8_t>::const_iterator it = maintenanceResult.getInconsistentNodes().begin(); it != maintenanceResult.getInconsistentNodes().end(); ++it)
              inconsistentNodesJsonArray.PushBack(*it, allocator);
            Pointer("/data/rsp/inconsistentNodes").Set(response, inconsistentNodesJsonArray);
          }
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
            TimeConversion::encodeTimestamp(transResult->getRequestTs()),
            allocator
          );
          rawObject.AddMember(
            "confirmation",
            encodeBinary(transResult->getConfirmation().DpaPacket().Buffer, transResult->getConfirmation().GetLength()),
            allocator
          );
          rawObject.AddMember(
            "confirmationTs",
            TimeConversion::encodeTimestamp(transResult->getConfirmationTs()),
            allocator
          );
          rawObject.AddMember(
            "response",
            encodeBinary(transResult->getResponse().DpaPacket().Buffer, transResult->getResponse().GetLength()),
            allocator
          );
          rawObject.AddMember(
            "responseTs",
            TimeConversion::encodeTimestamp(transResult->getResponseTs()),
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

    //----------------------------------
    // Resolve inconsistence MIDs in [C]
    //----------------------------------
    void resolveInconsistentMIDsInCoord(MaintenanceResult& maintenanceResult)
    {
      TRC_FUNCTION_ENTER("");
      try
      {
        // Read Nodes MIDs from [C] eeeprom
        maintenanceResult.clearNodesMidMapCoord();
        std::basic_string<uint8_t> bondedNodes = maintenanceResult.getBondedNodes();
        const uint8_t maxDataLen = 54;
        const uint16_t startAddr = 0x4000;
        const uint16_t totalData = (*bondedNodes.rbegin() + 1) * 8;
        const uint8_t requests = totalData / maxDataLen;
        const uint8_t remainder = totalData % maxDataLen;
        std::vector<uint8_t> midData(0,0);
        for (uint8_t i = 0; i < requests + 1; ++i) {
          // Read MID from Coordinator eeprom
          uint8_t len = (uint8_t)(i < requests ? maxDataLen : remainder);
          if (len == 0) {
            break;
          }
          uint16_t address = startAddr + i * maxDataLen;
          std::basic_string<uint8_t> mids = readCoordXMemory(maintenanceResult, address, len);
          midData.insert(midData.end(), mids.begin(), mids.begin() + len);
        }
        for (uint8_t addr : bondedNodes) {
          uint16_t idx = addr * 8;
          MaintenanceResult::TMID mid;
          mid.bytes[0] = midData[idx];
          mid.bytes[1] = midData[idx + 1];
          mid.bytes[2] = midData[idx + 2];
          mid.bytes[3] = midData[idx + 3];
          maintenanceResult.setNodesMidMapCoord(addr, mid);
        }
        // Read Nodes MIDs from bonded Nodes
        uint8_t inaccessibleNodesNr = 0;
        maintenanceResult.clearNodesMidMap();
        std::basic_string<uint8_t> inaccessibleNodes;
        std::basic_string<uint8_t>::iterator node = bondedNodes.begin();
        do
        {
          std::basic_string<uint8_t> selectedNodes;
          selectedNodes.clear();
          uint8_t c = 0x00;
          do
          {
            selectedNodes.push_back(*node);
          } while ((++c != 15) && (++node != bondedNodes.end()));
          std::basic_string<uint8_t> memoryData = FRC_MemoryRead4BPlus1(maintenanceResult, selectedNodes, 0x04a0);
          uint8_t index = 0x00;
          for (uint8_t nodeAddr : selectedNodes)
          {
            MaintenanceResult::TMID mid;
            mid.bytes[0x00] = memoryData[index++];
            mid.bytes[0x01] = memoryData[index++];
            mid.bytes[0x02] = memoryData[index++];
            mid.bytes[0x03] = memoryData[index++];
            if (mid.value != 0)
              mid.value--;
            else
            {
              inaccessibleNodesNr++;
              inaccessibleNodes.push_back(nodeAddr);
            }
            maintenanceResult.setNodesMidMap(nodeAddr, mid);
          }
        } while (node != bondedNodes.end());
        maintenanceResult.setInaccessibleNodes(inaccessibleNodes);

        // Compare MID's from [C] eeeprom with nodes real MID's
        std::map<uint16_t, MaintenanceResult::TMID> nodesMidMapCoord = maintenanceResult.getNodesMidMapCoord();
        std::map<uint16_t, MaintenanceResult::TMID> nodesMidMapReal = maintenanceResult.getNodesMidMap();
        std::basic_string<uint8_t> inconsistentNodes;
        uint8_t inconsistentNodesNr = 0;
        for(auto nodeAddr : bondedNodes)
        {
          // Check the Node responded to FRC
          if (nodesMidMapReal[nodeAddr].value != 0)
          {
            // OK, compare the [C] MID with real Node MID
            if (nodesMidMapCoord[nodeAddr].value != nodesMidMapReal[nodeAddr].value)
            {
              // MID inconsistence found
              TRC_WARNING(
                "Inconsistent MID found at [C] side. Node " << (int)nodeAddr
                << NAME_PAR_HEX(": [C] MID = ", (int)nodesMidMapCoord[nodeAddr].value)
                << NAME_PAR_HEX(", [N] (real) MID = ", (int)nodesMidMapReal[nodeAddr].value)
              );
              // Set correct (real) MID at [C] side
              setMid(maintenanceResult, nodeAddr, nodesMidMapReal[nodeAddr]);
              inconsistentNodesNr++;
              inconsistentNodes.push_back(nodeAddr);
            }
          }
        }
        maintenanceResult.setInaccessibleNodesNr(inaccessibleNodesNr);
        maintenanceResult.setInconsistentNodesNr(inconsistentNodesNr);
        maintenanceResult.setInconsistentNodes(inconsistentNodes);

        TRC_FUNCTION_LEAVE("");
      }
      catch (std::exception& e)
      {
        CATCH_EXC_TRC_WAR(std::exception, e, e.what());
        TRC_FUNCTION_LEAVE("");
      }
    }

    //----------------------------
    // Resolve duplicated adresses
    //----------------------------
    void resolveDuplicatedAddresses(MaintenanceResult& maintenanceResult)
    {
      TRC_FUNCTION_ENTER("");
      try
      {
        // Read Nodes MIDs from [C] eeeprom
        maintenanceResult.clearNodesMidMapCoord();
        std::basic_string<uint8_t> bondedNodes = maintenanceResult.getBondedNodes();
        if (bondedNodes.length() > 0) {
          const uint8_t maxDataLen = 54;
          const uint16_t startAddr = 0x4000;
          const uint16_t totalData = (*bondedNodes.rbegin() + 1) * 8;
          const uint8_t requests = totalData / maxDataLen;
          const uint8_t remainder = totalData % maxDataLen;
          std::vector<uint8_t> midData(0,0);
          for (uint8_t i = 0; i < requests + 1; ++i) {
            // Read MID from Coordinator eeprom
            uint8_t len = (uint8_t)(i < requests ? maxDataLen : remainder);
            if (len == 0) {
              break;
            }
            uint16_t address = startAddr + i * maxDataLen;
            std::basic_string<uint8_t> mids = readCoordXMemory(maintenanceResult, address, len);
            midData.insert(midData.end(), mids.begin(), mids.begin() + len);
          }
          for (uint8_t addr : bondedNodes) {
            uint16_t idx = addr * 8;
            MaintenanceResult::TMID mid;
            mid.bytes[0] = midData[idx];
            mid.bytes[1] = midData[idx + 1];
            mid.bytes[2] = midData[idx + 2];
            mid.bytes[3] = midData[idx + 3];
            maintenanceResult.setNodesMidMapCoord(addr, mid);
          }
          // Validate bonds by broarcast request
          validateBonds(maintenanceResult);
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
        msgType.m_type != m_mTypeName_iqmeshNetwork_MaintenanceInconsistentMIDsInCoord &&
        msgType.m_type != m_mTypeName_iqmeshNetwork_MaintenanceDuplicatedAddresses &&
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
            if (maintenanceResult.getBondedNodes().size() == 0) {
              std::string errorStr = "There are no bonded nodes in network.";
              maintenanceResult.setStatus(noBondedNodesError, errorStr);
              THROW_EXC(std::logic_error, errorStr);
            }
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

        // m_mTypeName_iqmeshNetwork_MaintenanceInconsistentMIDsInCoord ?
        if (msgType.m_type == m_mTypeName_iqmeshNetwork_MaintenanceInconsistentMIDsInCoord)
        {
          if (maintenanceResult.getBondedNodes().size() == 0) {
            std::string errorStr = "There are no bonded nodes in network.";
            maintenanceResult.setStatus(noBondedNodesError, errorStr);
            THROW_EXC(std::logic_error, errorStr);
          }
          // Resolve Inconsistent MIDs in Coordinator
          resolveInconsistentMIDsInCoord(maintenanceResult);
        }

        // m_mTypeName_iqmeshNetwork_MaintenanceDuplicatedAddresses ?
        if (msgType.m_type == m_mTypeName_iqmeshNetwork_MaintenanceDuplicatedAddresses)
        {
          if (maintenanceResult.getBondedNodes().size() == 0) {
            std::string errorStr = "There are no bonded nodes in network.";
            maintenanceResult.setStatus(noBondedNodesError, errorStr);
            THROW_EXC(std::logic_error, errorStr);
          }
          // Resolve Duplicated addresses
          resolveDuplicatedAddresses(maintenanceResult);
        }

        // m_mTypeName_iqmeshNetwork_MaintenanceUselessPrebondedNodes ?
        if (msgType.m_type == m_mTypeName_iqmeshNetwork_MaintenanceUselessPrebondedNodes)
        {
          if (maintenanceResult.getBondedNodes().size() == 0) {
            std::string errorStr = "There are no bonded nodes in network.";
            maintenanceResult.setStatus(noBondedNodesError, errorStr);
            THROW_EXC(std::logic_error, errorStr);
          }
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
        m_mTypeName_iqmeshNetwork_MaintenanceInconsistentMIDsInCoord,
        m_mTypeName_iqmeshNetwork_MaintenanceDuplicatedAddresses,
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
        m_mTypeName_iqmeshNetwork_MaintenanceInconsistentMIDsInCoord,
        m_mTypeName_iqmeshNetwork_MaintenanceDuplicatedAddresses,
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