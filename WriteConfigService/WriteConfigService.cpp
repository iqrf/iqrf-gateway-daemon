#define IWriteConfigService_EXPORTS

#include "DpaTransactionTask.h"
#include "WriteConfigService.h"
#include "IMessagingSplitterService.h"
#include "Trace.h"
#include "ComMngIqmeshWriteConfig.h"
#include "ObjectFactory.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"

#include "iqrf__WriteConfigService.hxx"

#include <list>
#include <memory>
#include <math.h>
#include <bitset>

TRC_INIT_MODULE(iqrf::WriteConfigService);


using namespace rapidjson;

namespace {

  // maximum number of repeats
  static const int REPEAT_MAX = 3;

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

  // coordinator's RF channel band address
  static const uint8_t CONFIG_BYTES_COORD_RF_CHANNEL_BAND_ADDR = 0x21;

  // maximum length of FRC user data [in bytes] 
  static const uint8_t FRC_MAX_USER_DATA_LEN = 30;

  // maximum length of security password
  static const uint8_t SECURITY_PASSWORD_MAX_LEN = 16;

  // maximum length of security user key
  static const uint8_t SECURITY_USER_KEY_MAX_LEN = 16;

  // RF channel band
  enum class RF_ChannelBand {
    UNSPECIFIED,
    BAND_433,
    BAND_868,
    BAND_916
  };

  // service general fail code - may and probably will be changed later in the future
  static const int SERVICE_ERROR = 1000;

  static const int SERVICE_ERROR_READ_HWP = SERVICE_ERROR + 1;
};


namespace iqrf {

  // Holds information about errors, which encounter during configuration write
  class WriteError {
  public:

    // Type of error
    enum class Type {
      NoError,
      NodeNotBonded,
      Write,
      SecurityPassword,
      SecurityUserKey
    };

    WriteError() : m_type(Type::NoError), m_message("") {};
    WriteError(Type errorType) : m_type(errorType), m_message("") {};
    WriteError(Type errorType, const std::string& message) : m_type(errorType), m_message(message) {};
   
    Type getType() const { return m_type; };
    std::string getMessage() const { return m_message; };

