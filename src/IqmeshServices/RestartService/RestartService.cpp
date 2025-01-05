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

#define IRestartService_EXPORTS

#include "RestartService.h"
#include "Trace.h"
#include "ComIqmeshNetworkRestart.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "iqrf__RestartService.hxx"
#include <list>
#include <memory>
#include <math.h>
#include <bitset>

TRC_INIT_MODULE(iqrf::RestartService);

using namespace rapidjson;

namespace
{
  static const int serviceError = 1000;
  static const int parsingRequestError = 1001;
  static const int exclusiveAccessError = 1002;
  static const int noBondedNodesError = 1003;
};

namespace iqrf {

  // Holds information about result of read Tr configuration
  class RestartResult
  {
  private:
    // Status
    int m_status = 0;
    std::string m_statusStr = "ok";

    // Bonded nodes
    std::basic_string<uint8_t> m_bondedNodes;

    // Restart result
    std::map<uint16_t, bool> m_restartResult;

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

    const std::map<uint16_t, bool> &getRestartResult() const
    {
      return m_restartResult;
    }

    void setRestartResult(const uint16_t address, const bool result)
    {
      m_restartResult[address] = result;
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
  class RestartService::Imp {
  private:
    // Parent object
    RestartService& m_parent;

    // Message type: IQMESH Network Read TR Configuration
    const std::string m_mTypeName_iqmeshNetworkRestart = "iqmeshNetwork_Restart";
    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    IIqrfDpaService* m_iIqrfDpaService = nullptr;
    std::unique_ptr<IIqrfDpaService::ExclusiveAccess> m_exclusiveAccess;
    const std::string* m_messagingId = nullptr;
    const IMessagingSplitterService::MsgType* m_msgType = nullptr;
    const ComIqmeshNetworkRestart* m_comRestart = nullptr;

    // Service input parameters
    TRestartInputParams m_restartParams;

  public:
    explicit Imp(RestartService& parent) : m_parent(parent)
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
    IDpaTransaction2::FrcResponseTime setFrcReponseTime(RestartResult& restartResult, IDpaTransaction2::FrcResponseTime FRCresponseTime)
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
        m_exclusiveAccess->executeDpaTransactionRepeat(setFrcParamRequest, transResult, m_restartParams.repeat);
        TRC_DEBUG("Result from Set Hops transaction as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("Set Hops successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, setFrcParamRequest.PeripheralType())
          << NAME_PAR(Node address, setFrcParamRequest.NodeAddress())
          << NAME_PAR(Command, (int)setFrcParamRequest.PeripheralCommand())
        );
        restartResult.addTransactionResult(transResult);
        TRC_FUNCTION_LEAVE("");
        return((IDpaTransaction2::FrcResponseTime)dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSetParams_RequestResponse.FrcParams);
      }
      catch (const std::exception& e)
      {
        restartResult.setStatus(transResult->getErrorCode(), e.what());
        restartResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //-----------------------------
    // Returns list of bonded nodes
    //-----------------------------
    std::basic_string<uint8_t> getBondedNodes(RestartResult& restartResult)
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
        m_exclusiveAccess->executeDpaTransactionRepeat(getBondedNodesRequest, transResult, m_restartParams.repeat);
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
        restartResult.addTransactionResult(transResult);
        std::basic_string<uint8_t> bondedNodes = bitmapToNodes(dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData);
        restartResult.setNodesList(bondedNodes);
        TRC_FUNCTION_LEAVE("");
        return(bondedNodes);
      }
      catch (const std::exception& e)
      {
        restartResult.setStatus(transResult->getErrorCode(), e.what());
        restartResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //------------------------------
    // FRC_AcknowledgedBroadcastBits
    //------------------------------
    TPerFrcSend_Response FRCAcknowledgedBroadcastBits(RestartResult& restartResult, const uint8_t PNUM, const uint8_t PCMD, const uint16_t hwpId, const std::basic_string<uint8_t>& data)
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
        m_exclusiveAccess->executeDpaTransactionRepeat(frcAckBroadcastRequest, transResult, m_restartParams.repeat);
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
        if (status <= MAX_ADDRESS)
        {
          // Add FRC result
          restartResult.addTransactionResult(transResult);
          TRC_INFORMATION("FRC_AcknowledgedBroadcastBits OK." << NAME_PAR_HEX("Status", (int)status));
          TRC_FUNCTION_LEAVE("");
          return dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response;
        }
        else
        {
          TRC_WARNING("FRC_AcknowledgedBroadcastBits NOK." << NAME_PAR_HEX("Status", (int)status));
          THROW_EXC(std::logic_error, "Bad FRC status: " << PAR((int)status));
        }
      }
      catch (std::exception& e)
      {
        restartResult.setStatus(transResult->getErrorCode(), e.what());
        restartResult.addTransactionResult(transResult);
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
      Pointer("/data/msgId").Set(response, m_comRestart->getMsgId());

      // Set status
      Pointer("/data/status").Set(response, status);
      Pointer("/data/statusStr").Set(response, statusStr);

      // Send message
      m_iMessagingSplitterService->sendMessage(*m_messagingId, std::move(response));
    }

