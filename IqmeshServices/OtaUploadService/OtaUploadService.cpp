#define IOtaUploadService_EXPORTS

#include "OtaUploadService.h"
#include "DataPreparer.h"
#include "Trace.h"
#include "ComIqmeshNetworkOtaUpload.h"
#include "ObjectFactory.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"

#include "iqrf__OtaUploadService.hxx"

#include <list>
#include <memory>
#include <math.h>
#include <bitset>

TRC_INIT_MODULE(iqrf::OtaUploadService);


using namespace rapidjson;

namespace {

  // maximum number of repeats
  static const uint8_t REPEAT_MAX = 3;

  // loading code action
  enum class LoadingAction {
    WithoutCodeLoading,
    WithCodeLoading
  };


  // service general fail code - may and probably will be changed later in the future
  static const int SERVICE_ERROR = 1000;

  static const int SERVICE_ERROR_NOERROR = 0;
  static const int SERVICE_ERROR_INTERNAL = SERVICE_ERROR + 1;
  static const int SERVICE_ERROR_UNSUPPORTED_LOADING_CONTENT = SERVICE_ERROR + 2;
  static const int SERVICE_ERROR_DATA_PREPARE = SERVICE_ERROR + 3;
  static const int SERVICE_ERROR_WRITE = SERVICE_ERROR + 4;
  static const int SERVICE_ERROR_LOAD = SERVICE_ERROR + 5;
};


namespace iqrf {

  // Holds information about errors, which encounter during upload
  class UploadError {
  public:
    
    enum class Type {
      NoError,
      UnsupportedLoadingContent,
      DataPrepare,
      Write,
      Load
    };

    UploadError() : m_type(Type::NoError), m_message("") {};
    UploadError(Type errorType) : m_type(errorType), m_message("") {};
    UploadError(Type errorType, const std::string& message) : m_type(errorType), m_message(message) {};

    Type getType() const { return m_type; };
    std::string getMessage() const { return m_message; };

