#define IWriteTrConfService_EXPORTS

#include "WriteTrConfService.h"
#include "IMessagingSplitterService.h"
#include "Trace.h"
#include "ComMngIqmeshWriteConfig.h"
#include "ObjectFactory.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"

#include "iqrf__WriteTrConfService.hxx"

#include <list>
#include <memory>
#include <math.h>
#include <bitset>

TRC_INIT_MODULE(iqrf::WriteTrConfService);


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

  // baud rates
  static uint8_t BAUD_RATES_SIZE = 9;

  static uint32_t BaudRates[] = {
    1200,
    2400,
    4800,
    9600,
    19200,
    38400,
    57600,
    115200,
    230400
  };


  // max number of triplets per one write config byte request
  static const uint8_t MAX_TRIPLETS_PER_REQUEST = DPA_MAX_DATA_LENGTH / sizeof(TPerOSWriteCfgByteTriplet);


  // service general fail code - may and probably will be changed later in the future
  static const int SERVICE_ERROR = 1000;

  static const int SERVICE_ERROR_NOERROR = 0;
  static const int SERVICE_ERROR_INTERNAL = SERVICE_ERROR + 1;
  static const int SERVICE_ERROR_GET_BONDED_NODES = SERVICE_ERROR + 2;
  static const int SERVICE_ERROR_NO_BONDED_NODES = SERVICE_ERROR + 3;
  static const int SERVICE_ERROR_UPDATE_COORD_CHANNEL_BAND = SERVICE_ERROR + 4;
  static const int SERVICE_ERROR_ENABLE_FRC = SERVICE_ERROR + 5;
  static const int SERVICE_ERROR_DISABLE_FRC = SERVICE_ERROR + 6;
};


namespace iqrf {

  // Holds information about errors, which encounter during configuration write
  class WriteError {
  public:

    // Type of error
    enum class Type {
      NoError,
      GetBondedNodes,
      NoBondedNodes,
      UpdateCoordChannelBand,
      NodeNotBonded,
      EnableFrc,
      DisableFrc,
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
    WriteError m_writeError;

    // map of configuration bytes, which failed to write
    // indexed by byte addresses
    std::map<uint8_t, HWP_ConfigByte> m_failedBytesMap;

    void setWriteError() {
      if (this->m_writeError.getType() != WriteError::Type::Write) {
        WriteError writeError(WriteError::Type::Write);
        this->m_writeError = writeError;
      }
    }

  public:
    NodeWriteResult() {};

    WriteError getError() const { return m_writeError; };

    void setError(const WriteError& error) {
      this->m_writeError = error;
    }

    // Puts specified configuration byte, which failed to write.
    void putFailedByte(const HWP_ConfigByte& failedByte) {
      m_failedBytesMap[failedByte.address] = failedByte;
      setWriteError();
    }

    // Puts specified configuration bytes, which failed to write.
    void putFailedBytes(const std::vector<HWP_ConfigByte>& failedBytes) {
      for (const HWP_ConfigByte failedByte : failedBytes) {
        m_failedBytesMap[failedByte.address] = failedByte;
      }
      setWriteError();
    }

    // Returns map of failed bytes indexed by their addresses
    std::map<uint8_t, HWP_ConfigByte> getFailedBytesMap() const { return m_failedBytesMap; };
  };


  // holds information about configuration writing result for each node
  class WriteResult {
  private:
    // device addresses
    std::list<uint16_t> m_deviceAddrs;

    WriteError m_error;

    // map of write results on nodes indexed by node address
    std::map<uint16_t, NodeWriteResult> m_resultsMap;

    // transaction results
    std::list<std::unique_ptr<IDpaTransactionResult2>> m_transResults;

  public:

    std::list<uint16_t> getDeviceAddrs() const {
      return m_deviceAddrs;
    }

    void setDeviceAddrs(const std::list<uint16_t>& deviceAddrs) {
      m_deviceAddrs = deviceAddrs;
    }

    WriteError getError() const { return m_error; };

    void setError(const WriteError& error) {
      m_error = error;
    }

    // Puts specified write result for specified node into results.
    void putResult(uint16_t nodeAddr, const NodeWriteResult& result) {
      if (m_resultsMap.find(nodeAddr) != m_resultsMap.end()) {
        m_resultsMap.erase(nodeAddr);
      }
      m_resultsMap.insert(std::pair<uint16_t, NodeWriteResult>(nodeAddr, result));
    };

    // returns map of write results on nodes indexed by node address
    const std::map<uint16_t, NodeWriteResult>& getResultsMap() const { return m_resultsMap; };
    
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
  class WriteTrConfService::Imp {
  private:
    // parent object
    WriteTrConfService& m_parent;

    // message type: network management write configuration
    // for temporal reasons
    const std::string m_mTypeName_iqmeshNetwork_WriteTrConf = "iqmeshNetwork_WriteTrConf";
    //IMessagingSplitterService::MsgType* m_msgType_mngIqmeshWriteConfig;

    //iqrf::IJsCacheService* m_iJsCacheService = nullptr;
    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    IIqrfDpaService* m_iIqrfDpaService = nullptr;
    std::unique_ptr<IIqrfDpaService::ExclusiveAccess> m_exclusiveAccess;

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

    // indication, if FRC has been temporarily enabled
    bool m_frcEnabled = false;