    //--------------------------
    // Creates and send response
    //--------------------------
    void createResponse(RestartResult& restartResult)
    {
      Document response;

      // Set common parameters
      Pointer("/mType").Set(response, m_msgType->m_type);
      Pointer("/data/msgId").Set(response, m_comRestart->getMsgId());

      // hwpId
      Pointer("/data/rsp/hwpId").Set(response, m_restartParams.hwpId);

      // nodesNr
      Pointer("/data/rsp/nodesNr").Set(response, restartResult.getNodesList().length());

      // If success, populate restart result
      int status = restartResult.getStatus();
      if (status == 0) {
        // inaccessibleNodesNr
        Pointer("/data/rsp/inaccessibleNodesNr").Set(response, restartResult.getInaccessibleNodes());

        // Array of objects
        Document::AllocatorType &allocator = response.GetAllocator();
        rapidjson::Value frcRestartResult(kArrayType);
        std::map<uint16_t, bool> result = restartResult.getRestartResult();
        for (std::map<uint16_t, bool>::iterator i = result.begin(); i != result.end(); ++i)
        {
          rapidjson::Value frcRestartResultItem(kObjectType);
          frcRestartResultItem.AddMember("address", i->first, allocator);
          frcRestartResultItem.AddMember("result", i->second, allocator);
          frcRestartResult.PushBack(frcRestartResultItem, allocator);
        }
        Pointer("/data/rsp/restartResult").Set(response, frcRestartResult);
      }

      // Set raw fields, if verbose mode is active
      if (m_comRestart->getVerbose())
      {
        rapidjson::Value rawArray(kArrayType);
        Document::AllocatorType& allocator = response.GetAllocator();
        while (restartResult.isNextTransactionResult())
        {
          std::unique_ptr<IDpaTransactionResult2> transResult = restartResult.consumeNextTransactionResult();
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
      Pointer("/data/statusStr").Set(response, restartResult.getStatusStr());

      // Send message
      m_iMessagingSplitterService->sendMessage(*m_messagingId, std::move(response));
    }

    //----------------
    // Restart service
    //----------------
    void restart(RestartResult& restartResult)
    {
      TRC_FUNCTION_ENTER("");
      try
      {
        // Get bonded nodes
        getBondedNodes(restartResult);
        if (restartResult.getNodesList().size() == 0) {
          std::string errorStr = "There are no bonded nodes in network.";
          restartResult.setStatus(noBondedNodesError, errorStr);
          THROW_EXC(std::logic_error, errorStr);
        }

        // Set calculated FRC response time
        m_iIqrfDpaService->setFrcResponseTime(IDpaTransaction2::FrcResponseTime::k40Ms);
        IDpaTransaction2::FrcResponseTime FRCresponseTime = setFrcReponseTime(restartResult, IDpaTransaction2::FrcResponseTime::k40Ms);

        // Restart nodes
        TPerFrcSend_Response response = FRCAcknowledgedBroadcastBits(restartResult, PNUM_OS, CMD_OS_RESTART, m_restartParams.hwpId, std::basic_string<uint8_t> {});

        // Finally set FRC param back to initial value
        m_iIqrfDpaService->setFrcResponseTime(FRCresponseTime);
        setFrcReponseTime(restartResult, FRCresponseTime);

        // Check the response
        uint8_t inaccessibleNodes = 0;
        for (uint8_t addr : restartResult.getNodesList())
        {
          uint8_t byteIndex = addr / 8;
          uint8_t bitIndex = addr % 8;
          uint8_t bitMask = 1 << bitIndex;
          bool nodeOnline = ((response.FrcData[byteIndex] & bitMask) != 0);
          if (nodeOnline == false)
            inaccessibleNodes++;
          restartResult.setRestartResult(addr, nodeOnline);
          restartResult.setInaccessibleNodes(inaccessibleNodes);
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
      if (msgType.m_type != m_mTypeName_iqmeshNetworkRestart)
        THROW_EXC(std::logic_error, "Unsupported message type: " << PAR(msgType.m_type));

      // Creating representation object
      ComIqmeshNetworkRestart comRestart(doc);
      m_msgType = &msgType;
      m_messagingId = &messagingId;
      m_comRestart = &comRestart;

      // Parsing and checking service parameters
      try
      {
        m_restartParams = comRestart.getRestartInputParams();
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
        RestartResult restartResult;

        // Restart nodes
        restart(restartResult);

        // Create and send response
        createResponse(restartResult);
      } catch (const std::exception& e) {
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
        "ReadTrConfService instance activate" << std::endl <<
        "************************************"
      );

      (void)props;

      // for the sake of register function parameters
      std::vector<std::string> supportedMsgTypes =
      {
        m_mTypeName_iqmeshNetworkRestart
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
        m_mTypeName_iqmeshNetworkRestart
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

  RestartService::RestartService()
  {
    m_imp = shape_new Imp(*this);
  }

  RestartService::~RestartService()
  {
    delete m_imp;
  }

  void RestartService::attachInterface(iqrf::IIqrfDpaService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void RestartService::detachInterface(iqrf::IIqrfDpaService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void RestartService::attachInterface(iqrf::IMessagingSplitterService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void RestartService::detachInterface(iqrf::IMessagingSplitterService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void RestartService::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void RestartService::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

  void RestartService::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void RestartService::deactivate()
  {
    m_imp->deactivate();
  }

  void RestartService::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
  }
}