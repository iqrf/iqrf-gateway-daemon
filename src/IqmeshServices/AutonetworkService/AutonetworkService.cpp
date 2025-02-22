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

#define IAutonetworkService_EXPORTS

#include "AutonetworkService.h"
#include "Trace.h"
#include "ComAutonetwork.h"
#include "iqrf__AutonetworkService.hxx"
#include <list>
#include <cmath>
#include <thread>
#include <bitset>
#include <chrono>

TRC_INIT_MODULE(iqrf::AutonetworkService);

using namespace rapidjson;

namespace {
  // ToDo Timeout step
  const int TIMEOUT_STEP = 500;
  const int TIMEOUT_REPEAT = 2000;

  // Default bonding mask. No masking effect.
  static const uint8_t DEFAULT_BONDING_MASK = 0;

  static const int serviceError = 1000;
  static const int parsingRequestError = 1001;
  static const int exclusiveAccessError = 1002;
};

namespace iqrf {

  // Result of AutonetworkResult algorithm
  class AutonetworkResult {
  public:
    // Information related to node newly added into the network
    struct NewNode {
      uint8_t address;
      uint32_t MID;
    };

  private:
    // Status
    int m_status = 0;
    std::string m_statusStr = "ok";

    std::vector<NewNode> m_newNodes;

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

    void putNewNode(uint8_t address, uint32_t MID) {
      NewNode newNode = { address, MID };
      m_newNodes.push_back(newNode);
    }

    std::vector<NewNode> getNewNodes() {
      return m_newNodes;
    }

    void clearNewNodes() {
      m_newNodes.clear();
    }