    WriteError& operator=(const WriteError& error) {
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


  // Configuration byte - according to DPA spec
  struct HWP_ConfigByte {
    uint8_t address;
    uint8_t value;
    uint8_t mask;

    HWP_ConfigByte() {
      address = 0;
      value = 0;
      mask = 0;
    }

    HWP_ConfigByte(const uint8_t addressVal, const uint8_t valueVal, const uint8_t maskVal) {
      address = addressVal;
      value = valueVal;
      mask = maskVal;
    }
  };


  // Holds information about configuration writing result on one node
  class NodeWriteResult {
  private:
    // write error
    WriteError writeError;

    // map of configuration bytes, which failed to write
    // indexed by byte addresses
    std::map<uint8_t, HWP_ConfigByte> failedBytesMap;

    void setWriteError() {
      if (this->writeError.getType() != WriteError::Type::Write) {
        WriteError writeError(WriteError::Type::Write);
        this->writeError = writeError;
      }
    }

  public:
    NodeWriteResult() {};

    WriteError getError() const { return writeError; };

    void setError(const WriteError& error) {
      this->writeError = error;
    }

    // Puts specified configuration byte, which failed to write.
    void putFailedByte(const HWP_ConfigByte& failedByte) {
      failedBytesMap[failedByte.address] = failedByte;
      setWriteError();
    }

    // Puts specified configuration bytes, which failed to write.
    void putFailedBytes(const std::vector<HWP_ConfigByte>& failedBytes) {
      for (const HWP_ConfigByte failedByte : failedBytes) {
        failedBytesMap[failedByte.address] = failedByte;
      }
      setWriteError();
    }

    // Returns map of failed bytes indexed by their addresses
    std::map<uint8_t, HWP_ConfigByte> getFailedBytesMap() const { return failedBytesMap; };
  };


  // holds information about configuration writing result for each node
  class WriteResult {
  private:
    // map of write results on nodes indexed by node address
    std::map<uint16_t, NodeWriteResult> resultsMap;

    // transaction results
    std::list<std::unique_ptr<IDpaTransactionResult2>> m_transResults;

  public:
    // Puts specified write result for specified node into results.
    void putResult(uint16_t nodeAddr, const NodeWriteResult& result) {
      resultsMap[nodeAddr] = result;
    };

    // returns map of write results on nodes indexed by node address
    std::map<uint16_t, NodeWriteResult> getResultsMap() const { return resultsMap; };
    
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
  class WriteConfigService::Imp {
  private:
    // parent object
    WriteConfigService& m_parent;

    // message type: network management write configuration
    // for temporal reasons
    const std::string m_mTypeName_mngIqmeshWriteConfig = "mngIqmeshWriteTrConf";
    //IMessagingSplitterService::MsgType* m_msgType_mngIqmeshWriteConfig;

    //iqrf::IJsCacheService* m_iJsCacheService = nullptr;
    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    IIqrfDpaService* m_iIqrfDpaService = nullptr;

    // number of repeats
    uint8_t m_repeat = 0;

    // Coordinator's RF channel band
    RF_ChannelBand m_coordRfChannelBand = RF_ChannelBand::UNSPECIFIED;

    // security password
    std::basic_string<uint8_t> m_securityPassword;

    // security user key
    std::basic_string<uint8_t> m_securityUserKey;


    // indicators, whether upper mentioned fields was specified in request json document
    bool m_isSetRepeat = false;
    bool m_isSetCoordRfChannelBand = false;
    bool m_isSetSecurityPassword = false;
    bool m_isSetSecurityUserKey = false;

    // if is set Verbose mode
    bool m_returnVerbose = false;


  public:
    Imp(WriteConfigService& parent) : m_parent(parent)
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

    void checkConfigBytes(const std::vector<HWP_ConfigByte>& configBytes)
    {
      // initialized with zeroes
      std::bitset<CONFIG_BYTES_LEN> configBytesUsed;

      for (const HWP_ConfigByte configByte : configBytes) {
        if (
          (configByte.address < CONFIG_BYTES_START_ADDR)
          || (configByte.address > CONFIG_BYTES_END_ADDR)
          ) {
          THROW_EXC(
            std::out_of_range, "Address of config byte out of valid range: " << NAME_PAR_HEX("Address", configByte.address)
          );
        }

        if (configBytesUsed.test(configByte.address - 1)) {
          THROW_EXC(std::out_of_range, "Config byte with the same address already exist.");
        }

        configBytesUsed[configByte.address - 1] = 1;
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

    void processSecurityError(
      WriteResult& writeResult,
      const std::list<uint16_t>& targetNodes,
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
        }
        else {
          NodeWriteResult nodeWriteResult;
          nodeWriteResult.setError(writeError);
          writeResult.putResult(targetNode, nodeWriteResult);
        }
      }
    }

    // counts checksum for specified configuration bytes
    // RFPGM byte IS NOT included in checksum count
    uint8_t countChecksum(const std::vector<HWP_ConfigByte>& configBytes) {
      uint8_t checksum = 0x5F;

      for (const HWP_ConfigByte configByte : configBytes) {
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

      THROW_EXC(std::logic_error, "RFPGM byte NOT found.");
    }


    // writes configuration bytes into specified node using
    // standard Write HWP Configuration DPA request
    void writeHwpConfiguration(
      WriteResult& writeResult,
      const std::vector<HWP_ConfigByte>& configBytes,
      const uint16_t nodeAddr
    )
    {
      TRC_FUNCTION_ENTER("");

      DpaMessage writeConfigRequest;
      DpaMessage::DpaPacket_t writeConfigPacket;
      writeConfigPacket.DpaRequestPacket_t.NADR = nodeAddr;
      writeConfigPacket.DpaRequestPacket_t.PNUM = PNUM_OS;
      writeConfigPacket.DpaRequestPacket_t.PCMD = CMD_OS_WRITE_CFG;
      writeConfigPacket.DpaRequestPacket_t.HWPID = HWPID_Default;

      TPerOSWriteCfg_Request* tOsWriteCfgRequest = &writeConfigRequest.DpaPacket().DpaRequestPacket_t.DpaMessage.PerOSWriteCfg_Request;

      tOsWriteCfgRequest->Checksum = countChecksum(configBytes);
      setWriteRequestConfigurationField(tOsWriteCfgRequest->Configuration, configBytes);
      tOsWriteCfgRequest->RFPGM = getRfpgmByte(configBytes);

      writeConfigRequest.DataToBuffer(
        writeConfigPacket.Buffer, 
        sizeof(TDpaIFaceHeader) + 1 + 31 + 1
      );

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> writeConfigTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      try {
        writeConfigTransaction = m_iIqrfDpaService->executeDpaTransaction(writeConfigRequest);
        transResult = writeConfigTransaction->get();
      }
      catch (std::exception& e) {
        TRC_DEBUG("DPA transaction error : " << e.what());
        THROW_EXC(std::logic_error, "Could not write configuration.");
      }

      TRC_DEBUG("Result from write config transaction as string:" << PAR(transResult->getErrorString()));

      IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

      writeResult.addTransactionResult(transResult);

      if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
        TRC_INFORMATION("Write config successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(writeConfigRequest.PeripheralType(), writeConfigRequest.NodeAddress())
          << PAR(writeConfigRequest.PeripheralCommand())
        );

        TRC_FUNCTION_LEAVE("");
        return;
      }

      // transaction error
      if (errorCode < 0) {
        TRC_DEBUG("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));
        processWriteError(writeResult, nodeAddr, configBytes, WriteError::Type::Write, "Transaction error.");
      } // DPA error
      else {
        TRC_DEBUG("DPA error. " << NAME_PAR_HEX("Error code", errorCode));
        processWriteError(writeResult, nodeAddr, configBytes, WriteError::Type::Write, "DPA error.");
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
    std::vector<HWP_ConfigByte> getConfigBytesPart(
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

    // puts nodes results of write config into overall write config results
    void putWriteConfigFrcResults(
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

    // puts nodes results of set security into overall write config results
    void putSetSecurityFrcResults(
      WriteResult& writeResult,
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
        }
        else {
          NodeWriteResult nodeWriteResult;
          nodeWriteResult.setError(writeError);
          writeResult.putResult(p.first, nodeWriteResult);
        }
      }
    }

    // writes configuration bytes into target nodes
    void _writeConfigBytes(
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
        std::vector<HWP_ConfigByte> configBytesPart = getConfigBytesPart(partId, partsTotal, configBytes);
        setUserDataForFrcWriteConfigByteRequest(frcRequest, configBytesPart);

        // issue the DPA request
        frcRequest.DataToBuffer(
          frcPacket.Buffer, 
          sizeof(TDpaIFaceHeader) + 1 + 30 + 5 + configBytesPart.size()
        );

        // issue the DPA request
        std::shared_ptr<IDpaTransaction2> frcWriteConfigTransaction;
        std::unique_ptr<IDpaTransactionResult2> transResult;

        try {
          frcWriteConfigTransaction = m_iIqrfDpaService->executeDpaTransaction(frcRequest);
          transResult = frcWriteConfigTransaction->get();
        }
        catch (std::exception& e) {
          TRC_DEBUG("DPA transaction error : " << e.what());
          processWriteError(writeResult, targetNodes, configBytesPart, WriteError::Type::Write, e.what());
          continue;
        }

        TRC_DEBUG("Result from FRC write config transaction as string:" << PAR(transResult->getErrorString()));

        // data from FRC
        std::basic_string<uns8> frcData;

        IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();
        
        // because of the move-semantics
        DpaMessage dpaResponse = transResult->getResponse();
        writeResult.addTransactionResult(transResult);

        if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
          TRC_INFORMATION("FRC write config successful!");
          TRC_DEBUG(
            "DPA transaction: "
            << NAME_PAR(frcRequest.PeripheralType(), frcRequest.NodeAddress())
            << PAR(frcRequest.PeripheralCommand())
          );

          // check status
          uns8 status = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.Status;
          if ((status >= 0x00) && (status <= 0xEF)) {
            TRC_INFORMATION("FRC write config status OK.");
            frcData.append(dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData);
          }
          else {
            TRC_DEBUG("FRC write config status NOT ok." << NAME_PAR_HEX("Status", status));
            processWriteError(writeResult, targetNodes, configBytesPart, WriteError::Type::Write, "Bad status.");
            continue;
          }
        }

        // transaction error
        if (errorCode < 0) {
          TRC_DEBUG("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));
          processWriteError(writeResult, targetNodes, configBytesPart, WriteError::Type::Write, "Transaction error.");
          continue;
        } // DPA error
        else {
          TRC_DEBUG("DPA error. " << NAME_PAR_HEX("Error code", errorCode));
          processWriteError(writeResult, targetNodes, configBytesPart, WriteError::Type::Write, "DPA error.");
          continue;
        }

        // get extra results
        DpaMessage extraResultRequest;
        DpaMessage::DpaPacket_t extraResultPacket;
        frcPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        frcPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
        frcPacket.DpaRequestPacket_t.PCMD = CMD_FRC_EXTRARESULT;
        frcPacket.DpaRequestPacket_t.HWPID = HWPID_Default;
        frcRequest.DataToBuffer(extraResultPacket.Buffer, sizeof(TDpaIFaceHeader));

        // issue the DPA request
        std::shared_ptr<IDpaTransaction2> extraResultTransaction;

        try {
          extraResultTransaction = m_iIqrfDpaService->executeDpaTransaction(extraResultRequest);
          transResult = extraResultTransaction->get();
        }
        catch (std::exception& e) {
          TRC_DEBUG("DPA transaction error : " << e.what());
          processWriteError(writeResult, targetNodes, configBytesPart, WriteError::Type::Write, e.what());
          continue;
        }

        TRC_DEBUG("Result from FRC write config extra result transaction as string:" << PAR(transResult->getErrorString()));

        errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();
        
        // because of the move-semantics
        dpaResponse = transResult->getResponse();
        writeResult.addTransactionResult(transResult);

        if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
          TRC_INFORMATION("FRC write config extra result successful!");
          TRC_DEBUG(
            "DPA transaction: "
            << NAME_PAR(extraResultRequest.PeripheralType(), extraResultRequest.NodeAddress())
            << PAR(extraResultRequest.PeripheralCommand())
          );

          // check status
          uns8 status = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.Status;
          if ((status >= 0x00) && (status <= 0xEF)) {
            TRC_INFORMATION("FRC write config extra result status OK.");
            frcData.append(dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData);
          }
          else {
            TRC_DEBUG("FRC write config extra result status NOT ok." << NAME_PAR_HEX("Status", status));
            processWriteError(writeResult, targetNodes, configBytesPart, WriteError::Type::Write, "Bad status.");
            continue;
          }
        }

        // transaction error
        if (errorCode < 0) {
          TRC_DEBUG("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));
          processWriteError(writeResult, targetNodes, configBytesPart, WriteError::Type::Write, "Transaction error.");
          continue;
        } // DPA error
        else {
          TRC_DEBUG("DPA error. " << NAME_PAR_HEX("Error code", errorCode));
          processWriteError(writeResult, targetNodes, configBytesPart, WriteError::Type::Write, "DPA error.");
          continue;
        }

        // FRC data parsing
        std::map<uint16_t, uint8_t> nodesResultsMap = parse2bitsFrcData(frcData);

        // putting nodes results into overall write config results
        putWriteConfigFrcResults(writeResult, configBytesPart, WriteError::Type::Write, nodesResultsMap);
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
    std::list<uint16_t> getBondedNodes(WriteResult& writeResult)
    {
      TRC_FUNCTION_ENTER("");

      std::list<uint16_t> bondedNodes;

      DpaMessage bondedNodesRequest;
      DpaMessage::DpaPacket_t bondedNodesPacket;
      bondedNodesPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      bondedNodesPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
      bondedNodesPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_BONDED_DEVICES;
      bondedNodesPacket.DpaRequestPacket_t.HWPID = HWPID_Default;
      bondedNodesRequest.DataToBuffer(bondedNodesPacket.Buffer, sizeof(TDpaIFaceHeader));

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> getBondedNodesTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      try {
        getBondedNodesTransaction = m_iIqrfDpaService->executeDpaTransaction(bondedNodesRequest);
        transResult = getBondedNodesTransaction->get();
      }
      catch (std::exception& e) {
        TRC_DEBUG("DPA transaction error : " << e.what());
        THROW_EXC(std::logic_error, "Could not get bonded nodes.");
      }

      TRC_DEBUG("Result from get bonded nodes transaction as string:" << PAR(transResult->getErrorString()));

      IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

      // because of the move-semantics
      DpaMessage dpaResponse = transResult->getResponse();
      writeResult.addTransactionResult(transResult);

      if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
        TRC_INFORMATION("Get bonded nodes successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(bondedNodesRequest.PeripheralType(), bondedNodesRequest.NodeAddress())
          << PAR(bondedNodesRequest.PeripheralCommand())
        );

        // parsing response data
        uns8* bondedNodesArr = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData;
        parseBondedNodes(bondedNodesArr, bondedNodes);
        
        TRC_FUNCTION_LEAVE("");
        return bondedNodes;
      }

      // transaction error
      if (errorCode < 0) {
        TRC_DEBUG("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));
      } // DPA error
      else {
        TRC_DEBUG("DPA error. " << NAME_PAR_HEX("Error code", errorCode));
      }

      THROW_EXC(std::logic_error, "Could not get bonded nodes.");
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
        }
        else {
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

    // for usage in the next function
    //RF_ChannelBand parseAndCheckRfChannelBand(const uint8_t rfBandInt);

    // updates coordinator's RF channel band
    void updateCoordRfChannelBand(WriteResult& writeResult) {
      TRC_FUNCTION_ENTER("");

      DpaMessage readConfigRequest;
      DpaMessage::DpaPacket_t readConfigPacket;
      readConfigPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      readConfigPacket.DpaRequestPacket_t.PNUM = PNUM_OS;
      readConfigPacket.DpaRequestPacket_t.PCMD = CMD_OS_READ_CFG;
      readConfigPacket.DpaRequestPacket_t.HWPID = HWPID_Default;
      readConfigRequest.DataToBuffer(readConfigPacket.Buffer, sizeof(TDpaIFaceHeader));

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> readConfigTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      try {
        readConfigTransaction = m_iIqrfDpaService->executeDpaTransaction(readConfigRequest);
        transResult = readConfigTransaction->get();
      }
      catch (std::exception& e) {
        TRC_DEBUG("DPA transaction error : " << e.what());
        THROW_EXC(std::logic_error, "Could not read HWP configuration.");
      }

      TRC_DEBUG("Result from read HWP configuration transaction as string:" << PAR(transResult->getErrorString()));

      IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

      // because of the move-semantics
      DpaMessage dpaResponse = transResult->getResponse();
      writeResult.addTransactionResult(transResult);

      if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
        TRC_INFORMATION("Read HWP configuration successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(readConfigRequest.PeripheralType(), readConfigRequest.NodeAddress())
          << PAR(readConfigRequest.PeripheralCommand())
        );

        // updating RF Channel band
        uns8 rfChannelBandInt = 
          dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData[CONFIG_BYTES_COORD_RF_CHANNEL_BAND_ADDR];
        m_coordRfChannelBand = parseAndCheckRfChannelBand(rfChannelBandInt);

        TRC_FUNCTION_LEAVE("");
        return;
      }

      // transaction error
      if (errorCode < 0) {
        TRC_DEBUG("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));
      } // DPA error
      else {
        TRC_DEBUG("DPA error. " << NAME_PAR_HEX("Error code", errorCode));
      }

      THROW_EXC(std::logic_error, "Could not read HWP configuration.");
    }

    // indicates, whether specified RF channel is in specified channel band
    bool isInBand(uint8_t rfChannel, RF_ChannelBand band) {
      bool result = false;

      switch (band) {
      case RF_ChannelBand::BAND_868:
        if (rfChannel >= 0 && rfChannel <= 67) {
          result = true;
        }
        break;

      case RF_ChannelBand::BAND_916:
        if (rfChannel >= 0 && rfChannel <= 255) {
          result = true;
        }
        break;

      case RF_ChannelBand::BAND_433:
        if (rfChannel >= 0 && rfChannel <= 16) {
          result = true;
        }
        break;

      default:
        THROW_EXC(std::out_of_range, "Unsupported RF band. " << NAME_PAR_HEX("Band", (int)band));
      }

      return result;
    }

    // checks, if RF channel config bytes are in accordance with coordinator RF channel band
    void checkRfChannelIfPresent(
      const std::vector<HWP_ConfigByte>& configBytes, 
      WriteResult& writeResult
    )
    {
      TRC_FUNCTION_ENTER("");

      bool isRfBandActual = m_isSetCoordRfChannelBand;

      for (const HWP_ConfigByte configByte : configBytes) {
        switch (configByte.address) {
          case CONFIG_BYTES_MAIN_RF_CHANNEL_A_ADDR:
          case CONFIG_BYTES_MAIN_RF_CHANNEL_B_ADDR:
          case CONFIG_BYTES_SUBORD_RF_CHANNEL_A_ADDR:
          case CONFIG_BYTES_SUBORD_RF_CHANNEL_B_ADDR:
            if (!isRfBandActual) {
              updateCoordRfChannelBand(writeResult);
              isRfBandActual = true;
            }

            if (!isInBand(configByte.value, m_coordRfChannelBand)) {
              THROW_EXC(
                std::out_of_range, NAME_PAR_HEX("RF channel", configByte.value) << " not in band: " << PAR((int)m_coordRfChannelBand)
              );
            }
          default:
            break;
        }
      }

      TRC_FUNCTION_LEAVE("");
    }
    
    // sets User data part for "Set security" DPA request
    // securityString can be either password or user key
    void setUserDataForSetSecurityFrcRequest(
      DpaMessage& frcRequest, 
      const std::basic_string<uint8_t>& securityString
    )
    {
      uns8* userData = frcRequest.DpaPacket().DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData;

      // length
      userData[0] = 1 + 1 + 1 + 2 + securityString.length();
      
      userData[1] = PNUM_OS;
      userData[2] = CMD_OS_SET_SECURITY;
      userData[3] = HWPID_Default & 0xFF;
      userData[4] = (HWPID_Default >> 8) & 0xFF;

      std::copy(securityString.begin(), securityString.end(), userData + 5);
    }

    // sets security
    void setSecurityString(
      WriteResult& writeResult, 
      const std::list<uint16_t>& targetNodes,
      const std::basic_string<uint8_t>& securityString,
      const bool isPassword
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
      
      setSelectedNodesForFrcRequest(frcRequest, targetNodes);

      uint8_t writeConfigPacketFreeSpace = FRC_MAX_USER_DATA_LEN
        - 1
        - sizeof(frcPacket.DpaRequestPacket_t.PCMD)
        - sizeof(frcPacket.DpaRequestPacket_t.PNUM)
        - sizeof(frcPacket.DpaRequestPacket_t.HWPID)
        ;

      // set security string as a data for FRC request
      setUserDataForSetSecurityFrcRequest(frcRequest, securityString);

      // issue the DPA request
      frcRequest.DataToBuffer(
        frcPacket.Buffer, 
        sizeof(TDpaIFaceHeader) + 1 + 30 + 5 + securityString.size()
      );

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> frcWriteConfigTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      // type of error
      WriteError::Type errorType = (isPassword) ? WriteError::Type::SecurityPassword : WriteError::Type::SecurityUserKey;

      try {
        frcWriteConfigTransaction = m_iIqrfDpaService->executeDpaTransaction(frcRequest);
        transResult = frcWriteConfigTransaction->get();
      }
      catch (std::exception& e) {
        TRC_DEBUG("DPA transaction error : " << e.what());
        processSecurityError(writeResult, targetNodes, errorType, e.what());
        return;
      }

      TRC_DEBUG("Result from FRC set security transaction as string:" << PAR(transResult->getErrorString()));

      // data from FRC
      std::basic_string<uns8> frcData;

      IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

      // because of the move-semantics
      DpaMessage dpaResponse = transResult->getResponse();
      writeResult.addTransactionResult(transResult);

      if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
        TRC_INFORMATION("FRC set security successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(frcRequest.PeripheralType(), frcRequest.NodeAddress())
          << PAR(frcRequest.PeripheralCommand())
        );

        // check status
        uns8 status = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.Status;
        if ((status >= 0x00) && (status <= 0xEF)) {
          TRC_INFORMATION("FRC write config status OK.");
          frcData.append(dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData);
        }
        else {
          TRC_DEBUG("FRC write config status NOT ok." << NAME_PAR_HEX("Status", status));
          processSecurityError(writeResult, targetNodes, errorType, "Bad status.");
          return;
        }
      }

      // transaction error
      if (errorCode < 0) {
        TRC_DEBUG("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));
        processSecurityError(writeResult, targetNodes, errorType, "Transaction error.");
        return;
      } // DPA error
      else {
        TRC_DEBUG("DPA error. " << NAME_PAR_HEX("Error code", errorCode));
        processSecurityError(writeResult, targetNodes, errorType, "DPA error.");
        return;
      }

      // get extra results
      DpaMessage extraResultRequest;
      DpaMessage::DpaPacket_t extraResultPacket;
      frcPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      frcPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
      frcPacket.DpaRequestPacket_t.PCMD = CMD_FRC_EXTRARESULT;
      frcPacket.DpaRequestPacket_t.HWPID = HWPID_Default;
      frcRequest.DataToBuffer(extraResultPacket.Buffer, sizeof(TDpaIFaceHeader));

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> extraResultTransaction;

      try {
        extraResultTransaction = m_iIqrfDpaService->executeDpaTransaction(extraResultRequest);
        transResult = extraResultTransaction->get();
      }
      catch (std::exception& e) {
        TRC_DEBUG("DPA transaction error : " << e.what());
        processSecurityError(writeResult, targetNodes, errorType, e.what());
        return;
      }

      TRC_DEBUG("Result from FRC write config extra result transaction as string:" << PAR(transResult->getErrorString()));

      errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

      // because of the move-semantics
      dpaResponse = transResult->getResponse();
      writeResult.addTransactionResult(transResult);

      if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
        TRC_INFORMATION("FRC write config extra result successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(extraResultRequest.PeripheralType(), extraResultRequest.NodeAddress())
          << PAR(extraResultRequest.PeripheralCommand())
        );

        // check status
        uns8 status = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.Status;
        if ((status >= 0x00) && (status <= 0xEF)) {
          TRC_INFORMATION("FRC write config extra result status OK.");
          frcData.append(dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData);
        }
        else {
          TRC_DEBUG("FRC write config extra result status NOT ok." << NAME_PAR_HEX("Status", status));
          processSecurityError(writeResult, targetNodes, errorType, "Bad status.");
          return;
        }
      }

      // transaction error
      if (errorCode < 0) {
        TRC_DEBUG("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));
        processSecurityError(writeResult, targetNodes, errorType, "Transaction error.");
        return;
      } // DPA error
      else {
        TRC_DEBUG("DPA error. " << NAME_PAR_HEX("Error code", errorCode));
        processSecurityError(writeResult, targetNodes, errorType, "DPA error.");
        return;
      }

      // FRC data parsing
      std::map<uint16_t, uint8_t> nodesResultsMap = parse2bitsFrcData(frcData);

      // putting nodes results into overall write config results
      putSetSecurityFrcResults(writeResult, errorType, nodesResultsMap);
      

      TRC_FUNCTION_LEAVE("");
    }
    
    WriteResult writeConfigBytes(
      const std::vector<HWP_ConfigByte>& configBytes,
      const std::list<uint16_t>& targetNodes
    )
    {
      // result of writing configuration
      WriteResult writeResult;

      // getting list of all bonded nodes
      std::list<uint16_t> bondedNodes = getBondedNodes(writeResult);

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
      checkRfChannelIfPresent(configBytes, writeResult);

      // if there is only one node and it is needed to write ALL configuration bytes
      // use traditional Write HWP Configuration
      if ((targetBondedNodes.size() == 1) && (configBytes.size() == CONFIG_BYTES_LEN)) {
        writeHwpConfiguration(writeResult, configBytes, targetBondedNodes.front());
      }
      else {
        _writeConfigBytes(writeResult, configBytes, targetBondedNodes);
      }

      // if there is specified security password and/or security user key
      // issue DPA set security
      // TODO: Will be specified more precisely and activated later
      /*
      if (m_isSetSecurityPassword) {
        setSecurityString(writeResult, targetBondedNodes, m_securityPassword, true);
      }

      if (m_isSetSecurityUserKey) {
        setSecurityString(writeResult, targetBondedNodes, m_securityUserKey, false);
      }
      */
      return writeResult;
    }

    
    /* Parsing and checking of parameters obtained from request json document */

    RF_ChannelBand parseAndCheckRfChannelBand(const std::string& rfBandStr) {
      if (rfBandStr.empty()) {
        return RF_ChannelBand::UNSPECIFIED;
      }

      if (rfBandStr == "433") {
        return RF_ChannelBand::BAND_433;
      }
      
      if (rfBandStr == "868") {
        return RF_ChannelBand::BAND_868;
      }
      
      if (rfBandStr == "916") {
        return RF_ChannelBand::BAND_916;
      }

      THROW_EXC(std::out_of_range, "Unsupported coordinator RF band: " << PAR(rfBandStr));
    }
    
    RF_ChannelBand parseAndCheckRfChannelBand(const uint8_t rfBandInt) {
      switch (rfBandInt) {
        case 0b00:
          return RF_ChannelBand::BAND_868;
        case 0b01:
          return RF_ChannelBand::BAND_916;
        case 0b10:
          return RF_ChannelBand::BAND_433;
        default:
          THROW_EXC(std::out_of_range, "Unsupported coordinator RF band: " << PAR(rfBandInt));
      }
    }

    std::basic_string<uint8_t> createDefaultSecurityPassword() {
      std::basic_string<uint8_t> securityPassword;

      for (int i = 0; i < SECURITY_PASSWORD_MAX_LEN; i++) {
        securityPassword.push_back(0);
      }
      return securityPassword;
    }

    std::basic_string<uint8_t> createDefaultSecurityUserKey() {
      std::basic_string<uint8_t> securityUserKey;

      for (int i = 0; i < SECURITY_USER_KEY_MAX_LEN; i++) {
        securityUserKey.push_back(0);
      }
      return securityUserKey;
    }

    std::basic_string<uint8_t> parseAndCheckSecurityPassword(const std::string& securityPasswordStr) {
      std::basic_string<uint8_t> securityPassword;
      
      // default password used
      if (securityPasswordStr.empty()) {
        return createDefaultSecurityPassword();
      }

      // invalid length
      if (securityPasswordStr.length() > SECURITY_PASSWORD_MAX_LEN) {
        THROW_EXC(std::out_of_range, "Invalid security password length: " << PAR(securityPasswordStr.length()));
      }

      for (int i = 0; i < securityPasswordStr.length(); i++) {
        securityPassword.push_back(securityPasswordStr[i]);
      }

      return securityPassword;
    }

    std::basic_string<uint8_t> parseAndCheckSecurityUserKey(const std::string& userKeyStr) {
      std::basic_string<uint8_t> securityUserKey;
      
      // default user key used
      if (userKeyStr.empty()) {
        return createDefaultSecurityUserKey();
      }

      // invalid length
      if (userKeyStr.length() > SECURITY_USER_KEY_MAX_LEN) {
        THROW_EXC(std::out_of_range, "Invalid security user key length: " << PAR(userKeyStr.length()));
      }

      for (int i = 0; i < userKeyStr.length(); i++) {
        securityUserKey.push_back(userKeyStr[i]);
      }

      return securityUserKey;
    }

    uint8_t parseAndCheckRepeat(const int repeat) {
      if (repeat < 0) {
        TRC_WARNING("Repeat parameter cannot be less than 0. It will be set to 0.");
        return 0;
      }

      if (repeat > 0xFF) {
        TRC_WARNING("Repeat parameter exceeds maximum. It will be trimmed to maximum of: " << PAR(REPEAT_MAX));
        return REPEAT_MAX;
      }

      return repeat;
    }

    std::list<uint16_t> parseAndCheckDeviceAddr(const std::vector<int> deviceAddr) {
      if (deviceAddr.empty()) {
        THROW_EXC(std::out_of_range, "No device addresses.");
      }

      std::list<uint16_t> checkedAddrs;

      for (int devAddr : deviceAddr) {
        if ((devAddr < 0) || (devAddr > 0xEF)) {
          THROW_EXC(
            std::out_of_range, "Device address outside of valid range. " << NAME_PAR_HEX("Address", devAddr)
          );
        }
        checkedAddrs.push_back(devAddr);
      }
      return checkedAddrs;
    }


    uint8_t checkRfChannel(const int rfChannel) {
      if ((rfChannel < 0) || (rfChannel > 255)) {
        THROW_EXC(std::out_of_range, "RF channel out of valid bounds. Value: " << PAR(rfChannel));
      }
      return rfChannel;
    }

    uint8_t checkTxPower(const int txPower) {
      if ((txPower < 0) || (txPower > 7)) {
        THROW_EXC(std::out_of_range, "Tx power out of valid bounds. Value: " << PAR(txPower));
      }
      return txPower;
    }

    uint8_t checkRxFilter(const int rxFilter) {
      if ((rxFilter < 0) || (rxFilter > 64)) {
        THROW_EXC(std::out_of_range, "Rx filter out of valid bounds. Value: " << PAR(rxFilter));
      }
      return rxFilter;
    }

    uint8_t checkLpRxTimeout(const int lpRxTimeout) {
      if ((lpRxTimeout < 1) || (lpRxTimeout > 255)) {
        THROW_EXC(std::out_of_range, "LP Rx timeout out of valid bounds. Value: " << PAR(lpRxTimeout));
      }
      return lpRxTimeout;
    }

    uint8_t checkRfPgmAltChannel(const int rfPgmAltChannel) {
      if ((rfPgmAltChannel < 1) || (rfPgmAltChannel > 255)) {
        THROW_EXC(std::out_of_range, "Alternative DPA service channel out of valid bounds. Value: " << PAR(rfPgmAltChannel));
      }
      return rfPgmAltChannel;
    }

    uint8_t checkUartBaudrate(const int uartBaudrate) {
      if ((uartBaudrate < 0) || (uartBaudrate > 8)) {
        THROW_EXC(std::out_of_range, "UART baud rate out of valid bounds. Value: " << PAR(uartBaudrate));
      }
      return uartBaudrate;
    }

    std::vector<HWP_ConfigByte> parseAndCheckConfigBytes(ComMngIqmeshWriteConfig comWriteConfig) {
      std::vector<HWP_ConfigByte> configBytes;

      if (comWriteConfig.isSetRfChannelA()) {
        uint8_t rfChannelA = checkRfChannel(comWriteConfig.getRfChannelA());
        HWP_ConfigByte rfChannelA_configByte(0x11, rfChannelA, 0xFF);
        configBytes.push_back(rfChannelA_configByte);
      }

      if (comWriteConfig.isSetRfChannelB()) {
        uint8_t rfChannelB = checkRfChannel(comWriteConfig.getRfChannelB());
        HWP_ConfigByte rfChannelB_configByte(0x11, rfChannelB, 0xFF);
        configBytes.push_back(rfChannelB_configByte);
      }

      if (comWriteConfig.isSetRfSubChannelA()) {
        uint8_t rfSubChannelA = checkRfChannel(comWriteConfig.getRfSubChannelA());
        HWP_ConfigByte rfSubChannelA_configByte(0x06, rfSubChannelA, 0xFF);
        configBytes.push_back(rfSubChannelA_configByte);
      }

      if (comWriteConfig.isSetRfSubChannelB()) {
        uint8_t rfSubChannelB = checkRfChannel(comWriteConfig.getRfSubChannelB());
        HWP_ConfigByte rfSubChannelB_configByte(0x12, rfSubChannelB, 0xFF);
        configBytes.push_back(rfSubChannelB_configByte);
      }

      if (comWriteConfig.isSetTxPower()) {
        uint8_t txPower = checkTxPower(comWriteConfig.getTxPower());
        HWP_ConfigByte txPower_configByte(0x08, txPower, 0xFF);
        configBytes.push_back(txPower_configByte);
      }

      if (comWriteConfig.isSetRxFilter()) {
        uint8_t rxFilter = checkRxFilter(comWriteConfig.getRxFilter());
        HWP_ConfigByte rxFilter_configByte(0x09, rxFilter, 0xFF);
        configBytes.push_back(rxFilter_configByte);
      }

      if (comWriteConfig.isSetLpRxTimeout()) {
        uint8_t lpRxTimeout = checkLpRxTimeout(comWriteConfig.getLpRxTimeout());
        HWP_ConfigByte lpRxTimeout_configByte(0x09, lpRxTimeout, 0xFF);
        configBytes.push_back(lpRxTimeout_configByte);
      }

      if (comWriteConfig.isSetRfPgmAltChannel()) {
        uint8_t rfPgmAltChannel = checkRfPgmAltChannel(comWriteConfig.getRfPgmAltChannel());
        HWP_ConfigByte rfPgmAltChannel_configByte(0x09, rfPgmAltChannel, 0xFF);
        configBytes.push_back(rfPgmAltChannel_configByte);
      }

      if (comWriteConfig.isSetUartBaudrate()) {
        uint8_t uartBaudrate = checkUartBaudrate(comWriteConfig.getUartBaudrate());
        HWP_ConfigByte uartBaudrate_configByte(0x09, uartBaudrate, 0xFF);
        configBytes.push_back(uartBaudrate_configByte);
      }


      // byte 0x06 - configuration bits
      uint8_t byte06ConfigBits = 0;
      bool isSetByte06ConfigBits = false;

      if (comWriteConfig.isSetCustomDpaHandler()) {
        if (comWriteConfig.getCustomDpaHandler()) {
          byte06ConfigBits |= 0b00000001;
        }
        isSetByte06ConfigBits = true;
      }

      if (comWriteConfig.isSetNodeDpaInterface()) {
        if (comWriteConfig.getNodeDpaInterface()) {
          byte06ConfigBits |= 0b00000010;
        }
        isSetByte06ConfigBits = true;
      }

      if (comWriteConfig.isSetDpaAutoexec()) {
        if (comWriteConfig.getDpaAutoexec()) {
          byte06ConfigBits |= 0b00000100;
        }
        isSetByte06ConfigBits = true;
      }

      if (comWriteConfig.isSetRoutingOff()) {
        if (comWriteConfig.getRoutingOff()) {
          byte06ConfigBits |= 0b00001000;
        }
        isSetByte06ConfigBits = true;
      }

      if (comWriteConfig.isSetIoSetup()) {
        if (comWriteConfig.getIoSetup()) {
          byte06ConfigBits |= 0b00010000;
        }
        isSetByte06ConfigBits = true;
      }

      if (comWriteConfig.isSetPeerToPeer()) {
        if (comWriteConfig.getPeerToPeer()) {
          byte06ConfigBits |= 0b00100000;
        }
        isSetByte06ConfigBits = true;
      }

      // if there is at minimal one bit set, add byte06
      if (isSetByte06ConfigBits) {
        HWP_ConfigByte byte06(0x06, byte06ConfigBits, 0xFF);
        configBytes.push_back(byte06);
      }
     

      // RFPGM configuration bits
      uint8_t rfpgmConfigBits = 0;
      bool isSetRfpgmConfigBits = false;

      if (comWriteConfig.isSetRfPgmDualChannel()) {
        if (comWriteConfig.getRfPgmDualChannel()) {
          byte06ConfigBits |= 0b00000011;
        }
        isSetRfpgmConfigBits = true;
      }

      if (comWriteConfig.isSetRfPgmLpMode()) {
        if (comWriteConfig.getRfPgmLpMode()) {
          byte06ConfigBits |= 0b00000100;
        }
        isSetRfpgmConfigBits = true;
      }

      if (comWriteConfig.isSetRfPgmEnableAfterReset()) {
        if (comWriteConfig.getRfPgmEnableAfterReset()) {
          byte06ConfigBits |= 0b00010000;
        }
        isSetRfpgmConfigBits = true;
      }
      
      if (comWriteConfig.isSetRfPgmTerminateAfter1Min()) {
        if (comWriteConfig.getRfPgmTerminateAfter1Min()) {
          byte06ConfigBits |= 0b01000000;
        }
        isSetRfpgmConfigBits = true;
      }

      if (comWriteConfig.isSetRfPgmTerminateMcuPin()) {
        if (comWriteConfig.getRfPgmTerminateMcuPin()) {
          byte06ConfigBits |= 0b10000000;
        }
        isSetRfpgmConfigBits = true;
      }

      // if there is at minimal one bit set, add RFPGM byte
      if (isSetRfpgmConfigBits) {
        HWP_ConfigByte rfpgmByte(0x20, rfpgmConfigBits, 0xFF);
        configBytes.push_back(rfpgmByte);
      }

      return configBytes;
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

    // creates response on the basis of write result
    Document createResponse(
      const std::string& messagingId,
      const IMessagingSplitterService::MsgType& msgType,
      WriteResult& writeResult,
      ComMngIqmeshWriteConfig& comWriteConfig,
      bool restartNeeded
    )
    {
      Document response;

      std::map<uint16_t, NodeWriteResult> nodeResultsMap = writeResult.getResultsMap();

      // only one node - for the present time
      std::map<uint16_t, NodeWriteResult>::iterator iter = nodeResultsMap.begin();

      Pointer("/data/rsp/deviceAddr").Set(response, iter->first);
      if (iter->second.getError().getType() == WriteError::Type::NoError) {
        Pointer("/data/rsp/writeSuccess").Set(response, true);
      }
      else {
        Pointer("/data/rsp/writeSuccess").Set(response, false);
      }

      if (restartNeeded) {
        Pointer("/data/rsp/restartNeeded").Set(response, true);
      }
      else {
        Pointer("/data/rsp/restartNeeded").Set(response, false);
      }
      
      // set common parameters
      Pointer("/mType").Set(response, msgType.m_type);
      Pointer("/data/msgId").Set(response, messagingId);      

      // set raw fields, if verbose mode is active
      if (comWriteConfig.getVerbose()) {
        rapidjson::Value rawArray(kArrayType);
        Document::AllocatorType& allocator = response.GetAllocator();

        while (writeResult.isNextTransactionResult()) {
          std::unique_ptr<IDpaTransactionResult2> transResult = writeResult.consumeNextTransactionResult();
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

    // indicates, whether restart is needed 
    bool isRestartNeeded(const std::vector<HWP_ConfigByte>& configBytes) {
      for (HWP_ConfigByte configByte : configBytes) {
        if (
          configByte.address == 0x05
          || configByte.address == 0x09
          || configByte.address == 0x0A
          || configByte.address == 0x0B
        ) {
          return true;
        }
      }
      return false;
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
      if (msgType.m_type != m_mTypeName_mngIqmeshWriteConfig) {
        THROW_EXC(std::out_of_range, "Unsupported message type: " << PAR(msgType.m_type));
      }

      // creating representation object
      ComMngIqmeshWriteConfig comWriteConfig(doc);
      
      // service input parameters
      std::list<uint16_t> deviceAddrs;
      std::vector<HWP_ConfigByte> configBytes;
      
      try {
        m_repeat = parseAndCheckRepeat(comWriteConfig.getRepeat());

        if (!comWriteConfig.isSetDeviceAddr()) {
          THROW_EXC(std::logic_error, "deviceAddr not set");
        }
        deviceAddrs = parseAndCheckDeviceAddr(comWriteConfig.getDeviceAddr());
      
        configBytes = parseAndCheckConfigBytes(comWriteConfig);

        // no config bytes specified - error message
        if (configBytes.empty()) {
          THROW_EXC(std::out_of_range, "No config bytes specified");
        }

        if (comWriteConfig.isSetRfBand()) {
          m_coordRfChannelBand = parseAndCheckRfChannelBand(comWriteConfig.getRfBand());
          m_isSetCoordRfChannelBand = true;
        }
        else {
          m_coordRfChannelBand = RF_ChannelBand::UNSPECIFIED;
        }

        if (comWriteConfig.isSetSecurityPassword()) {
          m_securityPassword = parseAndCheckSecurityPassword(comWriteConfig.getSecurityPassword());
          m_isSetSecurityPassword = true;
        }
        else {
          m_securityPassword = createDefaultSecurityPassword();
        }

        if (comWriteConfig.isSetSecurityUserKey()) {
          m_securityUserKey = parseAndCheckSecurityUserKey(comWriteConfig.getSecurityUserKey());
          m_isSetSecurityUserKey = true;
        }
        else {
          m_securityUserKey = createDefaultSecurityUserKey();
        }

        m_returnVerbose = comWriteConfig.getVerbose();
      }
      // all errors are generally taken as a service general fail  
      catch (std::exception& ex) {
        Document failResponse = createCheckParamsFailedResponse(comWriteConfig.getMsgId(), msgType, ex.what());
        m_iMessagingSplitterService->sendMessage(messagingId, std::move(failResponse));

        TRC_FUNCTION_LEAVE("");
        return;
      }
      
      // call service with checked params
      WriteResult writeResult = writeConfigBytes(configBytes, deviceAddrs);

      // create and send response
      // will be changed later - indication of restart used here only temporary
      Document responseDoc = createResponse(
        comWriteConfig.getMsgId(), msgType, writeResult, comWriteConfig, isRestartNeeded(configBytes)
      );
      m_iMessagingSplitterService->sendMessage(messagingId, std::move(responseDoc));

      TRC_FUNCTION_LEAVE("");
    }
    

  public:
    void activate(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "************************************" << std::endl <<
        "WriteConfigService instance activate" << std::endl <<
        "************************************"
      );

      // for the sake of register function parameters 
      std::vector<std::string> supportedMsgTypes;
      supportedMsgTypes.push_back(m_mTypeName_mngIqmeshWriteConfig);

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
        "WriteConfigService instance deactivate" << std::endl <<
        "**************************************"
      );

      // for the sake of unregister function parameters 
      std::vector<std::string> supportedMsgTypes;
      supportedMsgTypes.push_back(m_mTypeName_mngIqmeshWriteConfig);

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



  WriteConfigService::WriteConfigService()
  {
    m_imp = shape_new Imp(*this);
  }

  WriteConfigService::~WriteConfigService()
  {
    delete m_imp;
  }


  void WriteConfigService::attachInterface(iqrf::IIqrfDpaService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void WriteConfigService::detachInterface(iqrf::IIqrfDpaService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void WriteConfigService::attachInterface(iqrf::IMessagingSplitterService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void WriteConfigService::detachInterface(iqrf::IMessagingSplitterService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void WriteConfigService::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void WriteConfigService::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }


  void WriteConfigService::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void WriteConfigService::deactivate()
  {
    m_imp->deactivate();
  }

  void WriteConfigService::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
  }

}