    UploadError& operator=(const UploadError& error) {
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


  class UploadResult {
  private:
    UploadError m_error;

    // device addresses
    std::list<uint16_t> m_deviceAddrs;

    // map of upload results on nodes indexed by node ID
    std::map<uint16_t, bool> m_resultsMap;

    // map of upload errors on nodes indexed by node ID
    std::map<uint16_t, UploadError> m_errorsMap;

    // transaction results
    std::list<std::unique_ptr<IDpaTransactionResult2>> m_transResults;


  public:
    UploadError getError() const { return m_error; };

    void setError(const UploadError& error) {
      m_error = error;
    }

    std::list<uint16_t> getDeviceAddrs() const {
      return m_deviceAddrs;
    }

    void setDeviceAddrs(const std::list<uint16_t>& deviceAddrs) {
      m_deviceAddrs = deviceAddrs;
    }

    // puts specified result for specified node
    void putResult(uint16_t nodeAddr, bool result) {
      m_resultsMap[nodeAddr] = result;
    }

    // puts specified error for specified node
    void putError(uint16_t nodeAddr, const UploadError& error) {
      if (m_errorsMap.find(nodeAddr) != m_errorsMap.end()) {
       m_errorsMap.erase(nodeAddr);
      }
      m_errorsMap.insert(std::pair<uint8_t, UploadError>(nodeAddr, error));
    }

    const std::map<uint16_t, bool>& getResultsMap() const { return m_resultsMap; };

    const std::map<uint16_t, UploadError>& getErrorsMap() const { return m_errorsMap; };

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
  class OtaUploadService::Imp {
  private:
    // parent object
    OtaUploadService& m_parent;

    // message type: IQMESH Network OTA Upload
    // for temporal reasons
    const std::string m_mTypeName_iqmeshNetworkOtaUpload = "iqmeshNetwork_OtaUpload";
    //IMessagingSplitterService::MsgType* m_msgType_mngIqmeshWriteConfig;

    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    IIqrfDpaService* m_iIqrfDpaService = nullptr;

    // number of repeats
    uint8_t m_repeat;

    // if is set Verbose mode
    bool m_returnVerbose = false;

    // absolute path with hex file to upload
    std::string m_uploadPath;


  public:
    Imp(OtaUploadService& parent) : m_parent(parent)
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
    
    // indicates, whether will be used broadcast for loading data into EEEPROM
    bool willBeUsedBroadcast(const std::list<uint16_t>& deviceAddrs) {
      if (deviceAddrs.size() == 1) {
        if (deviceAddrs.front() == COORDINATOR_ADDRESS) {
          return false;
        }
      }
      return true;
    }
     
    // returns file suffix
    std::string getFileSuffix(const std::string& fileName) {
      size_t dotPos = fileName.find_last_of('.');
      if ((dotPos == std::string::npos)  || (dotPos == (fileName.length()-1))) {
        THROW_EXC(std::logic_error, "Bad file name format - no suffix");
      }

      return fileName.substr(dotPos+1);
    }

    // encounters type of loading content
    // TODO
    IOtaUploadService::LoadingContentType parseLoadingContentType(const std::string& fileName) 
    {
      std::string fileSuffix = getFileSuffix(fileName);

      if (fileSuffix == "hex") {
        return IOtaUploadService::LoadingContentType::Hex;
      }

      if (fileSuffix == "iqrf") {
        return IOtaUploadService::LoadingContentType::Iqrf_plugin;
      }

      THROW_EXC(std::logic_error, "Not implemented.");
    }

    // decides, whether there is coordinator address in the list
    bool isCoordinatorPresent(const std::list<uint16_t>& deviceAddrs) {
      for (uint16_t addr : deviceAddrs) {
        if (addr == COORDINATOR_ADDRESS) {
          return true;
        }
      }
      return false;
    }

    // sets PData of specified EEEPROM extended write packet
    void setExtendedWritePacketData(
      DpaMessage::DpaPacket_t& packet, uint16_t address, const std::basic_string<uint8_t>& data
    ) 
    {
      uns8* pData = packet.DpaRequestPacket_t.DpaMessage.Request.PData;
      pData[0] = address & 0xFF;
      pData[1] = (address >> 8) & 0xFF;

      for (int i = 0; i < data.size(); i++) {
        pData[i + 2] = data[i];
      }
    }

    // size of the embedded write packet INCLUDING target address, which to write data to
    const uint8_t EMB_WRITE_PACKET_HEADER_SIZE 
      = sizeof(TDpaIFaceHeader)-1 + 2;

 
    // sets specified EEEPROM extended write packet
    void setExtWritePacket(
      DpaMessage::DpaPacket_t& packet, 
      const uint16_t address, 
      const std::basic_string<uint8_t>& data,
      const uint16_t nodeAddr
    ) 
    {
      setExtendedWritePacketData(packet, address, data);
      packet.DpaRequestPacket_t.NADR = nodeAddr;
    }

    // adds embedded write packet data into batch data
    void addEmbeddedWritePacket(
      uint8_t* batchPacketPData, 
      const uint16_t address, 
      const std::basic_string<uint8_t>& data,
      const uint8_t offset
    ) {
      // length
      batchPacketPData[offset] = EMB_WRITE_PACKET_HEADER_SIZE + data.size();
      batchPacketPData[offset + 1] = PNUM_EEEPROM;
      batchPacketPData[offset + 2] = CMD_EEEPROM_XWRITE;
      batchPacketPData[offset + 3] = HWPID_DoNotCheck & 0xFF;
      batchPacketPData[offset + 4] = (HWPID_DoNotCheck >> 8 ) & 0xFF;

      batchPacketPData[offset + 5] = address & 0xFF;
      batchPacketPData[offset + 6] = (address >> 8) & 0xFF;

      for (int i = 0; i < data.size(); i++) {
        batchPacketPData[offset + EMB_WRITE_PACKET_HEADER_SIZE + i] = data[i];
      }
    }


    void writeDataToMemoryInOneNode(
      UploadResult& uploadResult,
      const uint16_t startMemAddress,
      const std::vector<std::basic_string<uint8_t>>& data,
      const uint16_t nodeAddress
    )
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION("Creating DPA request to execute batch command.");

      // eeeprom extended write packet
      DpaMessage::DpaPacket_t extendedWritePacket;
      extendedWritePacket.DpaRequestPacket_t.NADR = nodeAddress;
      extendedWritePacket.DpaRequestPacket_t.PNUM = PNUM_EEEPROM;
      extendedWritePacket.DpaRequestPacket_t.PCMD = CMD_EEEPROM_XWRITE;
      extendedWritePacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
      // PData will be specified inside loop in advance

      // batch packet
      DpaMessage::DpaPacket_t batchPacket;
      batchPacket.DpaRequestPacket_t.NADR = nodeAddress;
      batchPacket.DpaRequestPacket_t.PNUM = PNUM_OS;
      batchPacket.DpaRequestPacket_t.PCMD = CMD_OS_BATCH;
      batchPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
      // PData will be specified inside loop in advance 

      // batch request
      DpaMessage batchRequest;

      uint16_t actualAddress = startMemAddress;
      size_t index = 0;

      while (index < data.size()) {
        if ((index + 1) < data.size()) {
          if ((data[index].size() == 16) && (data[index + 1].size() == 16)) {
            // delete previous batch request data
            uns8* batchRequestData = batchPacket.DpaRequestPacket_t.DpaMessage.Request.PData;
            memset(batchRequestData, 0, DPA_MAX_DATA_LENGTH);

            // add 1st embedded packet into PData of the BATCH
            addEmbeddedWritePacket(batchRequestData, actualAddress, data[index], 0);
            actualAddress += data[index].size();

            // size of the first packet
            uint8_t firstPacketSize = EMB_WRITE_PACKET_HEADER_SIZE + data[index].size();

            // add 2nd embedded packet into PData of the BATCH
            addEmbeddedWritePacket(batchRequestData, actualAddress, data[index + 1], firstPacketSize);

            batchRequest.DataToBuffer(
              batchPacket.Buffer, 
              sizeof(TDpaIFaceHeader) 
              + 2* EMB_WRITE_PACKET_HEADER_SIZE 
              + data[index].size() 
              + data[index + 1].size()
              + 1 // end of BATCH
            );

            // issue batch request
            std::shared_ptr<IDpaTransaction2> batchTransaction;
            std::unique_ptr<IDpaTransactionResult2> transResult;

            for (int rep = 0; rep <= m_repeat; rep++) {
              try {
                batchTransaction = m_iIqrfDpaService->executeDpaTransaction(batchRequest);
                transResult = batchTransaction->get();
              }
              catch (std::exception& e) {
                TRC_DEBUG("DPA transaction error : " << e.what());

                if (rep < m_repeat) {
                  continue;
                }

                processUploadError(uploadResult, nodeAddress, UploadError::Type::Write, "DPA transaction error.");
                
                TRC_FUNCTION_LEAVE("");
                return;
              }

              TRC_DEBUG("Result from batch extended write transaction as string:" << PAR(transResult->getErrorString()));

              IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

              // because of the move-semantics
              DpaMessage dpaResponse = transResult->getResponse();
              uploadResult.addTransactionResult(transResult);

              if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
                TRC_INFORMATION("Batch extended write done!");
                TRC_DEBUG(
                  "DPA transaction: "
                  << NAME_PAR(batchRequest.PeripheralType(), batchRequest.NodeAddress())
                  << PAR(batchRequest.PeripheralCommand())
                );
                break;
              }
              else {
                // transaction error
                if (errorCode < 0) {
                  TRC_DEBUG("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

                  if (rep < m_repeat) {
                    continue;
                  }

                  processUploadError(uploadResult, nodeAddress, UploadError::Type::Write, "DPA transaction error.");

                  TRC_FUNCTION_LEAVE("");
                  return;
                }

                // DPA error
                TRC_DEBUG("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

                if (rep < m_repeat) {
                  continue;
                }

                processUploadError(uploadResult, nodeAddress, UploadError::Type::Write, "DPA error.");

                TRC_FUNCTION_LEAVE("");
                return;
              }
            }

            actualAddress += data[index + 1].size();
            index += 2;
          }
          else {
            DpaMessage extendedWriteRequest;
            setExtWritePacket(extendedWritePacket, actualAddress, data[index], nodeAddress);
            extendedWriteRequest.DataToBuffer(
              extendedWritePacket.Buffer, 
              sizeof(TDpaIFaceHeader) + 2 + data[index].size()
            );

            // issue extended write request
            std::shared_ptr<IDpaTransaction2> extWriteTransaction;
            std::unique_ptr<IDpaTransactionResult2> transResult;

            for (int rep = 0; rep <= m_repeat; rep++) {
              try {
                extWriteTransaction = m_iIqrfDpaService->executeDpaTransaction(extendedWriteRequest);
                transResult = extWriteTransaction->get();
              }
              catch (std::exception& e) {
                TRC_DEBUG("DPA transaction error : " << e.what());

                if (rep < m_repeat) {
                  continue;
                }
                processUploadError(uploadResult, nodeAddress, UploadError::Type::Write, "DPA transaction error.");

                TRC_FUNCTION_LEAVE("");
                return;
              }

              TRC_DEBUG("Result from extended write transaction as string:" << PAR(transResult->getErrorString()));

              IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

              // because of the move-semantics
              DpaMessage dpaResponse = transResult->getResponse();
              uploadResult.addTransactionResult(transResult);

              if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
                TRC_INFORMATION("Extended write done!");
                TRC_DEBUG(
                  "DPA transaction: "
                  << NAME_PAR(extendedWriteRequest.PeripheralType(), extendedWriteRequest.NodeAddress())
                  << PAR(extendedWriteRequest.PeripheralCommand())
                );
                break;
              }
              else {
                // transaction error
                if (errorCode < 0) {
                  TRC_DEBUG("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

                  if (rep < m_repeat) {
                    continue;
                  }

                  processUploadError(uploadResult, nodeAddress, UploadError::Type::Write, "Transaction error.");

                  TRC_FUNCTION_LEAVE("");
                  return;
                }

                // DPA error
                TRC_DEBUG("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

                if (rep < m_repeat) {
                  continue;
                }

                processUploadError(uploadResult, nodeAddress, UploadError::Type::Write, "DPA error.");

                TRC_FUNCTION_LEAVE("");
                return;
              }
            }

            actualAddress += data[index].size();
            index++;
          }
        }
        else {
          DpaMessage extendedWriteRequest;
          setExtWritePacket(extendedWritePacket, actualAddress, data[index], nodeAddress);
          extendedWriteRequest.DataToBuffer(
            extendedWritePacket.Buffer, sizeof(TDpaIFaceHeader) + 2 + data[index].size()
          );

          // issue extended write request
          std::shared_ptr<IDpaTransaction2> extWriteTransaction;
          std::unique_ptr<IDpaTransactionResult2> transResult;

          for (int rep = 0; rep <= m_repeat; rep++) {
            try {
              extWriteTransaction = m_iIqrfDpaService->executeDpaTransaction(extendedWriteRequest);
              transResult = extWriteTransaction->get();
            }
            catch (std::exception& e) {
              TRC_DEBUG("DPA transaction error : " << e.what());

              if (rep < m_repeat) {
                continue;
              }

              processUploadError(uploadResult, nodeAddress, UploadError::Type::Write, "DPA transaction error.");

              TRC_FUNCTION_LEAVE("");
              return;
            }

            TRC_DEBUG("Result from extended write transaction as string:" << PAR(transResult->getErrorString()));

            IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

            // because of the move-semantics
            DpaMessage dpaResponse = transResult->getResponse();
            uploadResult.addTransactionResult(transResult);

            if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
              TRC_INFORMATION("Extended write done!");
              TRC_DEBUG(
                "DPA transaction: "
                << NAME_PAR(extendedWriteRequest.PeripheralType(), extendedWriteRequest.NodeAddress())
                << PAR(extendedWriteRequest.PeripheralCommand())
              );
              break;
            }
            else {
              // transaction error
              if (errorCode < 0) {
                TRC_DEBUG("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

                if (rep < m_repeat) {
                  continue;
                }

                processUploadError(uploadResult, nodeAddress, UploadError::Type::Write, "Transaction error.");

                TRC_FUNCTION_LEAVE("");
                return;
              }

              // DPA error
              TRC_DEBUG("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

              if (rep < m_repeat) {
                continue;
              }

              processUploadError(uploadResult, nodeAddress, UploadError::Type::Write, "DPA error.");

              TRC_FUNCTION_LEAVE("");
              return;
            }
          }

          actualAddress += data[index].size();
          index++;
        }
      }

      // write into node result
      uploadResult.putResult(nodeAddress, true);
    }

    void setSelectedNodesForFrcRequest(
      TPerFrcSendSelective_Request* frcPacket,
      const std::list<uint16_t>& targetNodes
    )
    {
      // initialize "SelectedNodes" section
      memset(frcPacket->SelectedNodes, 0, 30 * sizeof(uns8));
      
      for (uint8_t i : targetNodes) {
        uns8 byteIndex = i / 8;
        uns8 bitIndex = i % 8;
        frcPacket->SelectedNodes[byteIndex] |= (uns8)pow(2, bitIndex);
      }
    }

    void setUserDataForFrcEeepromXWriteRequest(
      TPerFrcSendSelective_Request* frcPacket,
      const uint16_t startAddress,
      const std::basic_string<uint8_t>& data
    )
    {
      uns8* userData = frcPacket->UserData;

      // initialize user data to zero
      memset(userData, 0, 25 * sizeof(uns8));

      // copy foursome
      userData[0] = sizeof(TDpaIFaceHeader) - 1 + 2 + data.size();
      userData[1] = PNUM_EEEPROM;
      userData[2] = CMD_EEEPROM_XWRITE;
      userData[3] = HWPID_Default & 0xFF;
      userData[4] = (HWPID_Default >> 8) & 0xFF;

      // copy address
      userData[5] = startAddress & 0xFF;
      userData[6] = (startAddress >> 8) & 0xFF;

      // copy data
      for (size_t i = 0; i < data.size(); i++) {
        userData[7 + i] = data[i];
      }
    }

    void processUploadError(
      UploadResult& uploadResult, 
      uint16_t nodeId, 
      UploadError::Type errType, 
      const std::string& errMsg
    )
    {
      uploadResult.putResult(nodeId, false);
      UploadError uploadError(errType, errMsg);
      uploadResult.putError(nodeId, uploadError);
    }

    void processUploadError(
      UploadResult& uploadResult,
      const std::list<uint16_t>& failedNodes,
      UploadError::Type errType,
      const std::string& errMsg
    )
    {
      UploadError uploadError(errType, errMsg);

      for (uint8_t nodeId : failedNodes) {
        uploadResult.putResult(nodeId, false);
        uploadResult.putError(nodeId, uploadError);
      }
    }

    // parses 2bits FRC results into map: keys are nodes IDs, values are returned 2 bits
    std::map<uint16_t, uint8_t> parse2bitsFrcData(
      const std::basic_string<uint8_t>& frcData,
      const std::list<uint16_t>& targetNodes
    ) 
    {
      std::map<uint16_t, uint8_t> nodesResults;
      std::list<uint16_t>::const_iterator findIter;

      uint8_t nodeId = 0;
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

    // puts nodes results into overall load code results
    void putFrcResults(
      UploadResult& uploadResult,
      UploadError::Type resultType,
      const std::map<uint16_t, uint8_t>& nodesResultsMap
    )
    {
      for (const std::pair<uint16_t, uint8_t> p : nodesResultsMap) {
        if ((p.second & 0x01) == 0x01) {
          uploadResult.putResult(p.first, true);
        }
        else {
          uploadResult.putResult(p.first, false);

          std::string errorMsg;
          if (p.second == 0) {
            errorMsg = "Node device did not respond to FRC at all.";
          }
          else {
            errorMsg = "HWPID did not match HWPID of the device.";
          }

         UploadError uploadError(resultType, errorMsg);
         uploadResult.putError(p.first, uploadError);
        }
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

    // excludes failed nodes from the list according to results
    void excludeFailedNodes(
      std::list<uint16_t>& nodes, 
      const UploadResult& uploadResult
    )
    {
      const std::map<uint16_t, bool> resultsMap = uploadResult.getResultsMap();
      std::map<uint16_t, bool>::const_iterator resultIter = resultsMap.end();

      std::list<uint16_t>::iterator nodesIter = nodes.begin();

      while (nodesIter != nodes.end()) {
        resultIter = uploadResult.getResultsMap().find(*nodesIter);
        if (resultIter != resultsMap.end()) {
          if (resultIter->second == false) {
            nodesIter = nodes.erase(nodesIter);
            continue;
          }
        }
        nodesIter++;
      }
    }

    void removeUnsuccesfullNodes(
      std::list<uint16_t>& succNodes, 
      const std::list<uint16_t>& nodesToRemove
    )
    {
      for (const uint16_t node : nodesToRemove) {
        succNodes.remove(node);
      }
    }

    void writeDataToMemoryInNodes(
      UploadResult& uploadResult,
      const uint16_t startAddress,
      const std::vector<std::basic_string<uint8_t>>& data,
      const std::list<uint16_t>& deviceAddrs
    )
    {
      DpaMessage frcRequest;
      DpaMessage::DpaPacket_t frcPacket;
      frcPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      frcPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
      frcPacket.DpaRequestPacket_t.PCMD = CMD_FRC_SEND_SELECTIVE;
      frcPacket.DpaRequestPacket_t.HWPID = HWPID_Default;
      frcPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.FrcCommand = FRC_AcknowledgedBroadcastBits;

      TPerFrcSendSelective_Request* frcRequestPacket = &frcPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request;

      uint16_t actualAddress = startAddress;
      size_t index = 0;

      // list of nodes to write into
      std::list<uint16_t> nodesToWrite(deviceAddrs);
      
      // nodes, which were successfully written to so far
      std::list<uint16_t> succNodes(deviceAddrs);

      while ( (index < data.size()) && (!succNodes.empty()) ) {
        // will be write only to successfully written nodes - from previous data
        nodesToWrite = succNodes;

        setSelectedNodesForFrcRequest(frcRequestPacket, nodesToWrite);
        setUserDataForFrcEeepromXWriteRequest(frcRequestPacket, actualAddress, data[index]);

        frcRequest.DataToBuffer(
          frcPacket.Buffer,
          sizeof(TDpaIFaceHeader)
          + 1                           // FRC Command 
          + 30                          // nodes
          + sizeof(TDpaIFaceHeader) - 1 // embedded command header 
          + 2                           // target address for write 
          + data[index].size()          // size of data to write
        );

        // issue FRC request
        std::shared_ptr<IDpaTransaction2> frcTransaction;
        std::unique_ptr<IDpaTransactionResult2> transResult;

        // data from FRC
        std::basic_string<uns8> frcData;

        for (int rep = 0; rep <= m_repeat; rep++) {
          try {
            frcTransaction = m_iIqrfDpaService->executeDpaTransaction(frcRequest);
            transResult = frcTransaction->get();
          }
          catch (std::exception& e) {
            TRC_DEBUG("DPA transaction error : " << e.what());

            if (rep < m_repeat) {
              continue;
            }

            removeUnsuccesfullNodes(succNodes, nodesToWrite);
            processUploadError(uploadResult, nodesToWrite, UploadError::Type::Write, "FRC unsuccessful.");
            break;
          }

          TRC_DEBUG("Result from FRC extended write transaction as string:" << PAR(transResult->getErrorString()));

          IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

          // because of the move-semantics
          DpaMessage dpaResponse = transResult->getResponse();
          uploadResult.addTransactionResult(transResult);

          if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
            TRC_INFORMATION("FRC extended write done!");
            TRC_DEBUG(
              "DPA transaction: "
              << NAME_PAR(frcRequest.PeripheralType(), frcRequest.NodeAddress())
              << PAR(frcRequest.PeripheralCommand())
            );

            // getting response data
            uns8 status = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.Status;
            if ((status >= 0x00) && (status <= 0xEF)) {
              TRC_INFORMATION("FRC extended write successful.");
              frcData.append(dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData);
            }
            else {
              TRC_DEBUG("FRC Extended write NOT successful." << NAME_PAR("Status", status));

              if (rep < m_repeat) {
                continue;
              }

              removeUnsuccesfullNodes(succNodes, nodesToWrite);
              processUploadError(uploadResult, nodesToWrite, UploadError::Type::Write, "FRC unsuccessful.");
              break;
            }
          }
          else {
            // transaction error
            if (errorCode < 0) {
              TRC_DEBUG("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

              if (rep < m_repeat) {
                continue;
              }

              removeUnsuccesfullNodes(succNodes, nodesToWrite);
              processUploadError(uploadResult, nodesToWrite, UploadError::Type::Write, "FRC unsuccessful.");
              break;
            }

            // DPA error
            TRC_DEBUG("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

            if (rep < m_repeat) {
              continue;
            }

            removeUnsuccesfullNodes(succNodes, nodesToWrite);
            processUploadError(uploadResult, nodesToWrite, UploadError::Type::Write, "FRC unsuccessful.");
            break;
          }


          DpaMessage extraResultRequest;
          DpaMessage::DpaPacket_t extraResultPacket;
          extraResultPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
          extraResultPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
          extraResultPacket.DpaRequestPacket_t.PCMD = CMD_FRC_EXTRARESULT;
          extraResultPacket.DpaRequestPacket_t.HWPID = HWPID_Default;
          extraResultRequest.DataToBuffer(extraResultPacket.Buffer, sizeof(TDpaIFaceHeader));

          // getting FRC extra result
          std::shared_ptr<IDpaTransaction2> extraResultTransaction;

          try {
            extraResultTransaction = m_iIqrfDpaService->executeDpaTransaction(extraResultRequest);
            transResult = extraResultTransaction->get();
          }
          catch (std::exception& e) {
            TRC_DEBUG("DPA transaction error : " << e.what());

            if (rep < m_repeat) {
              continue;
            }

            removeUnsuccesfullNodes(succNodes, nodesToWrite);
            processUploadError(uploadResult, nodesToWrite, UploadError::Type::Write, "FRC unsuccessful.");
            break;
          }

          TRC_DEBUG("Result from FRC extended write extra result transaction as string:" << PAR(transResult->getErrorString()));

          errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

          // because of the move-semantics
          dpaResponse = transResult->getResponse();
          uploadResult.addTransactionResult(transResult);

          if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
            TRC_INFORMATION("FRC extended write extra result done!");
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
              TRC_DEBUG("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

              if (rep < m_repeat) {
                continue;
              }

              removeUnsuccesfullNodes(succNodes, nodesToWrite);
              processUploadError(uploadResult, nodesToWrite, UploadError::Type::Write, "FRC unsuccessful.");
              break;
            }

            // DPA error
            TRC_DEBUG("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

            if (rep < m_repeat) {
              continue;
            }

            removeUnsuccesfullNodes(succNodes, nodesToWrite);
            processUploadError(uploadResult, nodesToWrite, UploadError::Type::Write, "FRC unsuccessful.");
            break;
          }


          // FRC data parsing
          std::map<uint16_t, uint8_t> nodesResultsMap = parse2bitsFrcData(frcData, nodesToWrite);

          // putting nodes results into overall load code results
          putFrcResults(uploadResult, UploadError::Type::Write, nodesResultsMap);

          // update unsuccessful nodes for next iteration
          nodesToWrite = getUnsuccessfulNodes(nodesToWrite, nodesResultsMap);

          // if all nodes were successfull, go to the next chunk of data
          if (nodesToWrite.empty()) {
            break;
          }

          // if this is the last iteration, remove nodes, which failed to write into
          if (rep == m_repeat) {
            removeUnsuccesfullNodes(succNodes, nodesToWrite);
            processUploadError(uploadResult, nodesToWrite, UploadError::Type::Write, "Write failed.");
          }
        }

        // increase target address
        actualAddress += data[index].length();
        index++;
      }

      TRC_FUNCTION_LEAVE("");
    }

    // writes data into external EEPROM memory
    void writeDataToMemory(
      UploadResult& result,
      const uint16_t startMemAddr,
      const std::vector<std::basic_string<uint8_t>>& data,
      const std::list<uint16_t> deviceAddrs
    )
    {
      if (deviceAddrs.size() == 1) {
        writeDataToMemoryInOneNode(result, startMemAddr, data, deviceAddrs.front());
        return;
      }
      
      bool isCoordPresent = false;

      if (isCoordinatorPresent(deviceAddrs)) {
        writeDataToMemoryInOneNode(result, startMemAddr, data, 0);
        isCoordPresent = true;
      }

      // exclude coordinator's address from target device addresses
      if (isCoordPresent) {
        std::list<uint16_t> onlyNodesDeviceAddrs(deviceAddrs);
        onlyNodesDeviceAddrs.remove(0);
        writeDataToMemoryInNodes(result, startMemAddr, data, onlyNodesDeviceAddrs);
      }
      else {
        writeDataToMemoryInNodes(result, startMemAddr, data, deviceAddrs);
      }
    }

    // returns list of node addesses, which have successful results
    std::list<uint16_t> getSuccessfulResultNodes(const UploadResult& uploadResult)
    {
      std::list<uint16_t> successfulResultNodes;

      for (const std::pair<uint16_t, bool> p : uploadResult.getResultsMap()) {
        if (p.second == true) {
          successfulResultNodes.push_back(p.first);
        }
      }
      return successfulResultNodes;
    }

    // load code into into coordinator using unicast
    void loadCodeUnicast(
      const uint16_t startAddress,
      const LoadingAction loadingAction,
      const IOtaUploadService::LoadingContentType loadingContentType,
      const uint16_t length,
      const uint16_t checksum,
      UploadResult& uploadResult
    )
    {
      DpaMessage loadCodeRequest;
      DpaMessage::DpaPacket_t loadCodePacket;
      loadCodePacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      loadCodePacket.DpaRequestPacket_t.PNUM = PNUM_OS;
      loadCodePacket.DpaRequestPacket_t.PCMD = CMD_OS_LOAD_CODE;
      loadCodePacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;

      TPerOSLoadCode_Request* tOsLoadCodeRequest = &loadCodePacket.DpaRequestPacket_t.DpaMessage.PerOSLoadCode_Request;
      tOsLoadCodeRequest->Address = startAddress;
      tOsLoadCodeRequest->CheckSum = checksum;
      tOsLoadCodeRequest->Length = length;

      uint8_t flags = 0;
      if (loadingAction == LoadingAction::WithCodeLoading) {
        flags |= 0x01;
      }
      if (loadingContentType == IOtaUploadService::LoadingContentType::Iqrf_plugin) {
        flags |= 0x02;
      }
      tOsLoadCodeRequest->Flags = flags;

      loadCodeRequest.DataToBuffer(loadCodePacket.Buffer, sizeof(TDpaIFaceHeader) + 7);


      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> loadCodeTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      for (int rep = 0; rep <= m_repeat; rep++) {
        try {
          loadCodeTransaction = m_iIqrfDpaService->executeDpaTransaction(loadCodeRequest, 30000);
          transResult = loadCodeTransaction->get();
        }
        catch (std::exception& e) {
          TRC_DEBUG("DPA transaction error : " << e.what());

          if (rep < m_repeat) {
            continue;
          }

          processUploadError(uploadResult, COORDINATOR_ADDRESS, UploadError::Type::Load, e.what());

          TRC_FUNCTION_LEAVE("");
          return;
        }

        TRC_DEBUG("Result from loaad code transaction as string:" << PAR(transResult->getErrorString()));

        IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

        // because of the move-semantics
        DpaMessage dpaResponse = transResult->getResponse();
        uploadResult.addTransactionResult(transResult);

        if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
          TRC_INFORMATION("Load code done!");
          TRC_DEBUG(
            "DPA transaction: "
            << NAME_PAR(loadCodeRequest.PeripheralType(), loadCodeRequest.NodeAddress())
            << PAR(loadCodeRequest.PeripheralCommand())
          );

          uns8 responseResult = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData[0];
          if ((responseResult & 0x01) == 0x01) {
            TRC_INFORMATION("Load code successful.");
            return;
          }
          else {
            TRC_DEBUG("Load code NOT successful." << PAR("The checksum does not match."));
            
            if (rep < m_repeat) {
              continue;
            }
            
            processUploadError(uploadResult, COORDINATOR_ADDRESS, UploadError::Type::Load, "The checksum does not match.");
            
            TRC_FUNCTION_LEAVE("");
            return;
          }
        }

        // transaction error
        if (errorCode < 0) {
          TRC_DEBUG("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

          if (rep < m_repeat) {
            continue;
          }

          processUploadError(uploadResult, COORDINATOR_ADDRESS, UploadError::Type::Load, "Transaction error.");

          TRC_FUNCTION_LEAVE("");
          return;
        }

        // DPA error
        TRC_DEBUG("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

        if (rep < m_repeat) {
          continue;
        }

        processUploadError(uploadResult, COORDINATOR_ADDRESS, UploadError::Type::Load, "Dpa error.");

        TRC_FUNCTION_LEAVE("");
      }
    }

    void setUserDataForFrcLoadCodeRequest(
      TPerFrcSendSelective_Request* frcPacket,
      const LoadingAction loadingAction,
      const IOtaUploadService::LoadingContentType loadingContentType,
      const uint16_t startAddress,
      const uint16_t length,
      const uint16_t checksum
    )
    {
      uns8* userData = frcPacket->UserData;

      // initialize user data to zero
      memset(userData, 0, 25 * sizeof(uns8));

      // copy foursome
      userData[0] = sizeof(TDpaIFaceHeader) - 1
        + 1     // flags
        + 2     // address
        + 2     // length
        + 2     // checksum
      ;
      userData[1] = PNUM_OS;
      userData[2] = CMD_OS_LOAD_CODE;
      userData[3] = HWPID_Default & 0xFF;
      userData[4] = (HWPID_Default >> 8) & 0xFF;

      // flags
      userData[5] = 0;
      if (loadingAction == LoadingAction::WithCodeLoading) {
        userData[5] |= 0x01;
      }

      if (loadingContentType == IOtaUploadService::LoadingContentType::Iqrf_plugin) {
        userData[5] |= 0x02;
      }

      // address
      userData[6] = (uns8)(startAddress & 0xFF);
      userData[7] = (uns8)((startAddress >> 8) & 0xFF);

      // length
      userData[8] = (uns8)(length & 0xFF);
      userData[9] = (uns8)((length >> 8) & 0xFF);

      // checksum
      userData[10] = (uns8)(checksum & 0xFF);
      userData[11] = (uns8)((checksum >> 8) & 0xFF);
    }

    // load code into into nodes using broadcast
    void loadCodeBroadcast(
      const uint16_t startAddress,
      const LoadingAction loadingAction,
      const IOtaUploadService::LoadingContentType loadingContentType,
      const uint16_t length,
      const uint16_t checksum,
      const std::list<uint16_t>& targetNodes,
      UploadResult& uploadResult
    )
    {
      DpaMessage frcRequest;
      DpaMessage::DpaPacket_t frcPacket;
      frcPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      frcPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
      frcPacket.DpaRequestPacket_t.PCMD = CMD_FRC_SEND_SELECTIVE;
      frcPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
      frcPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.FrcCommand = FRC_AcknowledgedBroadcastBits;

      TPerFrcSendSelective_Request* frcRequestPacket = &frcPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request;


      // will be updated through iteration
      std::list<uint16_t> nodesToLoad(targetNodes);

      setSelectedNodesForFrcRequest(frcRequestPacket, nodesToLoad);
      setUserDataForFrcLoadCodeRequest(
        frcRequestPacket,
        loadingAction, 
        loadingContentType, 
        startAddress, 
        length,
        checksum
      );

      frcRequest.DataToBuffer(
        frcPacket.Buffer, 
        + sizeof(TDpaIFaceHeader)       // "main command header"
        + 1                             // FRC command ID
        + 30                            // target nodes
        + sizeof(TDpaIFaceHeader) - 1   // embedded header
        + 1                             // flags
        + 2                             // address
        + 2                             // length
        + 2                             // checksum
      );

      // issue FRC request
      std::shared_ptr<IDpaTransaction2> frcTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      // data from FRC
      std::basic_string<uns8> frcData;

      for (int rep = 0; rep <= m_repeat; rep++) {
        try {
          frcTransaction = m_iIqrfDpaService->executeDpaTransaction(frcRequest);
          transResult = frcTransaction->get();
        }
        catch (std::exception& e) {
          TRC_DEBUG("DPA transaction error : " << e.what());

          if (rep < m_repeat) {
            continue;
          }

          processUploadError(uploadResult, nodesToLoad, UploadError::Type::Load, "FRC unsuccessful.");
          break;
        }

        TRC_DEBUG("Result from FRC load code transaction as string:" << PAR(transResult->getErrorString()));

        IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

        // because of the move-semantics
        DpaMessage dpaResponse = transResult->getResponse();
        uploadResult.addTransactionResult(transResult);

        if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
          TRC_INFORMATION("FRC load code done!");
          TRC_DEBUG(
            "DPA transaction: "
            << NAME_PAR(frcRequest.PeripheralType(), frcRequest.NodeAddress())
            << PAR(frcRequest.PeripheralCommand())
          );

          // getting response data
          uns8 status = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.Status;
          if ((status >= 0x00) && (status <= 0xEF)) {
            TRC_INFORMATION("FRC load code successful.");
            frcData.append(
              dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.FrcData,
              DPA_MAX_DATA_LENGTH - sizeof(uns8)
            );
            break;
          }
          else {
            TRC_DEBUG("FRC Extended write NOT successful." << NAME_PAR("Status", status));

            if (rep < m_repeat) {
              continue;
            }

            processUploadError(uploadResult, nodesToLoad, UploadError::Type::Load, "FRC unsuccessful.");
            break;
          }
        }
        else {
          // transaction error
          if (errorCode < 0) {
            TRC_DEBUG("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

            if (rep < m_repeat) {
              continue;
            }

            processUploadError(uploadResult, nodesToLoad, UploadError::Type::Load, "FRC unsuccessful.");
            break;
          }
          else {
            // DPA error
            TRC_DEBUG("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

            if (rep < m_repeat) {
              continue;
            }

            processUploadError(uploadResult, nodesToLoad, UploadError::Type::Load, "FRC unsuccessful.");
            break;
          }
        }


        // getting FRC extra result
        DpaMessage extraResultRequest;
        DpaMessage::DpaPacket_t extraResultPacket;
        extraResultPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        extraResultPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
        extraResultPacket.DpaRequestPacket_t.PCMD = CMD_FRC_EXTRARESULT;
        extraResultPacket.DpaRequestPacket_t.HWPID = HWPID_Default;
        extraResultRequest.DataToBuffer(extraResultPacket.Buffer, sizeof(TDpaIFaceHeader));

        // issue request
        std::shared_ptr<IDpaTransaction2> extraResultTransaction;

        try {
          extraResultTransaction = m_iIqrfDpaService->executeDpaTransaction(extraResultRequest);
          transResult = extraResultTransaction->get();
        }
        catch (std::exception& e) {
          TRC_DEBUG("DPA transaction error : " << e.what());

          if (rep < m_repeat) {
            continue;
          }

          processUploadError(uploadResult, nodesToLoad, UploadError::Type::Load, "FRC unsuccessful.");

          TRC_FUNCTION_LEAVE("");
          return;
        }

        TRC_DEBUG("Result from FRC extra result transaction as string:" << PAR(transResult->getErrorString()));

        errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

        // because of the move-semantics
        dpaResponse = transResult->getResponse();
        uploadResult.addTransactionResult(transResult);

        if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
          TRC_INFORMATION("FRC load code extra result done!");
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
            TRC_DEBUG("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

            if (rep < m_repeat) {
              continue;
            }

            processUploadError(uploadResult, nodesToLoad, UploadError::Type::Load, "FRC unsuccessful.");
            break;
          }

          // DPA error
          TRC_DEBUG("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

          if (rep < m_repeat) {
            continue;
          }

          processUploadError(uploadResult, nodesToLoad, UploadError::Type::Load, "FRC unsuccessful.");
          break;
        }

        // FRC data parsing
        std::map<uint16_t, uint8_t> nodesResultsMap = parse2bitsFrcData(frcData, nodesToLoad);

        // putting nodes results into overall load code results
        putFrcResults(uploadResult, UploadError::Type::Load, nodesResultsMap);

        // update unsuccessful nodes for next iteration
        nodesToLoad = getUnsuccessfulNodes(nodesToLoad, nodesResultsMap);

        // if all nodes were successfull, go to the end
        if (nodesToLoad.empty()) {
          break;
        }
      }

      TRC_FUNCTION_LEAVE("");
    }



    // loads code into specified nodes
    void _loadCode(
      const uint16_t startAddress,
      const LoadingAction loadingAction,
      const IOtaUploadService::LoadingContentType loadingContentType,
      const uint16_t length,
      const uint16_t checksum,
      const std::list<uint16_t>& nodes,
      UploadResult& result
    )
    {
      std::list<uint16_t> pureNodes;
      bool isCoordPresent = false;

      // filter out non-coordinator nodes
      for (uint16_t nodeId : nodes) {
        if (nodeId == COORDINATOR_ADDRESS) {
          isCoordPresent = true;
        }
        else {
          pureNodes.push_back(nodeId);
        }
      }

      if (isCoordPresent) {
        loadCodeUnicast(startAddress, loadingAction, loadingContentType, length, checksum, result);
      }

      if (!pureNodes.empty()) {
        loadCodeBroadcast(
          startAddress, loadingAction, loadingContentType, length, checksum, pureNodes, result
        );
      }
    }

    // creates and returns full file name
    std::string getFullFileName(const std::string& uploadPath, const std::string& fileName) 
    {
      char fileSeparator;
      
      #if defined(WIN32) || defined(_WIN32)
          fileSeparator = '\\';
      #else
          fileSeparator = '/';
      #endif
      
      std::string fullFileName = uploadPath;
      
      if (uploadPath[uploadPath.size() - 1] != fileSeparator) {
        fullFileName += fileSeparator;
      }

      fullFileName += fileName;

      return fullFileName;
    }

    UploadResult upload(
      const std::list<uint16_t> deviceAddrs, 
      const std::string& fileName, 
      const uint16_t startMemAddr, 
      const LoadingAction loadingAction
    ) 
    {
      TRC_FUNCTION_ENTER("");
      
      // result
      UploadResult uploadResult;

      // set list of addresses of target nodes
      uploadResult.setDeviceAddrs(deviceAddrs);

      IOtaUploadService::LoadingContentType loadingContentType;
      try {
        loadingContentType = parseLoadingContentType(fileName);
      }
      catch (std::exception& ex) {
        UploadError error(UploadError::Type::UnsupportedLoadingContent, ex.what());
        uploadResult.setError(error);
        return uploadResult;
      }

      // indicates, whether the data will be prepared for broadcast or not 
      bool isDataForBroadcast = willBeUsedBroadcast(deviceAddrs);

      // prepare data to write and load
      std::unique_ptr<PreparedData> preparedData;
      try {
        preparedData = DataPreparer::prepareData(
          loadingContentType, fileName, isDataForBroadcast
        );
      }
      catch (std::exception& ex) {
        UploadError error(UploadError::Type::DataPrepare, ex.what());
        uploadResult.setError(error);
        return uploadResult;
      }
      
      // write prepared data into memory
      writeDataToMemory(uploadResult, startMemAddr, preparedData->getData(), deviceAddrs);

      // identify, which nodes were code successfully written into
      std::list<uint16_t> succWrittenNodes = getSuccessfulResultNodes(uploadResult);

      // if no nodes were successfully written into, it is useless to proceed with code load
      if (succWrittenNodes.empty()) {
        return uploadResult;
      }

      // final loading of code
      _loadCode(
        startMemAddr,
        loadingAction,
        loadingContentType,
        preparedData->getLength(),
        preparedData->getChecksum(),
        succWrittenNodes,
        uploadResult
      );

      TRC_FUNCTION_LEAVE("");
      return uploadResult;
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

    // sets response VERBOSE data
    void setVerboseData(rapidjson::Document& response, UploadResult& uploadResult)
    {
      rapidjson::Value rawArray(kArrayType);
      Document::AllocatorType& allocator = response.GetAllocator();

      while (uploadResult.isNextTransactionResult()) {
        std::unique_ptr<IDpaTransactionResult2> transResult = uploadResult.consumeNextTransactionResult();
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
    void setResponseStatus(Document& response, const UploadError& error)
    {
      switch (error.getType()) {
      case UploadError::Type::NoError:
        Pointer("/data/status").Set(response, SERVICE_ERROR_NOERROR);
        break;
      case UploadError::Type::DataPrepare:
        Pointer("/data/status").Set(response, SERVICE_ERROR_DATA_PREPARE);
        break;
      case UploadError::Type::UnsupportedLoadingContent:
        Pointer("/data/status").Set(response, SERVICE_ERROR_UNSUPPORTED_LOADING_CONTENT);
        break;
      case UploadError::Type::Write:
        Pointer("/data/status").Set(response, SERVICE_ERROR_WRITE);
        break;
      case UploadError::Type::Load:
        Pointer("/data/status").Set(response, SERVICE_ERROR_LOAD);
        break;
      default:
        // some other unsupported error
        Pointer("/data/status").Set(response, SERVICE_ERROR);
        break;
      }

      if (error.getType() == UploadError::Type::NoError) {
        Pointer("/data/statusStr").Set(response, "ok");
      }
      else {
        Pointer("/data/statusStr").Set(response, error.getMessage());
      }
    }

    // creates response on the basis of read TR config result
    Document createResponse(
      const std::string& msgId,
      const IMessagingSplitterService::MsgType& msgType,
      UploadResult& uploadResult,
      const ComIqmeshNetworkOtaUpload& comOtaUpload
    )
    {
      Document response;

      // set common parameters
      Pointer("/mType").Set(response, msgType.m_type);
      Pointer("/data/msgId").Set(response, msgId);

      // only one node - for the present time
      uint8_t firstAddr = uploadResult.getDeviceAddrs().front();
      Pointer("/data/rsp/deviceAddr").Set(response, firstAddr);
      
      // checking of error
      UploadError error = uploadResult.getError();

      if (error.getType() != UploadError::Type::NoError) {
        Pointer("/data/rsp/writeSuccess").Set(response, false);

        // set raw fields, if verbose mode is active
        if (comOtaUpload.getVerbose()) {
          setVerboseData(response, uploadResult);
        }

        setResponseStatus(response, error);
        return response;
      }


      std::map<uint16_t, bool>::const_iterator iter = uploadResult.getResultsMap().find(firstAddr);
      if (iter != uploadResult.getResultsMap().end()) {
        Pointer("/data/rsp/writeSuccess").Set(response, iter->second);

        // set raw fields, if verbose mode is active
        if (comOtaUpload.getVerbose()) {
          setVerboseData(response, uploadResult);
        }

        // write was successfull
        if (iter->second) {
          UploadError noError(UploadError::Type::NoError);
          setResponseStatus(response, noError);
        }
        else {
          std::map<uint16_t, UploadError>::const_iterator errorIter = uploadResult.getErrorsMap().find(firstAddr);
          if (errorIter != uploadResult.getErrorsMap().end()) {
            setResponseStatus(response, errorIter->second);
          }
          else {
            // shouldn't reach this branch - would be probably an internal bug
            TRC_WARNING("Service internal error - no error info");
            Pointer("/data/status").Set(response, SERVICE_ERROR_INTERNAL);
            Pointer("/data/statusStr").Set(response, "Service internal error - no error info");
          }
        }

        return response;
      }

      // shouldn't reach this branch - would be probably an internal bug
      TRC_WARNING("Service internal error - no nodes in the result");
      
      // to fullfill requirements of json response schema 
      Pointer("/data/rsp/writeSuccess").Set(response, false);

      if (comOtaUpload.getVerbose()) {
        setVerboseData(response, uploadResult);
      }

      Pointer("/data/status").Set(response, SERVICE_ERROR_INTERNAL);
      Pointer("/data/statusStr").Set(response, "Service internal error - no nodes in the result");
      
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

    uint16_t parseAndCheckDeviceAddr(const int& deviceAddr) {
      if ((deviceAddr < 0) || (deviceAddr > 0xEF)) {
        THROW_EXC(
          std::out_of_range, "Device address outside of valid range. " << NAME_PAR_HEX("Address", deviceAddr)
        );
      }
      return deviceAddr;
    }

    std::string checkFileName(const std::string& fileName) {
      if (fileName.empty()) {
        THROW_EXC(std::logic_error, "Empty file name.")
      }
      return fileName;
    }

    uint16_t parseAndCheckStartMemAddr(int startMemAddr) {
      if (startMemAddr < 0) {
        THROW_EXC(std::logic_error, "Start address must be nonnegative.");
      }
      return startMemAddr;
    }

    LoadingAction parseAndCheckLoadingAction(const std::string& loadingActionStr) {
      if (loadingActionStr == "WithoutCodeLoading") {
        return LoadingAction::WithoutCodeLoading;
      }

      if (loadingActionStr == "WithCodeLoading") {
        return LoadingAction::WithCodeLoading;
      }

      THROW_EXC(std::out_of_range, "Unsupported loading action: " << PAR(loadingActionStr));
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
      if (msgType.m_type != m_mTypeName_iqmeshNetworkOtaUpload) {
        THROW_EXC(std::logic_error, "Unsupported message type: " << PAR(msgType.m_type));
      }

      // creating representation object
      ComIqmeshNetworkOtaUpload comOtaUpload(doc);

      // if upload path (service's configuration parameter) is empty, return error message
      if (m_uploadPath.empty()) {
        Document failResponse = createCheckParamsFailedResponse(comOtaUpload.getMsgId(), msgType, "Empty upload path");
        m_iMessagingSplitterService->sendMessage(messagingId, std::move(failResponse));

        TRC_FUNCTION_LEAVE("");
        return;
      }

      // service input parameters
      uint16_t deviceAddr;
      std::string fileName;
      uint16_t startMemAddr;
      LoadingAction loadingAction;

      // parsing and checking service parameters
      try {
        m_repeat = parseAndCheckRepeat(comOtaUpload.getRepeat());
       
        if (!comOtaUpload.isSetDeviceAddr()) {
          THROW_EXC(std::logic_error, "deviceAddr not set");
        }
        deviceAddr = parseAndCheckDeviceAddr(comOtaUpload.getDeviceAddr());

        if (!comOtaUpload.isSetFileName()) {
          THROW_EXC(std::logic_error, "fileName not set");
        }
        fileName = checkFileName(comOtaUpload.getFileName());

        if (!comOtaUpload.isSetStartMemAddr()) {
          THROW_EXC(std::logic_error, "startMemAddr not set");
        }
        startMemAddr = parseAndCheckStartMemAddr(comOtaUpload.getStartMemAddr());

        if (!comOtaUpload.isSetLoadingAction()) {
          THROW_EXC(std::logic_error, "loadingAction not set");
        }
        loadingAction = parseAndCheckLoadingAction(comOtaUpload.getLoadingAction());

        m_returnVerbose = comOtaUpload.getVerbose();
      }
      // parsing and checking service parameters failed 
      catch (std::exception& ex) {
        Document failResponse = createCheckParamsFailedResponse(comOtaUpload.getMsgId(), msgType, ex.what());
        m_iMessagingSplitterService->sendMessage(messagingId, std::move(failResponse));

        TRC_FUNCTION_LEAVE("");
        return;
      }
      
      // construct full file name
      std::string fullFileName = getFullFileName(m_uploadPath, fileName);

      // just for temporary reasons
      std::list<uint16_t> deviceAddrs =
      {
        deviceAddr
      };

      // call service with checked params
      UploadResult uploadResult = upload(deviceAddrs, fullFileName, startMemAddr, loadingAction);

      // create and send response
      Document responseDoc = createResponse(comOtaUpload.getMsgId(), msgType, uploadResult, comOtaUpload);
      m_iMessagingSplitterService->sendMessage(messagingId, std::move(responseDoc));

      TRC_FUNCTION_LEAVE("");
    }
    

  public:
    void activate(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "************************************" << std::endl <<
        "OtaUploadService instance activate" << std::endl <<
        "************************************"
      );

      props->getMemberAsString("uploadPath", m_uploadPath);
      TRC_INFORMATION(PAR(m_uploadPath));

      if (m_uploadPath.empty()) {
        TRC_ERROR("Upload path is empty.");
      }

      // for the sake of register function parameters 
      std::vector<std::string> supportedMsgTypes = 
      {
        m_mTypeName_iqmeshNetworkOtaUpload
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
        "OtaUploadService instance deactivate" << std::endl <<
        "**************************************"
      );

      // for the sake of unregister function parameters 
      std::vector<std::string> supportedMsgTypes =
      {
        m_mTypeName_iqmeshNetworkOtaUpload
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



  OtaUploadService::OtaUploadService()
  {
    m_imp = shape_new Imp(*this);
  }

  OtaUploadService::~OtaUploadService()
  {
    delete m_imp;
  }


  void OtaUploadService::attachInterface(iqrf::IIqrfDpaService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void OtaUploadService::detachInterface(iqrf::IIqrfDpaService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void OtaUploadService::attachInterface(iqrf::IMessagingSplitterService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void OtaUploadService::detachInterface(iqrf::IMessagingSplitterService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void OtaUploadService::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void OtaUploadService::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }


  void OtaUploadService::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void OtaUploadService::deactivate()
  {
    m_imp->deactivate();
  }

  void OtaUploadService::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
  }

}