    // Adds transaction result into the list of results
    void addTransactionResult(std::unique_ptr<IDpaTransactionResult2>& transResult) {
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
  class AutonetworkService::Imp
  {
  private:

    // Node authorization error definition
    enum class TAuthorizeErr
    {
      eNo,
      eMIDFiltering,
      eHWPIDFiltering,
      eFRC,
      eAddress,
      eNetworkNum,
      eNodeBonded
    };

    // waveState codes definition
    enum class TWaveStateCode
    {
      cannotStartProcessMaxAddress = -10,
      cannotStartProcessTotalNodesNr,
      cannotStartProcessNewNodesNr,
      cannotStartProcessTotalNodesNrMidList,
      cannotStartProcessNewNodesNrMidList,
      cannotStartProcessAllNodesMidListBonded,
      cannotStartProcessDuplicitMidInCoord,
      cannotStartProcessAddressSpaceNoFreeAddress,
      waveFinished = 0,
      discoveryBeforeStart,
      smartConnect,
      checkPrebondedAlive,
      readingDPAVersion,
      readPrebondedMID,
      readPrebondedHWPID,
      enumeration,
      authorize,
      ping,
      removeNotResponded,
      discovery,
      stopOnMaxNumWaves,
      stopOnNumberOfTotalNodes,
      stopOnMaxEmptyWaves,
      stopOnNumberOfNewNodes,
      abortOnTooManyNodesFound,
      abortOnAllAddressesAllocated,
      abortOnAllAddressesFromAddressSpaceAllocated,
      abortOnAllMIDsFromMidListAllocated
    };

    // MID union
    typedef union
    {
      uint8_t bytes[sizeof( uint32_t )];
      uint32_t value;
    }TMID;

    // Prebonded node
    typedef struct
    {
      uint8_t node;
      TMID mid;
      uint8_t addrBond;
      uint16_t HWPID;
      uint16_t HWPIDVer;
      bool supportMultipleAuth;
      bool authorize;
      TAuthorizeErr authorizeErr;
    }TPrebondedNode;

    // Network node
    typedef struct
    {
      uint8_t address;
      TMID mid;
      uint16_t HWPID;
      uint16_t HWPIDVer;
      bool bonded;
      bool discovered;
      bool online;
    }TNode;

    // Autonetwork process paramaters
    typedef struct
    {
      // Bonded nodes map
      std::bitset<MAX_ADDRESS + 1> bondedNodes;
      uint8_t initialBondedNodesNr, bondedNodesNr;
      // Discovered nodes map
      std::bitset<MAX_ADDRESS + 1> discoveredNodes;
      uint8_t discoveredNodesNr;
      // Nodes
      std::vector<AutonetworkResult::NewNode> respondedNewNodes;
      // Duplicit MIDs
      std::vector<uint8_t> duplicitMID;
      // Prebonded nodes map
      std::map<uint8_t, TPrebondedNode> prebondedNodes;
      // Network nodes map
      std::map<uint8_t, TNode> networkNodes;
      // FRC param value
      uint8_t FrcResponseTime;
      // DPA param value
      uint8_t DpaParam;
      // TX and RX hops
      uint8_t RequestHops, ResponseHops;
      int countWaves, countEmpty, countNewNodes, countWaveNewNodes, unbondedNodes;
      TWaveStateCode waveStateCode;
      int progress;
      int progressStep;
    }TAutonetworkProcessParams;

    // Parent object
    AutonetworkService & m_parent;

    // Service input parameters
    TAutonetworkInputParams antwInputParams;

    // Service process parameters
    TAutonetworkProcessParams antwProcessParams;

    // Message type
    const std::string m_mTypeName_Autonetwork = "iqmeshNetwork_AutoNetwork";
    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    IIqrfDpaService* m_iIqrfDpaService = nullptr;
    std::unique_ptr<IIqrfDpaService::ExclusiveAccess> m_exclusiveAccess;
    const MessagingInstance* m_messaging = nullptr;
    const IMessagingSplitterService::MsgType* m_msgType = nullptr;
    const ComAutonetwork* m_comAutonetwork = nullptr;

  public:
    Imp( AutonetworkService& parent ) : m_parent( parent )
    {
    }

    ~Imp()
    {
    }

    // Parses bit array of nodes into bitmap
    std::bitset<MAX_ADDRESS + 1> toNodesBitmap(const unsigned char* pData)
    {
      std::bitset<MAX_ADDRESS + 1> nodesMap;
      for (uint8_t nodeAddr = 0; nodeAddr <= MAX_ADDRESS; nodeAddr++)
        nodesMap[nodeAddr] = (pData[nodeAddr / 8] & (1 << (nodeAddr % 8))) != 0;

      return nodesMap;
    }

    // Check presence of Coordinator and OS peripherals on coordinator node
    void checkPresentCoordAndCoordOs(AutonetworkResult& autonetworkResult)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage perEnumRequest;
        DpaMessage::DpaPacket_t perEnumPacket;
        perEnumPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        perEnumPacket.DpaRequestPacket_t.PNUM = PNUM_ENUMERATION;
        perEnumPacket.DpaRequestPacket_t.PCMD = CMD_GET_PER_INFO;
        perEnumPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        perEnumRequest.DataToBuffer(perEnumPacket.Buffer, sizeof(TDpaIFaceHeader));
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(perEnumRequest, transResult, antwInputParams.repeat);
        TRC_DEBUG("Result from Device Exploration transaction as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("Device exploration successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, perEnumRequest.PeripheralType())
          << NAME_PAR(Node address, perEnumRequest.NodeAddress())
          << NAME_PAR(Command, (int)perEnumRequest.PeripheralCommand())
        );
        // Check Coordinator and OS peripherals
        if ((dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.EnumPeripheralsAnswer.EmbeddedPers[PNUM_COORDINATOR / 8] & (1 << PNUM_COORDINATOR)) != (1 << PNUM_COORDINATOR))
          THROW_EXC(std::logic_error, "Coordinator peripheral NOT found.");
        if ((dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.EnumPeripheralsAnswer.EmbeddedPers[PNUM_OS / 8] & (1 << PNUM_OS)) != (1 << PNUM_OS))
          THROW_EXC(std::logic_error, "OS peripheral NOT found.");
        autonetworkResult.addTransactionResult(transResult);
        TRC_FUNCTION_LEAVE("");
      }
      catch (const std::exception& e)
      {
        autonetworkResult.setStatus(transResult->getErrorCode(), e.what());
        autonetworkResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    // Set FRC response time
    uint8_t setFrcReponseTime(AutonetworkResult& autonetworkResult, uint8_t FRCresponseTime)
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
        setFrcParamPacket.DpaRequestPacket_t.DpaMessage.PerFrcSetParams_RequestResponse.FrcParams = FRCresponseTime;
        setFrcParamRequest.DataToBuffer(setFrcParamPacket.Buffer, sizeof(TDpaIFaceHeader) + sizeof(TPerFrcSetParams_RequestResponse));
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(setFrcParamRequest, transResult, antwInputParams.repeat);
        TRC_DEBUG("Result from Set Hops transaction as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("Set Hops successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, setFrcParamRequest.PeripheralType())
          << NAME_PAR(Node address, setFrcParamRequest.NodeAddress())
          << NAME_PAR(Command, (int)setFrcParamRequest.PeripheralCommand())
        );
        autonetworkResult.addTransactionResult(transResult);
        TRC_FUNCTION_LEAVE("");
        return dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSetParams_RequestResponse.FrcParams;
      }
      catch (const std::exception& e)
      {
        autonetworkResult.setStatus(transResult->getErrorCode(), e.what());
        autonetworkResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    // Sets no LED indication and optimal timeslot
    uint8_t setNoLedAndOptimalTimeslot(AutonetworkResult& autonetworkResult, uint8_t DpaParam)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage setDpaParamsRequest;
        DpaMessage::DpaPacket_t setDpaParamsPacket;
        setDpaParamsPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        setDpaParamsPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
        setDpaParamsPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_SET_DPAPARAMS;
        setDpaParamsPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        setDpaParamsPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSetDpaParams_Request_Response.DpaParam = DpaParam;
        setDpaParamsRequest.DataToBuffer(setDpaParamsPacket.Buffer, sizeof(TDpaIFaceHeader) + sizeof(TPerCoordinatorSetDpaParams_Request_Response));
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(setDpaParamsRequest, transResult, antwInputParams.repeat);
        TRC_DEBUG("Result from Set DPA params transaction as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("Set DPA params successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, setDpaParamsRequest.PeripheralType())
          << NAME_PAR(Node address, setDpaParamsRequest.NodeAddress())
          << NAME_PAR(Command, (int)setDpaParamsRequest.PeripheralCommand())
        );
        autonetworkResult.addTransactionResult(transResult);
        TRC_FUNCTION_LEAVE("");
        return dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerCoordinatorSetDpaParams_Request_Response.DpaParam;
      }
      catch (const std::exception& e)
      {
        autonetworkResult.setStatus(transResult->getErrorCode(), e.what());
        autonetworkResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    // Sets DPA hops to the number of routers
    TPerCoordinatorSetHops_Request_Response setDpaHopsToTheNumberOfRouters(AutonetworkResult& autonetworkResult, uint8_t RequestHops, uint8_t ResponseHops)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage setHopsRequest;
        DpaMessage::DpaPacket_t setHopsPacket;
        setHopsPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        setHopsPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
        setHopsPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_SET_HOPS;
        setHopsPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        setHopsPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSetHops_Request_Response.RequestHops = RequestHops;
        setHopsPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSetHops_Request_Response.ResponseHops = ResponseHops;
        setHopsRequest.DataToBuffer(setHopsPacket.Buffer, sizeof(TDpaIFaceHeader) + sizeof(TPerCoordinatorSetHops_Request_Response));
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(setHopsRequest, transResult, antwInputParams.repeat);
        TRC_DEBUG("Result from Set Hops transaction as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("Set Hops successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, setHopsRequest.PeripheralType())
          << NAME_PAR(Node address, setHopsRequest.NodeAddress())
          << NAME_PAR(Command, (int)setHopsRequest.PeripheralCommand())
        );
        autonetworkResult.addTransactionResult(transResult);
        TRC_FUNCTION_LEAVE("");
        return dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerCoordinatorSetHops_Request_Response;
      }
      catch (const std::exception& e)
      {
        autonetworkResult.setStatus(transResult->getErrorCode(), e.what());
        autonetworkResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    // Read from coordinator extended eeprom
    std::basic_string<uint8_t> readCoordXMemory(AutonetworkResult& autonetworkResult, uint16_t address, uint8_t length)
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
        m_exclusiveAccess->executeDpaTransactionRepeat(XMemoryReadRequest, transResult, antwInputParams.repeat);
        TRC_DEBUG("Result from XMemoryRequest transaction as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("Read XMemoryRequest successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, XMemoryReadRequest.PeripheralType())
          << NAME_PAR(Node address, XMemoryReadRequest.NodeAddress())
          << NAME_PAR(Command, (int)XMemoryReadRequest.PeripheralCommand())
        );
        autonetworkResult.addTransactionResult(transResult);
        // Get response data
        std::basic_string<uint8_t> XMemoryData;
        XMemoryData.append(dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData, length);
        TRC_FUNCTION_LEAVE("");
        return XMemoryData;
      }
      catch (const std::exception& e)
      {
        autonetworkResult.setStatus(transResult->getErrorCode(), e.what());
        autonetworkResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    // Returns addressing info
    TPerCoordinatorAddrInfo_Response getAddressingInfo(AutonetworkResult& autonetworkResult)
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
        m_exclusiveAccess->executeDpaTransactionRepeat(addrInfoRequest, transResult, antwInputParams.repeat);
        TRC_DEBUG("Result from Get addressing information transaction as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("Get addressing information successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, addrInfoRequest.PeripheralType())
          << NAME_PAR(Node address, addrInfoRequest.NodeAddress())
          << NAME_PAR(Command, (int)addrInfoRequest.PeripheralCommand())
        );
        autonetworkResult.addTransactionResult(transResult);
        TRC_FUNCTION_LEAVE("");
        return dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerCoordinatorAddrInfo_Response;
      }
      catch (const std::exception& e)
      {
        autonetworkResult.setStatus(transResult->getErrorCode(), e.what());
        autonetworkResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    // Returns map of bonded nodes
    std::bitset<MAX_ADDRESS + 1> getBondedNodes(AutonetworkResult& autonetworkResult)
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
        m_exclusiveAccess->executeDpaTransactionRepeat(getBondedNodesRequest, transResult, antwInputParams.repeat);
        TRC_DEBUG("Result from get bonded nodes transaction as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("Get bonded nodes successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, getBondedNodesRequest.PeripheralType())
          << NAME_PAR(Node address, getBondedNodesRequest.NodeAddress())
          << NAME_PAR(Command, (int)getBondedNodesRequest.PeripheralCommand())
        );
        // Get response data
        autonetworkResult.addTransactionResult(transResult);
        TRC_FUNCTION_LEAVE("");
        return toNodesBitmap(dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData);
      }
      catch (const std::exception& e)
      {
        autonetworkResult.setStatus(transResult->getErrorCode(), e.what());
        autonetworkResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    // Returns map of discovered nodes
    std::bitset<MAX_ADDRESS + 1> getDiscoveredNodes(AutonetworkResult& autonetworkResult)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage getDiscoveredNodesRequest;
        DpaMessage::DpaPacket_t getDiscoveredNodesPacket;
        getDiscoveredNodesPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        getDiscoveredNodesPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
        getDiscoveredNodesPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_DISCOVERED_DEVICES;
        getDiscoveredNodesPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        getDiscoveredNodesRequest.DataToBuffer(getDiscoveredNodesPacket.Buffer, sizeof(TDpaIFaceHeader));
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(getDiscoveredNodesRequest, transResult, antwInputParams.repeat);
        TRC_DEBUG("Result from Get discovered nodes transaction as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("Get discovered nodes successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, getDiscoveredNodesRequest.PeripheralType())
          << NAME_PAR(Node address, getDiscoveredNodesRequest.NodeAddress())
          << NAME_PAR(Command, (int)getDiscoveredNodesRequest.PeripheralCommand())
        );
        // Get response data
        autonetworkResult.addTransactionResult(transResult);
        TRC_FUNCTION_LEAVE("");
        return toNodesBitmap(dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData);
      }
      catch (const std::exception& e)
      {
        autonetworkResult.setStatus(transResult->getErrorCode(), e.what());
        autonetworkResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    // Update network info
    void updateNetworkInfo(AutonetworkResult& autonetworkResult)
    {
      // Get addressing info
      TPerCoordinatorAddrInfo_Response addressingInfo = getAddressingInfo(autonetworkResult);
      // Bonded nodes
      antwProcessParams.bondedNodesNr = addressingInfo.DevNr;
      antwProcessParams.bondedNodes = getBondedNodes(autonetworkResult);
      // Discovered nodes
      antwProcessParams.discoveredNodes = getDiscoveredNodes(autonetworkResult);
      antwProcessParams.discoveredNodesNr = (uint8_t)antwProcessParams.discoveredNodes.count();
      // Clear discoveredNodes bitmap and discoveredNodesNr if no node is bonded
      if (antwProcessParams.bondedNodesNr == 0)
      {
        antwProcessParams.discoveredNodes.reset();
        antwProcessParams.discoveredNodesNr = 0;
      }

      // Update networkNodes structure
      for (uint8_t addr = 1; addr <= MAX_ADDRESS; addr++)
      {
        // Node bonded ?
        antwProcessParams.networkNodes[addr].bonded = antwProcessParams.bondedNodes[addr];
        if (antwProcessParams.networkNodes[addr].bonded == true)
        {
          // Yes, node MID known ?
          if (antwProcessParams.networkNodes[addr].mid.value == 0)
          {
            // No, read MID from Coordinator eeprom
            uint16_t address = 0x4000 + addr * 0x08;
            std::basic_string<uint8_t> mid = readCoordXMemory(autonetworkResult, address, sizeof(TMID));
            antwProcessParams.networkNodes[addr].mid.bytes[0] = mid[0];
            antwProcessParams.networkNodes[addr].mid.bytes[1] = mid[1];
            antwProcessParams.networkNodes[addr].mid.bytes[2] = mid[2];
            antwProcessParams.networkNodes[addr].mid.bytes[3] = mid[3];
            antwProcessParams.networkNodes[addr].discovered = antwProcessParams.discoveredNodes[addr];
          }
        }
        else
        {
          // No, node isn't bondes, clear discovered flag and MID
          antwProcessParams.networkNodes[addr].discovered = false;
          antwProcessParams.networkNodes[addr].mid.value = 0;
        }
      }
    }

    // Returns comma-separated list of nodes, whose bits are set to 1 in the bitmap
    std::string toNodesListStr(const std::bitset<MAX_ADDRESS + 1>& nodes)
    {
      std::string nodesListStr;
      for (uint8_t nodeAddr = 1; nodeAddr <= MAX_ADDRESS; nodeAddr++)
      {
        if (nodes[nodeAddr] && !nodesListStr.empty()) {
          nodesListStr += ", ";
        }
        nodesListStr += std::to_string((int)nodeAddr);
      }

      return nodesListStr;
    }

    // Check unbonded nodes
    bool checkUnbondedNodes(const std::bitset<MAX_ADDRESS + 1>& bondedNodes, const std::bitset<MAX_ADDRESS + 1>& discoveredNodes)
    {
      std::stringstream unbondedNodesStream;

      for (uint8_t nodeAddr = 1; nodeAddr <= MAX_ADDRESS; nodeAddr++)
        if ((bondedNodes[nodeAddr] == false) && (discoveredNodes[nodeAddr] == true))
          unbondedNodesStream << nodeAddr << ", ";

      std::string unbondedNodesStr = unbondedNodesStream.str();
      if (unbondedNodesStr.empty())
        return true;

      // Log unbonded nodes
      TRC_INFORMATION("Nodes are discovered but NOT bonded. Discover the network!" << unbondedNodesStr);
      return false;
    }

    // SmartConnect
    TPerCoordinatorBondNodeSmartConnect_Response smartConnect(AutonetworkResult& autonetworkResult)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage smartConnectRequest;
        DpaMessage::DpaPacket_t smartConnectPacket;
        smartConnectPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        smartConnectPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
        smartConnectPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_SMART_CONNECT;
        smartConnectPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        // Address
        smartConnectPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSmartConnect_Request.ReqAddr = TEMPORARY_ADDRESS;
        // Bonding test retries
        smartConnectPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSmartConnect_Request.BondingTestRetries = 0x00;
        // IBK - zeroes
        std::fill_n(smartConnectPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSmartConnect_Request.IBK, 16, 0);
        // MID - zeroes
        std::fill_n(smartConnectPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSmartConnect_Request.MID, 4, 0);
        // Optimized bonding ?
        IIqrfDpaService::CoordinatorParameters coordParams = m_iIqrfDpaService->getCoordinatorParameters();
        if ((coordParams.dpaVerWord >= 0x0414) && (antwInputParams.bondingControl.overlappingNetworks.networks != 0) && (antwInputParams.bondingControl.overlappingNetworks.network != 0))
        {
          // Optimize bonging (applied for DPA >= 0x414)
          smartConnectPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSmartConnect_Request.MID[0] = antwInputParams.bondingControl.overlappingNetworks.network - 1;
          smartConnectPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSmartConnect_Request.MID[1] = antwInputParams.bondingControl.overlappingNetworks.networks;
          smartConnectPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSmartConnect_Request.MID[2] = 0xff;
          smartConnectPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSmartConnect_Request.MID[3] = 0xff;
        }
        // Set res0 to zero
        smartConnectPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSmartConnect_Request.reserved0 = 0x00;
        // Virtual Device Address - must equal 0xFF if not used.
        smartConnectPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSmartConnect_Request.VirtualDeviceAddress = 0xff;
        // Fill res1 with zeros
        std::fill_n(smartConnectPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSmartConnect_Request.reserved1, 10, 0);
        // User data - zeroes
        std::fill_n(smartConnectPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSmartConnect_Request.UserData, 4, 0);
        // Data to buffer
        smartConnectRequest.DataToBuffer(smartConnectPacket.Buffer, sizeof(TDpaIFaceHeader) + sizeof(TPerCoordinatorSmartConnect_Request));
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(smartConnectRequest, transResult, antwInputParams.repeat);
        TRC_DEBUG("Result from Smart Connect transaction as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("Smart Connect successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, smartConnectRequest.PeripheralType())
          << NAME_PAR(Node address, smartConnectRequest.NodeAddress())
          << NAME_PAR(Command, (int)smartConnectRequest.PeripheralCommand())
        );
        autonetworkResult.addTransactionResult(transResult);
        TRC_FUNCTION_LEAVE("");
        return dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerCoordinatorBondNodeSmartConnect_Response;
      }
      catch (const std::exception& e)
      {
        autonetworkResult.setStatus(transResult->getErrorCode(), e.what());
        autonetworkResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    // Returns prebonded alive nodes
    std::basic_string<uint8_t> FrcPrebondedAliveNodes(AutonetworkResult& autonetworkResult, const uint8_t nodeSeed)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage prebondedAliveRequest;
        DpaMessage::DpaPacket_t prebondedAlivePacket;
        prebondedAlivePacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        prebondedAlivePacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
        prebondedAlivePacket.DpaRequestPacket_t.PCMD = CMD_FRC_SEND;
        prebondedAlivePacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        // FRC Command
        prebondedAlivePacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.FrcCommand = FRC_PrebondedAlive;
        // Node seed
        prebondedAlivePacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[0x00] = nodeSeed;
        // 0x00
        prebondedAlivePacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[0x01] = 0;
        prebondedAliveRequest.DataToBuffer(prebondedAlivePacket.Buffer, sizeof(TDpaIFaceHeader) + 3);
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(prebondedAliveRequest, transResult, antwInputParams.repeat);
        TRC_DEBUG("Result from FRC Prebonded Alive transaction as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("FRC Prebonded Alive successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, prebondedAliveRequest.PeripheralType())
          << NAME_PAR(Node address, prebondedAliveRequest.NodeAddress())
          << NAME_PAR(Command, (int)prebondedAliveRequest.PeripheralCommand())
        );
        // Check FRC status
        uint8_t status = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.Status;
        if (status <= MAX_ADDRESS)
        {
          // Add FRC result
          autonetworkResult.addTransactionResult(transResult);
          TRC_INFORMATION("FRC Prebonded Alive status OK." << NAME_PAR_HEX("Status", (int)status));
          // Get list of nodes responded FRC_PrebondedAlive
          std::basic_string<uint8_t> prebondedNodes;
          prebondedNodes.clear();
          // Check FRC data - bit0
          for (uint8_t nodeAddr = 1; nodeAddr <= MAX_ADDRESS; nodeAddr++)
            if (dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.FrcData[nodeAddr / 8] & (1 << (nodeAddr % 8)))
              prebondedNodes.push_back(nodeAddr);
          TRC_FUNCTION_LEAVE("");
          return prebondedNodes;
        }
        else
        {
          TRC_WARNING("FRC Prebonded Alive NOK!" << NAME_PAR_HEX("Status", (int)status));
          THROW_EXC(std::logic_error, "Bad FRC status: " << PAR((int)status));
        }
      }
      catch (const std::exception& e)
      {
        autonetworkResult.setStatus(transResult->getErrorCode(), e.what());
        autonetworkResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    // Sets selected nodes to specified PData of FRC command
    void setFRCSelectedNodes(uint8_t* pData, const std::basic_string<uint8_t>& selectedNodes)
    {
      // Initialize to zero values
      memset(pData, 0, 30 * sizeof(uint8_t));
      for (uint8_t i : selectedNodes)
        pData[i / 0x08] |= (0x01 << (i % 8));
    }

    // FRC_PrebondedMemoryRead4BPlus1 (used to read MIDs and HWPID)
    std::basic_string<uint8_t> FrcPrebondedMemoryRead4BPlus1(AutonetworkResult& autonetworkResult, const std::basic_string<uint8_t>& prebondedNodes, const uint8_t nodeSeed, const uint8_t offset, const uint16_t address, const uint8_t PNUM, const uint8_t PCMD)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage prebondedMemoryRequest;
        DpaMessage::DpaPacket_t prebondedMemoryPacket;
        prebondedMemoryPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        prebondedMemoryPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
        prebondedMemoryPacket.DpaRequestPacket_t.PCMD = CMD_FRC_SEND_SELECTIVE;
        prebondedMemoryPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        // FRC Command
        prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.FrcCommand = FRC_PrebondedMemoryRead4BPlus1;
        // Selected nodes - prebonded alive nodes
        setFRCSelectedNodes(prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.SelectedNodes, prebondedNodes);
        // Node seed, offset
        prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x00] = nodeSeed;
        prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x01] = offset;
        // OS Read command
        prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x02] = address & 0xff;
        prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x03] = address >> 0x08;
        prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x04] = PNUM;
        prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x05] = PCMD;
        prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x06] = 0x00;
        prebondedMemoryRequest.DataToBuffer(prebondedMemoryPacket.Buffer, sizeof(TDpaIFaceHeader) + 38);
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(prebondedMemoryRequest, transResult, antwInputParams.repeat);
        TRC_DEBUG("Result from FRC Prebonded Memory Read transaction as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("FRC FRC Prebonded Memory Read successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, prebondedMemoryRequest.PeripheralType())
          << NAME_PAR(Node address, prebondedMemoryRequest.NodeAddress())
          << NAME_PAR(Command, (int)prebondedMemoryRequest.PeripheralCommand())
        );
        // Data from FRC
        std::basic_string<uint8_t> prebondedMemoryData;
        // Check status
        uint8_t status = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.Status;
        if (status <= MAX_ADDRESS)
        {
          // Add FRC result
          autonetworkResult.addTransactionResult(transResult);
          TRC_INFORMATION("FRC Prebonded Memory Read status ok." << NAME_PAR_HEX("Status", (int)status));
          prebondedMemoryData.append(dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.FrcData + sizeof(TMID), 51);
          TRC_DEBUG("Size of FRC data: " << PAR(prebondedMemoryData.size()));
        }
        else
        {
          TRC_WARNING("FRC Prebonded Memory Read NOT ok." << NAME_PAR_HEX("Status", (int)status));
          THROW_EXC(std::logic_error, "Bad FRC status: " << PAR((int)status));
        }

        // Read FRC extra result (if needed)
        if (prebondedNodes.size() > 12)
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
          m_exclusiveAccess->executeDpaTransactionRepeat(extraResultRequest, transResult, antwInputParams.repeat);
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
          prebondedMemoryData.append(dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData, 9);
          // Add FRC extra result
          autonetworkResult.addTransactionResult(transResult);
        }
        TRC_FUNCTION_LEAVE("");
        return prebondedMemoryData;
      }
      catch (const std::exception& e)
      {
        autonetworkResult.setStatus(transResult->getErrorCode(), e.what());
        autonetworkResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    // FRC_PrebondedMemoryCompare2B
    std::basic_string<uint8_t> FrcPrebondedMemoryCompare2B(AutonetworkResult& autonetworkResult, const uint8_t nodeSeed, const uint16_t valueToCompare, const uint16_t address, const uint8_t PNUM, const uint8_t PCMD)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage prebondedMemoryCompare2BRequest;
        DpaMessage::DpaPacket_t prebondedMemoryCompare2BPacket;
        prebondedMemoryCompare2BPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        prebondedMemoryCompare2BPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
        prebondedMemoryCompare2BPacket.DpaRequestPacket_t.PCMD = CMD_FRC_SEND;
        prebondedMemoryCompare2BPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        // FRC Command - https://doc.iqrf.org/DpaTechGuide/414/
        prebondedMemoryCompare2BPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.FrcCommand = FRC_PrebondedMemoryCompare2B;
        // Node seed
        prebondedMemoryCompare2BPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[0x00] = nodeSeed;
        // Zero
        prebondedMemoryCompare2BPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[0x01] = 0x00;
        // Flags
        prebondedMemoryCompare2BPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[0x02] = 0x01;
        // Value to compare
        prebondedMemoryCompare2BPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[0x03] = valueToCompare & 0xff;
        prebondedMemoryCompare2BPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[0x04] = valueToCompare >> 0x08;
        //
        prebondedMemoryCompare2BPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[0x05] = address & 0xff;
        prebondedMemoryCompare2BPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[0x06] = address >> 0x08;
        prebondedMemoryCompare2BPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[0x07] = PNUM;
        prebondedMemoryCompare2BPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[0x08] = PCMD;
        prebondedMemoryCompare2BPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[0x09] = 0x00;
        prebondedMemoryCompare2BRequest.DataToBuffer(prebondedMemoryCompare2BPacket.Buffer, sizeof(TDpaIFaceHeader) + 11);
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(prebondedMemoryCompare2BRequest, transResult, antwInputParams.repeat);
        TRC_DEBUG("Result from FRC Prebonded Memory Read transaction as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("FRC FRC Prebonded Memory Read successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, prebondedMemoryCompare2BRequest.PeripheralType())
          << NAME_PAR(Node address, prebondedMemoryCompare2BRequest.NodeAddress())
          << NAME_PAR(Command, (int)prebondedMemoryCompare2BRequest.PeripheralCommand())
        );
        // Data from FRC
        std::basic_string<uint8_t> prebondedMemoryData;
        // Check status
        uint8_t status = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.Status;
        if (status <= MAX_ADDRESS)
        {
          // Add FRC result
          autonetworkResult.addTransactionResult(transResult);
          TRC_INFORMATION("FRC Prebonded Memory Read status ok." << NAME_PAR_HEX("Status", (int)status));
          prebondedMemoryData.append(dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.FrcData, 55);
          TRC_DEBUG("Size of FRC data: " << PAR(prebondedMemoryData.size()));
        }
        else
        {
          TRC_WARNING("FRC Prebonded Memory Read NOT ok." << NAME_PAR_HEX("Status", (int)status));
          THROW_EXC(std::logic_error, "Bad FRC status: " << PAR((int)status));
        }

        // Read FRC extra results
        DpaMessage extraResultRequest;
        DpaMessage::DpaPacket_t extraResultPacket;
        extraResultPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        extraResultPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
        extraResultPacket.DpaRequestPacket_t.PCMD = CMD_FRC_EXTRARESULT;
        extraResultPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        extraResultRequest.DataToBuffer(extraResultPacket.Buffer, sizeof(TDpaIFaceHeader));
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(extraResultRequest, transResult, antwInputParams.repeat);
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
        prebondedMemoryData.append(dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData, 7);
        // Add FRC extra result
        autonetworkResult.addTransactionResult(transResult);
        TRC_FUNCTION_LEAVE("");
        return prebondedMemoryData;
      }
      catch (const std::exception& e)
      {
        autonetworkResult.setStatus(transResult->getErrorCode(), e.what());
        autonetworkResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    // Bond authorization
    TPerCoordinatorAuthorizeBond_Response authorizeBond(AutonetworkResult& autonetworkResult, std::basic_string<TPrebondedNode> &nodes)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage authorizeBondRequest;
        DpaMessage::DpaPacket_t authorizeBondPacket;
        authorizeBondPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        authorizeBondPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
        authorizeBondPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_AUTHORIZE_BOND;
        authorizeBondPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        // Add nodes
        uint8_t index = 0;
        for (TPrebondedNode node : nodes)
        {
          // Requested address
          authorizeBondPacket.DpaRequestPacket_t.DpaMessage.Request.PData[index++] = node.addrBond;
          // MID to authorize
          authorizeBondPacket.DpaRequestPacket_t.DpaMessage.Request.PData[index++] = node.mid.bytes[0x00];
          authorizeBondPacket.DpaRequestPacket_t.DpaMessage.Request.PData[index++] = node.mid.bytes[0x01];
          authorizeBondPacket.DpaRequestPacket_t.DpaMessage.Request.PData[index++] = node.mid.bytes[0x02];
          authorizeBondPacket.DpaRequestPacket_t.DpaMessage.Request.PData[index++] = node.mid.bytes[0x03];
        }
        // Data to buffer
        authorizeBondRequest.DataToBuffer(authorizeBondPacket.Buffer, sizeof(TDpaIFaceHeader) + index);
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(authorizeBondRequest, transResult, antwInputParams.repeat);
        TRC_DEBUG("Result from Authorize Bond transaction as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("Authorize Bond ok!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, authorizeBondRequest.PeripheralType())
          << NAME_PAR(Node address, authorizeBondRequest.NodeAddress())
          << NAME_PAR(Command, (int)authorizeBondRequest.PeripheralCommand())
        );
        // Add FRC extra result
        autonetworkResult.addTransactionResult(transResult);
        TRC_FUNCTION_LEAVE("");
        return dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerCoordinatorAuthorizeBond_Response;
      }
      catch (const std::exception& e)
      {
        autonetworkResult.setStatus(transResult->getErrorCode(), e.what());
        autonetworkResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    // Ping new nodes
    TPerFrcSend_Response FrcPingNodes(AutonetworkResult& autonetworkResult)
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
        checkNewNodesPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        // FRC command - Ping
        checkNewNodesPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.FrcCommand = FRC_Ping;
        // User data
        checkNewNodesPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[0x00] = 0x00;
        checkNewNodesPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[0x01] = 0x00;
        // Data to buffer
        checkNewNodesRequest.DataToBuffer(checkNewNodesPacket.Buffer, sizeof(TDpaIFaceHeader) + 3);
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(checkNewNodesRequest, transResult, antwInputParams.repeat);
        TRC_DEBUG("Result from Check new nodes transaction as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("Check new nodes ok!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, checkNewNodesRequest.PeripheralType())
          << NAME_PAR(Node address, checkNewNodesRequest.NodeAddress())
          << NAME_PAR(Command, (int)checkNewNodesRequest.PeripheralCommand())
        );
        // Check FRC status
        uint8_t status = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.Status;
        if (status <= MAX_ADDRESS)
        {
          // Add FRC result
          autonetworkResult.addTransactionResult(transResult);
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
        autonetworkResult.setStatus(transResult->getErrorCode(), e.what());
        autonetworkResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    // Removes new nodes, which not responded to control FRC (Ping)
    TPerFrcSend_Response removeNotRespondedNewNodes(AutonetworkResult& autonetworkResult, const std::basic_string<uint8_t>& notRespondedNewNodes)
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
        frcAckBroadcastPacket.DpaRequestPacket_t.PCMD = CMD_FRC_SEND_SELECTIVE;
        frcAckBroadcastPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        // FRC - Acknowledge Broadcast - Bits
        frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.FrcCommand = FRC_AcknowledgedBroadcastBits;
        // Put selected nodes
        setFRCSelectedNodes(frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.SelectedNodes, notRespondedNewNodes);
        // Clear UserData
        memset((void*)frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData, 0, sizeof(frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData));
        // Request length
        uint8_t requestLength = sizeof(TDpaIFaceHeader);
        requestLength += sizeof(frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.FrcCommand);
        requestLength += sizeof(frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.SelectedNodes);
        // Get DPA version
        IIqrfDpaService::CoordinatorParameters coordParams = m_iIqrfDpaService->getCoordinatorParameters();
        if (coordParams.dpaVerWord >= 0x0400)
        {
          // DPA >= 0x0400 - send Remove bond command to Node peripheral
          frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x00] = 0x05;
          frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x01] = PNUM_NODE;
          frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x02] = CMD_NODE_REMOVE_BOND;
          frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x03] = HWPID_DoNotCheck >> 0x08;
          frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x04] = HWPID_DoNotCheck & 0xff;
        }
        else
        {
          // DPA < 0x0400 - send Batch command (Remove bond + restart)
          frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x00] = 15;
          frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x01] = PNUM_OS;
          frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x02] = CMD_OS_BATCH;
          frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x03] = HWPID_DoNotCheck >> 0x08;;
          frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x04] = HWPID_DoNotCheck & 0xff;
          // Remove bond
          frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x05] = 5;
          frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x06] = PNUM_NODE;
          frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x07] = CMD_NODE_REMOVE_BOND;
          frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x08] = HWPID_DoNotCheck >> 0x08;
          frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x09] = HWPID_DoNotCheck & 0xff;
          // Restart
          frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x0a] = 5;
          frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x0b] = PNUM_OS;
          frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x0c] = CMD_OS_RESTART;
          frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x0d] = HWPID_DoNotCheck >> 0x08;
          frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x0e] = HWPID_DoNotCheck & 0xff;
          // End of batch
          frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x0f] = 0;
        }
        requestLength += frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x00];
        // Data to buffer
        frcAckBroadcastRequest.DataToBuffer(frcAckBroadcastPacket.Buffer, requestLength);
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(frcAckBroadcastRequest, transResult, antwInputParams.repeat);
        TRC_DEBUG("Result from Remove bond and restart (SELECTIVE BROADCAST BATCH) transaction as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("Selective BATCH Remove bond and restart ok!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, frcAckBroadcastRequest.PeripheralType())
          << NAME_PAR(Node address, frcAckBroadcastRequest.NodeAddress())
          << NAME_PAR(Command, (int)frcAckBroadcastRequest.PeripheralCommand())
        );
        // Check FRC status
        uint8_t status = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.Status;
        if (status <= MAX_ADDRESS)
        {
          // Add FRC result
          autonetworkResult.addTransactionResult(transResult);
          TRC_INFORMATION("FRC Prebonded Alive status OK." << NAME_PAR_HEX("Status", (int)status));
          TRC_FUNCTION_LEAVE("");
          return dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response;
        }
        else
        {
          TRC_WARNING("FRC Prebonded Alive status NOK!" << NAME_PAR_HEX("Status", (int)status));
          THROW_EXC(std::logic_error, "Bad FRC status: " << PAR((int)status));
        }
      }
      catch (const std::exception& e)
      {
        autonetworkResult.setStatus(transResult->getErrorCode(), e.what());
        autonetworkResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    // Removes specified node address at the coordinator side
    void removeBondAtCoordinator(AutonetworkResult& autonetworkResult, const uint8_t nodeAddress)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage removeBondAtCoordinatorRequest;
        DpaMessage::DpaPacket_t removeBondAtCoordinatorPacket;
        removeBondAtCoordinatorPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        removeBondAtCoordinatorPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
        removeBondAtCoordinatorPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_REMOVE_BOND;
        removeBondAtCoordinatorPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        // BondAddr
        removeBondAtCoordinatorPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorRemoveBond_Request.BondAddr = nodeAddress;
        // Data to buffer
        removeBondAtCoordinatorRequest.DataToBuffer(removeBondAtCoordinatorPacket.Buffer, sizeof(TDpaIFaceHeader) + sizeof(TPerCoordinatorRemoveBond_Request));
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(removeBondAtCoordinatorRequest, transResult, antwInputParams.repeat);
        TRC_DEBUG("Result from Remove bond at Coordinator transaction as string:" << PAR(transResult->getErrorString()));
        TRC_INFORMATION("Remove bond and restart ok!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, removeBondAtCoordinatorRequest.PeripheralType())
          << NAME_PAR(Node address, removeBondAtCoordinatorRequest.NodeAddress())
          << NAME_PAR(Command, (int)removeBondAtCoordinatorRequest.PeripheralCommand())
        );
        autonetworkResult.addTransactionResult(transResult);
        TRC_FUNCTION_LEAVE("");
      }
      catch (const std::exception& e)
      {
        autonetworkResult.setStatus(transResult->getErrorCode(), e.what());
        autonetworkResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    // Run discovery process
    uint8_t runDiscovery(AutonetworkResult& autonetworkResult, const uint8_t txPower)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        DpaMessage runDiscoveryRequest;
        DpaMessage::DpaPacket_t runDiscoveryPacket;
        runDiscoveryPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        runDiscoveryPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
        runDiscoveryPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_DISCOVERY;
        runDiscoveryPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        // TX power
        runDiscoveryPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorDiscovery_Request.TxPower = txPower;
        // Max address
        runDiscoveryPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorDiscovery_Request.MaxAddr = 0x00;
        // Data to buffer
        runDiscoveryRequest.DataToBuffer(runDiscoveryPacket.Buffer, sizeof(TDpaIFaceHeader) + sizeof(TPerCoordinatorDiscovery_Request));
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(runDiscoveryRequest, transResult, antwInputParams.repeat);
        TRC_INFORMATION("Run discovery ok!");
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, runDiscoveryRequest.PeripheralType())
          << NAME_PAR(Node address, runDiscoveryRequest.NodeAddress())
          << NAME_PAR(Command, (int)runDiscoveryRequest.PeripheralCommand())
        );
        TRC_DEBUG("Result from Run discovery transaction as string:" << PAR(transResult->getErrorString()));
        autonetworkResult.addTransactionResult(transResult);
        TRC_FUNCTION_LEAVE("");
        return  dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerCoordinatorDiscovery_Response.DiscNr;
      }
      catch (const std::exception& e)
      {
        autonetworkResult.setStatus(transResult->getErrorCode(), e.what());
        autonetworkResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    // Clear duplicit MID
    void clearDuplicitMID(AutonetworkResult& autonetworkResult)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Any duplicitMIDs ?
        if (antwProcessParams.duplicitMID.empty() == false)
        {
          DpaMessage validateBondRequest;
          DpaMessage::DpaPacket_t validateBondPacket;
          validateBondPacket.DpaRequestPacket_t.NADR = BROADCAST_ADDRESS;
          validateBondPacket.DpaRequestPacket_t.PNUM = PNUM_NODE;
          validateBondPacket.DpaRequestPacket_t.PCMD = CMD_NODE_VALIDATE_BONDS;
          validateBondPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
          uint8_t index = 0;
          for (uint8_t address = 1; address <= MAX_ADDRESS; address++)
          {
            auto node = std::find(antwProcessParams.duplicitMID.begin(), antwProcessParams.duplicitMID.end(), address);
            if (node != antwProcessParams.duplicitMID.end())
            {
              validateBondPacket.DpaRequestPacket_t.DpaMessage.PerNodeValidateBonds_Request.Bonds[index].Address = address;
              if (antwProcessParams.networkNodes[address].bonded == true)
              {
                validateBondPacket.DpaRequestPacket_t.DpaMessage.PerNodeValidateBonds_Request.Bonds[index].MID[0x00] = antwProcessParams.networkNodes[address].mid.bytes[0x00];
                validateBondPacket.DpaRequestPacket_t.DpaMessage.PerNodeValidateBonds_Request.Bonds[index].MID[0x01] = antwProcessParams.networkNodes[address].mid.bytes[0x01];
                validateBondPacket.DpaRequestPacket_t.DpaMessage.PerNodeValidateBonds_Request.Bonds[index].MID[0x02] = antwProcessParams.networkNodes[address].mid.bytes[0x02];
                validateBondPacket.DpaRequestPacket_t.DpaMessage.PerNodeValidateBonds_Request.Bonds[index].MID[0x03] = antwProcessParams.networkNodes[address].mid.bytes[0x03];
                antwProcessParams.duplicitMID.erase(node);
              }
              else
              {
                validateBondPacket.DpaRequestPacket_t.DpaMessage.PerNodeValidateBonds_Request.Bonds[index].MID[0x00] = 0;
                validateBondPacket.DpaRequestPacket_t.DpaMessage.PerNodeValidateBonds_Request.Bonds[index].MID[0x01] = 0;
                validateBondPacket.DpaRequestPacket_t.DpaMessage.PerNodeValidateBonds_Request.Bonds[index].MID[0x02] = 0;
                validateBondPacket.DpaRequestPacket_t.DpaMessage.PerNodeValidateBonds_Request.Bonds[index].MID[0x03] = 0;
              }
              index++;
            }

            if ((index == 11) || (address == MAX_ADDRESS))
            {
              if ((index != 11) && (address == MAX_ADDRESS))
              {
                validateBondPacket.DpaRequestPacket_t.DpaMessage.PerNodeValidateBonds_Request.Bonds[index].Address = TEMPORARY_ADDRESS;
                validateBondPacket.DpaRequestPacket_t.DpaMessage.PerNodeValidateBonds_Request.Bonds[index].MID[0x00] = 0;
                validateBondPacket.DpaRequestPacket_t.DpaMessage.PerNodeValidateBonds_Request.Bonds[index].MID[0x01] = 0;
                validateBondPacket.DpaRequestPacket_t.DpaMessage.PerNodeValidateBonds_Request.Bonds[index].MID[0x02] = 0;
                validateBondPacket.DpaRequestPacket_t.DpaMessage.PerNodeValidateBonds_Request.Bonds[index].MID[0x03] = 0;
                index++;
              }

              // Data to buffer
              validateBondRequest.DataToBuffer(validateBondPacket.Buffer, sizeof(TDpaIFaceHeader) + index * sizeof(TPerNodeValidateBondsItem));
              // Execute the DPA request
              m_exclusiveAccess->executeDpaTransactionRepeat(validateBondRequest, transResult, antwInputParams.repeat);
              TRC_INFORMATION("CMD_NODE_VALIDATE_BONDS ok!");
              DpaMessage dpaResponse = transResult->getResponse();
              TRC_DEBUG(
                "DPA transaction: "
                << NAME_PAR(Peripheral type, validateBondRequest.PeripheralType())
                << NAME_PAR(Node address, validateBondRequest.NodeAddress())
                << NAME_PAR(Command, (int)validateBondRequest.PeripheralCommand())
              );
              index = 0;
            }
          }
        }
        TRC_FUNCTION_LEAVE("");
      }
      catch (const std::exception& e)
      {
        autonetworkResult.setStatus(transResult->getErrorCode(), e.what());
        autonetworkResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    // Unbond nodes with temporary address
    void unbondTemporaryAddress(AutonetworkResult& autonetworkResult)
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
        m_exclusiveAccess->executeDpaTransactionRepeat(validateBondRequest, transResult, antwInputParams.repeat);
        TRC_INFORMATION("CMD_NODE_VALIDATE_BONDS ok!");
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, validateBondRequest.PeripheralType())
          << NAME_PAR(Node address, validateBondRequest.NodeAddress())
          << NAME_PAR(Command, (int)validateBondRequest.PeripheralCommand())
        );
        autonetworkResult.addTransactionResult(transResult);
        TRC_FUNCTION_LEAVE("");
      }
      catch (const std::exception& e)
      {
        autonetworkResult.setStatus(transResult->getErrorCode(), e.what());
        autonetworkResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    // FRC_AcknowledgedBroadcastBits - OS restart
    TPerFrcSend_Response FrcRestartNodes(AutonetworkResult& autonetworkResult)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage frcRestartNodesRequest;
        DpaMessage::DpaPacket_t frcRestartNodesPacket;
        frcRestartNodesPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        frcRestartNodesPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
        frcRestartNodesPacket.DpaRequestPacket_t.PCMD = CMD_FRC_SEND;
        frcRestartNodesPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        // FRC command - Ping
        frcRestartNodesPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.FrcCommand = FRC_AcknowledgedBroadcastBits;
        frcRestartNodesPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[0x00] = 5;
        frcRestartNodesPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[0x01] = PNUM_OS;
        frcRestartNodesPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[0x02] = CMD_OS_RESTART;
        frcRestartNodesPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[0x03] = HWPID_DoNotCheck >> 0x08;
        frcRestartNodesPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[0x04] = HWPID_DoNotCheck & 0xff;
        // Data to buffer
        frcRestartNodesRequest.DataToBuffer(frcRestartNodesPacket.Buffer, sizeof(TDpaIFaceHeader) + 6);
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(frcRestartNodesRequest, transResult, antwInputParams.repeat);
        TRC_DEBUG("Result from FRC_AcknowledgedBroadcastBits Restart transaction as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("FRC_AcknowledgedBroadcastBits Restart nodes ok!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, frcRestartNodesRequest.PeripheralType())
          << NAME_PAR(Node address, frcRestartNodesRequest.NodeAddress())
          << NAME_PAR(Command, (int)frcRestartNodesRequest.PeripheralCommand())
        );
        // Check FRC status
        uint8_t status = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.Status;
        if (status <= MAX_ADDRESS)
        {
          // Add FRC result
          autonetworkResult.addTransactionResult(transResult);
          TRC_INFORMATION("FRC_AcknowledgedBroadcastBits: status OK." << NAME_PAR_HEX("Status", (int)status));
          TRC_FUNCTION_LEAVE("");
          return dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response;
        }
        else
        {
          TRC_WARNING("FRC_AcknowledgedBroadcastBits: status NOK!" << NAME_PAR_HEX("Status", (int)status));
          THROW_EXC(std::logic_error, "Bad FRC status: " << PAR((int)status));
        }
      }
      catch (const std::exception& e)
      {
        autonetworkResult.setStatus(transResult->getErrorCode(), e.what());
        autonetworkResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    // Authorize control
    bool authorizeControl(uint32_t MID, uint16_t HWPID, uint8_t& bondAddr, TAuthorizeErr& authorizeErr)
    {
      bondAddr = 0;
      authorizeErr = TAuthorizeErr::eNo;

      // Check duplicit MID
      for (uint8_t address = 1; address <= MAX_ADDRESS; address++)
      {
        if (antwProcessParams.networkNodes[address].mid.value == MID)
        {
          TRC_WARNING("authorizeControl: duplicit MID found " << PAR((int)antwProcessParams.networkNodes[address].mid.value));
          authorizeErr = TAuthorizeErr::eNodeBonded;
          bondAddr = address;
          return false;
        }
      }

      // Overlapping networks
      bool overlappingNetworks = (antwInputParams.bondingControl.overlappingNetworks.networks != 0) && (antwInputParams.bondingControl.overlappingNetworks.network != 0);
      if ((overlappingNetworks == true) && ((MID % antwInputParams.bondingControl.overlappingNetworks.networks) != (uint32_t)(antwInputParams.bondingControl.overlappingNetworks.network - 1)))
      {
        TRC_WARNING("authorizeControl: MID:  " << PAR((int)MID) << ", Node not authorized! Network number error.");
        authorizeErr = TAuthorizeErr::eNetworkNum;
      }
      else
      {
        // MID List specified in JSON request ?
        if (antwInputParams.bondingControl.midListActive == true)
        {
          // Check the authorized MID is listed in the midList
          auto midListEntry = antwInputParams.bondingControl.midList.find(MID);
          // MID filtering active ?
          if ((antwInputParams.bondingControl.midFiltering == true) && (midListEntry == antwInputParams.bondingControl.midList.end()))
          {
            // No, authorization denied
            TRC_WARNING("authorizeControl: MID list doesn't contain MID:  " << PAR((int)MID) << ", Node not authorized!");
            authorizeErr = TAuthorizeErr::eMIDFiltering;
          }
          else
          {
            // HWPID filtering active ?
            if ((antwInputParams.hwpidFiltering.empty() == false) && (std::find(antwInputParams.hwpidFiltering.begin(), antwInputParams.hwpidFiltering.end(), HWPID) == antwInputParams.hwpidFiltering.end()))
            {
              // Authorization denied
              TRC_WARNING("authorizeControl: MID:  " << PAR((int)MID) << ", Node not authorized. HWPID not equal with HWPID filter!");
              authorizeErr = TAuthorizeErr::eHWPIDFiltering;
            }
            else
            {
              // Is authorized MID listed in the midList and device address is assigned ?
              if ((midListEntry != antwInputParams.bondingControl.midList.end()) && (midListEntry->second != 0))
              {
                // Address space specified in JSON request and device address assigned in MID list is specified also in Address space ?
                if ((antwInputParams.bondingControl.addressSpace.empty() == false) && (antwInputParams.bondingControl.addressSpaceBitmap[midListEntry->second] == false))
                {
                  // No, Device address assigned MID list is not specified in Address space
                  TRC_WARNING("authorizeControl: MID:  " << PAR((int)MID) << ", Node not authorized. Address assinged in MID list isn't specified in Address space!");
                  authorizeErr = TAuthorizeErr::eAddress;
                }
                else
                {
                  // OK
                  bondAddr = midListEntry->second;
                  return true;
                }
              }
              else
              {
                // Assign network address to authorized Node
                for (uint8_t addr = 1; addr <= MAX_ADDRESS; addr++)
                {
                  // Address already bonded ?
                  if (antwProcessParams.networkNodes[addr].bonded == false)
                  {
                    // No, address is free
                    bool usedAddress = false;

                    // Address assinged in MID list ?
                    for (auto node : antwInputParams.bondingControl.midList)
                    {
                      if (node.second == addr)
                      {
                        // Yes, set the flag
                        usedAddress = true;
                        break;
                      }
                    }

                    // Address assinged in MID list ?
                    if (usedAddress == false)
                    {
                      // No, Address space specified in JSON request and address specified in Address space list ?
                      if ((antwInputParams.bondingControl.addressSpace.empty() == false) && (antwInputParams.bondingControl.addressSpaceBitmap[addr] == false))
                        continue;

                      // Is Authorized MID listed in the midList ?
                      if (midListEntry != antwInputParams.bondingControl.midList.end())
                      {
                        // Yes, modify MID list entry - add Device address
                        antwInputParams.bondingControl.midList.at(midListEntry->first) = addr;
                      }
                      else
                      {
                        // No, add new entry to MID list
                        antwInputParams.bondingControl.midList.insert(std::make_pair(MID, addr));
                      }
                      bondAddr = addr;
                      return true;
                    }
                  }
                }
                if (bondAddr == 0)
                {
                  authorizeErr = TAuthorizeErr::eAddress;
                  TRC_WARNING("authorizeControl: MID:  " << PAR((int)MID) << ", Node not authorized! No free address.");
                }
              }
            }
          }
        }
        else
        {
          // HWPID filtering active ?
          if ((antwInputParams.hwpidFiltering.empty() == false) && (std::find(antwInputParams.hwpidFiltering.begin(), antwInputParams.hwpidFiltering.end(), HWPID) == antwInputParams.hwpidFiltering.end()))
          {
            // Authorization denied
            TRC_WARNING("authorizeControl: MID:  " << PAR((int)MID) << ", Node not authorized. HWPID not equal with HWPID filter!");
            authorizeErr = TAuthorizeErr::eHWPIDFiltering;
          }
          else
          {
            // Check the authorized MID is listed in the midList
            auto midListEntry = antwInputParams.bondingControl.midList.find(MID);
            // Is authorized MID listed in the midList and device address is assigned ?
            if ((midListEntry != antwInputParams.bondingControl.midList.end()) && (midListEntry->second != 0))
            {
              // Address space specified in JSON request and device address assigned in MID list is specified also in Address space ?
              if ((antwInputParams.bondingControl.addressSpace.empty() == false) && (antwInputParams.bondingControl.addressSpaceBitmap[midListEntry->second] == false))
              {
                // No, Device address assigned in MID list is not specified in Address space
                TRC_WARNING("authorizeControl: MID:  " << PAR((int)MID) << ", Node not authorized. Address assinged in MID list isn't specified in Address space!");
                authorizeErr = TAuthorizeErr::eAddress;
              }
              else
              {
                // OK
                bondAddr = midListEntry->second;
                return true;
              }
            }
            else
            {
              // Assign network address to authorized Node
              for (uint8_t addr = 1; addr <= MAX_ADDRESS; addr++)
              {
                // Address already bonded ?
                if (antwProcessParams.networkNodes[addr].bonded == false)
                {
                  // No, address is free
                  bool usedAddress = false;

                  // Address assinged in MID list ?
                  for (auto node : antwInputParams.bondingControl.midList)
                  {
                    if (node.second == addr)
                    {
                      // Yes, set the flag
                      usedAddress = true;
                      break;
                    }
                  }

                  // Address assinged in MID list ?
                  if (usedAddress == false)
                  {
                    // No, Address space specified in JSON request and address specified in Address space list ?
                    if ((antwInputParams.bondingControl.addressSpace.empty() == false) && (antwInputParams.bondingControl.addressSpaceBitmap[addr] == false))
                      continue;

                    // Is Authorized MID listed in the midList ?
                    if (midListEntry != antwInputParams.bondingControl.midList.end())
                    {
                      // Yes, modify MID list entry - add Device address
                      antwInputParams.bondingControl.midList.at(midListEntry->first) = addr;
                    }
                    else
                    {
                      // No, add new entry to MID list
                      antwInputParams.bondingControl.midList.insert(std::make_pair(MID, addr));
                    }
                    bondAddr = addr;
                    return true;
                  }
                }
              }
              if (bondAddr == 0)
              {
                authorizeErr = TAuthorizeErr::eAddress;
                TRC_WARNING("authorizeControl: MID:  " << PAR((int)MID) << ", Node not authorized! No free address.");
              }
            }
          }
        }
      }

      return false;
    }

    // Get string description of the wave state
    const std::string getWaveState(void)
    {
      std::string strWaveState = "";
      switch (antwProcessParams.waveStateCode)
      {
      case TWaveStateCode::discoveryBeforeStart:
        strWaveState = "Running discovery before start.";
        break;
      case TWaveStateCode::smartConnect:
        strWaveState = "Prebonding Nodes.";
        break;
      case TWaveStateCode::checkPrebondedAlive:
        strWaveState = "Looking for prebonded Nodes.";
        break;
      case TWaveStateCode::readingDPAVersion:
        strWaveState = "Reading DPA version of prebonded Nodes.";
        break;
      case TWaveStateCode::readPrebondedMID:
        strWaveState = "Reading MIDs of prebonded Nodes.";
        break;
      case TWaveStateCode::readPrebondedHWPID:
        strWaveState = "Reading HWPID of prebonded Nodes.";
        break;
      case TWaveStateCode::enumeration:
        strWaveState = "Enumerating authorized Nodes.";
        break;
      case TWaveStateCode::authorize:
        strWaveState = "Authorizing Nodes.";
        break;
      case TWaveStateCode::ping:
        strWaveState = "Running FRC to check new Nodes.";
        break;
      case TWaveStateCode::removeNotResponded:
        strWaveState = "Removing not responded Nodes.";
        break;
      case TWaveStateCode::discovery:
        strWaveState = "Running discovery.";
        break;
      case TWaveStateCode::stopOnMaxNumWaves:
        strWaveState = "Maximum number of waves reached.";
        break;
      case TWaveStateCode::stopOnNumberOfTotalNodes:
        strWaveState = "Number of total nodes bonded into network.";
        break;
      case TWaveStateCode::stopOnMaxEmptyWaves:
        strWaveState = "Maximum number of consecutive empty waves reached.";
        break;
      case TWaveStateCode::stopOnNumberOfNewNodes:
        strWaveState = "Number of new nodes bonded into network.";
        break;
      case TWaveStateCode::abortOnTooManyNodesFound:
        strWaveState = "Too many nodes found.";
        break;
      case TWaveStateCode::abortOnAllAddressesAllocated:
        strWaveState = "All available network addresses are already allocated.";
        break;
      case TWaveStateCode::waveFinished:
        strWaveState = "Wave finished.";
        break;
      case TWaveStateCode::cannotStartProcessMaxAddress:
        strWaveState = "The AutoNetwork process cannot be started because all available network addresses are already allocated.";
        break;
      case TWaveStateCode::cannotStartProcessTotalNodesNr:
        strWaveState = "The AutoNetwork process cannot be started because the number of total nodes is equal or lower than the size of the existing network.";
        break;
      case TWaveStateCode::cannotStartProcessNewNodesNr:
        strWaveState = "The AutoNetwork process cannot be started because the number of existing nodes plus number of new nodes exceeds the maximum network size.";
        break;
      case TWaveStateCode::cannotStartProcessTotalNodesNrMidList:
        strWaveState = "The AutoNetwork process cannot be started because the number of total Nodes stop condition is higher than number of already bonded nodes and not bonded Nodes in the MID list. Change stop conditions or add nodes to the MID list.";
        break;
      case TWaveStateCode::cannotStartProcessNewNodesNrMidList:
        strWaveState = "The AutoNetwork process cannot be started because the number of new Nodes stop condition is higher than number of not bonded nodes in the MID list. Change stop conditions or add not bonded Nodes to the MID list. Alternatively, disable the MID filtering option.";
        break;
      case TWaveStateCode::cannotStartProcessAllNodesMidListBonded:
        strWaveState = "The AutoNetwork process cannot be started because all nodes in the MID list are already bonded. Add not bonded nodes to the MID list or disable the MID filtering option.";
        break;
      case TWaveStateCode::cannotStartProcessDuplicitMidInCoord:
        strWaveState = "The AutoNetwork process cannot be started because the Coordinator's IQMESH database contains the same Node(s) bonded to more addresses. Please inspect the duplicate MID values in the MID column in the Table View and unbond the duplicate Node(s) in the Coordinator only.";
        break;
      case TWaveStateCode::cannotStartProcessAddressSpaceNoFreeAddress:
        strWaveState = "The AutoNetwork process cannot start because there is no free network address limited by address space. Change the value in the address space.";
        break;
      case TWaveStateCode::abortOnAllAddressesFromAddressSpaceAllocated:
        strWaveState = "All available network addresses limited by the address space were assigned. No new Node can be bonded.";
        break;
      case TWaveStateCode::abortOnAllMIDsFromMidListAllocated:
        strWaveState = "All Nodes with MIDs from the MID list were found. No new Node can be bonded.";
        break;

      default:
        THROW_EXC(std::logic_error, "Unknown waveStateCode.");
      }

      return strWaveState;
    }

    // Send autonetwok algorithm state
    void sendWaveState(void)
    {
      Document waveState;
      // Set common parameters
      Pointer("/mType").Set(waveState, m_msgType->m_type);
      Pointer("/data/msgId").Set(waveState, m_comAutonetwork->getMsgId());
      // Fill response
      rapidjson::Pointer("/data/rsp/wave").Set(waveState, antwProcessParams.countWaves);
      rapidjson::Pointer("/data/rsp/waveStateCode").Set(waveState, (int)antwProcessParams.waveStateCode);
      rapidjson::Pointer("/data/rsp/progress").Set(waveState, (int)antwProcessParams.progress);
      if (m_comAutonetwork->getVerbose() == true)
        rapidjson::Pointer("/data/rsp/waveState").Set(waveState, getWaveState());
      // Set status
      Pointer("/data/status").Set(waveState, 0);
      Pointer("/data/statusStr").Set(waveState, "ok");
      // Send message
      m_iMessagingSplitterService->sendMessage(*m_messaging, std::move(waveState));

      // Update progress
      if (antwProcessParams.progress < 100)
        antwProcessParams.progress += (100 / antwProcessParams.progressStep);
    }

    // Check the wave is last one
    bool checkLastWave(void)
    {
      // Set wave finished state
      antwProcessParams.waveStateCode = TWaveStateCode::waveFinished;

      // Maximum waves reached ?
      if ((antwInputParams.stopConditions.totalWaves != 0) && (antwProcessParams.countWaves == antwInputParams.stopConditions.totalWaves))
      {
        TRC_INFORMATION("Maximum number of waves reached.");
        antwProcessParams.waveStateCode = TWaveStateCode::stopOnMaxNumWaves;
      }

      // Maximum empty waves reached ?
      if ((antwInputParams.stopConditions.emptyWaves != 0) && (antwProcessParams.countEmpty >= antwInputParams.stopConditions.emptyWaves))
      {
        TRC_INFORMATION("Maximum number of consecutive empty waves reached.");
        antwProcessParams.waveStateCode = TWaveStateCode::stopOnMaxEmptyWaves;
      }

      // Number of new nodes bonded into network ?
      if ((antwInputParams.stopConditions.numberOfNewNodes != 0) && (antwProcessParams.countNewNodes >= antwInputParams.stopConditions.numberOfNewNodes))
      {
        TRC_INFORMATION("Number of new nodes bonded into network.");
        antwProcessParams.waveStateCode = TWaveStateCode::stopOnNumberOfNewNodes;
      }

      // Number of total nodes bonded into network ?
      if ((antwInputParams.stopConditions.numberOfTotalNodes != 0) && (antwProcessParams.bondedNodesNr >= antwInputParams.stopConditions.numberOfTotalNodes))
      {
        TRC_INFORMATION("Number of total nodes bonded into network.");
        antwProcessParams.waveStateCode = TWaveStateCode::stopOnNumberOfTotalNodes;
      }

      // Check all nodes bonded into network
      if (antwProcessParams.bondedNodesNr == MAX_ADDRESS)
      {
        TRC_INFORMATION("All available network addresses are already allocated - Autonetwork process aborted.");
        antwProcessParams.waveStateCode = TWaveStateCode::abortOnAllAddressesAllocated;
      }

      // Check all nodes from addressSpace are already bonded
      if (antwInputParams.bondingControl.addressSpace.empty() == false)
      {
        int addr = 1;
        for (; addr <= MAX_ADDRESS; addr++)
        {
          if (antwInputParams.bondingControl.addressSpaceBitmap[addr] == true) {
            if (antwProcessParams.networkNodes[addr].bonded == true) {
              antwInputParams.bondingControl.addressSpaceBitmap[addr] = false;
            } else {
              break;
            }
          }
        }
        if (addr == (MAX_ADDRESS + 1))
        {
          TRC_INFORMATION("All available network addresses limited by the Address space were assigned. No new Node can be bonded.The AutoNetwork process will stop.");
          antwProcessParams.waveStateCode = TWaveStateCode::abortOnAllAddressesFromAddressSpaceAllocated;
        }
      }

      // Check all MIDs from midList are already bonded
      if (antwInputParams.bondingControl.midFiltering == true)
      {
        int midCount = antwInputParams.bondingControl.midList.size();
        for (auto midListEntry : antwInputParams.bondingControl.midList)
        {
          for (auto networkNode : antwProcessParams.networkNodes)
          {
            if (networkNode.second.mid.value == midListEntry.first)
              midCount--;
          }
        }
        if (midCount == 0)
        {
          TRC_INFORMATION("All Nodes with MIDs from the MID list were found. No new Node can be bonded.");
          antwProcessParams.waveStateCode = TWaveStateCode::abortOnAllMIDsFromMidListAllocated;
        }
      }

      return antwProcessParams.waveStateCode != TWaveStateCode::waveFinished;
    }

    // Send wave result
    void sendWaveResult(AutonetworkResult& autonetworkResult)
    {
      Document waveResult;
      // Set common parameters
      Pointer("/mType").Set(waveResult, m_msgType->m_type);
      Pointer("/data/msgId").Set(waveResult, m_comAutonetwork->getMsgId());

      // Add wave result
      rapidjson::Pointer("/data/rsp/wave").Set(waveResult, antwProcessParams.countWaves);
      rapidjson::Pointer("/data/rsp/nodesNr").Set(waveResult, antwProcessParams.bondedNodesNr);
      rapidjson::Pointer("/data/rsp/newNodesNr").Set(waveResult, antwProcessParams.countWaveNewNodes);
      rapidjson::Pointer("/data/rsp/waveStateCode").Set(waveResult, (int)antwProcessParams.waveStateCode);
      antwProcessParams.progress = 100;
      rapidjson::Pointer("/data/rsp/progress").Set(waveResult, (int)antwProcessParams.progress);
      if (m_comAutonetwork->getVerbose() == true)
        rapidjson::Pointer("/data/rsp/waveState").Set(waveResult, getWaveState());
      rapidjson::Pointer("/data/rsp/lastWave").Set(waveResult, antwProcessParams.waveStateCode != TWaveStateCode::waveFinished);
      if (antwProcessParams.respondedNewNodes.empty() == false)
      {
        // Rsp object
        rapidjson::Pointer("/data/rsp/wave").Set(waveResult, antwProcessParams.countWaves);
        rapidjson::Pointer("/data/rsp/nodesNr").Set(waveResult, antwProcessParams.bondedNodesNr);
        rapidjson::Pointer("/data/rsp/newNodesNr").Set(waveResult, antwProcessParams.countWaveNewNodes);
        rapidjson::Value newNodesJsonArray(kArrayType);
        Document::AllocatorType& allocator = waveResult.GetAllocator();
        for (AutonetworkResult::NewNode newNode : antwProcessParams.respondedNewNodes)
        {
          rapidjson::Value newNodeObject(kObjectType);
          std::stringstream stream;
          stream << std::hex << newNode.MID;
          newNodeObject.AddMember("mid", stream.str(), allocator);
          newNodeObject.AddMember("address", newNode.address, allocator);
          newNodesJsonArray.PushBack(newNodeObject, allocator);
        }
        Pointer("/data/rsp/newNodes").Set(waveResult, newNodesJsonArray);
      }

      // Set raw fields, if verbose mode is active
      if (m_comAutonetwork->getVerbose() == true)
      {
        rapidjson::Value rawArray(kArrayType);
        Document::AllocatorType& allocator = waveResult.GetAllocator();

        while (autonetworkResult.isNextTransactionResult()) {
          std::unique_ptr<IDpaTransactionResult2> transResult = autonetworkResult.consumeNextTransactionResult();
          rapidjson::Value rawObject(kObjectType);

          rawObject.AddMember(
            "request",
            HexStringConversion::encodeBinary(transResult->getRequest().DpaPacket().Buffer, transResult->getRequest().GetLength()),
            allocator
          );

          rawObject.AddMember(
            "requestTs",
            TimeConversion::encodeTimestamp(transResult->getRequestTs()),
            allocator
          );

          rawObject.AddMember(
            "confirmation",
            HexStringConversion::encodeBinary(transResult->getConfirmation().DpaPacket().Buffer, transResult->getConfirmation().GetLength()),
            allocator
          );

          rawObject.AddMember(
            "confirmationTs",
            TimeConversion::encodeTimestamp(transResult->getConfirmationTs()),
            allocator
          );

          rawObject.AddMember(
            "response",
            HexStringConversion::encodeBinary(transResult->getResponse().DpaPacket().Buffer, transResult->getResponse().GetLength()),
            allocator
          );

          rawObject.AddMember(
            "responseTs",
            TimeConversion::encodeTimestamp(transResult->getResponseTs()),
            allocator
          );

          // add object into array
          rawArray.PushBack(rawObject, allocator);
        }

        // Add array into response document
        Pointer("/data/raw").Set(waveResult, rawArray);
      }

      // Set status
      Pointer("/data/status").Set(waveResult, autonetworkResult.getStatus());
      Pointer("/data/statusStr").Set(waveResult, autonetworkResult.getStatusStr());

      // Send message
      m_iMessagingSplitterService->sendMessage(*m_messaging, std::move(waveResult));
    }

    // Process the autonetwork algorithm
    void runAutonetwork(void)
    {
      TRC_FUNCTION_ENTER("");

      // Autonetwork result
      AutonetworkResult autonetworkResult;
      std::bitset<MAX_ADDRESS + 1> warningAddressSpaceBitmap;

      try
      {
        // Check, if Coordinator and OS peripherals are present at coordinator's
        checkPresentCoordAndCoordOs(autonetworkResult);
        TRC_INFORMATION("Initial network check.");

        // Initialize networkNodes structure
        antwProcessParams.networkNodes.clear();
        TNode node;
        node.mid.value = 0;
        node.discovered = false;
        node.bonded = false;
        node.online = false;
        for (uint8_t addr = 0; addr <= MAX_ADDRESS; addr++)
        {
          node.address = addr;
          antwProcessParams.networkNodes[addr] = node;
        }

        // Update network info
        updateNetworkInfo(autonetworkResult);
        TRC_INFORMATION(NAME_PAR(Bonded nodes, toNodesListStr(antwProcessParams.bondedNodes)));
        TRC_INFORMATION(NAME_PAR(Discovered nodes, toNodesListStr(antwProcessParams.discoveredNodes)));

        // Initialize process params
        antwProcessParams.countNewNodes = 0;
        antwProcessParams.countWaves = 1;
        antwProcessParams.countEmpty = 0;
        antwProcessParams.countWaveNewNodes = 0;
        antwProcessParams.respondedNewNodes.clear();

        // Check max address
        if (antwProcessParams.bondedNodesNr == MAX_ADDRESS)
        {
          TRC_INFORMATION("The AutoNetwork process cannot be started because all available network addresses are already allocated.");
          antwProcessParams.waveStateCode = TWaveStateCode::cannotStartProcessMaxAddress;
          sendWaveResult(autonetworkResult);
          TRC_FUNCTION_LEAVE("");
          return;
        }

        // Check duplicit MID in Coordinator
        if (antwProcessParams.bondedNodesNr != 0)
        {
          for (auto node : antwProcessParams.networkNodes)
          {
            if (node.second.bonded == true)
            {
              for (auto node1 : antwProcessParams.networkNodes)
              {
                if (node1.second.bonded)
                {
                  if ((node.second.address != node1.second.address) && (node.second.mid.value == node1.second.mid.value))
                  {
                    TRC_INFORMATION("The AutoNetwork process cannot be started because the Coordinator's IQMESH database contains the same Node(s) bonded to more addresses. Please inspect the duplicate MID values in the MID column in the Table View and unbond the duplicate Node(s) in the Coordinator only.");
                    antwProcessParams.waveStateCode = TWaveStateCode::cannotStartProcessDuplicitMidInCoord;
                    sendWaveResult(autonetworkResult);
                    TRC_FUNCTION_LEAVE("");
                    return;
                  }
                }
              }
            }
          }
        }

        // Check addressSpace
        int addressSpaceCount = (int)antwInputParams.bondingControl.addressSpaceBitmap.count();
        warningAddressSpaceBitmap.reset();
        if (addressSpaceCount != 0)
        {
          // Check already bonded nodes bitmap vs addressSpace bitmap
          for (int i = 1; i <= MAX_ADDRESS; i++)
          {
            if ((antwProcessParams.bondedNodes[i] == true) && (antwInputParams.bondingControl.addressSpaceBitmap[i] == true))
              addressSpaceCount--;
          }

          // All addresses dedicated in addressSpace already bonded ?
          if (addressSpaceCount == 0)
          {
            TRC_INFORMATION("The AutoNetwork process cannot start because there is no free network address limited by address space. Change the value in the address space.");
            antwProcessParams.waveStateCode = TWaveStateCode::cannotStartProcessAddressSpaceNoFreeAddress;
            sendWaveResult(autonetworkResult);
            TRC_FUNCTION_LEAVE("");
            return;
          }

          // Check midList address vs addressSpace
          if (antwInputParams.bondingControl.midList.empty() == false)
          {
            // Check all not bonded addresses listed in addressSpace are listed in midList too
            for (auto midListItem : antwInputParams.bondingControl.midList)
            {
              // Node already bonded ?
              if (antwProcessParams.bondedNodes[midListItem.second] == false)
              {
                // No, check both addressSpace and midList contain the address
                if ((midListItem.second != 0x00) && (antwInputParams.bondingControl.addressSpaceBitmap[midListItem.second] == false))
                {
                  warningAddressSpaceBitmap[midListItem.second] = true;
                }
              }
            }
          }
        }

        // Check maximum number of total/new nodes in case that MID list and filtering is active
        if ((antwInputParams.bondingControl.midList.empty() == false) && (antwInputParams.bondingControl.midFiltering == true))
        {
          int countNewNodes = 0;

          // Mid list specified in request JSON and no Node bonded ?
          if ((antwInputParams.bondingControl.midList.empty() == false) && (antwProcessParams.bondedNodesNr != 0))
          {
            // Compare MID's of all bondes Nodes with MID's specified in MID list
            for (auto midListNode : antwInputParams.bondingControl.midList)
            {
              countNewNodes++;
              for (auto bondedNode : antwProcessParams.networkNodes)
              {
                if (midListNode.first == bondedNode.second.mid.value)
                {
                  countNewNodes--;
                  break;
                }
              }
            }
          }

          if ((countNewNodes != 0) && ((antwInputParams.stopConditions.numberOfTotalNodes != 0) || (antwInputParams.stopConditions.numberOfNewNodes != 0)))
          {
            if ((antwInputParams.stopConditions.totalWaves != 0) && (antwInputParams.stopConditions.emptyWaves != 0))
            {
              if ((antwInputParams.stopConditions.numberOfTotalNodes != 0) && (antwInputParams.stopConditions.numberOfTotalNodes > antwProcessParams.bondedNodesNr + countNewNodes))
              {
                TRC_INFORMATION("The AutoNetwork process cannot be started because the Number of total Nodes stop condition is higher than number of already bonded Nodes and not bonded Nodes in the MID list. Change stop conditions or add Nodes to the MID list.");
                antwProcessParams.waveStateCode = TWaveStateCode::cannotStartProcessTotalNodesNrMidList;
                sendWaveResult(autonetworkResult);
                TRC_FUNCTION_LEAVE("");
                return;
              }

              if ((antwInputParams.stopConditions.numberOfNewNodes != 0) && (antwInputParams.stopConditions.numberOfNewNodes > countNewNodes))
              {
                TRC_INFORMATION("The AutoNetwork process cannot be started because the Number of new Nodes stop condition is higher than number of not bonded Nodes in the MID list. Change stop conditions or add not bonded Nodes to the MID list.Alternatively, disable the MID filtering option.");
                antwProcessParams.waveStateCode = TWaveStateCode::cannotStartProcessNewNodesNrMidList;
                sendWaveResult(autonetworkResult);
                TRC_FUNCTION_LEAVE("");
                return;
              }
            }
          }

          if ((countNewNodes == 0) && (antwProcessParams.bondedNodesNr != 0))
          {
            TRC_INFORMATION("The AutoNetwork process cannot be started because all Nodes in the MID list are already bonded. Add not bonded Nodes to the MID list or disable the MID filtering option.");
            antwProcessParams.waveStateCode = TWaveStateCode::cannotStartProcessAllNodesMidListBonded;
            sendWaveResult(autonetworkResult);
            TRC_FUNCTION_LEAVE("");
            return;
          }
        }

        // Check stop conditions - numberOfTotalNodes and numberOfNewNodes (if totalWaves and emptyWaves not set)
        if ((antwInputParams.stopConditions.totalWaves == 0) && (antwInputParams.stopConditions.emptyWaves == 0))
        {
          // Check stop condition - number of total nodes
          if ((antwInputParams.stopConditions.numberOfTotalNodes != 0) && (antwProcessParams.bondedNodesNr >= antwInputParams.stopConditions.numberOfTotalNodes))
          {
            TRC_INFORMATION("The AutoNetwork process cannot be started because the number of total nodes is equal or lower than the size of the existing network.");
            antwProcessParams.waveStateCode = TWaveStateCode::cannotStartProcessTotalNodesNr;
            sendWaveResult(autonetworkResult);
            TRC_FUNCTION_LEAVE("");
            return;
          }

          // Check stop conditions - number of new nodes
          if ((antwInputParams.stopConditions.numberOfNewNodes != 0) && ((antwInputParams.stopConditions.numberOfNewNodes + antwProcessParams.bondedNodesNr) > MAX_ADDRESS))
          {
            TRC_INFORMATION("The AutoNetwork process cannot be started because the number of existing nodes plus number of new nodes exceeds the maximum network size.");
            antwProcessParams.waveStateCode = TWaveStateCode::cannotStartProcessNewNodesNr;
            sendWaveResult(autonetworkResult);
            TRC_FUNCTION_LEAVE("");
            return;
          }

          // Check addressSpace in relation to stop conditions
          if (antwInputParams.bondingControl.addressSpace.empty() == false)
          {
            // Stop condition - number of total nodes
            if (antwInputParams.stopConditions.numberOfTotalNodes != 0)
            {
              if (antwInputParams.bondingControl.addressSpace.size() < (antwInputParams.stopConditions.numberOfTotalNodes - antwProcessParams.bondedNodesNr))
              {
                TRC_INFORMATION("The AutoNetwork process cannot be started because the number of free network addresses limited by the Address space is lower than number of total Nodes in the Stop condition. Change the value in the Address space or in the Stop condition.");
                antwProcessParams.waveStateCode = TWaveStateCode::cannotStartProcessTotalNodesNr;
                sendWaveResult(autonetworkResult);
                TRC_FUNCTION_LEAVE("");
                return;
              }
            }

            // Stop condition - number of new nodes
            if (antwInputParams.stopConditions.numberOfNewNodes != 0)
            {
              if (antwInputParams.bondingControl.addressSpace.size() < antwInputParams.stopConditions.numberOfNewNodes)
              {
                TRC_INFORMATION("The AutoNetwork process cannot be started because the number of free network addresses limited by the Address space is lower than number of new Nodes in the Stop condition. Change the value in the Address space or in the Stop condition.");
                antwProcessParams.waveStateCode = TWaveStateCode::cannotStartProcessNewNodesNr;
                sendWaveResult(autonetworkResult);
                TRC_FUNCTION_LEAVE("");
                return;
              }
            }
          }
        }

        // Set FRC param to 0, store previous value
        antwProcessParams.FrcResponseTime = 0;
        antwProcessParams.FrcResponseTime = setFrcReponseTime(autonetworkResult, antwProcessParams.FrcResponseTime);
        TRC_INFORMATION("Set FRC Response time to 0x00 (_FRC_RESPONSE_TIME_40_MS)");

        // Set DPA param to 0, store previous value
        antwProcessParams.DpaParam = 0;
        antwProcessParams.DpaParam = setNoLedAndOptimalTimeslot(autonetworkResult, antwProcessParams.DpaParam);
        TRC_INFORMATION("No LED indication and use of optimal time slot length");

        // Set DPA Hops Param to 0xff, 0xff and store previous values
        antwProcessParams.RequestHops = 0xff;
        antwProcessParams.ResponseHops = 0xff;
        TPerCoordinatorSetHops_Request_Response response = setDpaHopsToTheNumberOfRouters(autonetworkResult, antwProcessParams.RequestHops, antwProcessParams.ResponseHops);
        antwProcessParams.RequestHops = response.RequestHops;
        antwProcessParams.ResponseHops = response.ResponseHops;
        TRC_INFORMATION("Number of hops set to the number of routers");

        // Start autonetwork
        TRC_INFORMATION("Automatic network construction in progress");
        antwProcessParams.countWaves = 0;
        antwProcessParams.initialBondedNodesNr = antwProcessParams.bondedNodesNr;
        using std::chrono::system_clock;
        bool waveRun = true;
        std::basic_string<uint8_t> FrcSelect, FrcOnlineNodes, FrcOfflineNodes, FrcSupportMultipleAuth;
        uint8_t retryAction, countDiscNodes = antwProcessParams.discoveredNodesNr;
        uint8_t maxStep, step;
        bool stepBreak;
        bool errorNodeBonded;
        bool breakAuthorized;

        // Initialize random seed
        std::srand(std::time(nullptr));
        uint8_t nodeSeed = (uint8_t)std::rand();

        // Calculate progress step per wave
        antwProcessParams.progressStep = 6;
        if (!antwInputParams.skipDiscoveryEachWave)
          antwProcessParams.progressStep++;
        if (!antwInputParams.hwpidFiltering.empty())
          antwProcessParams.progressStep++;
        // Get DPA version
        // DPA >= 4.14 ?
        IIqrfDpaService::CoordinatorParameters coordParams = m_iIqrfDpaService->getCoordinatorParameters();
        if (coordParams.dpaVerWord >= 0x0414)
          antwProcessParams.progressStep++;

        // Main loop
        while (waveRun)
        {
          // Increment nodeSeed and countWaves
          nodeSeed++;
          antwProcessParams.countWaves++;

          // Clear progress
          antwProcessParams.progress = 0;

          // Run Discovery before start ?
          if ((antwProcessParams.countWaves == 1) && antwInputParams.discoveryBeforeStart && (antwProcessParams.bondedNodesNr > 0))
          {
            TRC_INFORMATION("Running Discovery before start.");
            antwProcessParams.waveStateCode = TWaveStateCode::discoveryBeforeStart;
            sendWaveState();
            runDiscovery(autonetworkResult, antwInputParams.discoveryTxPower);
          }

          antwProcessParams.countWaveNewNodes = 0;
          antwProcessParams.unbondedNodes = 0;
          antwProcessParams.respondedNewNodes.clear();
          // SmartConnect
          if (antwInputParams.skipPrebonding == false)
          {
            TRC_INFORMATION("SmartConnect.");
            antwProcessParams.waveStateCode = TWaveStateCode::smartConnect;
            sendWaveState();
            smartConnect(autonetworkResult);
            // ToDo
            std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_STEP));
          }
          else
            TRC_INFORMATION("SmartConnect skipped.");

          // Get pre-bonded alive nodes
          TRC_INFORMATION("Reading prebonded alive nodes.");
          antwProcessParams.waveStateCode = TWaveStateCode::checkPrebondedAlive;
          sendWaveState();
          FrcSelect = FrcPrebondedAliveNodes(autonetworkResult, nodeSeed);

          // Clear prebondedNodes map
          antwProcessParams.prebondedNodes.clear();
          // Clear new nodes for the next wave
          autonetworkResult.clearNewNodes();

          // Empty wave ?
          if (FrcSelect.size() == 0)
          {
            // Clear duplicit MIDs
            clearDuplicitMID(autonetworkResult);
            // Increase empty wave counter
            antwProcessParams.countEmpty++;
            // Check the wave is last one
            if (checkLastWave() == true)
              break;
            // Wave is not the last, send result and continue
            sendWaveResult(autonetworkResult);
            continue;
          }

          // Abort the autonetwork process when requested number of nodes (total/new) is found
          if (antwInputParams.abortOnTooManyNodesFound == true)
          {
            // Check number of total nodes
            if ((antwInputParams.stopConditions.numberOfTotalNodes != 0) && (antwProcessParams.bondedNodesNr + FrcSelect.size() > antwInputParams.stopConditions.numberOfTotalNodes))
            {
              TRC_INFORMATION("Too many nodes found - Autonetwork process aborted.");
              antwProcessParams.waveStateCode = TWaveStateCode::abortOnTooManyNodesFound;
              sendWaveResult(autonetworkResult);
              break;
            }

            // Check number of new nodes
            if ((antwInputParams.stopConditions.numberOfNewNodes != 0) && (antwProcessParams.countNewNodes + FrcSelect.size() > antwInputParams.stopConditions.numberOfNewNodes))
            {
              TRC_INFORMATION("Too many nodes found - Autonetwork process aborted.");
              antwProcessParams.waveStateCode = TWaveStateCode::abortOnTooManyNodesFound;
              sendWaveResult(autonetworkResult);
              break;
            }
          }

          // ToDo
          std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_STEP));

          // DPA >= 4.14 ?
          if ((coordParams.dpaVerWord >= 0x0414) && (FrcSelect.size() > 1))
          {
            // Check the DPA version of prebonded nodes is >= 0x0414
            TRC_INFORMATION("Reading prebonded alive nodes DPA version.");
            antwProcessParams.waveStateCode = TWaveStateCode::readingDPAVersion;
            sendWaveState();
            std::basic_string<uint8_t> frcData = FrcPrebondedMemoryCompare2B(autonetworkResult, nodeSeed, 0x0414, 0x04a0, PNUM_ENUMERATION, CMD_GET_PER_INFO);
            for (uint8_t node : FrcSelect)
            {
              // The condition was met ?
              if (((frcData[node / 8] & (1 << (node % 8))) == 0) && ((frcData[32 + node / 8] & (1 << (node % 8))) != 0))
                FrcSupportMultipleAuth.push_back(node);
            }
            // ToDo
            std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_STEP));
          }

          // Read MIDs of prebonded alive nodes
          TRC_INFORMATION(NAME_PAR(Prebonded alive nodes, FrcSelect.size()));
          antwProcessParams.waveStateCode = TWaveStateCode::readPrebondedMID;
          sendWaveState();
          uint8_t offset = 0x00;
          maxStep = 0x00;
          stepBreak = false;
          errorNodeBonded = false;
          do
          {
            // Prebonded memory read plus 1 - read MIDs
            std::basic_string<uint8_t> prebondedMemoryData = FrcPrebondedMemoryRead4BPlus1(autonetworkResult, FrcSelect, nodeSeed, offset, 0x04a0, PNUM_OS, CMD_OS_READ);
            uint8_t i = 0;
            do
            {
              // TPrebondedNode structure
              TPrebondedNode node;
              node.authorize = false;
              node.authorizeErr = TAuthorizeErr::eNo;
              node.node = FrcSelect[(i / 4) + offset];
              node.supportMultipleAuth = std::find(FrcSupportMultipleAuth.begin(), FrcSupportMultipleAuth.end(), node.node) != FrcSupportMultipleAuth.end();
              node.mid.bytes[0] = prebondedMemoryData[i];
              node.mid.bytes[1] = prebondedMemoryData[i + 1];
              node.mid.bytes[2] = prebondedMemoryData[i + 2];
              node.mid.bytes[3] = prebondedMemoryData[i + 3];
              // Node responded to prebondedMemoryRead plus 1 ?
              if (node.mid.value != 0)
              {
                // Yes, decrease MID
                node.mid.value--;

                // Check duplicit MID in antwProcessParams.prebondedNodes
                if (antwProcessParams.prebondedNodes.size() != 0)
                {
                  // Compare current node MID with all other prebonded nodes MID
                  for (auto n : antwProcessParams.prebondedNodes)
                  {
                    // Duplicit MID ?
                    if ((n.second.mid.value == node.mid.value) && (n.second.authorize == true))
                    {
                      // Duplicit nodes must not be authorized
                      TRC_WARNING("Reading prebonded MID: Duplicit MID " << PAR(node.mid.value) << " detected.");
                      node.authorizeErr = TAuthorizeErr::eFRC;
                      n.second.authorizeErr = TAuthorizeErr::eFRC;
                      n.second.authorize = false;
                      maxStep--;
                      break;
                    }
                  }
                }

                // HWPID filtering active ?
                if ((antwInputParams.hwpidFiltering.empty() == true) && (node.authorizeErr == TAuthorizeErr::eNo))
                {
                  // Authorize control
                  if (authorizeControl(node.mid.value, 0, node.addrBond, node.authorizeErr) == true)
                  {
                    if (stepBreak == false)
                    {
                      node.authorize = true;
                      maxStep++;
                      if (maxStep + antwProcessParams.bondedNodesNr >= MAX_ADDRESS)
                        stepBreak = true;

                      // Check number of total nodes
                      if ((antwInputParams.stopConditions.numberOfTotalNodes != 0) && (maxStep + antwProcessParams.bondedNodesNr >= antwInputParams.stopConditions.numberOfTotalNodes))
                        stepBreak = true;

                      // Check number of new nodes
                      if ((antwInputParams.stopConditions.numberOfNewNodes != 0) && (maxStep + antwProcessParams.countNewNodes >= antwInputParams.stopConditions.numberOfNewNodes))
                        stepBreak = true;
                    }
                  }
                  else
                  {
                    if ((antwInputParams.unbondUnrespondingNodes == false) && (node.authorizeErr == TAuthorizeErr::eNodeBonded))
                    {
                      errorNodeBonded = true;
                      node.authorize = true;
                    }
                  }
                }
              }
              else
              {
                // Node didn't respond to prebondedMemoryRead plus 1
                node.authorizeErr = TAuthorizeErr::eFRC;
                TRC_WARNING("Reading prebonded MID: Node " << PAR((int)node.node) << " doesn't respond to FRC.");
              }

              //  Add (or set) node to prebondedNodes map
              antwProcessParams.prebondedNodes[node.node] = node;

              // Next Node
              i += sizeof(TMID);
            } while ((i < 60) && (FrcSelect.size() > antwProcessParams.prebondedNodes.size()) && ((stepBreak == false) || (antwInputParams.unbondUnrespondingNodes == false)));
            offset += 15;
          } while ((FrcSelect.size() > antwProcessParams.prebondedNodes.size()) && ((stepBreak == false) || (antwInputParams.unbondUnrespondingNodes == false)));

          // ToDo
          std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_STEP));

          // Read HWPID of prebonded alive nodes if HWP filtering requested
          if (antwInputParams.hwpidFiltering.empty() == false)
          {
            // Check prebonded nodes
            for (auto n : antwProcessParams.prebondedNodes)
            {
              // MID reading error (or duplicit MID) ?
              if (n.second.authorizeErr == TAuthorizeErr::eFRC)
              {
                // Yes, remove node from FrcSelect vector - don't read HWPID
                auto nFrcSelect(std::find(FrcSelect.begin(), FrcSelect.end(), n.first));
                if (nFrcSelect != FrcSelect.end())
                  FrcSelect.erase(nFrcSelect);
              }
            }
            // Read HWPID of prebonded alive nodes
            TRC_INFORMATION("Reading HWPID of prebonded alive nodes.");
            antwProcessParams.waveStateCode = TWaveStateCode::readPrebondedHWPID;
            sendWaveState();
            offset = 0x00;
            uint8_t prebondedNodesCount = 0;
            maxStep = 0x00;
            stepBreak = false;
            do
            {
              // Prebonded memory read plus 1 - read HWPID and HWPVer
              std::basic_string<uint8_t> prebondedMemoryData = FrcPrebondedMemoryRead4BPlus1(autonetworkResult, FrcSelect, nodeSeed, offset, 0x04a7, PNUM_ENUMERATION, CMD_GET_PER_INFO);
              uint8_t i = 0;
              do
              {
                uint8_t addr = FrcSelect[(i / 4) + offset];
                auto n = antwProcessParams.prebondedNodes.find(addr);
                if (n != antwProcessParams.prebondedNodes.end())
                {
                  // Node responded to prebondedMemoryRead plus 1 ?
                  TPrebondedNode node = n->second;
                  node.authorize = false;
                  uint32_t HWPID_HWPVer = prebondedMemoryData[i];
                  HWPID_HWPVer |= (prebondedMemoryData[i + 1] << 8);
                  HWPID_HWPVer |= (prebondedMemoryData[i + 2] << 16);
                  HWPID_HWPVer |= (prebondedMemoryData[i + 3] << 24);
                  if (HWPID_HWPVer != 0)
                  {
                    // Yes, decrease HWPID_HWPVer
                    HWPID_HWPVer--;
                    node.HWPID = HWPID_HWPVer & 0xffff;
                    node.HWPIDVer = HWPID_HWPVer >> 16;
                    // Authorize control
                    if (authorizeControl(node.mid.value, node.HWPID, node.addrBond, node.authorizeErr) == true)
                    {
                      if (stepBreak == false)
                      {
                        node.authorize = true;
                        maxStep++;
                        if (maxStep + antwProcessParams.bondedNodesNr >= MAX_ADDRESS)
                          stepBreak = true;

                        // Check number of total nodes
                        if ((antwInputParams.stopConditions.numberOfTotalNodes != 0) && (maxStep + antwProcessParams.bondedNodesNr >= antwInputParams.stopConditions.numberOfTotalNodes))
                          stepBreak = true;

                        // Check number of new nodes
                        if ((antwInputParams.stopConditions.numberOfNewNodes != 0) && (maxStep + antwProcessParams.countNewNodes >= antwInputParams.stopConditions.numberOfNewNodes))
                          stepBreak = true;
                      }
                    }
                    else
                    {
                      if ((antwInputParams.unbondUnrespondingNodes == false) && (node.authorizeErr == TAuthorizeErr::eNodeBonded))
                      {
                        errorNodeBonded = true;
                        node.authorize = true;
                      }
                    }
                  }
                  else
                  {
                    // Node didn't respond to prebondedMemoryRead plus 1
                    node.authorizeErr = TAuthorizeErr::eFRC;
                    TRC_WARNING("Reading prebonded HWPID: Node " << PAR((int)node.node) << " doesn't respond to FRC.");
                  }

                  // Add (or set) node to prebondedNodes map
                  antwProcessParams.prebondedNodes[node.node] = node;
                }

                // Next Node
                i += 2 * sizeof(uint16_t);
              } while ((i < 60) && (FrcSelect.size() > ++prebondedNodesCount) && !stepBreak);
              offset += 15;
            } while ((FrcSelect.size() > prebondedNodesCount) && !stepBreak);
          }

          // ToDo
          std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_STEP));

          // Unbond (at [C] side) nodes that are already bonded and repeating the bonding request
          if ((antwInputParams.unbondUnrespondingNodes == false) || (errorNodeBonded == true))
          {
            for (std::pair<uint8_t, TPrebondedNode> node : antwProcessParams.prebondedNodes)
            {
              if (node.second.authorizeErr == TAuthorizeErr::eNodeBonded)
              {
                try
                {
                  // Remove node at [C] side only
                  removeBondAtCoordinator(autonetworkResult, node.second.addrBond);
                  node.second.authorizeErr = TAuthorizeErr::eNo;
                  // Actualize networkNodes
                  antwProcessParams.networkNodes[node.first].bonded = false;
                  antwProcessParams.networkNodes[node.first].discovered = false;
                  antwProcessParams.networkNodes[node.first].mid.value = 0;
                  maxStep++;
                  antwProcessParams.unbondedNodes++;
                  antwProcessParams.countNewNodes--;
                  antwProcessParams.countWaveNewNodes--;
                }
                catch (const std::exception& ex)
                {
                  TRC_WARNING("Removing the bond " << PAR((int)node.second.addrBond) << " at [C] error: " << ex.what());
                }
              }
            }
          }

          // Authorize prebonded alive nodes
          FrcSelect.clear();
          step = 0x00;
          breakAuthorized = false;
          TRC_INFORMATION("Authorizing prebonded alive nodes.");
          antwProcessParams.waveStateCode = TWaveStateCode::authorize;
          sendWaveState();

          // Multiple authorization for DPA >= 4.14 ?
          if (coordParams.dpaVerWord >= 0x0414)
          {
            std::basic_string<TPrebondedNode> authrozireNodes;
            authrozireNodes.clear();
            uint8_t index = 0;
            for (std::pair<uint8_t, TPrebondedNode> node : antwProcessParams.prebondedNodes)
            {
              if (antwProcessParams.bondedNodesNr + antwProcessParams.countWaveNewNodes >= MAX_ADDRESS)
              {
                TRC_INFORMATION("All available network addresses are already allocated.");
                breakAuthorized = true;
                break;
              }

              // Node passed the authorizeControl (is intended to authorize) and supports multiple authorization ?
              if ((node.second.authorize == true) && (node.second.supportMultipleAuth == true))
              {
                // Yes, add node to multiple authorization list
                authrozireNodes.push_back(node.second);
                // Add authorized node to FrcSelect
                FrcSelect.push_back(node.second.addrBond);
                // Actualize networkNodes
                antwProcessParams.networkNodes[node.second.addrBond].bonded = true;
                antwProcessParams.networkNodes[node.second.addrBond].discovered = false;
                antwProcessParams.networkNodes[node.second.addrBond].mid.value = node.second.mid.value;
                antwProcessParams.networkNodes[node.second.addrBond].HWPID = node.second.HWPID;
                antwProcessParams.networkNodes[node.second.addrBond].HWPIDVer = node.second.HWPIDVer;
                //if (antwProcessParams.unbondedNodes == 0)
                //{
                  antwProcessParams.countWaveNewNodes++;
                  antwProcessParams.countNewNodes++;
                //}
                // Increase number of authorized nodes
                step++;
              }

              // Multiple authorization
              index++;
              if ((authrozireNodes.size() == 11) || (step >= maxStep) || (index == antwProcessParams.prebondedNodes.size()))
              {
                // Any nodes in the list ?
                if (authrozireNodes.size() != 0)
                {
                  retryAction = antwInputParams.repeat + 1;
                  do
                  {
                    try
                    {
                      authorizeBond(autonetworkResult, authrozireNodes);
                      authrozireNodes.clear();
                      break;
                    }
                    catch (const std::exception& ex)
                    {
                      TRC_WARNING("Authorizing node " << PAR(node.second.mid.value) << " error: " << ex.what());
                    }
                  } while (--retryAction != 0);
                }
              }

              // Check number of authorized nodes
              if (step >= maxStep)
                break;
            }
          }

          // Single authorization for [N] with DPA < 0x0414
          if (breakAuthorized == false)
          {
            for (std::pair<uint8_t, TPrebondedNode> node : antwProcessParams.prebondedNodes)
            {
              if (antwProcessParams.bondedNodesNr + antwProcessParams.countWaveNewNodes >= MAX_ADDRESS)
              {
                TRC_INFORMATION("All available network addresses are already allocated.");
                break;
              }

              // Check number of authorized nodes
              if (step >= maxStep)
                break;

              // Node passed the authorizeControl (is intended to authorize) ?
              if ((node.second.authorize == true) && (node.second.supportMultipleAuth == false))
              {
                // Yes, authorize the node
                retryAction = antwInputParams.repeat + 1;
                do
                {
                  try
                  {
                    // Authorize nodes with DPA < 4.14 one by one
                    std::basic_string<TPrebondedNode> authrozireNodes;
                    authrozireNodes.clear();
                    authrozireNodes.push_back(node.second);
                    TPerCoordinatorAuthorizeBond_Response response = authorizeBond(autonetworkResult, authrozireNodes);
                    // Add authorized node to FrcSelect
                    FrcSelect.push_back(response.BondAddr);
                    // Actualize networkNodes
                    antwProcessParams.networkNodes[response.BondAddr].bonded = true;
                    antwProcessParams.networkNodes[response.BondAddr].discovered = false;
                    antwProcessParams.networkNodes[response.BondAddr].mid.value = node.second.mid.value;
                    antwProcessParams.networkNodes[response.BondAddr].HWPID = node.second.HWPID;
                    antwProcessParams.networkNodes[response.BondAddr].HWPIDVer = node.second.HWPIDVer;
                    antwProcessParams.countWaveNewNodes++;
                    antwProcessParams.countNewNodes++;
                    // Increase number of authorized nodes
                    step++;
                    break;
                  }
                  catch (const std::exception& ex)
                  {
                    TRC_WARNING("Authorizing node " << PAR(node.second.mid.value) << " error: " << ex.what());
                  }
                } while (--retryAction != 0);
              }
            }
          }

          // ToDo
          std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_STEP));

          // TestCase - overit chovani clearDuplicitMID
          if (FrcSelect.size() == 0)
          {
            try
            {
              clearDuplicitMID(autonetworkResult);
            }
            catch (const std::exception& ex)
            {
              TRC_WARNING("Clear Duplicit MID error: " << ex.what());
            }
            antwProcessParams.countEmpty++;
            // Check the wave is last one
            if (checkLastWave() == true)
              break;
            // Wave is not the last, send result and continue
            sendWaveResult(autonetworkResult);
            continue;
          }
          else
            antwProcessParams.countEmpty = 0;

          // Ping nodes
          if (antwInputParams.unbondUnrespondingNodes == true)
          {
            // TestCase - v prubehu ping odpojit uspesne autorizovany [N], overit, ze se odbonduje
            // TestCase - behem jednotlivych cyklu ping [N] odpojovat/zapojovat, overit, jak probehne odbondovani
            TRC_INFORMATION("Pinging nodes.");
            antwProcessParams.waveStateCode = TWaveStateCode::ping;
            sendWaveState();
            FrcOnlineNodes.clear();
            retryAction = antwInputParams.repeat + 1;
            while ((FrcSelect.size() != 0) && (retryAction-- != 0))
            {
              try
              {
                // Add delay at next retries
                if (retryAction != antwInputParams.repeat)
                {
                  // ToDo
                  std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_REPEAT));
                }

                // FRC_Ping
                TPerFrcSend_Response response = FrcPingNodes(autonetworkResult);
                // Clear FrcPingOfflineNodes
                FrcOfflineNodes.clear();
                // Check the response
                for (uint8_t address = 1; address <= MAX_ADDRESS; address++)
                {
                  // Node bonded ?
                  if (antwProcessParams.networkNodes[address].bonded == true)
                  {
                    // Node acknowledged to FRC_Ping (Bit0 is set) ?
                    bool nodeOnline = (response.FrcData[address / 8] & (uint8_t)(1 << (address % 8))) != 0;
                    antwProcessParams.networkNodes[address].online = nodeOnline;
                    // Is node in authorized nodes list
                    auto node = std::find(FrcSelect.begin(), FrcSelect.end(), address);
                    bool nodeInAuthList = node != FrcSelect.end();
                    // Is node in online nodes list
                    bool nodeInOnlineList = std::find(FrcOnlineNodes.begin(), FrcOnlineNodes.end(), address) != FrcOnlineNodes.end();
                    // Node is online and is in authorized nodes list ?
                    if (nodeOnline && nodeInAuthList)
                    {
                      // Remove the node from FrcSelect (authorized nodes list)
                      FrcSelect.erase(node);
                      // Add node to FrcOnlineNodes list
                      FrcOnlineNodes.push_back(address);
                      antwProcessParams.respondedNewNodes.push_back({ address, antwProcessParams.networkNodes[address].mid.value });
                    }
                    else
                    {
                      // Add offline nodes to FrcOfflineNodes list
                      if (!nodeOnline && !nodeInAuthList && !nodeInOnlineList)
                      {
                        // Add node to FrcOnlineNodes list
                        FrcOfflineNodes.push_back(address);
                        TRC_WARNING("FRC_Ping: Node " << PAR((int)address) << " is offline.");
                      }
                      else
                      {
                        // If node is online a subsequent FRC, mark it as online
                        antwProcessParams.networkNodes[address].online = true;
                      }
                    }
                  }
                }
              }
              catch (const std::exception& ex)
              {
                TRC_WARNING("FRC_Ping: error: " << ex.what());
              }
            }

            // ToDo
            std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_STEP));

            // Remove not responded nodes
            retryAction = antwInputParams.repeat + 1;
            while ((FrcSelect.size() != 0) && (retryAction-- != 0))
            {
              // Add dealy at next retries
              if (retryAction != antwInputParams.repeat)
              {
                // ToDo
                std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_REPEAT));
              }

              TRC_INFORMATION("Removing not responded nodes.");
              antwProcessParams.waveStateCode = TWaveStateCode::removeNotResponded;
              sendWaveState();
              // FRC_AcknowledgedBroadcastBits remove bond (for DPA < 0x0400 - send Batch command Remove bond + Restart)
              TPerFrcSend_Response response = removeNotRespondedNewNodes(autonetworkResult, FrcSelect);
              // Check the nodes contained in FrcSelect list acknowledged FRC_AcknowledgedBroadcastBits
              for (uint8_t address = 1; address <= MAX_ADDRESS; address++)
              {
                auto node = std::find(FrcSelect.begin(), FrcSelect.end(), address);
                if (node != FrcSelect.end())
                {
                  // Bit0 is set ?
                  if ((response.FrcData[address / 8] & (uint8_t)(1 << (address % 8))) != 0)
                  {
                    // Yes, remove node at [C] side too
                    try
                    {
                      removeBondAtCoordinator(autonetworkResult, address);
                      // Remove the node from FrcSelect
                      FrcSelect.erase(node);
                      // Actualize networkNodes
                      antwProcessParams.networkNodes[address].bonded = false;
                      antwProcessParams.networkNodes[address].discovered = false;
                      antwProcessParams.networkNodes[address].mid.value = 0;
                      antwProcessParams.countWaveNewNodes--;
                      antwProcessParams.countNewNodes--;
                      TRC_INFORMATION("Removing Node " << PAR((int)address) << " at [C].");
                    }
                    catch (const std::exception& ex)
                    {
                      TRC_WARNING("Removing the bond " << PAR((int)address) << " at [C] error: " << ex.what());
                    }
                  }
                  else
                  {
                    TRC_WARNING("Remove not responded Nodes: Node " << PAR((int)address) << " doesn't respond to FRC.");
                  }
                }
              }
            }

            // ToDo
            std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_STEP));

            // Unbond node at coordinator only ?
            if (FrcSelect.size() != 0)
            {
              TRC_INFORMATION("Unbonding Nodes only at Coordinator.");
              for (uint8_t address = 1; address <= MAX_ADDRESS; address++)
              {
                auto node = std::find(FrcSelect.begin(), FrcSelect.end(), address);
                if (node != FrcSelect.end())
                {
                  // Insert duplicit node to duplicitMID
                  if (std::find(antwProcessParams.duplicitMID.begin(), antwProcessParams.duplicitMID.end(), address) == antwProcessParams.duplicitMID.end())
                    antwProcessParams.duplicitMID.push_back(address);
                  try
                  {
                    // Remove node at [C] side only
                    removeBondAtCoordinator(autonetworkResult, address);
                    // Remove the node from FrcSelect
                    FrcSelect.erase(node);
                    // Actualize networkNodes
                    antwProcessParams.networkNodes[address].bonded = false;
                    antwProcessParams.networkNodes[address].discovered = false;
                    antwProcessParams.networkNodes[address].mid.value = 0;
                    antwProcessParams.countWaveNewNodes--;
                    antwProcessParams.countNewNodes--;
                  }
                  catch (const std::exception& ex)
                  {
                    TRC_WARNING("Removing the bond " << PAR((int)address) << " at [C] error: " << ex.what());
                  }
                }
              }
            }

            // ToDo
            std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_STEP));

            // Clear duplicit MIDs
            // TestCase - overit chovani clearDuplicitMID
            clearDuplicitMID(autonetworkResult);
          } else {
            for (auto nodeAddr : FrcSelect) {
              if (!antwProcessParams.networkNodes[nodeAddr].bonded) {
                continue;
              }
              antwProcessParams.respondedNewNodes.push_back({
                nodeAddr,
                antwProcessParams.networkNodes[nodeAddr].mid.value
              });
            }
          }

          // Skip discovery in each wave ?
          if (antwInputParams.skipDiscoveryEachWave == false)
          {
            if ((antwProcessParams.countWaveNewNodes != 0) || ((antwInputParams.unbondUnrespondingNodes == false) && (errorNodeBonded == true)))
            {
              retryAction = antwInputParams.repeat + 1;
              do
              {
                TRC_INFORMATION("Running discovery.");
                antwProcessParams.waveStateCode = TWaveStateCode::discovery;
                sendWaveState();
                try
                {
                  uint8_t discNodes = runDiscovery(autonetworkResult, antwInputParams.discoveryTxPower);
                  if (countDiscNodes <= discNodes)
                    break;
                }
                catch (const std::exception& ex)
                {
                  TRC_WARNING("Discovery failed: " << ex.what());
                }
              } while (--retryAction != 0);

              updateNetworkInfo(autonetworkResult);
              countDiscNodes = antwProcessParams.discoveredNodesNr;
            }
          }
          else
            updateNetworkInfo(autonetworkResult);

          // New nodes bonded ?
          if (antwProcessParams.countWaveNewNodes != 0)
          {
            // Copy new nodes to response
            for (auto node : antwProcessParams.respondedNewNodes)
            {
              autonetworkResult.putNewNode(node.address, node.MID);
            }
          }

          // Check the wave is last one
          if (checkLastWave() == true)
            break;

          // Wave is not the last, send result and continue
          sendWaveResult(autonetworkResult);
        }
      }
      catch (const std::exception& ex)
      {
        TRC_WARNING("Error during algorithm run: " << ex.what());
      }

      // Unbond temporary address, set initial FRC param, DPA param and DPA Hops param
      try
      {
        // Unbond temporary address
        if(antwInputParams.skipPrebonding == false)
          unbondTemporaryAddress(autonetworkResult);
        // Get DPA version
        IIqrfDpaService::CoordinatorParameters coordParams = m_iIqrfDpaService->getCoordinatorParameters();
        if ((coordParams.dpaVerWord < 0x0417) && (antwProcessParams.bondedNodesNr > 0))
        {
          TRC_INFORMATION("Restarting nodes.");
          std::basic_string<uint8_t> FrcOfflineNodes;
          uint8_t retryAction = antwInputParams.repeat + 1;
          do
          {
            try
            {
              // Add delay at next retries
              if (--retryAction != antwInputParams.repeat)
              {
                // ToDo
                std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_REPEAT));
              }

              // FRC_AcknowledgedBroadcastBits - Restart nodes
              TPerFrcSend_Response response = FrcRestartNodes(autonetworkResult);
              // Clear FrcPingOfflineNodes
              FrcOfflineNodes.clear();
              // Check the response
              for (uint8_t address = 1; address <= MAX_ADDRESS; address++)
              {
                // Node bonded ?
                if (antwProcessParams.networkNodes[address].bonded == true)
                {
                  // Node acknowledged to FRC_AcknowledgedBroadcastBits - Restart nodes (Bit0 is set) ?
                  bool nodeOnline = (response.FrcData[address / 8] & (uint8_t)(1 << (address % 8))) != 0;
                  if (nodeOnline == false)
                    FrcOfflineNodes.push_back(address);
                }
              }
            }
            catch (const std::exception& ex)
            {
              TRC_WARNING("FRC_AcknowledgedBroadcastBits: error: " << ex.what());
            }
          } while ((FrcOfflineNodes.size() != 0) && (retryAction != 0));
        }
        // Set initial FRC param
        if (antwProcessParams.FrcResponseTime != 0)
          antwProcessParams.FrcResponseTime = setFrcReponseTime(autonetworkResult, antwProcessParams.FrcResponseTime);
        // Set initial DPA param
        if (antwProcessParams.DpaParam != 0)
          antwProcessParams.DpaParam = setNoLedAndOptimalTimeslot(autonetworkResult, antwProcessParams.DpaParam);
        // Set initial DPA Hops param
        if ((antwProcessParams.RequestHops != 0xff) || (antwProcessParams.ResponseHops != 0xff))
          setDpaHopsToTheNumberOfRouters(autonetworkResult, antwProcessParams.RequestHops, antwProcessParams.ResponseHops);
      }
      catch (const std::exception& ex)
      {
        // Set error
        TRC_WARNING("Error during algorithm run: " << ex.what());
      }

      // Send result
      sendWaveResult(autonetworkResult);

      TRC_FUNCTION_LEAVE("");
    }

    // Process request
    void handleMsg(const MessagingInstance &messaging, const IMessagingSplitterService::MsgType& msgType, rapidjson::Document doc)
    {
      TRC_FUNCTION_ENTER(
        PAR( messaging.to_string() ) <<
        NAME_PAR(mType, msgType.m_type) <<
        NAME_PAR(major, msgType.m_major) <<
        NAME_PAR(minor, msgType.m_minor) <<
        NAME_PAR(micro, msgType.m_micro)
      );

      // Unsupported type of request
      if (msgType.m_type != m_mTypeName_Autonetwork)
        THROW_EXC(std::logic_error, "Unsupported message type: " << PAR(msgType.m_type));

      // Create representation object
      ComAutonetwork comAutonetwork(doc);

      // Parsing and checking service parameters
      try
      {
        // Get input params
        antwInputParams = comAutonetwork.getAutonetworkParams();

        // Check addressSpace
        if (antwInputParams.bondingControl.duplicitAddressSpace != 0)
          THROW_EXC(std::logic_error, "Duplicit Address in Address space.");

        // Check midList
        if (antwInputParams.bondingControl.midList.empty() == false)
        {
          if (antwInputParams.bondingControl.duplicitMidMidList != 0)
            THROW_EXC(std::logic_error, "Duplicit MID in MID list.");
          if (antwInputParams.bondingControl.duplicitAddressMidList != 0)
            THROW_EXC(std::logic_error, "Duplicit Address in MID list.");
        }
      }
      catch (const std::exception& e)
      {
        const char* errorStr = e.what();
        TRC_WARNING("Error while parsing service input parameters: " << PAR(errorStr));
        // Create error response
        Document response;
        Pointer("/mType").Set(response, msgType.m_type);
        Pointer("/data/msgId").Set(response, comAutonetwork.getMsgId());
        // Set result
        Pointer("/data/status").Set(response, parsingRequestError);
        Pointer("/data/statusStr").Set(response, errorStr);
        m_iMessagingSplitterService->sendMessage(messaging, std::move(response));

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
        Pointer("/data/msgId").Set(response, comAutonetwork.getMsgId());
        // Set result
        Pointer("/data/status").Set(response, exclusiveAccessError);
        Pointer("/data/statusStr").Set(response, errorStr);
        m_iMessagingSplitterService->sendMessage(messaging, std::move(response));

        TRC_FUNCTION_LEAVE("");
        return;
      }

      try {
        // Run autonetwork
        m_msgType = &msgType;
        m_messaging = &messaging;
        m_comAutonetwork = &comAutonetwork;
        runAutonetwork();
      } catch (const std::exception &e) {
        m_exclusiveAccess.reset();
        THROW_EXC_TRC_WAR(std::logic_error, e.what());
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
        "Autonetwork instance activate" << std::endl <<
        "************************************"
      );

      (void)props;

      // for the sake of register function parameters
      std::vector<std::string> supportedMsgTypes =
      {
        m_mTypeName_Autonetwork
      };


      m_iMessagingSplitterService->registerFilteredMsgHandler(
        supportedMsgTypes,
        [&](const MessagingInstance &messaging, const IMessagingSplitterService::MsgType & msgType, rapidjson::Document doc)
        {
          handleMsg(messaging, msgType, std::move(doc));
        });

      TRC_FUNCTION_LEAVE("")
    }

    void deactivate()
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "************************************" << std::endl <<
        "Autonetwork instance deactivate" << std::endl <<
        "************************************"
      );

      std::vector<std::string> supportedMsgTypes =
      {
        m_mTypeName_Autonetwork
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

  AutonetworkService::AutonetworkService()
  {
    m_imp = shape_new Imp( *this );
  }

  AutonetworkService::~AutonetworkService()
  {
    delete m_imp;
  }

  void AutonetworkService::attachInterface(IIqrfDpaService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void AutonetworkService::detachInterface(IIqrfDpaService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void AutonetworkService::attachInterface(IMessagingSplitterService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void AutonetworkService::detachInterface(IMessagingSplitterService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void AutonetworkService::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void AutonetworkService::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }


  void AutonetworkService::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void AutonetworkService::deactivate()
  {
    m_imp->deactivate();
  }

  void AutonetworkService::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
  }
}