  public:
    Imp(WriteTrConfService& parent) : m_parent(parent)
    {
      /*
      m_msgType_mngIqmeshWriteConfig
        = shape_new IMessagingSplitterService::MsgType(m_mTypeName_mngIqmeshWriteConfig, 1, 0, 0);
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

      NodeWriteResult nodeWriteResult;
      nodeWriteResult.setError(writeError);
      nodeWriteResult.putFailedBytes(failedConfigBytes);

      writeResult.putResult(nodeAddr, nodeWriteResult);
    }

    void processWriteError(
      WriteResult& writeResult,
      const std::list<uint16_t>& targetNodes,
      const std::vector<HWP_ConfigByte>& failedConfigBytes,
      const WriteError::Type errType,
      const std::string& errMsg
    )
    {
      WriteError writeError(errType, errMsg);

      for (const uint16_t targetNode : targetNodes) {
        NodeWriteResult nodeWriteResult;
        nodeWriteResult.setError(writeError);
        nodeWriteResult.putFailedBytes(failedConfigBytes);

        writeResult.putResult(targetNode, nodeWriteResult);
      }
    }

    void processSecurityError(
      WriteResult& writeResult,
      const uint16_t targetNode,
      const WriteError::Type errType,
      const std::string& errMsg
    )
    {
      WriteError writeError(errType, errMsg);

      NodeWriteResult nodeWriteResult;
      nodeWriteResult.setError(writeError);

      writeResult.putResult(targetNode, nodeWriteResult);
    }

    void processSecurityError(
      WriteResult& writeResult,
      const std::list<uint16_t>& targetNodes,
      const WriteError::Type errType,
      const std::string& errMsg
    )
    {
      WriteError writeError(errType, errMsg);

      for (const uint16_t targetNode : targetNodes) {
        NodeWriteResult nodeWriteResult;
        nodeWriteResult.setError(writeError);

        writeResult.putResult(targetNode, nodeWriteResult);
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
      writeConfigPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;

      //TPerOSWriteCfg_Request* tOsWriteCfgRequest = &writeConfigPacket.DpaRequestPacket_t.DpaMessage.PerOSWriteCfg_Request;
      uns8* pData = writeConfigPacket.DpaRequestPacket_t.DpaMessage.Request.PData;

      // getting DPA version
      IIqrfDpaService::CoordinatorParameters coordParams = m_iIqrfDpaService->getCoordinatorParameters();
      uint16_t dpaVer = (coordParams.dpaVerMajor << 8) + coordParams.dpaVerMinor;

      if (dpaVer < 0x0303) {
        pData[0] = countChecksum(configBytes);
      }

      setWriteRequestConfigurationField(pData+1, configBytes);
      pData[32] = getRfpgmByte(configBytes);

      writeConfigRequest.DataToBuffer(
        writeConfigPacket.Buffer, 
          sizeof(TDpaIFaceHeader) 
            + 1       // checksum
            + 31      // configuration
            + 1       // RFPGM
      );

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> writeConfigTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      for (int rep = 0; rep <= m_repeat; rep++) {
        try {
          //writeConfigTransaction = m_iIqrfDpaService->executeDpaTransaction(writeConfigRequest);
          writeConfigTransaction = m_exclusiveAccess->executeDpaTransaction(writeConfigRequest);
          transResult = writeConfigTransaction->get();
        }
        catch (std::exception& e) {
          TRC_WARNING("DPA transaction error : " << e.what());

          if (rep < m_repeat) {
            continue;
          }

          processWriteError(writeResult, nodeAddr, configBytes, WriteError::Type::Write, e.what());

          TRC_FUNCTION_LEAVE("");
          return;
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

          // add successfull node's result
          WriteError writeError(WriteError::Type::NoError);

          NodeWriteResult nodeWriteResult;
          nodeWriteResult.setError(writeError);

          writeResult.putResult(nodeAddr, nodeWriteResult);

          TRC_FUNCTION_LEAVE("");
          return;
        }

        // transaction error
        if (errorCode < 0) {
          TRC_WARNING("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

          if (rep < m_repeat) {
            continue;
          }

          processWriteError(writeResult, nodeAddr, configBytes, WriteError::Type::Write, "Transaction error.");
        } // DPA error
        else {
          TRC_WARNING("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

          if (rep < m_repeat) {
            continue;
          }

          processWriteError(writeResult, nodeAddr, configBytes, WriteError::Type::Write, "DPA error.");
        }
      }

      TRC_FUNCTION_LEAVE("");
    }

    // puts selected nodes nodes into FRC request
    void setSelectedNodesForFrcRequest(
      TPerFrcSendSelective_Request* frcPacket,
      const std::list<uint16_t>& targetNodes
    )
    {
      // initialize "SelectedNodes" section
      memset(frcPacket->SelectedNodes, 0, 30*sizeof(uns8));

      for (uint16_t i : targetNodes) {
        uns8 byteIndex = i / 8;
        uns8 bitIndex = i % 8;
        frcPacket->SelectedNodes[byteIndex] |= (uns8)pow(2, bitIndex);
      }
    }

    void setUserDataForFrcWriteConfigByteRequest(
      TPerFrcSendSelective_Request* frcPacket,
      const std::vector<HWP_ConfigByte>& configBytes
    )
    {
      uns8* userData = frcPacket->UserData;

      // initialize user data to zero
      memset(userData, 0, 25 * sizeof(uns8));

      // copy foursome
      userData[0] = 1 + 1 + 1 + 2 + configBytes.size() * 3;
      userData[1] = PNUM_OS;
      userData[2] = CMD_OS_WRITE_CFG_BYTE;
      userData[3] = HWPID_DoNotCheck & 0xFF;
      userData[4] = (HWPID_DoNotCheck >> 8) & 0xFF;

      // fill in config bytes
      uint8_t dataIndex = 5;

      for (const HWP_ConfigByte configByte : configBytes) {
        userData[dataIndex++] = configByte.address;
        userData[dataIndex++] = configByte.value;
        userData[dataIndex++] = configByte.mask;
      }
    }

    // returns specified part of config bytes
    // partSize mod 3 == 0 (config byte contains 3 bytes of information)
    std::vector<HWP_ConfigByte> getConfigBytesPart(
      const int partId,
      const int partSize,
      const std::vector<HWP_ConfigByte>& configBytes
    )
    {
      std::vector<HWP_ConfigByte>::const_iterator first = configBytes.begin() + partId * (partSize/3);
      std::vector<HWP_ConfigByte>::const_iterator last;

      // check, if partId denotes the last part of config bytes
      if (((partId + 1) * (partSize / 3) ) >= configBytes.size()) {
        last = configBytes.end();
      }
      else {
        last = configBytes.begin() + partId * (partSize/3 + 1);
        if (partId == 0) {
          last = configBytes.begin() + partSize/3;
        }
      }

      return std::vector<HWP_ConfigByte>(first, last);
    }

    // parses 2bits FRC results into map: keys are nodes IDs, values are returned 2 bits
    std::map<uint16_t, uint8_t> parse2bitsFrcData(
      const std::basic_string<uint8_t>& frcData,
      const std::list<uint16_t>& targetNodes
    ) 
    {
      std::map<uint16_t, uint8_t> nodesResults;
      std::list<uint16_t>::const_iterator findIter;

      uint16_t nodeId = 0;

      for (int byteId = 0; byteId <= 29; byteId++) {
        int bitComp = 1;
        for (int bitId = 0; bitId < 8; bitId++) {
          uint8_t bit0 = ((frcData[byteId] & bitComp) == bitComp) ? 1 : 0;
          uint8_t bit1 = ((frcData[byteId + 32] & bitComp) == bitComp) ? 1 : 0;

          findIter = std::find(targetNodes.begin(), targetNodes.end(), nodeId);
          if (findIter != targetNodes.end()) {
            nodesResults.insert(std::pair<uint16_t, uint8_t>(nodeId, bit0 + 2 * bit1));
          }
          
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
      for (const std::pair<uint16_t, uint8_t> p : nodesResultsMap) {
        // all ok
        if ((p.second & 0b1) == 0b1) {
          // add successfull node's result
          WriteError writeError(WriteError::Type::NoError);

          NodeWriteResult nodeWriteResult;
          nodeWriteResult.setError(writeError);

          writeResult.putResult(p.first, nodeWriteResult);

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

        NodeWriteResult nodeWriteResult;
        nodeWriteResult.setError(writeError);
        nodeWriteResult.putFailedBytes(configBytes);

        writeResult.putResult(p.first, nodeWriteResult);
      }
    }

    // puts nodes results of set security into overall write config results
    void putSetSecurityFrcResults(
      WriteResult& writeResult,
      WriteError::Type errorType,
      const std::map<uint16_t, uint8_t>& nodesResultsMap
    )
    {
      for (const std::pair<uint16_t, uint8_t> p : nodesResultsMap) {
        // all ok
        if ((p.second & 0b1) == 0b1) {
          // add successfull node's result
          WriteError writeError(WriteError::Type::NoError);

          NodeWriteResult nodeWriteResult;
          nodeWriteResult.setError(writeError);

          writeResult.putResult(p.first, nodeWriteResult);

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

        NodeWriteResult nodeWriteResult;
        nodeWriteResult.setError(writeError);
        writeResult.putResult(p.first, nodeWriteResult);
      }
    }

    // returns nodes, which were writen unsuccessfully into
    std::list<uint16_t> getUnsuccessfulNodes(
      const std::list<uint16_t>& nodesToWrite,
      const std::map<uint16_t, uint8_t>& nodesResultsMap
    ) 
    {
      std::list<uint16_t> unsuccessfulNodes;

      for (std::pair<uint16_t, uint8_t> p : nodesResultsMap) {
        std::list<uint16_t>::const_iterator findIter 
          = std::find(nodesToWrite.begin(), nodesToWrite.end(), p.first);

        if (findIter == nodesToWrite.end()) {
          continue;
        }

        if ((p.second & 0b1) != 0b1) {
          unsuccessfulNodes.push_back(p.first);
        }
      }

      return unsuccessfulNodes;
    }

    // returns a number, which is floor of the multiple of 3
    uint8_t toMultiple3Floor(uint8_t writeConfigPacketFreeSpace)
    {
      uint8_t ratio = writeConfigPacketFreeSpace / 3;
      return (ratio * 3);
    }

    // fills write config byte packet data with config bytes up to packet's capacity
    void fillConfigBytePacketData(
      TPerOSWriteCfgByte_Request* writeConfigPacketRequest,
      const std::vector<HWP_ConfigByte>& configBytes
    )
    {
      // zero all previous triplets in the packet
      for (uint8_t i = 0; i < MAX_TRIPLETS_PER_REQUEST; i++) {
        writeConfigPacketRequest->Triplets[i].Address = 0;
        writeConfigPacketRequest->Triplets[i].Value = 0;
        writeConfigPacketRequest->Triplets[i].Mask = 0;
      }

      for (uint8_t i = 0; i < configBytes.size(); i++) {
        writeConfigPacketRequest->Triplets[i].Address = configBytes[i].address;
        writeConfigPacketRequest->Triplets[i].Value = configBytes[i].value;
        writeConfigPacketRequest->Triplets[i].Mask = configBytes[i].mask;
      }
    }

    std::vector<HWP_ConfigByte> getNextConfigBytesPart(
      const std::vector<HWP_ConfigByte>& configBytes, 
      size_t& pos
    )
    {
      std::vector<HWP_ConfigByte> configBytesPart;

      uint8_t bytesToWriteNum = MAX_TRIPLETS_PER_REQUEST;
      if ((pos + MAX_TRIPLETS_PER_REQUEST) > configBytes.size()) {
        bytesToWriteNum = configBytes.size() - pos;
      }

      for (uint8_t i = 0; i < bytesToWriteNum; i++) {
        configBytesPart.push_back(configBytes[pos + i]);
      }

      // shift position
      pos += bytesToWriteNum;

      return configBytesPart;
    }


    void putCoordRightWrittenConfigBytesResult(
      WriteResult& writeResult, 
      const std::vector<HWP_ConfigByte>& configBytes
    )
    {
      WriteError noError(WriteError::Type::NoError);
      
      NodeWriteResult nodeWriteResult;
      nodeWriteResult.setError(noError);

      for (const HWP_ConfigByte configByte : configBytes) {
        writeResult.putResult(COORDINATOR_ADDRESS, nodeWriteResult);
      }
    }

    // writes config bytes to coordinator
    void _writeConfigBytesToCoordinator(
      WriteResult& writeResult,
      const std::vector<HWP_ConfigByte>& configBytes
    ) 
    {
      TRC_FUNCTION_ENTER("");

      DpaMessage writeConfigByteRequest;
      DpaMessage::DpaPacket_t writeConfigBytePacket;
      writeConfigBytePacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      writeConfigBytePacket.DpaRequestPacket_t.PNUM = PNUM_OS;
      writeConfigBytePacket.DpaRequestPacket_t.PCMD = CMD_OS_WRITE_CFG_BYTE;
      writeConfigBytePacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;

      TPerOSWriteCfgByte_Request* writeConfigPacketRequest
        = &writeConfigBytePacket.DpaRequestPacket_t.DpaMessage.PerOSWriteCfgByte_Request;

      // position of the next config byte to write
      size_t configBytesPos = 0;

      while (configBytesPos < configBytes.size()) {
        // config bytes, which will be written in one packet
        std::vector<HWP_ConfigByte> configBytesPart = getNextConfigBytesPart(configBytes, configBytesPos);

        // fill PData of the packet with config bytes
        fillConfigBytePacketData(writeConfigPacketRequest, configBytesPart);

        writeConfigByteRequest.DataToBuffer(
          writeConfigBytePacket.Buffer,
          sizeof(TDpaIFaceHeader) + configBytesPart.size() * sizeof(TPerOSWriteCfgByteTriplet)
        );

        for (int rep = 0; rep <= m_repeat; rep++) {
          // issue the DPA request
          std::shared_ptr<IDpaTransaction2> writeConfigTransaction;
          std::unique_ptr<IDpaTransactionResult2> transResult;

          try {
            //writeConfigTransaction = m_iIqrfDpaService->executeDpaTransaction(writeConfigByteRequest);
            writeConfigTransaction = m_exclusiveAccess->executeDpaTransaction(writeConfigByteRequest);
            transResult = writeConfigTransaction->get();
          }
          catch (std::exception& e) {
            TRC_WARNING("DPA transaction error : " << e.what());

            if (rep < m_repeat) {
              continue;
            }

            processWriteError(writeResult, COORDINATOR_ADDRESS, configBytesPart, WriteError::Type::Write, e.what());
            break;
          }

          TRC_DEBUG("Result from Write config byte transaction as string:" << PAR(transResult->getErrorString()));

          
          IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

          // because of the move-semantics
          DpaMessage dpaResponse = transResult->getResponse();
          writeResult.addTransactionResult(transResult);

          if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
            TRC_INFORMATION("Write config byte successful!");
            TRC_DEBUG(
              "DPA transaction: "
              << NAME_PAR(writeConfigByteRequest.PeripheralType(), writeConfigByteRequest.NodeAddress())
              << PAR(writeConfigByteRequest.PeripheralCommand())
            );

            // put right written bytes into result
            putCoordRightWrittenConfigBytesResult(writeResult, configBytesPart);
            break;
          }
          else {
            // transaction error
            if (errorCode < 0) {
              TRC_WARNING("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

              if (rep < m_repeat) {
                continue;
              }

              processWriteError(writeResult, COORDINATOR_ADDRESS, configBytesPart, WriteError::Type::Write, "Transaction error.");
              break;
            } // DPA error
            else {
              TRC_WARNING("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

              if (rep < m_repeat) {
                continue;
              }

              processWriteError(writeResult, COORDINATOR_ADDRESS, configBytesPart, WriteError::Type::Write, "DPA error.");
              break;
            }
          }
        }
      }
    }

    // writes configuration bytes into target nodes (no coordinator)
    void _writeConfigBytesToNodes(
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
      frcPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;

      TPerFrcSendSelective_Request* frcPacketRequest = &frcPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request;

      frcPacketRequest->FrcCommand = FRC_AcknowledgedBroadcastBits;
      setSelectedNodesForFrcRequest(frcPacketRequest,targetNodes);

      uint8_t writeConfigPacketFreeSpace = 25
        - 1
        - sizeof(frcPacket.DpaRequestPacket_t.PCMD)
        - sizeof(frcPacket.DpaRequestPacket_t.PNUM)
        - sizeof(frcPacket.DpaRequestPacket_t.HWPID)
        ;

      // normalize writeConfigPacketFreeSpace to divisible by 3 - size of the one config byte in the bytes
      writeConfigPacketFreeSpace = toMultiple3Floor(writeConfigPacketFreeSpace);

      // number of parts with config. bytes to send by FRC request
      uint8_t partsTotal = ceil((configBytes.size() * 3) / (double)writeConfigPacketFreeSpace);

      for (int partId = 0; partId < partsTotal; partId++) {

        // for each part try to write data m_rep times
        // at the end of each iteration, discover unsuccessful nodes
        std::list<uint16_t> nodesToWrite(targetNodes);

        for (int rep = 0; rep <= m_repeat; rep++) {
          // all nodes were successfully writen into for actual data part
          if (nodesToWrite.empty()) {
            break;
          }

          setSelectedNodesForFrcRequest(frcPacketRequest, nodesToWrite);

          // prepare part of config bytes to fill into the FRC request user data
          std::vector<HWP_ConfigByte> configBytesPart = getConfigBytesPart(
            partId, 
            writeConfigPacketFreeSpace, 
            configBytes
          );
          setUserDataForFrcWriteConfigByteRequest(frcPacketRequest, configBytesPart);

          // issue the DPA request
          frcRequest.DataToBuffer(
            frcPacket.Buffer,
            sizeof(TDpaIFaceHeader) + 1 + 30 + 5 + configBytesPart.size() * 3
          );

          // issue the DPA request
          std::shared_ptr<IDpaTransaction2> frcWriteConfigTransaction;
          std::unique_ptr<IDpaTransactionResult2> transResult;

          try {
            //frcWriteConfigTransaction = m_iIqrfDpaService->executeDpaTransaction(frcRequest);
            frcWriteConfigTransaction = m_exclusiveAccess->executeDpaTransaction(frcRequest, 0);
            transResult = frcWriteConfigTransaction->get();
          }
          catch (std::exception& e) {
            TRC_WARNING("DPA transaction error : " << e.what());

            if (rep < m_repeat) {
              continue;
            }

            processWriteError(writeResult, nodesToWrite, configBytesPart, WriteError::Type::Write, e.what());
            break;
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
              frcData.append(
                dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.FrcData,
                DPA_MAX_DATA_LENGTH - sizeof(uns8)
              );
              TRC_DEBUG("Size of FRC data: " << PAR(frcData.size()));
            }
            else {
              TRC_WARNING("FRC write config status NOT ok." << NAME_PAR_HEX("Status", status));
              
              if (rep < m_repeat) {
                continue;
              }
              
              processWriteError(writeResult, nodesToWrite, configBytesPart, WriteError::Type::Write, "Bad status.");
              break;
            }
          }
          else {
            // transaction error
            if (errorCode < 0) {
              TRC_WARNING("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

              if (rep < m_repeat) {
                continue;
              }

              processWriteError(writeResult, nodesToWrite, configBytesPart, WriteError::Type::Write, "Transaction error.");
              break;
            } // DPA error
            else {
              TRC_WARNING("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

              if (rep < m_repeat) {
                continue;
              }

              processWriteError(writeResult, nodesToWrite, configBytesPart, WriteError::Type::Write, "DPA error.");
              break;
            }
          }


          // get extra results
          DpaMessage extraResultRequest;
          DpaMessage::DpaPacket_t extraResultPacket;
          extraResultPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
          extraResultPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
          extraResultPacket.DpaRequestPacket_t.PCMD = CMD_FRC_EXTRARESULT;
          extraResultPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
          extraResultRequest.DataToBuffer(extraResultPacket.Buffer, sizeof(TDpaIFaceHeader));

          // issue the DPA request
          std::shared_ptr<IDpaTransaction2> extraResultTransaction;

          try {
            //extraResultTransaction = m_iIqrfDpaService->executeDpaTransaction(extraResultRequest);
            extraResultTransaction = m_exclusiveAccess->executeDpaTransaction(extraResultRequest, 0);
            transResult = extraResultTransaction->get();
          }
          catch (std::exception& e) {
            TRC_WARNING("DPA transaction error : " << e.what());

            if (rep < m_repeat) {
              continue;
            }

            processWriteError(writeResult, nodesToWrite, configBytesPart, WriteError::Type::Write, e.what());
            break;
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

            frcData.append(
              dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData,
              64 - frcData.size()
            );
          }
          else {
            // transaction error
            if (errorCode < 0) {
              TRC_WARNING("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

              if (rep < m_repeat) {
                continue;
              }

              processWriteError(writeResult, nodesToWrite, configBytesPart, WriteError::Type::Write, "Transaction error.");
              break;
            } // DPA error
            else {
              TRC_WARNING("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

              if (rep < m_repeat) {
                continue;
              }

              processWriteError(writeResult, nodesToWrite, configBytesPart, WriteError::Type::Write, "DPA error.");
              break;
            }
          }

     
          // FRC data parsing
          std::map<uint16_t, uint8_t> nodesResultsMap = parse2bitsFrcData(
            frcData, nodesToWrite
          );

          // update group of nodes needed to write into
          nodesToWrite = getUnsuccessfulNodes(nodesToWrite, nodesResultsMap);

          // putting nodes results into overall write config results
          putWriteConfigFrcResults(writeResult, configBytesPart, WriteError::Type::Write, nodesResultsMap);

          if (nodesToWrite.empty()) {
            break;
          }
        }
      }

      TRC_FUNCTION_LEAVE("");
    }

    // indication, if FRC is enabled on Coordinator's HWP Configuration
    bool frcEnabledOnCoord(WriteResult& writeResult) 
    {
      TRC_FUNCTION_ENTER("");

      DpaMessage readHwpConfigRequest;
      DpaMessage::DpaPacket_t readHwpConfigPacket;
      readHwpConfigPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      readHwpConfigPacket.DpaRequestPacket_t.PNUM = PNUM_OS;
      readHwpConfigPacket.DpaRequestPacket_t.PCMD = CMD_OS_READ_CFG;
      readHwpConfigPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
      readHwpConfigRequest.DataToBuffer(readHwpConfigPacket.Buffer, sizeof(TDpaIFaceHeader));

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> readHwpConfigTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;


      for (int rep = 0; rep <= m_repeat; rep++) {
        try {
          readHwpConfigTransaction = m_exclusiveAccess->executeDpaTransaction(readHwpConfigRequest);
          transResult = readHwpConfigTransaction->get();
        }
        catch (std::exception& e) {
          TRC_WARNING("DPA transaction error : " << e.what());

          if (rep < m_repeat) {
            continue;
          }

          THROW_EXC(std::logic_error, "DPA transaction error.");
        }

        TRC_DEBUG("Result from Read HWP Configuration transaction as string:" << PAR(transResult->getErrorString()));

        IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

        // because of the move-semantics
        DpaMessage dpaResponse = transResult->getResponse();
        writeResult.addTransactionResult(transResult);

        if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
          TRC_INFORMATION("Read HWP Configuration successful!");
          TRC_DEBUG(
            "DPA transaction: "
            << NAME_PAR(readHwpConfigRequest.PeripheralType(), readHwpConfigRequest.NodeAddress())
            << PAR(readHwpConfigRequest.PeripheralCommand())
          );

          // parsing response data
          uns8* readConfigRespData = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData;

          TRC_FUNCTION_LEAVE("");
          return (readConfigRespData[2] & 0b00100000) == 0b00100000;
        }

        // transaction error
        if (errorCode < 0) {
          TRC_WARNING("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

          if (rep < m_repeat) {
            continue;
          }

          THROW_EXC(std::logic_error, "Transaction error.");
        }

        // DPA error
        TRC_WARNING("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

        if (rep < m_repeat) {
          continue;
        }

        THROW_EXC(std::logic_error, "Dpa error.");
      }
    }

    // sets FRC on coordinator - enables or disables it - 2. parameter
    void setFrcOnCoord(WriteResult& writeResult, bool enable)
    {
      TRC_FUNCTION_ENTER("");

      DpaMessage setFrcRequest;
      DpaMessage::DpaPacket_t setFrcPacket;
      setFrcPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      setFrcPacket.DpaRequestPacket_t.PNUM = PNUM_OS;
      setFrcPacket.DpaRequestPacket_t.PCMD = CMD_OS_WRITE_CFG_BYTE;
      setFrcPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
      setFrcRequest.DataToBuffer(setFrcPacket.Buffer, sizeof(TDpaIFaceHeader) + 3);

      // FRC configuration byte
      uns8* pData = setFrcPacket.DpaRequestPacket_t.DpaMessage.Request.PData;
      pData[0] = 0x02;
      pData[1] = enable? 0b00100000 : 0b00000000;
      pData[2] = 0b00100000;

      setFrcRequest.DataToBuffer(setFrcPacket.Buffer, sizeof(TDpaIFaceHeader) + 3);

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> setFrcTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;


      for (int rep = 0; rep <= m_repeat; rep++) {
        try {
          setFrcTransaction = m_exclusiveAccess->executeDpaTransaction(setFrcRequest);
          transResult = setFrcTransaction->get();
        }
        catch (std::exception& e) {
          TRC_WARNING("DPA transaction error : " << e.what());

          if (rep < m_repeat) {
            continue;
          }

          THROW_EXC(std::logic_error, "DPA transaction error.");
        }

        TRC_DEBUG("Result from Set FRC on Coordinator transaction as string:" << PAR(transResult->getErrorString()));

        IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

        // because of the move-semantics
        DpaMessage dpaResponse = transResult->getResponse();
        writeResult.addTransactionResult(transResult);

        if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
          TRC_INFORMATION("Set FRC on Coordinator successful!");
          TRC_DEBUG(
            "DPA transaction: "
            << NAME_PAR(setFrcRequest.PeripheralType(), setFrcRequest.NodeAddress())
            << PAR(setFrcRequest.PeripheralCommand())
          );
        }

        // transaction error
        if (errorCode < 0) {
          TRC_WARNING("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

          if (rep < m_repeat) {
            continue;
          }

          THROW_EXC(std::logic_error, "Transaction error.");
        }

        // DPA error
        TRC_WARNING("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

        if (rep < m_repeat) {
          continue;
        }

        THROW_EXC(std::logic_error, "Dpa error.");
      }
    }

    void _writeConfigBytes(
      WriteResult& writeResult, 
      const std::vector<HWP_ConfigByte>& configBytes,
      const std::list<uint16_t>& targetNodes
    )
    {
      bool isCoordPresent = false;

      std::list<uint16_t> targetNodesCopy;

      // only nodes - coordinator filtered out
      for (const uint16_t nodeAddr : targetNodes) {
        if (nodeAddr == COORDINATOR_ADDRESS) {
          isCoordPresent = true;
          continue;
        }
        targetNodesCopy.push_back(nodeAddr);
      }

      if (isCoordPresent) {
        _writeConfigBytesToCoordinator(writeResult, configBytes);
      }

      if (!targetNodesCopy.empty()) {
        
        // first of all it, it is needed to temporarily enable FRC peripheral, if it is not enabled by Configuration 
        bool isFrcEnabledOnCoord = false; 
        try {
          // issues DPA request - read HWP configuration
          isFrcEnabledOnCoord = frcEnabledOnCoord(writeResult);
          if (!isFrcEnabledOnCoord) {
            setFrcOnCoord(writeResult, true);

            // indication - before finishing the service, FRC must be disabled to restore the state before
            m_frcEnabled = true;
          }
        }
        catch (std::exception& ex) {
          WriteError error(WriteError::Type::EnableFrc, ex.what());
          writeResult.setError(error);
          return;
        }

        _writeConfigBytesToNodes(writeResult, configBytes, targetNodesCopy);
      }
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
      bondedNodesPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
      bondedNodesRequest.DataToBuffer(bondedNodesPacket.Buffer, sizeof(TDpaIFaceHeader));

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> getBondedNodesTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;


      for (int rep = 0; rep <= m_repeat; rep++) {
        try {
          //getBondedNodesTransaction = m_iIqrfDpaService->executeDpaTransaction(bondedNodesRequest);
          getBondedNodesTransaction = m_exclusiveAccess->executeDpaTransaction(bondedNodesRequest);
          transResult = getBondedNodesTransaction->get();
        }
        catch (std::exception& e) {
          TRC_WARNING("DPA transaction error : " << e.what());

          if (rep < m_repeat) {
            continue;
          }

          THROW_EXC(std::logic_error, "DPA transaction error.");
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
          TRC_WARNING("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

          if (rep < m_repeat) {
            continue;
          }

          THROW_EXC(std::logic_error, "Transaction error.");
        } 
        
        // DPA error
        TRC_WARNING("DPA error. " << NAME_PAR_HEX("Error code", errorCode));
        
        if (rep < m_repeat) {
          continue;
        }

        THROW_EXC(std::logic_error, "Dpa error.");
      }

      THROW_EXC(std::logic_error, "Service internal error. Getting bonded nodes failed.");
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
        if (node == COORDINATOR_ADDRESS) {
          targetBondedNodes.push_back(node);
          continue;
        }

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
      readConfigPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
      readConfigRequest.DataToBuffer(readConfigPacket.Buffer, sizeof(TDpaIFaceHeader));

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> readConfigTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      for (int rep = 0; rep <= m_repeat; rep++) {
        try {
          //readConfigTransaction = m_iIqrfDpaService->executeDpaTransaction(readConfigRequest);
          readConfigTransaction = m_exclusiveAccess->executeDpaTransaction(readConfigRequest);
          transResult = readConfigTransaction->get();
        }
        catch (std::exception& e) {
          TRC_WARNING("DPA transaction error : " << e.what());

          if (rep < m_repeat) {
            continue;
          }

          THROW_EXC(std::logic_error, "Dpa transaction error.");
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
          TRC_WARNING("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

          if (rep < m_repeat) {
            continue;
          }

          THROW_EXC(std::logic_error, "Transaction error.");
        } 
        
        // DPA error
        TRC_WARNING("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

        if (rep < m_repeat) {
          continue;
        }

        THROW_EXC(std::logic_error, "Dpa error.");
      }
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
              try {
                updateCoordRfChannelBand(writeResult);
              }
              catch (std::exception& ex) {
                THROW_EXC(std::logic_error, "Cannot update coordinator RF channel band");
              }
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
      TPerFrcSendSelective_Request* frcPacket,
      const std::basic_string<uint8_t>& securityString,
      const bool isPassword
    )
    {
      uns8* userData = frcPacket->UserData;

      // initialize user data to zero
      memset(userData, 0, 25 * sizeof(uns8));

      // length
      userData[0] = sizeof (TDpaIFaceHeader) - 1 + 1 + securityString.length();
      
      userData[1] = PNUM_OS;
      userData[2] = CMD_OS_SET_SECURITY;
      userData[3] = HWPID_DoNotCheck & 0xFF;
      userData[4] = (HWPID_DoNotCheck >> 8) & 0xFF;
      userData[5] = (isPassword) ? 0 : 1;

      std::copy(securityString.begin(), securityString.end(), userData + 6);
    }

    // sets security string into one concrete node - issues one DPA Set Security request
    void _setSecurityStringToOneNode(
      WriteResult& writeResult,
      const uint16_t nodeAddr,
      const std::basic_string<uint8_t>& securityString,
      const bool isPassword
    )
    {
      TRC_FUNCTION_ENTER("");

      DpaMessage securityRequest;
      DpaMessage::DpaPacket_t securityPacket;
      securityPacket.DpaRequestPacket_t.NADR = nodeAddr;
      securityPacket.DpaRequestPacket_t.PNUM = PNUM_OS;
      securityPacket.DpaRequestPacket_t.PCMD = CMD_OS_SET_SECURITY;
      securityPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;

      TPerOSSetSecurity_Request* securityPacketRequest = &securityPacket.DpaRequestPacket_t.DpaMessage.PerOSSetSecurity_Request;
      securityPacketRequest->Type = (isPassword) ? 0 : 1;

      // copy security string into packet
      memset(securityPacketRequest->Data, 0, 16 * sizeof(uint8_t));
      std::copy(securityString.begin(), securityString.end(), securityPacketRequest->Data);

      securityRequest.DataToBuffer(
        securityPacket.Buffer,
        sizeof(TDpaIFaceHeader) + (1 + 16) * sizeof(uint8_t)
      );

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> setSecurityTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      // type of error
      WriteError::Type errorType = (isPassword) ? WriteError::Type::SecurityPassword : WriteError::Type::SecurityUserKey;

      for (int rep = 0; rep <= m_repeat; rep++) {
        try {
          //setSecurityTransaction = m_iIqrfDpaService->executeDpaTransaction(securityRequest);
          setSecurityTransaction = m_exclusiveAccess->executeDpaTransaction(securityRequest);
          transResult = setSecurityTransaction->get();
        }
        catch (std::exception& e) {
          TRC_WARNING("DPA transaction error : " << e.what());

          if (rep < m_repeat) {
            continue;
          }

          processSecurityError(writeResult, nodeAddr, errorType, e.what());
          break;
        }

        TRC_DEBUG("Result from set security transaction as string:" << PAR(transResult->getErrorString()));

        IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

        // because of the move-semantics
        DpaMessage dpaResponse = transResult->getResponse();
        writeResult.addTransactionResult(transResult);

        if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
          TRC_INFORMATION("Set security successful!");
          TRC_DEBUG(
            "DPA transaction: "
            << NAME_PAR(securityRequest.PeripheralType(), securityRequest.NodeAddress())
            << PAR(securityRequest.PeripheralCommand())
          );
        }
        else {
          // transaction error
          if (errorCode < 0) {
            TRC_WARNING("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

            if (rep < m_repeat) {
              continue;
            }

            processSecurityError(writeResult, nodeAddr, errorType, "Transaction error.");
            break;
          } // DPA error
          else {
            TRC_WARNING("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

            if (rep < m_repeat) {
              continue;
            }

            processSecurityError(writeResult, nodeAddr, errorType, "DPA error.");
            break;
          }
        }
      }
    }

    // sets security to pure nodes (without coordinator)
    void _setSecurityStringToNodes(
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
      frcPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;

      TPerFrcSendSelective_Request* frcPacketRequest = &frcPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request;

      // at the end of each iteration, discover unsuccessful nodes
      std::list<uint16_t> nodesToWrite(targetNodes);

      frcPacketRequest->FrcCommand = FRC_AcknowledgedBroadcastBits;
      setSelectedNodesForFrcRequest(frcPacketRequest, nodesToWrite);

      // set security string as a data for FRC request
      setUserDataForSetSecurityFrcRequest(frcPacketRequest, securityString, isPassword);

      // issue the DPA request
      frcRequest.DataToBuffer(
        frcPacket.Buffer,
        + sizeof(TDpaIFaceHeader)       // "main command header"
        + 1                             // FRC command ID
        + 30                            // target nodes
        + sizeof(TDpaIFaceHeader) - 1   // embedded header
        + 1                             // type of security string data 
        + securityString.size()         // security string
      );

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> frcWriteConfigTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      // type of error
      WriteError::Type errorType = (isPassword) ? WriteError::Type::SecurityPassword : WriteError::Type::SecurityUserKey;

      for (int rep = 0; rep <= m_repeat; rep++) {
        try {
          //frcWriteConfigTransaction = m_iIqrfDpaService->executeDpaTransaction(frcRequest);
          frcWriteConfigTransaction = m_exclusiveAccess->executeDpaTransaction(frcRequest, 0);
          transResult = frcWriteConfigTransaction->get();
        }
        catch (std::exception& e) {
          TRC_WARNING("DPA transaction error : " << e.what());

          if (rep < m_repeat) {
            continue;
          }

          processSecurityError(writeResult, nodesToWrite, errorType, e.what());
          break;
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
            frcData.append(
              dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData,
              DPA_MAX_DATA_LENGTH - sizeof(uns8)
            );
          }
          else {
            TRC_WARNING("FRC write config status NOT ok." << NAME_PAR_HEX("Status", status));

            if (rep < m_repeat) {
              continue;
            }

            processSecurityError(writeResult, nodesToWrite, errorType, "Bad status.");
            break;
          }
        }
        else {
          // transaction error
          if (errorCode < 0) {
            TRC_WARNING("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

            if (rep < m_repeat) {
              continue;
            }

            processSecurityError(writeResult, nodesToWrite, errorType, "Transaction error.");
            break;
          } // DPA error
          else {
            TRC_WARNING("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

            if (rep < m_repeat) {
              continue;
            }

            processSecurityError(writeResult, nodesToWrite, errorType, "DPA error.");
            break;
          }
        }

        
        // get extra results
        DpaMessage extraResultRequest;
        DpaMessage::DpaPacket_t extraResultPacket;
        extraResultPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        extraResultPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
        extraResultPacket.DpaRequestPacket_t.PCMD = CMD_FRC_EXTRARESULT;
        extraResultPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        extraResultRequest.DataToBuffer(extraResultPacket.Buffer, sizeof(TDpaIFaceHeader));

        // issue the DPA request
        std::shared_ptr<IDpaTransaction2> extraResultTransaction;

        try {
          //extraResultTransaction = m_iIqrfDpaService->executeDpaTransaction(extraResultRequest);
          extraResultTransaction = m_exclusiveAccess->executeDpaTransaction(extraResultRequest, 0);
          transResult = extraResultTransaction->get();
        }
        catch (std::exception& e) {
          TRC_WARNING("DPA transaction error : " << e.what());

          if (rep < m_repeat) {
            continue;
          }

          processSecurityError(writeResult, nodesToWrite, errorType, e.what());
          break;
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

          frcData.append(
            dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData,
            64 - frcData.size()
          );
        }
        else {
          // transaction error
          if (errorCode < 0) {
            TRC_WARNING("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

            if (rep < m_repeat) {
              continue;
            }

            processSecurityError(writeResult, nodesToWrite, errorType, "Transaction error.");
            break;
          } // DPA error
          else {
            TRC_WARNING("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

            if (rep < m_repeat) {
              continue;
            }

            processSecurityError(writeResult, nodesToWrite, errorType, "DPA error.");
            break;
          }
        }

        // FRC data parsing
        std::map<uint16_t, uint8_t> nodesResultsMap = parse2bitsFrcData(frcData, nodesToWrite);

        // putting nodes results into overall write config results
        putSetSecurityFrcResults(writeResult, errorType, nodesResultsMap);
        
        // update unsuccessful nodes for next iteration
        nodesToWrite = getUnsuccessfulNodes(nodesToWrite, nodesResultsMap);

        // if all nodes were successfull, go to the end
        if (nodesToWrite.empty()) {
          break;
        }
      }

      TRC_FUNCTION_LEAVE("");
    }
    
    void setSecurityString(
      WriteResult& writeResult,
      const std::list<uint16_t>& targetNodes,
      const std::basic_string<uint8_t>& securityPassword,
      const bool isPassword
    ) 
    {
      bool isCoordPresent = false;

      std::list<uint16_t> targetNodesCopy;

      // only nodes - coordinator filtered out
      for (const uint16_t nodeAddr : targetNodes) {
        if (nodeAddr == COORDINATOR_ADDRESS) {
          isCoordPresent = true;
          continue;
        }
        targetNodesCopy.push_back(nodeAddr);
      }

      if (isCoordPresent) {
        _setSecurityStringToOneNode(writeResult, 0, securityPassword, isPassword);
      }

      if (!targetNodesCopy.empty()) {
        if (targetNodesCopy.size() == 1) {
          _setSecurityStringToOneNode(writeResult, targetNodesCopy.front(), securityPassword, isPassword);
        }
        else {
          _setSecurityStringToNodes(writeResult, targetNodesCopy, securityPassword, isPassword);
        }
      }
    }

    WriteResult writeConfigBytes(
      const std::vector<HWP_ConfigByte>& configBytes,
      const std::list<uint16_t>& targetNodes
    )
    {
      // result of writing configuration
      WriteResult writeResult;

      // set list of addresses of target nodes
      writeResult.setDeviceAddrs(targetNodes);

      // getting list of all bonded nodes
      std::list<uint16_t> bondedNodes;
      try {
        bondedNodes = getBondedNodes(writeResult);
      }
      catch (std::exception& ex) {
        WriteError error(WriteError::Type::NodeNotBonded, ex.what());
        writeResult.setError(error);
        return writeResult;
      }

      // filter out, which one's of the target nodes are bonded and which not
      // coordinator will be NOT filtered out, if it is in the target nodes
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

        WriteError error(WriteError::Type::NoBondedNodes, "No bonded nodes");
        writeResult.setError(error);
        return writeResult;
      }

      // if there are RF channel config. bytes, it is needed to check theirs values 
      // to be in accordance with coordinator's RF band
      try {
        checkRfChannelIfPresent(configBytes, writeResult);
      }
      catch (std::exception& ex) {
        WriteError error(WriteError::Type::NodeNotBonded, ex.what());
        writeResult.setError(error);
        return writeResult;
      }

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
      if (m_isSetSecurityPassword) {
        setSecurityString(writeResult, targetBondedNodes, m_securityPassword, true);
      }

      if (m_isSetSecurityUserKey) {
        setSecurityString(writeResult, targetBondedNodes, m_securityUserKey, false);
      }
      
      // disable - if previously enabled - FRC on the Coordinator
      try {
        if (m_frcEnabled) {
          setFrcOnCoord(writeResult, false);
        }
      }
      catch (std::exception& ex) {
        WriteError error(WriteError::Type::DisableFrc, ex.what());
        writeResult.setError(error);
      }
      
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

    uint16_t parseAndCheckDeviceAddr(const int deviceAddr) {
      if ((deviceAddr < 0) || (deviceAddr > 0xEF)) {
        THROW_EXC(
          std::out_of_range, "Device address outside of valid range. " << NAME_PAR_HEX("Address", deviceAddr)
        );
      }
      return deviceAddr;
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

    uint32_t checkUartBaudrate(const int uartBaudRate) {
      for (const uint32_t baudRate : BaudRates) {
        if (uartBaudRate == baudRate) {
          return uartBaudRate;
        }
      }

      THROW_EXC(std::out_of_range, "Unsupported UART baud rate: " << PAR(uartBaudRate));
    }

    // returns code of the specified baud rate
    uint8_t toBaudRateCode(const uint32_t uartBaudRate) {
      for (int i = 0; i < BAUD_RATES_SIZE; i++) {
        if (uartBaudRate == BaudRates[i]) {
          return i;
        }
      }

      THROW_EXC(std::out_of_range, "Unsupported UART baud rate: " << PAR(uartBaudRate));
    }

    std::vector<HWP_ConfigByte> parseAndCheckConfigBytes(ComMngIqmeshWriteConfig comWriteConfig) {
      std::vector<HWP_ConfigByte> configBytes;

      // byte 0x01 - configuration bits
      uint8_t byte01ConfigBits = 0;
      uint8_t byte01ConfigBitsMask = 0;
      bool isSetByte01ConfigBits = false;

      if (comWriteConfig.isSetCoordinator()) {
        if (comWriteConfig.getCoordinator()) {
          byte01ConfigBits |= 0b00000001;
        }
        byte01ConfigBitsMask |= 0b00000001;
        isSetByte01ConfigBits = true;
      }

      if (comWriteConfig.isSetNode()) {
        if (comWriteConfig.getNode()) {
          byte01ConfigBits |= 0b00000010;
        }
        byte01ConfigBitsMask |= 0b00000010;
        isSetByte01ConfigBits = true;
      }

      if (comWriteConfig.isSetOs()) {
        if (comWriteConfig.getOs()) {
          byte01ConfigBits |= 0b00000100;
        }
        byte01ConfigBitsMask |= 0b00000100;
        isSetByte01ConfigBits = true;
      }

      if (comWriteConfig.isSetEeprom()) {
        if (comWriteConfig.getEeprom()) {
          byte01ConfigBits |= 0b00001000;
        }
        byte01ConfigBitsMask |= 0b00001000;
        isSetByte01ConfigBits = true;
      }

      if (comWriteConfig.isSetEeeprom()) {
        if (comWriteConfig.getEeeprom()) {
          byte01ConfigBits |= 0b00010000;
        }
        byte01ConfigBitsMask |= 0b00010000;
        isSetByte01ConfigBits = true;
      }

      if (comWriteConfig.isSetRam()) {
        if (comWriteConfig.getRam()) {
          byte01ConfigBits |= 0b00100000;
        }
        byte01ConfigBitsMask |= 0b00100000;
        isSetByte01ConfigBits = true;
      }

      if (comWriteConfig.isSetLedr()) {
        if (comWriteConfig.getLedr()) {
          byte01ConfigBits |= 0b01000000;
        }
        byte01ConfigBitsMask |= 0b01000000;
        isSetByte01ConfigBits = true;
      }

      if (comWriteConfig.isSetLedg()) {
        if (comWriteConfig.getLedg()) {
          byte01ConfigBits |= 0b10000000;
        }
        byte01ConfigBitsMask |= 0b10000000;
        isSetByte01ConfigBits = true;
      }

      // if there is at minimal one bit set, add byte01
      if (isSetByte01ConfigBits) {
        HWP_ConfigByte byte01(0x01, byte01ConfigBits, byte01ConfigBitsMask);
        configBytes.push_back(byte01);
      }

      // byte 0x02 - configuration bits
      uint8_t byte02ConfigBits = 0;
      uint8_t byte02ConfigBitsMask = 0;
      bool isSetByte02ConfigBits = false;

      if (comWriteConfig.isSetSpi()) {
        if (comWriteConfig.getSpi()) {
          byte02ConfigBits |= 0b00000001;
        }
        byte02ConfigBitsMask |= 0b00000001;
        isSetByte02ConfigBits = true;
      }

      if (comWriteConfig.isSetIo()) {
        if (comWriteConfig.getIo()) {
          byte02ConfigBits |= 0b00000010;
        }
        byte02ConfigBitsMask |= 0b00000010;
        isSetByte02ConfigBits = true;
      }

      if (comWriteConfig.isSetThermometer()) {
        if (comWriteConfig.getThermometer()) {
          byte02ConfigBits |= 0b00000100;
        }
        byte02ConfigBitsMask |= 0b00000100;
        isSetByte02ConfigBits = true;
      }

      if (comWriteConfig.isSetPwm()) {
        if (comWriteConfig.getPwm()) {
          byte02ConfigBits |= 0b00001000;
        }
        byte02ConfigBitsMask |= 0b00001000;
        isSetByte02ConfigBits = true;
      }

      if (comWriteConfig.isSetUart()) {
        if (comWriteConfig.getUart()) {
          byte02ConfigBits |= 0b00010000;
        }
        byte02ConfigBitsMask |= 0b00010000;
        isSetByte02ConfigBits = true;
      }

      if (comWriteConfig.isSetFrc()) {
        if (comWriteConfig.getFrc()) {
          byte02ConfigBits |= 0b00100000;
        }
        byte02ConfigBitsMask |= 0b00100000;
        isSetByte02ConfigBits = true;
      }

      // if there is at minimal one bit set, add byte02
      if (isSetByte02ConfigBits) {
        HWP_ConfigByte byte02(0x02, byte02ConfigBits, byte02ConfigBitsMask);
        configBytes.push_back(byte02);
      }

      // main RF channel A of the main network
      if (comWriteConfig.isSetRfChannelA()) {
        uint8_t rfChannelA = checkRfChannel(comWriteConfig.getRfChannelA());
        HWP_ConfigByte rfChannelA_configByte(0x11, rfChannelA, 0xFF);
        configBytes.push_back(rfChannelA_configByte);
      }

      // main RF channel B of the main network
      if (comWriteConfig.isSetRfChannelB()) {
        uint8_t rfChannelB = checkRfChannel(comWriteConfig.getRfChannelB());
        HWP_ConfigByte rfChannelB_configByte(0x12, rfChannelB, 0xFF);
        configBytes.push_back(rfChannelB_configByte);
      }

      // Main RF channel A of the optional subordinate network
      if (comWriteConfig.isSetRfSubChannelA()) {
        uint8_t rfSubChannelA = checkRfChannel(comWriteConfig.getRfSubChannelA());
        HWP_ConfigByte rfSubChannelA_configByte(0x06, rfSubChannelA, 0xFF);
        configBytes.push_back(rfSubChannelA_configByte);
      }

      // Main RF channel B of the optional subordinate network
      if (comWriteConfig.isSetRfSubChannelB()) {
        uint8_t rfSubChannelB = checkRfChannel(comWriteConfig.getRfSubChannelB());
        HWP_ConfigByte rfSubChannelB_configByte(0x07, rfSubChannelB, 0xFF);
        configBytes.push_back(rfSubChannelB_configByte);
      }

      // RF output power
      if (comWriteConfig.isSetTxPower()) {
        uint8_t txPower = checkTxPower(comWriteConfig.getTxPower());
        HWP_ConfigByte txPower_configByte(0x08, txPower, 0xFF);
        configBytes.push_back(txPower_configByte);
      }

      // RF signal filter
      if (comWriteConfig.isSetRxFilter()) {
        uint8_t rxFilter = checkRxFilter(comWriteConfig.getRxFilter());
        HWP_ConfigByte rxFilter_configByte(0x09, rxFilter, 0xFF);
        configBytes.push_back(rxFilter_configByte);
      }

      // Timeout for receiving RF packets at LP mode at N device.
      if (comWriteConfig.isSetLpRxTimeout()) {
        uint8_t lpRxTimeout = checkLpRxTimeout(comWriteConfig.getLpRxTimeout());
        HWP_ConfigByte lpRxTimeout_configByte(0x0A, lpRxTimeout, 0xFF);
        configBytes.push_back(lpRxTimeout_configByte);
      }

      // an alternative DPA service mode channel
      if (comWriteConfig.isSetRfPgmAltChannel()) {
        uint8_t rfPgmAltChannel = checkRfPgmAltChannel(comWriteConfig.getRfPgmAltChannel());
        HWP_ConfigByte rfPgmAltChannel_configByte(0x0C, rfPgmAltChannel, 0xFF);
        configBytes.push_back(rfPgmAltChannel_configByte);
      }

      // Baud rate of the UART interface if one is used
      if (comWriteConfig.isSetUartBaudrate()) {
        uint32_t uartBaudrate = checkUartBaudrate(comWriteConfig.getUartBaudrate());
        HWP_ConfigByte uartBaudrate_configByte(0x0B, toBaudRateCode(uartBaudrate), 0xFF);
        configBytes.push_back(uartBaudrate_configByte);
      }


      // byte 0x05 - configuration bits
      uint8_t byte05ConfigBits = 0;
      uint8_t byte05ConfigBitsMask = 0;
      bool isSetByte05ConfigBits = false;

      if (comWriteConfig.isSetCustomDpaHandler()) {
        if (comWriteConfig.getCustomDpaHandler()) {
          byte05ConfigBits |= 0b00000001;
        }
        byte05ConfigBitsMask |= 0b00000001;
        isSetByte05ConfigBits = true;
      }

      if (comWriteConfig.isSetNodeDpaInterface()) {
        if (comWriteConfig.getNodeDpaInterface()) {
          byte05ConfigBits |= 0b00000010;
        }
        byte05ConfigBitsMask |= 0b00000010;
        isSetByte05ConfigBits = true;
      }

      if (comWriteConfig.isSetDpaAutoexec()) {
        if (comWriteConfig.getDpaAutoexec()) {
          byte05ConfigBits |= 0b00000100;
        }
        byte05ConfigBitsMask |= 0b00000100;
        isSetByte05ConfigBits = true;
      }

      if (comWriteConfig.isSetRoutingOff()) {
        if (comWriteConfig.getRoutingOff()) {
          byte05ConfigBits |= 0b00001000;
        }
        byte05ConfigBitsMask |= 0b00001000;
        isSetByte05ConfigBits = true;
      }

      if (comWriteConfig.isSetIoSetup()) {
        if (comWriteConfig.getIoSetup()) {
          byte05ConfigBits |= 0b00010000;
        }
        byte05ConfigBitsMask |= 0b00010000;
        isSetByte05ConfigBits = true;
      }

      if (comWriteConfig.isSetPeerToPeer()) {
        if (comWriteConfig.getPeerToPeer()) {
          byte05ConfigBits |= 0b00100000;
        }
        byte05ConfigBitsMask |= 0b00100000;
        isSetByte05ConfigBits = true;
      }

      // only for DPA 3.03 onwards - needs to control
      if (comWriteConfig.isSetNeverSleep()) {
        IIqrfDpaService::CoordinatorParameters coordParams = m_iIqrfDpaService->getCoordinatorParameters();
        uint16_t dpaVer = (coordParams.dpaVerMajor << 8) + coordParams.dpaVerMinor;

        if (dpaVer >= 0x0303) {
          if (comWriteConfig.getNeverSleep()) {
            byte05ConfigBits |= 0b01000000;
          }
          byte05ConfigBitsMask |= 0b01000000;
          isSetByte05ConfigBits = true;
        }
        else {
          THROW_EXC(std::logic_error, "NeverSleep parameter accessible from DPA v3.03");
        }
      }

      // if there is at minimal one bit set, add byte05
      if (isSetByte05ConfigBits) {
        HWP_ConfigByte byte05(0x05, byte05ConfigBits, byte05ConfigBitsMask);
        configBytes.push_back(byte05);
      }
     

      // RFPGM configuration bits
      uint8_t rfpgmConfigBits = 0;
      uint8_t rfpgmConfigBitsMask = 0;
      bool isSetRfpgmConfigBits = false;

      if (comWriteConfig.isSetRfPgmDualChannel()) {
        if (comWriteConfig.getRfPgmDualChannel()) {
          rfpgmConfigBits |= 0b00000011;
        }
        rfpgmConfigBitsMask |= 0b00000011;
        isSetRfpgmConfigBits = true;
      }

      if (comWriteConfig.isSetRfPgmLpMode()) {
        if (comWriteConfig.getRfPgmLpMode()) {
          rfpgmConfigBits |= 0b00000100;
        }
        rfpgmConfigBitsMask |= 0b00000100;
        isSetRfpgmConfigBits = true;
      }

      if (comWriteConfig.isSetRfPgmEnableAfterReset()) {
        if (comWriteConfig.getRfPgmEnableAfterReset()) {
          rfpgmConfigBits |= 0b00010000;
        }
        rfpgmConfigBitsMask |= 0b00010000;
        isSetRfpgmConfigBits = true;
      }
      
      if (comWriteConfig.isSetRfPgmTerminateAfter1Min()) {
        if (comWriteConfig.getRfPgmTerminateAfter1Min()) {
          rfpgmConfigBits |= 0b01000000;
        }
        rfpgmConfigBitsMask |= 0b01000000;
        isSetRfpgmConfigBits = true;
      }

      if (comWriteConfig.isSetRfPgmTerminateMcuPin()) {
        if (comWriteConfig.getRfPgmTerminateMcuPin()) {
          rfpgmConfigBits |= 0b10000000;
        }
        rfpgmConfigBitsMask |= 0b10000000;
        isSetRfpgmConfigBits = true;
      }

      // if there is at minimal one bit set, add RFPGM byte
      if (isSetRfpgmConfigBits) {
        HWP_ConfigByte rfpgmByte(0x20, rfpgmConfigBits, rfpgmConfigBitsMask);
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
      Pointer("/data/status").Set(response, SERVICE_ERROR);
      Pointer("/data/statusStr").Set(response, errorMsg);

      return response;
    }

    // creates error response about failed exclusive access
    rapidjson::Document getExclusiveAccessFailedResponse(
      const std::string& msgId,
      const IMessagingSplitterService::MsgType& msgType,
      const std::string& errorMsg
    )
    {
      rapidjson::Document response;

      Pointer("/mType").Set(response, msgType.m_type);
      Pointer("/data/msgId").Set(response, msgId);

      Pointer("/data/status").Set(response, SERVICE_ERROR_INTERNAL);
      Pointer("/data/statusStr").Set(response, errorMsg);

      return response;
    }

    // sets response VERBOSE data
    void setVerboseData(rapidjson::Document& response, WriteResult& writeResult)
    {
      // set raw fields, if verbose mode is active
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

    // sets status inside specified response accoding to specified error
    void setReponseStatus(Document& response, const WriteError& error) 
    {
      switch (error.getType()) {
        case WriteError::Type::NoError:
          Pointer("/data/status").Set(response, SERVICE_ERROR_NOERROR);
          break;
        case WriteError::Type::GetBondedNodes:
          Pointer("/data/status").Set(response, SERVICE_ERROR_GET_BONDED_NODES);
          break;
        case WriteError::Type::NoBondedNodes:
          Pointer("/data/status").Set(response, SERVICE_ERROR_NO_BONDED_NODES);
          break;
        case WriteError::Type::UpdateCoordChannelBand:
          Pointer("/data/status").Set(response, SERVICE_ERROR_UPDATE_COORD_CHANNEL_BAND);
          break;
        case WriteError::Type::EnableFrc:
          Pointer("/data/status").Set(response, SERVICE_ERROR_ENABLE_FRC);
          break;
        case WriteError::Type::DisableFrc:
          Pointer("/data/status").Set(response, SERVICE_ERROR_DISABLE_FRC);
          break;
        default:
          // some other unsupported error
          Pointer("/data/status").Set(response, SERVICE_ERROR);
          break;
      }

      if (error.getType() == WriteError::Type::NoError) {
        Pointer("/data/statusStr").Set(response, "ok");
      }
      else {
        Pointer("/data/statusStr").Set(response, error.getMessage());
      }
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

      // set common parameters
      Pointer("/mType").Set(response, msgType.m_type);
      Pointer("/data/msgId").Set(response, messagingId);

      // only one node - for the present time
      uint8_t firstAddr = writeResult.getDeviceAddrs().front();
      Pointer("/data/rsp/deviceAddr").Set(response, firstAddr);

      // checking of error
      WriteError error = writeResult.getError();

      if (error.getType() != WriteError::Type::NoError) {
        
        // do "normal" processing
        if (error.getType() == WriteError::Type::DisableFrc) {
          goto NORMAL_PROCESSING;
        }

        // set raw fields, if verbose mode is active
        if (comWriteConfig.getVerbose()) {
          setVerboseData(response, writeResult);
        }

        setReponseStatus(response, error);
        return response;
      }


      NORMAL_PROCESSING:

      // only one node - for the present time
      std::map<uint16_t, NodeWriteResult>::const_iterator iter = writeResult.getResultsMap().find(firstAddr);
      if (iter != writeResult.getResultsMap().end()) {
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

        // set raw fields, if verbose mode is active
        if (comWriteConfig.getVerbose()) {
          setVerboseData(response, writeResult);
        }

        // do "normal" processing
        if (error.getType() == WriteError::Type::DisableFrc) {
          setReponseStatus(response, error);
        }
        else {
          setReponseStatus(response, iter->second.getError());
        }

        return response;
      }
      
     
      // shouldn't reach this branch - would be probably an internal bug
      TRC_WARNING("Service internal error - no nodes in the result");

      // to fulfill formal requirements of json response schema 
      Pointer("/data/rsp/writeSuccess").Set(response, false);

      if (comWriteConfig.getVerbose()) {
        setVerboseData(response, writeResult);
      }

      Pointer("/data/status").Set(response, SERVICE_ERROR_INTERNAL);
      Pointer("/data/statusStr").Set(response, "Service internal error - no nodes in the result");

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
      if (msgType.m_type != m_mTypeName_iqmeshNetwork_WriteTrConf) {
        THROW_EXC(std::out_of_range, "Unsupported message type: " << PAR(msgType.m_type));
      }

      // creating representation object
      ComMngIqmeshWriteConfig comWriteConfig(doc);
      
      // service input parameters
      uint16_t deviceAddr;
      std::vector<HWP_ConfigByte> configBytes;
      
      try {
        m_repeat = parseAndCheckRepeat(comWriteConfig.getRepeat());

        if (!comWriteConfig.isSetDeviceAddr()) {
          THROW_EXC(std::logic_error, "deviceAddr not set");
        }
        deviceAddr = parseAndCheckDeviceAddr(comWriteConfig.getDeviceAddr());
      
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
        TRC_WARNING("Parsing service arguments failed." << PAR(ex.what()));

        Document failResponse = createCheckParamsFailedResponse(comWriteConfig.getMsgId(), msgType, ex.what());
        m_iMessagingSplitterService->sendMessage(messagingId, std::move(failResponse));

        TRC_FUNCTION_LEAVE("");
        return;
      }
      
      // just for temporary reasons
      std::list<uint16_t> deviceAddrs = 
      {
        deviceAddr
      };

      // try to establish exclusive access
      try {
        m_exclusiveAccess = m_iIqrfDpaService->getExclusiveAccess();
      }
      catch (std::exception &e) {
        const char* errorStr = e.what();
        TRC_WARNING("Error while establishing exclusive DPA access: " << PAR(errorStr));

        Document failResponse = getExclusiveAccessFailedResponse(comWriteConfig.getMsgId(), msgType, errorStr);
        m_iMessagingSplitterService->sendMessage(messagingId, std::move(failResponse));

        TRC_FUNCTION_LEAVE("");
        return;
      }

      // call service with checked params
      WriteResult writeResult = writeConfigBytes(configBytes, deviceAddrs);

      // release exclusive access
      m_exclusiveAccess.reset();

      // create and send response
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
        "WriteTrConfService instance activate" << std::endl <<
        "************************************"
      );

      // for the sake of register function parameters 
      std::vector<std::string> supportedMsgTypes =
      {
        m_mTypeName_iqmeshNetwork_WriteTrConf
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
        "WriteTrConfService instance deactivate" << std::endl <<
        "**************************************"
      );

      // for the sake of unregister function parameters 
      std::vector<std::string> supportedMsgTypes = 
      {
        m_mTypeName_iqmeshNetwork_WriteTrConf
      };

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



  WriteTrConfService::WriteTrConfService()
  {
    m_imp = shape_new Imp(*this);
  }

  WriteTrConfService::~WriteTrConfService()
  {
    delete m_imp;
  }


  void WriteTrConfService::attachInterface(iqrf::IIqrfDpaService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void WriteTrConfService::detachInterface(iqrf::IIqrfDpaService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void WriteTrConfService::attachInterface(iqrf::IMessagingSplitterService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void WriteTrConfService::detachInterface(iqrf::IMessagingSplitterService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void WriteTrConfService::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void WriteTrConfService::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }


  void WriteTrConfService::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void WriteTrConfService::deactivate()
  {
    m_imp->deactivate();
  }

  void WriteTrConfService::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
  }

}
