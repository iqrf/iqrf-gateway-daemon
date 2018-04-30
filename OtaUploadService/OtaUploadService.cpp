#define IOtaUploadService_EXPORTS

#include "DpaTransactionTask.h"
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
  static enum class LoadingAction {
    WithoutCodeLoading,
    WithCodeLoading
  };


  // service general fail code - may and probably will be changed later in the future
  static const int SERVICE_ERROR = 1000;

  static const int SERVICE_ERROR_UNSUPPORTED_LOADING_CONTENT = SERVICE_ERROR + 1;
  static const int SERVICE_ERROR_DATA_PREPARE = SERVICE_ERROR + 2;
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
    std::list<uint8_t> m_deviceAddrs;

    // map of upload results on nodes indexed by node ID
    std::map<uint8_t, bool> m_resultsMap;

    // map of upload errors on nodes indexed by node ID
    std::map<uint8_t, UploadError> m_errorsMap;

    // transaction results
    std::list<std::unique_ptr<IDpaTransactionResult2>> m_transResults;


  public:
    UploadError getError() const { return m_error; };

    void setError(const UploadError& error) {
      m_error = error;
    }

    std::list<uint8_t> getDeviceAddrs() const {
      return m_deviceAddrs;
    }

    void setDeviceAddrs(const std::list<uint8_t>& deviceAddrs) {
      m_deviceAddrs = deviceAddrs;
    }

    // puts specified result for specified node
    void putResult(uint8_t nodeAddr, bool result) {
      m_resultsMap[nodeAddr] = result;
    }

    // puts specified error for specified node
    void putError(uint8_t nodeAddr, const UploadError& error) {
      if (m_errorsMap.find(nodeAddr) != m_errorsMap.end()) {
       m_errorsMap.erase(nodeAddr);
      }
      m_errorsMap.insert(std::pair<uint8_t, UploadError>(nodeAddr, error));
    }

    std::map<uint8_t, bool> getResultsMap() const { return m_resultsMap; };

    std::map<uint8_t, UploadError> getErrorsMap() const { return m_errorsMap; };

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


  public:
    Imp(OtaUploadService& parent) : m_parent(parent)
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
    
    // indicates, whether will be used broadcast for loading data into EEEPROM
    bool willBeUsedBroadcast(const std::list<uint8_t>& deviceAddrs) {
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
    bool isCoordinatorPresent(const std::list<uint8_t>& deviceAddrs) {
      for (uint8_t addr : deviceAddrs) {
        if (addr == COORDINATOR_ADDRESS) {
          return true;
        }
      }
      return false;
    }

    // sets PData of specified EEEPROM packet
    void setExtendedWritePacketData(
      DpaMessage::DpaPacket_t& packet, uint16_t address, const std::basic_string<uint8_t>& data
    ) 
    {
      uns8* pData = packet.DpaRequestPacket_t.DpaMessage.Request.PData;
      pData[0] = address & 0xFF;
      pData[1] = (address >> 8) & 0xFF;

      for (int i = 0; i < 16; i++) {
        pData[i + 2] = data[i];
      }
    }

    // sets specified EEEPROM extended write packet to be embedded in batch request
    void setEmbeddedExtWritePacket(
      DpaMessage::DpaPacket_t& packet, uint16_t address, const std::basic_string<uint8_t>& data
    ) 
    {
      setExtendedWritePacketData(packet, address, data);

      // on the position of NADR there must be the length of the packet
      packet.DpaRequestPacket_t.NADR
        = 1
        + sizeof(packet.DpaRequestPacket_t.PCMD)
        + sizeof(packet.DpaRequestPacket_t.PNUM)
        + sizeof(packet.DpaRequestPacket_t.HWPID)
        + 2
        + data.size();
    }

    // sets specified EEEPROM extended write packet for coordinator
    void setExtWritePacketForCoord(
      DpaMessage::DpaPacket_t& packet, uint16_t address, const std::basic_string<uint8_t>& data
    ) 
    {
      setExtendedWritePacketData(packet, address, data);
      packet.DpaRequestPacket_t.NADR = 0;
    }

    void writeDataToMemoryInCoordinator(
      UploadResult& uploadResult,
      const uint16_t startMemAddress,
      const std::vector<std::basic_string<uint8_t>>& data
    )
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION("Creating DPA request to execute batch command.");

      // eeeprom extended write packet
      DpaMessage::DpaPacket_t extendedWritePacket;
      extendedWritePacket.DpaRequestPacket_t.NADR = 0x00;
      extendedWritePacket.DpaRequestPacket_t.PNUM = PNUM_EEEPROM;
      extendedWritePacket.DpaRequestPacket_t.PCMD = CMD_EEEPROM_XWRITE;
      extendedWritePacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
      // PData will be specified inside loop in advance

      // batch packet
      DpaMessage::DpaPacket_t batchPacket;
      batchPacket.DpaRequestPacket_t.NADR = 0x00;
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
            memset(batchRequestData, 0, sizeof(batchRequestData));

            // set 1st embedded packet
            setEmbeddedExtWritePacket(extendedWritePacket, actualAddress, data[index]);
            actualAddress += data[index].size();

            // add 1st write packet into batch
            memcpy(batchRequestData, extendedWritePacket.Buffer, sizeof(extendedWritePacket.Buffer));

            // set 2nd embedded packet
            setEmbeddedExtWritePacket(extendedWritePacket, actualAddress, data[index + 1]);

            // add 2nd write packet into batch
            memcpy(batchRequestData + sizeof(extendedWritePacket.Buffer), extendedWritePacket.Buffer, sizeof(extendedWritePacket.Buffer));

            batchRequest.DataToBuffer(
              batchPacket.Buffer, 
              sizeof(TDpaIFaceHeader) + 2*(16 + 2) + 1
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

                processUploadError(uploadResult, COORDINATOR_ADDRESS, UploadError::Type::Write, "DPA transaction error.");
                
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

              // transaction error
              if (errorCode < 0) {
                TRC_DEBUG("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

                if (rep < m_repeat) {
                  continue;
                }

                processUploadError(uploadResult, COORDINATOR_ADDRESS, UploadError::Type::Write, "DPA transaction error.");

                TRC_FUNCTION_LEAVE("");
                return;
              }

              // DPA error
              TRC_DEBUG("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

              if (rep < m_repeat) {
                continue;
              }

              processUploadError(uploadResult, COORDINATOR_ADDRESS, UploadError::Type::Write, "DPA error.");

              TRC_FUNCTION_LEAVE("");
              return;
            }

            actualAddress += data[index + 1].size();
            index += 2;
          }
          else {
            DpaMessage extendedWriteRequest;
            setExtWritePacketForCoord(extendedWritePacket, actualAddress, data[index]);
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
                processUploadError(uploadResult, COORDINATOR_ADDRESS, UploadError::Type::Write, "DPA transaction error.");

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

              // transaction error
              if (errorCode < 0) {
                TRC_DEBUG("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

                if (rep < m_repeat) {
                  continue;
                }

                processUploadError(uploadResult, COORDINATOR_ADDRESS, UploadError::Type::Write, "Transaction error.");

                TRC_FUNCTION_LEAVE("");
                return;
              }

              // DPA error
              TRC_DEBUG("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

              if (rep < m_repeat) {
                continue;
              }

              processUploadError(uploadResult, COORDINATOR_ADDRESS, UploadError::Type::Write, "DPA error.");

              TRC_FUNCTION_LEAVE("");
              return;
            }

            actualAddress += data[index].size();
            index++;
          }
        }
        else {
          DpaMessage extendedWriteRequest;
          setExtWritePacketForCoord(extendedWritePacket, actualAddress, data[index]);
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

              processUploadError(uploadResult, COORDINATOR_ADDRESS, UploadError::Type::Write, "DPA transaction error.");

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

            // transaction error
            if (errorCode < 0) {
              TRC_DEBUG("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

              if (rep < m_repeat) {
                continue;
              }

              processUploadError(uploadResult, COORDINATOR_ADDRESS, UploadError::Type::Write, "Transaction error.");

              TRC_FUNCTION_LEAVE("");
              return;
            }

            // DPA error
            TRC_DEBUG("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

            if (rep < m_repeat) {
              continue;
            }

            processUploadError(uploadResult, COORDINATOR_ADDRESS, UploadError::Type::Write, "DPA error.");

            TRC_FUNCTION_LEAVE("");
            return;
          }

          actualAddress += data[index].size();
          index++;
        }
      }
    }

    void setSelectedNodesForFrcRequest(
      DpaMessage& frcRequest, const std::list<uint8_t>& targetNodes
    )
    {
      uns8* selectedNodes = frcRequest.DpaPacket().DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.SelectedNodes;

      for (uint8_t i : targetNodes) {
        uns8 byteIndex = i / 8;
        uns8 bitIndex = i % 8;
        selectedNodes[byteIndex] |= (uns8)pow(2, bitIndex);
      }
    }

    void setUserDataForFrcEeepromXWriteRequest(
      DpaMessage& frcRequest,
      const uint16_t startAddress,
      const std::basic_string<uint8_t>& data
    )
    {
      uns8* userData = frcRequest.DpaPacket().DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData;

      // copy foursome
      userData[0] = 2 + data.size() + 1 + 1 + 1 + 2;
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
      uploadResult.putResult(0, false);
      UploadError uploadError(errType, errMsg);
      uploadResult.putError(0, uploadError);
    }

    void processUploadError(
      UploadResult& uploadResult,
      const std::list<uint8_t>& failedNodes,
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
    std::map<uint8_t, uint8_t> parse2bitsFrcData(const std::basic_string<uint8_t>& frcData) {
      std::map<uint8_t, uint8_t> nodesResults;

      uint8_t nodeId = 0;
      for (int byteId = 0; byteId <= 29; byteId++) {
        int bitComp = 1;
        for (int bitId = 0; bitId < 8; bitId++) {
          uint8_t bit0 = ((frcData[byteId] & bitComp) == bitComp) ? 1 : 0;
          uint8_t bit1 = ((frcData[byteId + 32] & bitComp) == bitComp) ? 1 : 0;
          nodesResults.insert(std::pair<uint8_t, uint8_t>(nodeId, bit0 + 2 * bit1));
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
      const std::map<uint8_t, uint8_t>& nodesResultsMap
    )
    {
      for (const std::pair<uint8_t, uint8_t> p : nodesResultsMap) {
        if (p.second & 0x01 == 0x01) {
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

    // excludes failed nodes from the list according to results
    void excludeFailedNodes(
      std::list<uint8_t>& nodes, 
      const UploadResult& uploadResult
    )
    {
      const std::map<uint8_t, bool> resultsMap = uploadResult.getResultsMap();
      std::map<uint8_t, bool>::const_iterator resultIter = resultsMap.end();

      std::list<uint8_t>::iterator nodesIter = nodes.begin();

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

    void writeDataToMemoryInNodes(
      UploadResult& uploadResult,
      const uint16_t startAddress,
      const std::vector<std::basic_string<uint8_t>>& data,
      const std::list<uint8_t>& deviceAddrs
    )
    {
      DpaMessage frcRequest;
      DpaMessage::DpaPacket_t frcPacket;
      frcPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      frcPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
      frcPacket.DpaRequestPacket_t.PCMD = CMD_FRC_SEND_SELECTIVE;
      frcPacket.DpaRequestPacket_t.HWPID = HWPID_Default;
      frcRequest.DpaPacket().DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.FrcCommand = FRC_AcknowledgedBroadcastBits;

      DpaMessage extraResultRequest;
      DpaMessage::DpaPacket_t extraResultPacket;
      frcPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      frcPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
      frcPacket.DpaRequestPacket_t.PCMD = CMD_FRC_EXTRARESULT;
      frcPacket.DpaRequestPacket_t.HWPID = HWPID_Default;
      frcRequest.DataToBuffer(extraResultPacket.Buffer, sizeof(TDpaIFaceHeader));

      uint16_t actualAddress = startAddress;
      size_t index = 0;

      // because some nodes can fail during write - nodes in this list are OK
      std::list<uint8_t> nodesToWriteInto(deviceAddrs);

      while (index < data.size()) {
        setSelectedNodesForFrcRequest(frcRequest, nodesToWriteInto);
        setUserDataForFrcEeepromXWriteRequest(frcRequest, actualAddress, data[index]);

        frcRequest.DataToBuffer(
          frcPacket.Buffer, sizeof(TDpaIFaceHeader) + 1 + 30 + 7 + data[index].size()
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

            processUploadError(uploadResult, nodesToWriteInto, UploadError::Type::Write, "FRC unsuccessful.");

            TRC_FUNCTION_LEAVE("");
            return;
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
              break;
            }
            else {
              TRC_DEBUG("FRC Extended write NOT successful." << NAME_PAR("Status", status));

              if (rep < m_repeat) {
                continue;
              }

              processUploadError(uploadResult, nodesToWriteInto, UploadError::Type::Write, "FRC unsuccessful.");
            
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

            processUploadError(uploadResult, nodesToWriteInto, UploadError::Type::Write, "FRC unsuccessful.");
            
            TRC_FUNCTION_LEAVE("");
            return;
          }

          // DPA error
          TRC_DEBUG("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

          if (rep < m_repeat) {
            continue;
          }

          processUploadError(uploadResult, nodesToWriteInto, UploadError::Type::Write, "FRC unsuccessful.");

          TRC_FUNCTION_LEAVE("");
          return;
        }

        // getting FRC extra result
        std::shared_ptr<IDpaTransaction2> extraResultTransaction;

        for (int rep = 0; rep <= m_repeat; rep++) {
          try {
            extraResultTransaction = m_iIqrfDpaService->executeDpaTransaction(extraResultRequest);
            transResult = extraResultTransaction->get();
          }
          catch (std::exception& e) {
            TRC_DEBUG("DPA transaction error : " << e.what());

            if (rep < m_repeat) {
              continue;
            }

            processUploadError(uploadResult, nodesToWriteInto, UploadError::Type::Write, "FRC unsuccessful.");

            TRC_FUNCTION_LEAVE("");
            return;
          }

          TRC_DEBUG("Result from FRC extended write extra result transaction as string:" << PAR(transResult->getErrorString()));

          IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

          // because of the move-semantics
          DpaMessage dpaResponse = transResult->getResponse();
          uploadResult.addTransactionResult(transResult);

          if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
            TRC_INFORMATION("FRC extended write extra result done!");
            TRC_DEBUG(
              "DPA transaction: "
              << NAME_PAR(extraResultRequest.PeripheralType(), extraResultRequest.NodeAddress())
              << PAR(extraResultRequest.PeripheralCommand())
            );

            uns8 status = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.Status;
            if (status >= 0x00 && status <= 0xEF) {
              TRC_INFORMATION("FRC Extended write extra result successful.");
              frcData.append(dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData);
              break;
            }
            else {
              TRC_DEBUG("FRC Extended write extra result NOT successful." << NAME_PAR("Status", status));
              
              if (rep < m_repeat) {
                continue;
              }

              processUploadError(uploadResult, nodesToWriteInto, UploadError::Type::Write, "FRC unsuccessful.");
              
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

            processUploadError(uploadResult, nodesToWriteInto, UploadError::Type::Write, "FRC unsuccessful.");

            TRC_FUNCTION_LEAVE("");
            return;
          }

          // DPA error
          TRC_DEBUG("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

          if (rep < m_repeat) {
            continue;
          }

          processUploadError(uploadResult, nodesToWriteInto, UploadError::Type::Write, "FRC unsuccessful.");

          TRC_FUNCTION_LEAVE("");
          return;
        }

        // FRC data parsing
        std::map<uint8_t, uint8_t> nodesResultsMap = parse2bitsFrcData(frcData);

        // putting nodes results into overall load code results
        putFrcResults(uploadResult, UploadError::Type::Write, nodesResultsMap);

        // excluding nodes, which failed to write into
        excludeFailedNodes(nodesToWriteInto, uploadResult);

        // increase target address
        actualAddress += data[index].length();
        index++;
      }

    }

    // writes data into external EEPROM memory
    void writeDataToMemory(
      UploadResult& result,
      const uint16_t startMemAddr,
      const std::vector<std::basic_string<uint8_t>>& data,
      const std::list<uint8_t> deviceAddrs
    )
    {
      if (isCoordinatorPresent(deviceAddrs)) {
        writeDataToMemoryInCoordinator(result, startMemAddr, data);

        // coordinator is the only address
        if (deviceAddrs.size() == 1) {
          return;
        }
      }
      writeDataToMemoryInNodes(result, startMemAddr, data, deviceAddrs);
    }

    // returns list of node addesses, which have successful results
    std::list<uint8_t> getSuccessfulResultNodes(const UploadResult& uploadResult)
    {
      std::list<uint8_t> successfulResultNodes;

      for (const std::pair<uint8_t, bool> p : uploadResult.getResultsMap()) {
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
      loadCodePacket.DpaRequestPacket_t.HWPID = HWPID_Default;

      TPerOSLoadCode_Request* tOsLoadCodeRequest = &loadCodeRequest.DpaPacket().DpaRequestPacket_t.DpaMessage.PerOSLoadCode_Request;
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
      DpaMessage& frcRequest,
      const LoadingAction loadingAction,
      const IOtaUploadService::LoadingContentType loadingContentType,
      const uint16_t startAddress,
      const uint16_t length,
      const uint16_t checksum
    )
    {
      uns8* userData = frcRequest.DpaPacket().DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData;

      // copy foursome
      userData[0] = 1 + 1 + 1 + 2 + 7;
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
      const std::list<uint8_t>& targetNodes,
      UploadResult& uploadResult
    )
    {
      DpaMessage frcRequest;
      DpaMessage::DpaPacket_t frcPacket;
      frcPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      frcPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
      frcPacket.DpaRequestPacket_t.PCMD = CMD_FRC_SEND_SELECTIVE;
      frcPacket.DpaRequestPacket_t.HWPID = HWPID_Default;
      frcRequest.DpaPacket().DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.FrcCommand = FRC_AcknowledgedBroadcastBits;

      setSelectedNodesForFrcRequest(frcRequest, targetNodes);
      setUserDataForFrcLoadCodeRequest(
        frcRequest, loadingAction, loadingContentType, startAddress, length, checksum
      );

      frcRequest.DataToBuffer(frcPacket.Buffer, sizeof(TDpaIFaceHeader) + 12);

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

          processUploadError(uploadResult, targetNodes, UploadError::Type::Load, "FRC unsuccessful.");

          TRC_FUNCTION_LEAVE("");
          return;
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
            frcData.append(dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData);
            break;
          }
          else {
            TRC_DEBUG("FRC Extended write NOT successful." << NAME_PAR("Status", status));

            if (rep < m_repeat) {
              continue;
            }

            processUploadError(uploadResult, targetNodes, UploadError::Type::Load, "FRC unsuccessful.");

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

          processUploadError(uploadResult, targetNodes, UploadError::Type::Load, "FRC unsuccessful.");

          TRC_FUNCTION_LEAVE("");
          return;
        }

        // DPA error
        TRC_DEBUG("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

        if (rep < m_repeat) {
          continue;
        }

        processUploadError(uploadResult, targetNodes, UploadError::Type::Load, "FRC unsuccessful.");

        TRC_FUNCTION_LEAVE("");
        return;
      }

      // getting FRC extra result
      DpaMessage extraResultRequest;
      DpaMessage::DpaPacket_t extraResultPacket;
      frcPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      frcPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
      frcPacket.DpaRequestPacket_t.PCMD = CMD_FRC_EXTRARESULT;
      frcPacket.DpaRequestPacket_t.HWPID = HWPID_Default;
      frcRequest.DataToBuffer(extraResultPacket.Buffer, sizeof(TDpaIFaceHeader));

      // issue request
      std::shared_ptr<IDpaTransaction2> extraResultTransaction;

      for (int rep = 0; rep <= m_repeat; rep++) {
        try {
          extraResultTransaction = m_iIqrfDpaService->executeDpaTransaction(extraResultRequest);
          transResult = extraResultTransaction->get();
        }
        catch (std::exception& e) {
          TRC_DEBUG("DPA transaction error : " << e.what());

          if (rep < m_repeat) {
            continue;
          }

          processUploadError(uploadResult, targetNodes, UploadError::Type::Load, "FRC unsuccessful.");

          TRC_FUNCTION_LEAVE("");
          return;
        }

        TRC_DEBUG("Result from FRC extra result transaction as string:" << PAR(transResult->getErrorString()));

        IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

        // because of the move-semantics
        DpaMessage dpaResponse = transResult->getResponse();
        uploadResult.addTransactionResult(transResult);

        if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
          TRC_INFORMATION("FRC load code extra result done!");
          TRC_DEBUG(
            "DPA transaction: "
            << NAME_PAR(extraResultRequest.PeripheralType(), extraResultRequest.NodeAddress())
            << PAR(extraResultRequest.PeripheralCommand())
          );

          uns8 status = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.Status;
          if (status >= 0x00 && status <= 0xEF) {
            TRC_INFORMATION("FRC load code extra result successful.");
            frcData.append(dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData);
            break;
          }
          else {
            TRC_DEBUG("FRC load code extra result NOT successful." << NAME_PAR("Status", status));

            if (rep < m_repeat) {
              continue;
            }

            processUploadError(uploadResult, targetNodes, UploadError::Type::Load, "FRC unsuccessful.");

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

          processUploadError(uploadResult, targetNodes, UploadError::Type::Load, "FRC unsuccessful.");

          TRC_FUNCTION_LEAVE("");
          return;
        }

        // DPA error
        TRC_DEBUG("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

        if (rep < m_repeat) {
          continue;
        }

        processUploadError(uploadResult, targetNodes, UploadError::Type::Load, "FRC unsuccessful.");

        TRC_FUNCTION_LEAVE("");
        return;
      }

      // FRC data parsing
      std::map<uint8_t, uint8_t> nodesResultsMap = parse2bitsFrcData(frcData);

      // putting nodes results into overall load code results
      putFrcResults(uploadResult, UploadError::Type::Load, nodesResultsMap);

      TRC_FUNCTION_LEAVE("");
    }



    // loads code into specified nodes
    void _loadCode(
      const uint16_t startAddress,
      const LoadingAction loadingAction,
      const IOtaUploadService::LoadingContentType loadingContentType,
      const uint16_t length,
      const uint16_t checksum,
      const std::list<uint8_t>& nodes,
      UploadResult& result
    )
    {
      std::list<uint8_t> pureNodes;
      bool isCoordPresent = false;

      // filter out non-coordinator nodes
      for (uint8_t nodeId : nodes) {
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


    UploadResult upload(
      const std::list<uint8_t> deviceAddrs, 
      const std::string& fileName, 
      const uint16_t startMemAddr, 
      const LoadingAction loadingAction
    ) 
    {
      TRC_FUNCTION_ENTER("");
      
      // result
      UploadResult uploadResult;

      IOtaUploadService::LoadingContentType loadingContentType;
      try {
        loadingContentType = parseLoadingContentType(fileName);
      }
      catch (std::exception& ex) {
        UploadError error(UploadError::Type::UnsupportedLoadingContent, ex.what());
        uploadResult.setError(error);
        return uploadResult;
      }

      // set list of addresses of target nodes
      uploadResult.setDeviceAddrs(deviceAddrs);

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
      std::list<uint8_t> succWrittenNodes = getSuccessfulResultNodes(uploadResult);

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
      Pointer("/status").Set(response, SERVICE_ERROR);
      Pointer("/statusStr").Set(response, errorMsg);

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

    // creates response on the basis of read TR config result
    Document createResponse(
      const std::string& msgId,
      const IMessagingSplitterService::MsgType& msgType,
      UploadResult& uploadResult,
      const ComIqmeshNetworkOtaUpload& comReadTrConf
    )
    {
      Document response;

      // set common parameters
      Pointer("/mType").Set(response, msgType.m_type);
      Pointer("/data/msgId").Set(response, msgId);

      // checking of error
      UploadError error = uploadResult.getError();
      if (error.getType() != UploadError::Type::NoError) {
        Pointer("/data/statusStr").Set(response, error.getMessage());

        switch (error.getType()) {
          case UploadError::Type::DataPrepare:
            Pointer("/data/status").Set(response, SERVICE_ERROR_DATA_PREPARE);
            break;
          case UploadError::Type::UnsupportedLoadingContent:
            Pointer("/data/status").Set(response, SERVICE_ERROR_UNSUPPORTED_LOADING_CONTENT);
            break;
          default:
            Pointer("/data/status").Set(response, SERVICE_ERROR);
        }

        // set raw fields, if verbose mode is active
        if (comReadTrConf.getVerbose()) {
          setVerboseData(response, uploadResult);
        }
        return response;
      }

      // all is ok

      // get the first address in the device addresses list - because of json schema
      uint8_t firstAddr = uploadResult.getDeviceAddrs().front();
      Pointer("/data/rsp/deviceAddr").Set(response, firstAddr);

      std::map<uint8_t, bool>::const_iterator iter = uploadResult.getResultsMap().find(firstAddr);
      if (iter != uploadResult.getResultsMap().end()) {
        Pointer("/data/rsp/writeSuccess").Set(response, iter->second);
      }

      // status - ok
      Pointer("/status").Set(response, 0);
      Pointer("/statusStr").Set(response, "ok");

      // set raw fields, if verbose mode is active
      if (comReadTrConf.getVerbose()) {
        setVerboseData(response, uploadResult);
      }

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

    std::list<uint8_t> parseAndCheckDeviceAddr(const std::vector<int>& deviceAddrs) {
      if (deviceAddrs.empty()) {
        THROW_EXC(std::logic_error, "No device address.");
      }

      std::list<uint8_t> checkedAddrsList;

      for (int devAddr : deviceAddrs) {
        if ((devAddr < 0) || (devAddr > 0xEF)) {
          THROW_EXC(
            std::out_of_range, "Device address outside of valid range. " << NAME_PAR_HEX("Address", devAddr)
          );
        }
        checkedAddrsList.push_back(devAddr);
      }
      return checkedAddrsList;
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

      // service input parameters
      std::list<uint8_t> deviceAddrs;
      std::string fileName;
      uint16_t startMemAddr;
      LoadingAction loadingAction;

      // parsing and checking service parameters
      try {
        m_repeat = parseAndCheckRepeat(comOtaUpload.getRepeat());
       
        if (!comOtaUpload.isSetDeviceAddr()) {
          THROW_EXC(std::logic_error, "deviceAddr not set");
        }
        deviceAddrs = parseAndCheckDeviceAddr(comOtaUpload.getDeviceAddr());

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
      
      // call service with checked params
      UploadResult uploadResult = upload(deviceAddrs, fileName, startMemAddr, loadingAction);

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