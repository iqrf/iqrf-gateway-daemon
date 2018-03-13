#define IWriteConfigService_EXPORTS

#include "DpaTransactionTask.h"
#include "WriteConfigService.h"
#include "Trace.h"
#include "PrfOs.h"
#include "DpaRaw.h"
#include "PrfFrc.h"

#include "iqrf__WriteConfigService.hxx"

//TODO workaround old tracing 
#include "IqrfLogging.h"

#include <list>
#include <memory>
#include <math.h>
#include <bitset>

TRC_INIT();

TRC_INIT_MODULE(iqrf::WriteConfigService);

namespace {

  // length of configuration field [in bytes]
  // see IQRF DPA Write HWP configuration
  static const uint8_t CONFIGURATION_LEN = 31;

  // range of valid addresses of configuration bytes
  static const uint8_t CONFIG_BYTES_START_ADDR = 0x01;
  static const uint8_t CONFIG_BYTES_END_ADDR = 0x20;
  
  // number of all configuration bytes, INCLUDING RFPGM byte
  static const uint8_t CONFIG_BYTES_LEN = CONFIG_BYTES_END_ADDR - CONFIG_BYTES_START_ADDR + 1;

  // main RF channel A of the optional subordinate network
  static const uint8_t CONFIG_BYTES_SUBORD_RF_CHANNEL_A_ADDR = 0x06;

  // RF channel B of the optional subordinate network
  static const uint8_t CONFIG_BYTES_SUBORD_RF_CHANNEL_B_ADDR = 0x07;

  // main RF channel A of the main network
  static const uint8_t CONFIG_BYTES_MAIN_RF_CHANNEL_A_ADDR = 0x11;

  // RF channel B of the main network
  static const uint8_t CONFIG_BYTES_MAIN_RF_CHANNEL_B_ADDR = 0x12;

  // address of RFPGM settings byte in HWP configuration block
  static const uint8_t CONFIG_BYTES_RFPGM_ADDR = 0x20;

  // coordinator's RF channel band
  static const uint8_t CONFIG_BYTES_COORD_RF_CHANNEL_BAND_ADDR = 0x21;

  // maximum length of FRC user data [in bytes] 
  static const uint8_t FRC_MAX_USER_DATA_LEN = 30;
};


namespace iqrf {
  WriteConfigService::WriteConfigService()
  {
    TRC_FUNCTION_ENTER("");
    TRC_FUNCTION_LEAVE("");
  }

  WriteConfigService::~WriteConfigService()
  {
    TRC_FUNCTION_ENTER("");
    TRC_FUNCTION_LEAVE("");
  }

  void WriteConfigService::activate(const shape::Properties *props)
  {
    TRC_FUNCTION_ENTER("");
    TRC_INFORMATION(std::endl <<
      "************************************" << std::endl <<
      "WriteConfigService instance activate" << std::endl <<
      "************************************"
    );

    //TODO workaround old tracing 
    TRC_START("TraceOldBaseService.txt", iqrf::Level::dbg, TRC_DEFAULT_FILE_MAXSIZE);

    TRC_FUNCTION_LEAVE("");
  }

  void WriteConfigService::deactivate()
  {
    TRC_FUNCTION_ENTER("");
    TRC_INFORMATION(std::endl <<
      "**************************************" << std::endl <<
      "WriteConfigService instance deactivate" << std::endl <<
      "**************************************"
    );
    TRC_FUNCTION_LEAVE("");
  }

  void WriteConfigService::modify(const shape::Properties *props)
  {
    TRC_FUNCTION_ENTER("");
    TRC_INFORMATION(std::endl <<
      "***********************************" << std::endl <<
      "WriteConfigService instance modfify" << std::endl <<
      "***********************************"
    );
    TRC_FUNCTION_LEAVE("");
  }

  void WriteConfigService::attachInterface(iqrf::IIqrfDpaService* iface)
  {
    TRC_FUNCTION_ENTER(PAR(iface));
    m_dpa = iface;
    TRC_FUNCTION_LEAVE("")
  }

  void WriteConfigService::detachInterface(iqrf::IIqrfDpaService* iface)
  {
    TRC_FUNCTION_ENTER(PAR(iface));
    if (m_dpa == iface) {
      m_dpa = nullptr;
    }
    TRC_FUNCTION_LEAVE("")
  }

  void WriteConfigService::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void WriteConfigService::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }



  void WriteConfigService::checkTargetNodes(const std::list<uint16_t>& targetNodes)
  {
    if (targetNodes.empty()) {
      THROW_EXC(std::exception, "No target nodes.");
    }

    for (uint16_t addr : targetNodes) {
      if (addr < 0 || addr > 0xEF) {
        THROW_EXC(
          std::exception, "Node address outside of valid range. " << NAME_PAR_HEX("Address", addr)
        );
      }
    }
  }

  void WriteConfigService::checkConfigBytes(const std::vector<HWP_ConfigByte>& configBytes) 
  {
    // initialized with zeroes
    std::bitset<CONFIG_BYTES_LEN> configBytesUsed;

    for (const HWP_ConfigByte configByte : configBytes) {
      if (
           (configByte.address < CONFIG_BYTES_START_ADDR) 
        || (configByte.address > CONFIG_BYTES_END_ADDR)
      ) {
        THROW_EXC(
          std::exception, "Address of config byte out of valid range: " << NAME_PAR_HEX("Address", configByte.address)
        );
      }

      if (configBytesUsed.test(configByte.address-1)) {
        THROW_EXC(std::exception, "Config byte with the same address already exist.");
      }

      configBytesUsed[configByte.address-1] = 1;
    }
  }

  void processWriteError(
    WriteResult& writeResult,
    const uint16_t nodeAddr,
    const std::vector<HWP_ConfigByte>& failedConfigBytes,
    const WriteError::Type errType, 
    const std::string& errMsg
  )
  {
    WriteError writeError(errType, errMsg);
    
    std::map<uint16_t, NodeWriteResult> nodeResultsMap = writeResult.getResultsMap();
    std::map<uint16_t, NodeWriteResult>::iterator it = nodeResultsMap.find(nodeAddr);

    if (it == nodeResultsMap.end()) {
      it->second.setError(writeError);
      it->second.putFailedBytes(failedConfigBytes);
    }
    else {
      NodeWriteResult nodeWriteResult;
      nodeWriteResult.setError(writeError);
      nodeWriteResult.putFailedBytes(failedConfigBytes);
      writeResult.putResult(nodeAddr, nodeWriteResult);
    }
  }

  void processWriteError(
    WriteResult& writeResult,
    const std::list<uint16_t>& targetNodes,
    const std::vector<HWP_ConfigByte>& failedConfigBytes,
    const WriteError::Type errType,
    const std::string& errMsg
  )
  {
    std::map<uint16_t, NodeWriteResult> nodeResultsMap = writeResult.getResultsMap();
    std::map<uint16_t, NodeWriteResult>::iterator it = nodeResultsMap.end();

    WriteError writeError(errType, errMsg);

    for (const uint16_t targetNode : targetNodes) {
      it = nodeResultsMap.find(targetNode);
      if (it == nodeResultsMap.end()) {
        it->second.setError(writeError);
        it->second.putFailedBytes(failedConfigBytes);
      }
      else {
        NodeWriteResult nodeWriteResult;
        nodeWriteResult.setError(writeError);
        nodeWriteResult.putFailedBytes(failedConfigBytes);
        writeResult.putResult(targetNode, nodeWriteResult);
      }
    }
  }

  // counts checksum for specified configuration bytes
  // RFPGM byte IS NOT included in checksum count
  uint8_t countChecksum(const std::vector<HWP_ConfigByte>& configBytes) {
    uint8_t checksum = 0x5F;

    for ( const HWP_ConfigByte configByte : configBytes) {
      if (configByte.address != CONFIG_BYTES_RFPGM_ADDR) {
        checksum ^= configByte.value;
      }
    }

    return checksum;
  }

  // sets Write HWP Configuration request 'configuration' field according to
  // confiuration bytes
  // RFPGM byte IS NOT included
  void setWriteRequestConfigurationField(
    uns8 configuration[], 
    const std::vector<HWP_ConfigByte>& configBytes
  )
  {
    for (const HWP_ConfigByte configByte : configBytes) {
      if (configByte.address != CONFIG_BYTES_RFPGM_ADDR) {
        configuration[configByte.address] = configByte.value;
      }
    }
  }

  // returns value of RFPGM byte
  uint8_t getRfpgmByte(const std::vector<HWP_ConfigByte>& configBytes) {
    for (const HWP_ConfigByte configByte : configBytes) {
      if (configByte.address == CONFIG_BYTES_RFPGM_ADDR) {
        return configByte.value;
      }
    }

    THROW_EXC(std::exception, "RFPGM byte NOT found.");
  }


  // writes configuration bytes into specified node using
  // standard Write HWP Configuration DPA request
  void WriteConfigService::writeHwpConfiguration(
    WriteResult& writeResult,
    const std::vector<HWP_ConfigByte>& configBytes,
    const uint16_t nodeAddr
  ) 
  {
    TRC_FUNCTION_ENTER("");
    
    DpaMessage bondedNodesRequest;
    DpaMessage::DpaPacket_t bondedNodesPacket;
    bondedNodesPacket.DpaRequestPacket_t.NADR = nodeAddr;
    bondedNodesPacket.DpaRequestPacket_t.PNUM = PNUM_OS;
    bondedNodesPacket.DpaRequestPacket_t.PCMD = CMD_OS_WRITE_CFG;
    bondedNodesPacket.DpaRequestPacket_t.HWPID = HWPID_Default;

    TPerOSWriteCfg_Request* tOsWriteCfgRequest = &bondedNodesRequest.DpaPacket().DpaRequestPacket_t.DpaMessage.PerOSWriteCfg_Request;
    
    tOsWriteCfgRequest->Checksum = countChecksum(configBytes);
    setWriteRequestConfigurationField(tOsWriteCfgRequest->Configuration, configBytes);
    tOsWriteCfgRequest->RFPGM = getRfpgmByte(configBytes);

    // issue the DPA request
    bondedNodesRequest.DataToBuffer(bondedNodesPacket.Buffer, sizeof(TDpaIFaceHeader) + 2);

    DpaRaw bondedNodesTask(bondedNodesRequest);
    DpaTransactionTask bondedNodesTransaction(bondedNodesTask);

    try {
      m_dpa->executeDpaTransaction(bondedNodesTransaction);
    }
    catch (std::exception& e) {
      CATCH_EXC_TRC_WAR(std::exception, e, "Error in ExecuteDpaTransaction");
      bondedNodesTransaction.processFinish(DpaTransfer::kError);
      processWriteError(writeResult, nodeAddr, configBytes, WriteError::Type::Write, e.what());
      TRC_FUNCTION_LEAVE("");
      return;
    }

    int transResult = bondedNodesTransaction.waitFinish();

    TRC_DEBUG("Result from write config transaction as string:" << PAR(bondedNodesTransaction.getErrorStr()));

    if (transResult == 0) {
      TRC_INFORMATION("Write configuration done!");
      TRC_DEBUG("DPA transaction :" << NAME_PAR(bondedNodesTask.getPrfName(), bondedNodesTask.getAddress()) << PAR(bondedNodesTask.encodeCommand()));
      
      // getting response code
      DpaMessage dpaResponse = bondedNodesTask.getResponse();
      
      uint8_t respCode = dpaResponse.DpaPacket().DpaResponsePacket_t.ResponseCode;
      if (respCode == STATUS_NO_ERROR) {
        TRC_INFORMATION("Write configuration successful.");
      }
      else {
        TRC_DEBUG("Write configuration NOT successful." << NAME_PAR_HEX("Response code", respCode));
        processWriteError(writeResult, nodeAddr, configBytes, WriteError::Type::Write, "DPA error.");
      }
    }
    else {
      TRC_DEBUG("DPA transaction error." << NAME_PAR_HEX("Result", transResult));
      processWriteError(writeResult, nodeAddr, configBytes, WriteError::Type::Write, "Transaction error.");
    }

    TRC_FUNCTION_LEAVE("");
  }

  // puts selected nodes nodes into FRC request
  void setSelectedNodesForFrcRequest(
    DpaMessage& frcRequest, const std::list<uint16_t>& targetNodes
  )
  {
    uns8* selectedNodes = frcRequest.DpaPacket().DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.SelectedNodes;

    for (uint16_t i : targetNodes) {
      uns8 byteIndex = i / 8;
      uns8 bitIndex = i % 8;
      selectedNodes[byteIndex] |= (uns8)pow(2, bitIndex);
    }
  }

  void setUserDataForFrcWriteConfigByteRequest(
    DpaMessage& frcRequest,
    const std::vector<HWP_ConfigByte>& configBytes
  )
  {
    uns8* userData = frcRequest.DpaPacket().DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData;

    // copy foursome
    userData[0] = 1 + 1 + 1 + 2 + 7;
    userData[1] = PNUM_OS;
    userData[2] = CMD_OS_WRITE_CFG_BYTE;
    userData[3] = HWPID_Default & 0xFF;
    userData[4] = (HWPID_Default >> 8) & 0xFF;

    // fill in config bytes
    uint8_t dataIndex = 5;

    for (const HWP_ConfigByte configByte : configBytes) {
      userData[dataIndex++] = configByte.address;
      userData[dataIndex++] = configByte.value;
      userData[dataIndex++] = configByte.mask;
    }
  }

  // returns specified part of config bytes
  std::vector<HWP_ConfigByte>& getConfigBytesPart(
    int partId, 
    int partSize,
    const std::vector<HWP_ConfigByte>& configBytes
  )
  {
    std::vector<HWP_ConfigByte>::const_iterator first = configBytes.begin() + partId * partSize;
    std::vector<HWP_ConfigByte>::const_iterator last = configBytes.begin() + partId * (partSize + 1);
    return std::vector<HWP_ConfigByte>(first, last);
  }

  // parses 2bits FRC results into map: keys are nodes IDs, values are returned 2 bits
  std::map<uint16_t, uint8_t> parse2bitsFrcData(const std::basic_string<uint8_t>& frcData) {
    std::map<uint16_t, uint8_t> nodesResults;

    uint16_t nodeId = 0;
    for (int byteId = 0; byteId <= 29; byteId++) {
      int bitComp = 1;
      for (int bitId = 0; bitId < 8; bitId++) {
        uint8_t bit0 = ((frcData[byteId] & bitComp) == bitComp) ? 1 : 0;
        uint8_t bit1 = ((frcData[byteId + 32] & bitComp) == bitComp) ? 1 : 0;
        nodesResults.insert(std::pair<uint16_t, uint8_t>(nodeId, bit0 + 2 * bit1));
        nodeId++;
        bitComp *= 2;
      }
    }

    return nodesResults;
  }

  // puts nodes results into overall write config results
  void putFrcResults(
    WriteResult& writeResult,
    const std::vector<HWP_ConfigByte>& configBytes,
    WriteError::Type errorType,
    const std::map<uint16_t, uint8_t>& nodesResultsMap
  )
  { 
    std::map<uint16_t, NodeWriteResult> writeResultsMap = writeResult.getResultsMap();
    std::map<uint16_t, NodeWriteResult>::iterator it = writeResultsMap.end();

    for (const std::pair<uint16_t, uint8_t> p : nodesResultsMap) {
      // all ok
      if ((p.second & 0b1) == 0b1) {
        continue;
      }

      std::string errorMsg;
      if (p.second == 0) {
        errorMsg = "Node device did not respond to FRC at all.";
      }
      else {
        errorMsg = "HWPID did not match HWPID of the device.";
      }

      WriteError writeError(errorType, errorMsg);

      it = writeResultsMap.find(p.first);
      if (it == writeResultsMap.end()) {
        it->second.setError(writeError);
        it->second.putFailedBytes(configBytes);
      }
      else {
        NodeWriteResult nodeWriteResult;
        nodeWriteResult.setError(writeError);
        nodeWriteResult.putFailedBytes(configBytes);
        writeResult.putResult(p.first, nodeWriteResult);
      }
    }
  }

  // writes configuration bytes into target nodes
  void WriteConfigService::_writeConfigBytes(
    WriteResult& writeResult,
    const std::vector<HWP_ConfigByte>& configBytes,
    const std::list<uint16_t>& targetNodes
  ) 
  { 
    TRC_FUNCTION_ENTER("");
    
    DpaMessage frcRequest;
    DpaMessage::DpaPacket_t frcPacket;
    frcPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
    frcPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
    frcPacket.DpaRequestPacket_t.PCMD = CMD_FRC_SEND_SELECTIVE;
    frcPacket.DpaRequestPacket_t.HWPID = HWPID_Default;
    frcRequest.DpaPacket().DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.FrcCommand = FRC_AcknowledgedBroadcastBits;
    frcRequest.DataToBuffer(frcPacket.Buffer, sizeof(TDpaIFaceHeader) + 2);

    setSelectedNodesForFrcRequest(frcRequest, targetNodes);

    uint8_t WriteConfigPacketFreeSpace = FRC_MAX_USER_DATA_LEN
      - 1
      - sizeof(frcPacket.DpaRequestPacket_t.PCMD)
      - sizeof(frcPacket.DpaRequestPacket_t.PNUM)
      - sizeof(frcPacket.DpaRequestPacket_t.HWPID)
      ;

    // number of parts with config. bytes to send by FRC request
    uint8_t partsTotal = (configBytes.size() * 3) / WriteConfigPacketFreeSpace;

    for (int partId = 0; partId < partsTotal; partId++) {
      // prepare part of config bytes to fill into the FRC request user data
      std::vector<HWP_ConfigByte>& configBytesPart = getConfigBytesPart(partId, partsTotal, configBytes);
      setUserDataForFrcWriteConfigByteRequest(frcRequest, configBytesPart);

      // issue the DPA request
      frcRequest.DataToBuffer(frcPacket.Buffer, sizeof(TDpaIFaceHeader) + 2);

      DpaRaw bondedNodesTask(frcRequest);
      DpaTransactionTask bondedNodesTransaction(bondedNodesTask);

      try {
        m_dpa->executeDpaTransaction(bondedNodesTransaction);
      }
      catch (std::exception& e) {
        CATCH_EXC_TRC_WAR(std::exception, e, "Error in ExecuteDpaTransaction");
        bondedNodesTransaction.processFinish(DpaTransfer::kError);
        processWriteError(writeResult, targetNodes, configBytesPart, WriteError::Type::Write, e.what());
        continue;
      }

      int transResult = bondedNodesTransaction.waitFinish();
      
      // data from FRC
      std::basic_string<uns8> frcData;

      TRC_DEBUG("Result from FRC write config transaction as string:" << PAR(bondedNodesTransaction.getErrorStr()));

      if (transResult == 0) {
        TRC_INFORMATION("FRC write configuration done!");
        TRC_DEBUG("DPA transaction :" << NAME_PAR(bondedNodesTask.getPrfName(), bondedNodesTask.getAddress()) << PAR(bondedNodesTask.encodeCommand()));

        // getting response data
        DpaMessage dpaResponse = bondedNodesTask.getResponse();

        // check response code
        uint8_t respCode = dpaResponse.DpaPacket().DpaResponsePacket_t.ResponseCode;
        if (respCode != STATUS_NO_ERROR) {
          TRC_DEBUG("FRC write HWP config NOT successful." << NAME_PAR_HEX("Response code", respCode));
          continue;
        }

        // check status
        uns8 status = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.Status;
        if (status >= 0x00 && status <= 0xEF) {
          TRC_INFORMATION("FRC write config successful.");
          frcData.append(dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData);
        }
        else {
          TRC_DEBUG("FRC write config NOT successful." << NAME_PAR_HEX("Status", status));
          bondedNodesTransaction.processFinish(DpaTransfer::kReceivedResponse);
          processWriteError(writeResult, targetNodes, configBytesPart, WriteError::Type::Write, "FRC unsuccessful.");
          continue;
        }
      }
      else {
        TRC_DEBUG("DPA transaction error." << NAME_PAR_HEX("Result", transResult));
        processWriteError(writeResult, targetNodes, configBytesPart, WriteError::Type::Write, "Transaction error.");
        continue;
      }
      
      // get extra results
      DpaMessage extraResultRequest;
      DpaMessage::DpaPacket_t extraResultPacket;
      frcPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      frcPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
      frcPacket.DpaRequestPacket_t.PCMD = CMD_FRC_EXTRARESULT;
      frcPacket.DpaRequestPacket_t.HWPID = HWPID_Default;
      frcRequest.DataToBuffer(extraResultPacket.Buffer, sizeof(TDpaIFaceHeader) + 2);

      // get extra result from nodes
      DpaRaw extraResultTask(extraResultRequest);
      DpaTransactionTask extraResultTransaction(extraResultTask);

      try {
        m_dpa->executeDpaTransaction(extraResultTransaction);
      }
      catch (std::exception& e) {
        CATCH_EXC_TRC_WAR(std::exception, e, "Error in ExecuteDpaTransaction");
        extraResultTransaction.processFinish(DpaTransfer::kError);
        processWriteError(writeResult, targetNodes, configBytesPart, WriteError::Type::Write, e.what());
        continue;
      }

      transResult = extraResultTransaction.waitFinish();

      TRC_DEBUG("Result from FRC write config extra result transaction as string :" << PAR(extraResultTransaction.getErrorStr()));

      if (transResult == 0) {
        TRC_INFORMATION("FRC write config extra result done!");
        TRC_DEBUG("DPA transaction :" << NAME_PAR(extraResultTask.getPrfName(), extraResultTask.getAddress()) << PAR(extraResultTask.encodeCommand()));

        // getting response data
        DpaMessage dpaResponse = extraResultTask.getResponse();

        // check response code
        uint8_t respCode = dpaResponse.DpaPacket().DpaResponsePacket_t.ResponseCode;
        if (respCode != STATUS_NO_ERROR) {
          TRC_DEBUG("FRC write HWP config NOT successful." << NAME_PAR_HEX("Response code", respCode));
          continue;
        }

        // check response data
        uns8 status = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.Status;
        if (status >= 0x00 && status <= 0xEF) {
          TRC_INFORMATION("FRC write config extra result successful.");
          frcData.append(dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData);
        }
        else {
          TRC_DEBUG("FRC write config extra result NOT successful." << NAME_PAR_HEX("Status", status));
          extraResultTransaction.processFinish(DpaTransfer::kReceivedResponse);
          processWriteError(writeResult, targetNodes, configBytesPart, WriteError::Type::Write, "FRC unsuccessful.");
          continue;
        }
      }
      else {
        processWriteError(writeResult, targetNodes, configBytesPart, WriteError::Type::Write, "Transaction error.");
        TRC_DEBUG("DPA transaction error. " << NAME_PAR_HEX("Result", transResult));
        continue;
      }

      // FRC data parsing
      std::map<uint16_t, uint8_t> nodesResultsMap = parse2bitsFrcData(frcData);

      // putting nodes results into overall write config results
      putFrcResults(writeResult, configBytesPart, WriteError::Type::Write, nodesResultsMap);
    }

    TRC_FUNCTION_LEAVE("");
  }

  // parses bonded nodes - fills output parameter bondedNodes
  void parseBondedNodes(uint8_t bondedNodesArr[], std::list<uint16_t>& bondedNodes) 
  {
    // maximal bonded node number
    const uint8_t MAX_BONDED_NODE_NUMBER = 0xEF;
    const uint8_t MAX_BYTES_USED = (uint8_t)ceil(MAX_BONDED_NODE_NUMBER / 8.0);

    for (int byteId = 0; byteId < DPA_MAX_DATA_LENGTH; byteId++) {
      if (byteId >= MAX_BYTES_USED) {
        break;
      }

      if (bondedNodesArr[byteId] == 0) {
        continue;
      }

      int bitComp = 1;
      for (int bitId = 0; bitId < 8; bitId++) {
        if ((bondedNodesArr[byteId] & bitComp) == bitComp) {
          bondedNodes.push_back(byteId * 8 + bitId);
        }
        bitComp *= 2;
      }
    }
  }

  // returns list of bonded nodes
  std::list<uint16_t> WriteConfigService::getBondedNodes() 
  {
    TRC_FUNCTION_ENTER("");
    
    std::list<uint16_t> bondedNodes;

    DpaMessage bondedNodesRequest;
    DpaMessage::DpaPacket_t bondedNodesPacket;
    bondedNodesPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
    bondedNodesPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
    bondedNodesPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_BONDED_DEVICES;
    bondedNodesPacket.DpaRequestPacket_t.HWPID = HWPID_Default;
    bondedNodesRequest.DataToBuffer(bondedNodesPacket.Buffer, sizeof(TDpaIFaceHeader) + 2);
    
    // issue the DPA request
    DpaRaw bondedNodesTask(bondedNodesRequest);
    DpaTransactionTask bondedNodesTransaction(bondedNodesTask);

    try {
      m_dpa->executeDpaTransaction(bondedNodesTransaction);
    }
    catch (std::exception& e) {
      bondedNodesTransaction.processFinish(DpaTransfer::kError);
      THROW_EXC(std::exception, "Error in ExecuteDpaTransaction");
    }

    int transResult = bondedNodesTransaction.waitFinish();

    TRC_DEBUG("Result from get bonded nodes transaction as string:" << PAR(bondedNodesTransaction.getErrorStr()));

    if (transResult == 0) {
      TRC_INFORMATION("Get bonded nodes done!");
      TRC_DEBUG("DPA transaction :" << NAME_PAR(bondedNodesTask.getPrfName(), bondedNodesTask.getAddress()) << PAR(bondedNodesTask.encodeCommand()));

      // getting response data
      DpaMessage dpaResponse = bondedNodesTask.getResponse();

      // get response code
      uint8_t respCode = dpaResponse.DpaPacket().DpaResponsePacket_t.ResponseCode;
      if (respCode != STATUS_NO_ERROR) {
        TRC_DEBUG("Get bonded nodes NOT successful. " << NAME_PAR_HEX("Response code", respCode));
        THROW_EXC(std::exception, "Could not get bonded nodes.");
      }

      // parsing response data
      uns8* bondedNodesArr = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData;
      parseBondedNodes(bondedNodesArr, bondedNodes);
    }
    else {
      TRC_DEBUG("DPA transaction error. " << NAME_PAR_HEX("Result", transResult));
      THROW_EXC(std::exception, "Could not get bonded nodes.");
    }

    TRC_FUNCTION_LEAVE("");
    return bondedNodes;
  }

  // sorting, which of target nodes are bonded and which not
  void filterBond(
    const std::list<uint16_t>& targetNodes,
    const std::list<uint16_t>& bondedNodes,
    std::list<uint16_t>& targetBondedNodes,
    std::list<uint16_t>& targetNotBondedNodes
  )
  {
    std::list<uint16_t>::const_iterator findIter = bondedNodes.end();

    for (const uint16_t node : targetNodes) {
      findIter = std::find(bondedNodes.begin(), bondedNodes.end(), node);
      if (findIter != bondedNodes.end()) {
        targetBondedNodes.push_back(node);
      } else {
        targetNotBondedNodes.push_back(node);
      }
    }
  }

  // sets error for specified not bonded nodes
  void setNotBondedNodes(WriteResult& writeResult, const std::list<uint16_t>& targetNotBondedNodes) 
  {
    WriteError writeError(WriteError::Type::NodeNotBonded);
    
    NodeWriteResult nodeResult;
    nodeResult.setError(writeError);
    
    for (const uint16_t node : targetNotBondedNodes) {
      writeResult.putResult(node, nodeResult);
    }
  }

  // updates coordinator's RF channel band
  void WriteConfigService::updateCoordRfChannelBand() {
    TRC_FUNCTION_ENTER("");

    DpaMessage readConfigRequest;
    DpaMessage::DpaPacket_t readConfigPacket;
    readConfigPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
    readConfigPacket.DpaRequestPacket_t.PNUM = PNUM_OS;
    readConfigPacket.DpaRequestPacket_t.PCMD = CMD_OS_READ_CFG;
    readConfigPacket.DpaRequestPacket_t.HWPID = HWPID_Default;
    readConfigRequest.DataToBuffer(readConfigPacket.Buffer, sizeof(TDpaIFaceHeader) + 2);

    // issue the DPA request
    DpaRaw readConfigTask(readConfigRequest);
    DpaTransactionTask readConfigTransaction(readConfigTask);

    try {
      m_dpa->executeDpaTransaction(readConfigTransaction);
    }
    catch (std::exception& e) {
      readConfigTransaction.processFinish(DpaTransfer::kError);
      TRC_DEBUG("DPA transaction error : " << e.what());
      THROW_EXC(std::exception, "Could not read HWP configuration.");
    }

    int transResult = readConfigTransaction.waitFinish();

    TRC_DEBUG("Result from read HWP config transaction as string:" << PAR(readConfigTransaction.getErrorStr()));

    if (transResult == 0) {
      TRC_INFORMATION("Read HWP config done!");
      TRC_DEBUG("DPA transaction :" << NAME_PAR(readConfigTask.getPrfName(), readConfigTask.getAddress()) << PAR(readConfigTask.encodeCommand()));

      // getting response data
      DpaMessage dpaResponse = readConfigTask.getResponse();

      // check response code
      uint8_t respCode = dpaResponse.DpaPacket().DpaResponsePacket_t.ResponseCode;
      if (respCode != STATUS_NO_ERROR) {
        TRC_DEBUG("Read HWP config NOT successful. " << NAME_PAR_HEX("Response code", respCode));
        THROW_EXC(std::exception, "Could not read HWP configuration.");
      }

      // updating RF Channel band
      m_coordRfChannelBand 
        = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData[CONFIG_BYTES_COORD_RF_CHANNEL_BAND_ADDR];
    }
    else {
      TRC_DEBUG("DPA transaction error. " << NAME_PAR_HEX("Result", transResult));
      THROW_EXC(std::exception, "Could not read HWP configuration.");
    }

    TRC_FUNCTION_LEAVE("");
  }

  // indicates, whether specified RF channel is in specified channel band
  bool isInBand(uint8_t rfChannel, uint8_t band) {
    bool result = false;

    switch (band) {
      // 868 MHz
      case 0b00:
        if (rfChannel >= 0 && rfChannel <= 67) {
          result = true;
        }
        break;

      // 916 MHz
      case 0b01:
        if (rfChannel >= 0 && rfChannel <= 255) {
          result = true;
        }
        break;

        // 916 MHz
      case 0b10:
        if (rfChannel >= 0 && rfChannel <= 16) {
          result = true;
        }
        break;
      
      default:
        THROW_EXC(std::exception, "Unsupported RF band. " << NAME_PAR_HEX("Band", band));
    }

    return result;
  }

  // checks, if RF channel config bytes are in accordance with coordinator RF channel band
  void WriteConfigService::checkRfChannelIfPresent(const std::vector<HWP_ConfigByte>& configBytes) 
  {
    TRC_FUNCTION_ENTER("");

    bool isRfBandActual = false;
    
    for (const HWP_ConfigByte configByte : configBytes) {
      switch (configByte.address) {
        case CONFIG_BYTES_MAIN_RF_CHANNEL_A_ADDR:
        case CONFIG_BYTES_MAIN_RF_CHANNEL_B_ADDR:
        case CONFIG_BYTES_SUBORD_RF_CHANNEL_A_ADDR:
        case CONFIG_BYTES_SUBORD_RF_CHANNEL_B_ADDR:
          if (!isRfBandActual) {
            updateCoordRfChannelBand();
            isRfBandActual = true;
          }

          if (!isInBand(configByte.value, m_coordRfChannelBand)) {
            THROW_EXC(
              std::exception, NAME_PAR_HEX("RF channel", configByte.value) << " not in band: " << PAR(m_coordRfChannelBand)
            );
          }
        default:
          break;
      }
    }

    TRC_FUNCTION_LEAVE("");
  }


  WriteResult WriteConfigService::writeConfigBytes(
    const std::vector<HWP_ConfigByte>& configBytes,
    const std::list<uint16_t>& targetNodes
  )
  {
    checkConfigBytes(configBytes);
    checkTargetNodes(targetNodes);

    // result of writing configuration
    WriteResult writeResult;

    // getting list of all bonded nodes
    std::list<uint16_t> bondedNodes = getBondedNodes();
  
    // filter out, which one's of the target nodes are bonded and which not
    std::list<uint16_t> targetBondedNodes;
    std::list<uint16_t> targetNotBondedNodes;
    filterBond(targetNodes, bondedNodes, targetBondedNodes, targetNotBondedNodes);

    // remove NOT bonded nodes from next processing and set error for them into final result
    if (!targetNotBondedNodes.empty()) {
      setNotBondedNodes(writeResult, targetNotBondedNodes);
    }

    // if there are no target nodes, which are bonded, return
    if (targetBondedNodes.empty()) {
      TRC_INFORMATION("No target nodes, which are bonded.");
      return writeResult;
    }

    // if there are RF channel config. bytes, it is needed to check theirs values 
    // to be in accordance with coordinator's RF band
    checkRfChannelIfPresent(configBytes);

    // if there is only one node and it is needed to write ALL configuration bytes
    // use traditional Write HWP Configuration
    if ((targetBondedNodes.size() == 1) && (configBytes.size() == CONFIG_BYTES_LEN)) {
      writeHwpConfiguration(writeResult, configBytes, targetBondedNodes.front());
    }
    else {
      _writeConfigBytes(writeResult, configBytes, targetBondedNodes);
    }

    return writeResult;
  }
  
}