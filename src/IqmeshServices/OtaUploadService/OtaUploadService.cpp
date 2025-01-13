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

#define IOtaUploadService_EXPORTS

#include "IntelHexParser.h"
#include "IqrfParser.h"
#include "OtaUploadService.h"
#include "Trace.h"
#include "ComIqmeshNetworkOtaUpload.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"

#include "iqrf__OtaUploadService.hxx"

#include <list>
#include <memory>
#include <math.h>
#include <bitset>
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>
#include <fstream>

#define MCU_TYPE_BITS 0x07
#define EEEPROM_PAGE_WRITE_TIME 5 // 5 ms per page

TRC_INIT_MODULE(iqrf::OtaUploadService);

using namespace rapidjson;

namespace
{
  // Loading code action
  enum class LoadingAction
  {
    Upload,
    Verify,
    Load,
    Undefinded = 0xff
  };

  // Size of the embedded write packet INCLUDING target address, which to write data to
  const uint8_t EMB_WRITE_PACKET_HEADER_SIZE = sizeof(TDpaIFaceHeader) - 1 + 2;

  static const int serviceError = 1000;
  static const int parsingRequestError = 1001;
  static const int exclusiveAccessError = 1002;
  static const int emptyUploadPathError = 1003;
  static const int uploadFileProcessingError = 1004;
  static const int invalidEepromAddress = 1005;
  static const int eepromContentNotUploaded = 1006;
  static const int incompatibleDevice = 1007;
  static const int noDevices = 1008;
  static const int deviceOffline = 1009;
  static const int noDeviceMatchedHwpid = 1010;

  static const std::string noDevicesStr("No devices in network.");
  static const std::string deviceOfflineStr("One or more devices were offline during the upload process.");
  static const std::string deviceHexIncompatibleStr("Selected HEX is incompatible with target device.");
  static const std::string deviceIqrfIncompatibleStr("Selected IQRF plugin is incompatible with target device.");
  static const std::string networkHexIncompatibleStr("Network contains device(s) incompatible with selected HEX.");
  static const std::string networkIqrfIncompatibleStr("Network contains device(s) incompatible with selected IQRF plugin.");
  static const std::string noDeviceMatchedHwpidStr("No device in network matched specified hwpid.");
};

namespace iqrf
{
  class UploadResult
  {
  public:
    UploadResult() = delete;
    UploadResult(LoadingAction action)
    {
      m_loadingAction = action;
      m_nodesList.clear();
      m_compatibleDevicesMap.clear();
      m_verifyResultMap.clear();
      m_loadResultMap.clear();
    }

  private:
    // Status
    int m_status = 0;
    std::string m_statusStr = "ok";

    // Loading action
    LoadingAction m_loadingAction;

    // Upload results
    bool m_uploadResult;

    std::basic_string<uint8_t> m_nodesList;

    // Map of compatible devices
    std::map<uint8_t, bool> m_compatibleDevicesMap;

    // Map of verify results
    std::map<uint16_t, bool> m_verifyResultMap;

    // Map of load result
    std::map<uint16_t, bool> m_loadResultMap;

    // List of transaction results
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

    LoadingAction getLoadingAction() const
    {
      return m_loadingAction;
    }

    void putUploadResult(const bool result)
    {
      m_uploadResult = result;
    }

    bool getUploadResult() const
    {
      return m_uploadResult;
    }

    const std::basic_string<uint8_t> &getNodesList() const
    {
      return m_nodesList;
    }

    bool isCompatible(const uint8_t &deviceAddr) {
      if (m_compatibleDevicesMap.find(deviceAddr) == m_compatibleDevicesMap.end()) {
        return false;
      }
      return m_compatibleDevicesMap[deviceAddr];
    }

    const std::map<uint8_t, bool> &getCompatibleDevicesMap() const {
      return m_compatibleDevicesMap;
    }

    void setDeviceCompatibility(const uint8_t address, const bool compatible) {
      m_compatibleDevicesMap[address] = compatible;
    }

    const std::map<uint16_t, bool> &getVerifyResultsMap() const
    {
      return m_verifyResultMap;
    }

    const std::map<uint16_t, bool> &getLoadResultsMap() const
    {
      return m_loadResultMap;
    }

    void setNodesList(const std::basic_string<uint8_t> &nodesList)
    {
      m_nodesList = nodesList;
    }

    bool getVerifyResult(const uint16_t address)
    {
      return m_verifyResultMap.find(address)->second;
    }

    void setVerifyResultsMap(const uint16_t address, const bool result)
    {
      m_verifyResultMap[address] = result;
    }

    void setLoadResultsMap(const uint16_t address, const bool result)
    {
      m_loadResultMap[address] = result;
    }

    // Adds transaction result into the list of results
    void addTransactionResult(std::unique_ptr<IDpaTransactionResult2> &transResult)
    {
      m_transResults.push_back(std::move(transResult));
    }

    bool isNextTransactionResult()
    {
      return (m_transResults.size() > 0);
    }

    // Consumes the first element in the transaction results list
    std::unique_ptr<IDpaTransactionResult2> consumeNextTransactionResult()
    {
      std::list<std::unique_ptr<IDpaTransactionResult2>>::iterator iter = m_transResults.begin();
      std::unique_ptr<IDpaTransactionResult2> tranResult = std::move(*iter);
      m_transResults.pop_front();
      return tranResult;
    }
  };

  // Implementation class
  class OtaUploadService::Imp
  {
  private:
    // Parent object
    OtaUploadService &m_parent;

    // Message type: IQMESH Network OTA Upload
    const std::string m_mTypeName_iqmeshNetworkOtaUpload = "iqmeshNetwork_OtaUpload";
    shape::ILaunchService *m_iLaunchService = nullptr;
    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    IIqrfDpaService* m_iIqrfDpaService = nullptr;
    std::unique_ptr<IIqrfDpaService::ExclusiveAccess> m_exclusiveAccess;
    const std::string* m_messagingId = nullptr;
    const IMessagingSplitterService::MsgType* m_msgType = nullptr;
    const ComIqmeshNetworkOtaUpload* m_comOtaUpload = nullptr;

    // Service input parameters
    TOtaUploadInputParams m_otaUploadParams;

    // Path suffix
    std::string m_uploadPathSuffix;

    // Absolute path with hex file to upload
    std::string m_uploadPath;

    /// Node memory address
    const uint16_t m_nodeMemoryAddress = 0x04A0;
    /// Device information header
    ihp::device::ModuleInfo m_headerInfo;
    /// Device module information
    std::map<uint8_t, ihp::device::ModuleInfo> m_devices;
  public:
    explicit Imp(OtaUploadService &parent) : m_parent(parent)
    {
    }

    ~Imp()
    {
    }

  private:

    //--------------------
    // Returns file suffix
    //--------------------
    std::string getFileSuffix(const std::string &fileName)
    {
      size_t dotPos = fileName.find_last_of('.');
      if ((dotPos == std::string::npos) || (dotPos == (fileName.length() - 1)))
      {
        THROW_EXC(std::logic_error, "File has no suffix.");
      }
      return fileName.substr(dotPos + 1);
    }

    //-----------------------------------
    // Encounters type of loading content
    //-----------------------------------
    IOtaUploadService::LoadingContentType parseLoadingContentType(const std::string &fileName)
    {
      std::string fileSuffix = getFileSuffix(fileName);

      if (fileSuffix == "hex")
      {
        return IOtaUploadService::LoadingContentType::Hex;
      }

      if (fileSuffix == "iqrf")
      {
        return IOtaUploadService::LoadingContentType::Iqrf_plugin;
      }

      THROW_EXC(std::logic_error, "File is not a HEX or IQRF file.");
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
    uint8_t setFrcReponseTime(UploadResult &uploadResult, IDpaTransaction2::FrcResponseTime FRCresponseTime)
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
        m_exclusiveAccess->executeDpaTransactionRepeat(setFrcParamRequest, transResult, m_otaUploadParams.repeat);
        TRC_DEBUG("Result from Set Hops transaction as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("Set Hops successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, setFrcParamRequest.PeripheralType())
          << NAME_PAR(Node address, setFrcParamRequest.NodeAddress())
          << NAME_PAR(Command, (int)setFrcParamRequest.PeripheralCommand())
        );
        uploadResult.addTransactionResult(transResult);
        TRC_FUNCTION_LEAVE("");
        return dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSetParams_RequestResponse.FrcParams;
      }
      catch (const std::exception& e)
      {
        uploadResult.setStatus(transResult->getErrorCode(), e.what());
        uploadResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //-----------------------------
    // Returns list of bonded nodes
    //-----------------------------
    std::basic_string<uint8_t> getBondedNodes(UploadResult& uploadResult)
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
        m_exclusiveAccess->executeDpaTransactionRepeat(getBondedNodesRequest, transResult, m_otaUploadParams.repeat);
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
        uploadResult.addTransactionResult(transResult);
        std::basic_string<uint8_t> bondedNodes = bitmapToNodes(dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData);
        TRC_FUNCTION_LEAVE("");
        return(bondedNodes);
      }
      catch (const std::exception& e)
      {
        uploadResult.setStatus(transResult->getErrorCode(), e.what());
        uploadResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //-------------------------------
    // Returns list of online devices
    //-------------------------------
    std::basic_string<uint8_t> getOnlineNodes(UploadResult &uploadResult) {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> result;
      try {
        // Build DPA request
        DpaMessage frcPingRequest;
        DpaMessage::DpaPacket_t frcPingPacket;
        frcPingPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        frcPingPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
        frcPingPacket.DpaRequestPacket_t.PCMD = CMD_FRC_SEND;
        frcPingPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        // FRC command - Ping
        frcPingPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.FrcCommand = FRC_Ping;
        // User data
        frcPingPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[0x00] = 0x00;
        frcPingPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[0x01] = 0x00;
        // Data to buffer
        frcPingRequest.DataToBuffer(frcPingPacket.Buffer, sizeof(TDpaIFaceHeader) + 3);
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(frcPingRequest, result, m_otaUploadParams.repeat);
        TRC_DEBUG("Result from PNUM_FRC Ping transaction as string:" << PAR(result->getErrorString()));
        DpaMessage frcPingResponse = result->getResponse();
        // Check FRC status
        uint8_t status = frcPingResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.Status;
        uploadResult.addTransactionResult(result);
        if (status == 0xFF) {
          return std::basic_string<uint8_t>();
        } else if (status > MAX_ADDRESS) {
          THROW_EXC_TRC_WAR(std::logic_error, "FRC ping failed with status " << PAR(status));
        } else {
          return bitmapToNodes(frcPingResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.FrcData);
        }
      } catch (const std::exception &e) {
        uploadResult.setStatus(result->getErrorCode(), e.what());
        uploadResult.addTransactionResult(result);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //------------------------------------------------------
    // Reads OS information about target device
    //------------------------------------------------------
    void osRead(UploadResult &uploadResult) {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> result;
      try {
        // Build DPA request
        DpaMessage osReadRequest;
        DpaMessage::DpaPacket_t osReadPacket;
        osReadPacket.DpaRequestPacket_t.NADR = m_otaUploadParams.deviceAddress;
        osReadPacket.DpaRequestPacket_t.PNUM = PNUM_OS;
        osReadPacket.DpaRequestPacket_t.PCMD = CMD_OS_READ;
        osReadPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        osReadRequest.DataToBuffer(osReadPacket.Buffer, sizeof(TDpaIFaceHeader));

        // Execute DPA request and parse response
        m_exclusiveAccess->executeDpaTransactionRepeat(osReadRequest, result, m_otaUploadParams.repeat);
        TRC_DEBUG("Result from OS read transaction as string: " << result->getErrorString());
        DpaMessage osReadResponse = result->getResponse();
        std::vector<uns8> responseData(osReadResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData, osReadResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData + DPA_MAX_DATA_LENGTH);
        ihp::device::ModuleInfo device = ihp::device::ModuleInfo();
        device.osMajor = (responseData[4] & 0xF0) >> 4;
        device.osMinor = responseData[4] & 0x0F;
        device.mcuType = responseData[5] & MCU_TYPE_BITS;
        device.trSeries = responseData[5] >> 4;
        device.osBuild = (responseData[7] << 8) | responseData[6];
        m_devices.insert(std::make_pair(m_otaUploadParams.deviceAddress, device));
        TRC_INFORMATION("OS read successful!");
        uploadResult.addTransactionResult(result);
      } catch (const std::exception &e) {
        uploadResult.setStatus(result->getErrorCode(), e.what());
        uploadResult.addTransactionResult(result);
        THROW_EXC(std::logic_error, e.what());
      }
      TRC_FUNCTION_LEAVE("");
    }

    //------------------------------------------------------
    // Reads OS version from bonded devices
    //------------------------------------------------------
    std::vector<uint8_t> frcOsMcuData(UploadResult &uploadResult, const std::basic_string<uint8_t> &bonded, const uint16_t &offset) {
      TRC_FUNCTION_ENTER("");
      uint16_t address = m_nodeMemoryAddress + offset;
      uint8_t processedNodes = 0;
      uint8_t requestMaxNodes = 15;
      uint8_t requestCount = std::floor(bonded.size() / requestMaxNodes);
      uint8_t remainingNodes = bonded.size() % requestMaxNodes;
      std::vector<uint8_t> data;
      data.clear();
      for (uint8_t i = 0, n = requestCount; i <= n; i++) {
        uint8_t requestNodeCount = (uint8_t)(i < requestCount ? requestMaxNodes : remainingNodes);
        if (requestNodeCount == 0) {
          break;
        }
        // selectnodes
        std::vector<uint8_t> selectedNodes = selectNodes(bonded, processedNodes, requestNodeCount);
        frcMemoryRead4BSelective(uploadResult, data, address, PNUM_OS, CMD_OS_READ, selectedNodes);
        processedNodes += requestNodeCount;
        if (requestNodeCount > 13) {
          frcExtraResult(uploadResult, data);
        }
      }
      TRC_FUNCTION_LEAVE("");
      return data;
    }

    //------------------------------------------------------
    // Select nodes for FRC request
    //------------------------------------------------------
    std::vector<uint8_t> selectNodes(const std::basic_string<uint8_t> &bonded, const uint8_t &idx, const uint8_t &count) {
      std::vector<uint8_t> selectedNodes(30, 0);
      for (uint8_t i = idx, n = idx + count; i < n; i++) {
        selectedNodes[bonded[i] / 8] |= (1 << (bonded[i] % 8));
      }
      return selectedNodes;
    }

    //------------------------------------------------------
    // FRC send selective request
    //------------------------------------------------------
    void frcMemoryRead4BSelective(UploadResult &uploadResult, std::vector<uint8_t> &data, const uint16_t &address, const uint8_t &pnum, const uint8_t &pcmd, const std::vector<uint8_t> &selectedNodes) {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> result;
      try {
        DpaMessage frcSendSelectiveRequest;
        DpaMessage::DpaPacket_t frcSendSelectivePacket;
        frcSendSelectivePacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        frcSendSelectivePacket.DpaRequestPacket_t.PNUM = PNUM_FRC,
        frcSendSelectivePacket.DpaRequestPacket_t.PCMD = CMD_FRC_SEND_SELECTIVE;
        frcSendSelectivePacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        // FRC command and user request data
        frcSendSelectivePacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.FrcCommand = FRC_MemoryRead4B;
        std::memset(frcSendSelectivePacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData, 0, 25 * sizeof(uint8_t));
        frcSendSelectivePacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0] = 0;
        frcSendSelectivePacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[1] = 0;
        frcSendSelectivePacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[2] = address & 0xFF;
        frcSendSelectivePacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[3] = address >> 8;
        frcSendSelectivePacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[4] = pnum;
        frcSendSelectivePacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[5] = pcmd;
        frcSendSelectivePacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[6] = 0;
        // Select nodes
        std::copy(selectedNodes.begin(), selectedNodes.end(), frcSendSelectivePacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.SelectedNodes);
        frcSendSelectiveRequest.DataToBuffer(frcSendSelectivePacket.Buffer, sizeof(TDpaIFaceHeader) + 38);
        // Execute FRC request
        m_exclusiveAccess->executeDpaTransactionRepeat(frcSendSelectiveRequest, result, m_otaUploadParams.repeat);
        DpaMessage frcSendSelectiveResponse = result->getResponse();
        // Process DPA response
        uint8_t status = frcSendSelectiveResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.Status;
        if (status > MAX_ADDRESS) {
          THROW_EXC_TRC_WAR(std::logic_error, "FRC Send Selective Memory read failed: " << PAR(pnum) << " " << PAR(pcmd) << " with status " << PAR(status));
        }
        const uint8_t *pData = frcSendSelectiveResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.FrcData;
        for (uint8_t i = 4; i < 55; i++) {
          data.push_back(pData[i]);
        }
        // Add FRC result
        uploadResult.addTransactionResult(result);
        TRC_FUNCTION_LEAVE("");
      } catch (const std::exception &e) {
        uploadResult.setStatus(result->getErrorCode(), e.what());
        uploadResult.addTransactionResult(result);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //------------------------------------------------------
    // FRC send selective request
    //------------------------------------------------------
    void frcExtraResult(UploadResult &uploadResult, std::vector<uint8_t> &data) {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> result;
      try {
        // Build FRC extra result
        DpaMessage frcExtraResultRequest;
        DpaMessage::DpaPacket_t frcExtraResultPacket;
        frcExtraResultPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        frcExtraResultPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
        frcExtraResultPacket.DpaRequestPacket_t.PCMD = CMD_FRC_EXTRARESULT;
        frcExtraResultPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        frcExtraResultRequest.DataToBuffer(frcExtraResultPacket.Buffer, sizeof(TDpaIFaceHeader));
        // Execute DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(frcExtraResultRequest, result, 1);
        DpaMessage frcExtraResultResponse = result->getResponse();
        const uint8_t *pData = frcExtraResultResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData;
        for (uint8_t i = 0; i < 8; i++) {
          data.push_back(pData[i]);
        }
        uploadResult.addTransactionResult(result);
        TRC_FUNCTION_LEAVE("");
      } catch (const std::exception &e) {
        uploadResult.setStatus(result->getErrorCode(), e.what());
        uploadResult.addTransactionResult(result);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //------------------------------------------------------
    // Sets PData of specified EEEPROM extended write packet
    //------------------------------------------------------
    void setExtendedWritePacketData(DpaMessage::DpaPacket_t &packet, uint16_t address, const std::basic_string<uint8_t> &data)
    {
      uint8_t *pData = packet.DpaRequestPacket_t.DpaMessage.Request.PData;
      pData[0] = address & 0xFF;
      pData[1] = (address >> 8) & 0xFF;

      for (uint8_t i = 0; i < (uint8_t)data.size(); i++)
        pData[i + 2] = data[i];
    }

    //---------------------------------------------
    // Sets specified EEEPROM extended write packet
    //---------------------------------------------
    void setExtWritePacket(DpaMessage::DpaPacket_t &packet, const uint16_t address, const std::basic_string<uint8_t> &data, const uint16_t nodeAddr)
    {
      setExtendedWritePacketData(packet, address, data);
      packet.DpaRequestPacket_t.NADR = nodeAddr;
    }

    //------------------------------------------------
    // Adds embedded write packet data into batch data
    //------------------------------------------------
    void addEmbeddedWritePacket(uint8_t *batchPacketPData, const uint16_t address, const uint16_t hwpId, const std::basic_string<uint8_t> &data, const uint8_t offset)
    {
      // Length
      batchPacketPData[offset] = (uint8_t)(EMB_WRITE_PACKET_HEADER_SIZE + data.size());
      batchPacketPData[offset + 1] = PNUM_EEEPROM;
      batchPacketPData[offset + 2] = CMD_EEEPROM_XWRITE;
      batchPacketPData[offset + 3] = hwpId & 0xFF;
      batchPacketPData[offset + 4] = (hwpId >> 8) & 0xFF;
      batchPacketPData[offset + 5] = address & 0xFF;
      batchPacketPData[offset + 6] = (address >> 8) & 0xFF;
      for (unsigned int i = 0; i < data.size(); i++)
        batchPacketPData[offset + EMB_WRITE_PACKET_HEADER_SIZE + i] = data[i];
    }

    //----------------------
    // Write internal eeprom
    //----------------------
    void writeInternalEeprom(UploadResult& uploadResult, const uint8_t address, const std::basic_string<uint8_t> &data)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Use requested hwpId for broadcast packet
        uint16_t hwpId = HWPID_DoNotCheck;
        if (m_otaUploadParams.deviceAddress == BROADCAST_ADDRESS)
          hwpId = m_otaUploadParams.hwpId;
        // Prepare DPA request
        DpaMessage writeEepromRequest;
        DpaMessage::DpaPacket_t writeEepromPacket;
        writeEepromPacket.DpaRequestPacket_t.NADR = m_otaUploadParams.deviceAddress;
        writeEepromPacket.DpaRequestPacket_t.PNUM = PNUM_EEPROM;
        writeEepromPacket.DpaRequestPacket_t.PCMD = CMD_EEPROM_WRITE;
        writeEepromPacket.DpaRequestPacket_t.HWPID = hwpId;
        // Add address and copy data
        writeEepromPacket.DpaRequestPacket_t.DpaMessage.MemoryRequest.Address = address;
        data.copy(writeEepromPacket.DpaRequestPacket_t.DpaMessage.MemoryRequest.ReadWrite.Write.PData, data.size());
        writeEepromRequest.DataToBuffer(writeEepromPacket.Buffer, sizeof(TDpaIFaceHeader) + (uint8_t)data.size() + MEMORY_WRITE_REQUEST_OVERHEAD);
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(writeEepromRequest, transResult, m_otaUploadParams.repeat);
        TRC_DEBUG("Result from CMD_EEPROM_WRITE transaction as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("CMD_EEPROM_WRITE successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, writeEepromRequest.PeripheralType())
          << NAME_PAR(Node address, writeEepromRequest.NodeAddress())
          << NAME_PAR(Command, (int)writeEepromRequest.PeripheralCommand())
        );
        uploadResult.addTransactionResult(transResult);
        TRC_FUNCTION_LEAVE("");
      }
      catch (const std::exception& e)
      {
        uploadResult.setStatus(transResult->getErrorCode(), e.what());
        uploadResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //----------------------
    // Write external eeprom
    //----------------------
    void writeExternalEeprom(UploadResult& uploadResult, const uint16_t address, const std::basic_string<uint8_t> &data)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Use requested hwpId for broadcast packet
        uint16_t hwpId = HWPID_DoNotCheck;
        if (m_otaUploadParams.deviceAddress == BROADCAST_ADDRESS)
          hwpId = m_otaUploadParams.hwpId;
        // Prepare DPA request
        DpaMessage writeEeepromRequest;
        DpaMessage::DpaPacket_t writeEeepromPacket;
        writeEeepromPacket.DpaRequestPacket_t.NADR = m_otaUploadParams.deviceAddress;
        writeEeepromPacket.DpaRequestPacket_t.PNUM = PNUM_EEEPROM;
        writeEeepromPacket.DpaRequestPacket_t.PCMD = CMD_EEEPROM_XWRITE;
        writeEeepromPacket.DpaRequestPacket_t.HWPID = hwpId;
        // Put address and copy data
        writeEeepromPacket.DpaRequestPacket_t.DpaMessage.XMemoryRequest.Address = address;
        data.copy(writeEeepromPacket.DpaRequestPacket_t.DpaMessage.XMemoryRequest.ReadWrite.Write.PData, data.size());
        writeEeepromRequest.DataToBuffer(writeEeepromPacket.Buffer, sizeof(TDpaIFaceHeader) + (uint8_t)data.size() + XMEMORY_WRITE_REQUEST_OVERHEAD);
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(writeEeepromRequest, transResult, m_otaUploadParams.repeat);
        TRC_DEBUG("Result from CMD_EEEPROM_XWRITE transaction as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("CMD_EEEPROM_XWRITE successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, writeEeepromRequest.PeripheralType())
          << NAME_PAR(Node address, writeEeepromRequest.NodeAddress())
          << NAME_PAR(Command, (int)writeEeepromRequest.PeripheralCommand())
        );
        uploadResult.addTransactionResult(transResult);
        TRC_FUNCTION_LEAVE("");
      }
      catch (const std::exception& e)
      {
        uploadResult.setStatus(transResult->getErrorCode(), e.what());
        uploadResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //---------------------
    // Write data to eeprom
    //---------------------
    void writeDataToExtEEPROM(UploadResult &uploadResult, const uint16_t startMemAddress, const std::vector<std::basic_string<uint8_t>> &data)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Use requested hwpId for broadcast packet
        uint16_t hwpId = HWPID_DoNotCheck;
        if (m_otaUploadParams.deviceAddress == BROADCAST_ADDRESS)
          hwpId = m_otaUploadParams.hwpId;

        // Eeeprom extended write packet
        DpaMessage extendedWriteRequest;
        DpaMessage::DpaPacket_t extendedWritePacket;
        extendedWritePacket.DpaRequestPacket_t.NADR = m_otaUploadParams.deviceAddress;
        extendedWritePacket.DpaRequestPacket_t.PNUM = PNUM_EEEPROM;
        extendedWritePacket.DpaRequestPacket_t.PCMD = CMD_EEEPROM_XWRITE;
        extendedWritePacket.DpaRequestPacket_t.HWPID = hwpId;

        // Batch packet
        DpaMessage batchRequest;
        DpaMessage::DpaPacket_t batchPacket;
        batchPacket.DpaRequestPacket_t.NADR = m_otaUploadParams.deviceAddress;
        batchPacket.DpaRequestPacket_t.PNUM = PNUM_OS;
        batchPacket.DpaRequestPacket_t.PCMD = CMD_OS_BATCH;
        batchPacket.DpaRequestPacket_t.HWPID = hwpId;

        // Write data to eeeprom
        uint16_t actualAddress = startMemAddress;
        size_t index = 0;
        while (index < data.size())
        {
          if ((index + 1) < data.size())
          {
            if ((data[index].size() == 16) && (data[index + 1].size() == 16))
            {
              // Delete previous batch request data
              uint8_t *batchRequestData = batchPacket.DpaRequestPacket_t.DpaMessage.Request.PData;
              memset(batchRequestData, 0, DPA_MAX_DATA_LENGTH);

              // Add 1st embedded packet into PData of the BATCH
              addEmbeddedWritePacket(batchRequestData, actualAddress, hwpId, data[index], 0);
              actualAddress += (uint8_t)(data[index].size());

              // Size of the first packet
              uint8_t firstPacketSize = (uint8_t)(EMB_WRITE_PACKET_HEADER_SIZE + data[index].size());

              // Add 2nd embedded packet into PData of the BATCH
              addEmbeddedWritePacket(batchRequestData, actualAddress, hwpId, data[index + 1], firstPacketSize);

              // Issue batch request
              uint8_t batchPacketLen = (uint8_t)(sizeof(TDpaIFaceHeader) + 2 * EMB_WRITE_PACKET_HEADER_SIZE + data[index].size() + data[index + 1].size() + 1);

              // Execute the DPA request
              batchRequest.DataToBuffer(batchPacket.Buffer, batchPacketLen);
              m_exclusiveAccess->executeDpaTransactionRepeat(batchRequest, transResult, m_otaUploadParams.repeat);
              TRC_DEBUG("Result from CMD_OS_BATCH as string:" << PAR(transResult->getErrorString()));
              DpaMessage dpaResponse = transResult->getResponse();
              TRC_INFORMATION("CMD_OS_BATCH successful!");
              TRC_DEBUG(
                "DPA transaction: "
                << NAME_PAR(Peripheral type, batchRequest.PeripheralType())
                << NAME_PAR(Node address, batchRequest.NodeAddress())
                << NAME_PAR(Command, (int)batchRequest.PeripheralCommand())
              );
              // Get response data
              uploadResult.addTransactionResult(transResult);
              actualAddress += (uint8_t)(data[index + 1].size());
              index += 2;
              // Wait for 2 write transactions to finish
              std::this_thread::sleep_for(std::chrono::milliseconds((2 * EEEPROM_PAGE_WRITE_TIME) * 2));
            }
            else
            {
              setExtWritePacket(extendedWritePacket, actualAddress, data[index], m_otaUploadParams.deviceAddress);
              // Execute the DPA request
              uint8_t extendedWritePacketLen = (uint8_t)(sizeof(TDpaIFaceHeader) + 2 + data[index].size());
              extendedWriteRequest.DataToBuffer(extendedWritePacket.Buffer, extendedWritePacketLen);
              m_exclusiveAccess->executeDpaTransactionRepeat(extendedWriteRequest, transResult, m_otaUploadParams.repeat);
              TRC_DEBUG("Result from CMD_EEEPROM_XWRITE as string:" << PAR(transResult->getErrorString()));
              DpaMessage dpaResponse = transResult->getResponse();
              TRC_INFORMATION("CMD_EEEPROM_XWRITE successful!");
              TRC_DEBUG(
                "DPA transaction: "
                << NAME_PAR(Peripheral type, extendedWriteRequest.PeripheralType())
                << NAME_PAR(Node address, extendedWriteRequest.NodeAddress())
                << NAME_PAR(Command, (int)extendedWriteRequest.PeripheralCommand())
              );
              // Get response data
              uploadResult.addTransactionResult(transResult);
              actualAddress += (uint8_t)(data[index].size());
              index++;
            }
          }
          else
          {
            setExtWritePacket(extendedWritePacket, actualAddress, data[index], m_otaUploadParams.deviceAddress);
            // Execute the DPA request
            uint8_t extendedWritePacketLen = (uint8_t)(sizeof(TDpaIFaceHeader) + 2 + data[index].size());
            extendedWriteRequest.DataToBuffer(extendedWritePacket.Buffer, extendedWritePacketLen);
            m_exclusiveAccess->executeDpaTransactionRepeat(extendedWriteRequest, transResult, m_otaUploadParams.repeat);
            TRC_DEBUG("Result from CMD_EEEPROM_XWRITE as string:" << PAR(transResult->getErrorString()));
            DpaMessage dpaResponse = transResult->getResponse();
            TRC_INFORMATION("CMD_EEEPROM_XWRITE successful!");
            TRC_DEBUG(
              "DPA transaction: "
              << NAME_PAR(Peripheral type, extendedWriteRequest.PeripheralType())
              << NAME_PAR(Node address, extendedWriteRequest.NodeAddress())
              << NAME_PAR(Command, (int)extendedWriteRequest.PeripheralCommand())
            );
            // Get response data
            uploadResult.addTransactionResult(transResult);
            actualAddress += (uint8_t)(data[index].size());
            index++;
          }
        }

        // Write into node result
        uploadResult.putUploadResult(true);

        TRC_FUNCTION_LEAVE("");
      }
      catch (const std::exception& e)
      {
        uploadResult.setStatus(transResult->getErrorCode(), e.what());
        uploadResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //---------------------
    // Get FRC extra result
    //---------------------
    DpaMessage getFrcExtraResult(UploadResult &uploadResult)
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
        m_exclusiveAccess->executeDpaTransactionRepeat(extraResultRequest, transResult, m_otaUploadParams.repeat);
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
        uploadResult.setStatus(transResult->getErrorCode(), e.what());
        uploadResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //-------------------------------------------------
    // Load code into into device using unicast request
    //-------------------------------------------------
    void loadCodeUnicast(const LoadingAction loadingAction, const IOtaUploadService::LoadingContentType loadingContentType, const uint16_t length, const uint16_t checksum, UploadResult &uploadResult)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare unicast OS LoadCode request
        DpaMessage loadCodeRequest;
        DpaMessage::DpaPacket_t loadCodePacket;
        loadCodePacket.DpaRequestPacket_t.NADR = m_otaUploadParams.deviceAddress;
        loadCodePacket.DpaRequestPacket_t.PNUM = PNUM_OS;
        loadCodePacket.DpaRequestPacket_t.PCMD = CMD_OS_LOAD_CODE;
        loadCodePacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        loadCodePacket.DpaRequestPacket_t.DpaMessage.PerOSLoadCode_Request.Flags = 0x00;
        if (loadingAction == LoadingAction::Load)
          loadCodePacket.DpaRequestPacket_t.DpaMessage.PerOSLoadCode_Request.Flags |= 0x01;
        if (loadingContentType == IOtaUploadService::LoadingContentType::Iqrf_plugin)
          loadCodePacket.DpaRequestPacket_t.DpaMessage.PerOSLoadCode_Request.Flags |= 0x02;
        loadCodePacket.DpaRequestPacket_t.DpaMessage.PerOSLoadCode_Request.Address = m_otaUploadParams.startMemAddr;
        loadCodePacket.DpaRequestPacket_t.DpaMessage.PerOSLoadCode_Request.CheckSum = checksum;
        loadCodePacket.DpaRequestPacket_t.DpaMessage.PerOSLoadCode_Request.Length = length;
        loadCodeRequest.DataToBuffer(loadCodePacket.Buffer, sizeof(TDpaIFaceHeader) + sizeof(TPerOSLoadCode_Request));
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(loadCodeRequest, transResult, m_otaUploadParams.repeat, 10000);
        TRC_DEBUG("Result from CMD_OS_LOAD_CODE as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("CMD_OS_LOAD_CODE successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, loadCodeRequest.PeripheralType())
          << NAME_PAR(Node address, loadCodeRequest.NodeAddress())
          << NAME_PAR(Command, (int)loadCodeRequest.PeripheralCommand())
        );
        uploadResult.addTransactionResult(transResult);
        uint8_t result = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData[0x00];
        if (loadingAction == LoadingAction::Load)
          uploadResult.setLoadResultsMap(m_otaUploadParams.deviceAddress, result == 0x01);
        else
          uploadResult.setVerifyResultsMap(m_otaUploadParams.deviceAddress, result == 0x01);
        TRC_FUNCTION_LEAVE("");
      }
      catch (const std::exception& e)
      {
        uploadResult.setStatus(transResult->getErrorCode(), e.what());
        uploadResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //-----------------------------------------------------
    // Verify code using selective FRC (Memory read plus 1)
    //-----------------------------------------------------
    void verifyCode(const LoadingAction loadingAction, const IOtaUploadService::LoadingContentType loadingContentType, const uint16_t length, const uint16_t checksum, UploadResult &uploadResult)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      bool forced = false;
      try
      {
        uint16_t hwpId = m_otaUploadParams.hwpId;
        std::basic_string<uint8_t> nodesList;
        nodesList.clear();
        DpaMessage frcSendRequest;
        DpaMessage::DpaPacket_t frcSendPacket;
        if (hwpId == HWPID_DoNotCheck)
        {
          // Verify code at all bonded nodes
          nodesList = getBondedNodes(uploadResult);
        }
        else
        {
          // Verify code at nodes with selected hwpId only
          frcSendPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
          frcSendPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
          frcSendPacket.DpaRequestPacket_t.PCMD = CMD_FRC_SEND;
          frcSendPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
          frcSendPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.FrcCommand = FRC_AcknowledgedBroadcastBits;
          frcSendPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[0] = 0x05;
          frcSendPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[1] = PNUM_OS;
          frcSendPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[2] = CMD_OS_READ;
          frcSendPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[3] = hwpId & 0xff;
          frcSendPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[4] = hwpId >> 0x08;
          frcSendRequest.DataToBuffer(frcSendPacket.Buffer, sizeof(TDpaIFaceHeader) + 6);
          // Execute the DPA request
          m_exclusiveAccess->executeDpaTransactionRepeat(frcSendRequest, transResult, m_otaUploadParams.repeat);
          TRC_DEBUG("Result from CMD_FRC_SEND as string:" << PAR(transResult->getErrorString()));
          DpaMessage dpaResponse = transResult->getResponse();
          TRC_INFORMATION("CMD_FRC_SEND successful!");
          TRC_DEBUG(
            "DPA transaction: "
            << NAME_PAR(Peripheral type, frcSendRequest.PeripheralType())
            << NAME_PAR(Node address, frcSendRequest.NodeAddress())
            << NAME_PAR(Command, (int)frcSendRequest.PeripheralCommand())
          );
          // Check FRC status
          uint8_t frcStatus = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.Status;
          if (frcStatus > MAX_ADDRESS)
          {
            TRC_WARNING("FRC Read OS Info failed." << NAME_PAR_HEX("Status", (int)frcStatus));
            THROW_EXC(std::logic_error, "Bad FRC status: " << PAR((int)frcStatus));
          }
          nodesList = bitmapToNodes(dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.FrcData);
          if (nodesList.empty()) {
            forced = true;
            uploadResult.setStatus(noDeviceMatchedHwpid, noDeviceMatchedHwpidStr);
            THROW_EXC(std::logic_error, uploadResult.getStatusStr());
          }
          // Add FRC result
          uploadResult.addTransactionResult(transResult);
        }
        uploadResult.setNodesList(nodesList);

        // Set FRC params according to file type and length
        IDpaTransaction2::FrcResponseTime frcResponsTimeOTA;
        if (loadingContentType == LoadingContentType::Hex)
        {
          if (length > 0x2700)
            frcResponsTimeOTA = IDpaTransaction2::FrcResponseTime::k5160Ms;
          else if (length > 0x0F00)
            frcResponsTimeOTA = IDpaTransaction2::FrcResponseTime::k2600Ms;
          else
            frcResponsTimeOTA = IDpaTransaction2::FrcResponseTime::k1320Ms;
        }
        else
        {
          if (length > 0x3100)
            frcResponsTimeOTA = IDpaTransaction2::FrcResponseTime::k20620Ms;
          else if (length > 0x1500)
            frcResponsTimeOTA = IDpaTransaction2::FrcResponseTime::k10280Ms;
          else if (length > 0x0B00)
            frcResponsTimeOTA = IDpaTransaction2::FrcResponseTime::k5160Ms;
          else
            frcResponsTimeOTA = IDpaTransaction2::FrcResponseTime::k2600Ms;
        }

        // Set calculated FRC response time
        m_iIqrfDpaService->setFrcResponseTime(frcResponsTimeOTA);
        setFrcReponseTime(uploadResult, frcResponsTimeOTA);

        // Verify the code at selected nodes
        while (nodesList.size() > 0)
        {
          frcSendPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
          frcSendPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
          frcSendPacket.DpaRequestPacket_t.PCMD = CMD_FRC_SEND_SELECTIVE;
          frcSendPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
          // Initialize command to FRC_MemoryReadPlus1
          frcSendPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.FrcCommand = FRC_MemoryReadPlus1;
          // Initialize SelectedNodes
          memset(frcSendPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.SelectedNodes, 0, 30 * sizeof(uint8_t));
          std::list<uint16_t> selectedNodes;
          selectedNodes.clear();
          do
          {
            uint8_t addr = nodesList.front();
            selectedNodes.push_back(addr);
            nodesList.erase(std::find(nodesList.begin(), nodesList.end(), addr));
            uint8_t byteIndex = (uint8_t)(addr / 8);
            uint8_t bitIndex = addr % 8;
            frcSendPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.SelectedNodes[byteIndex] |= (0x01 << bitIndex);
          } while (nodesList.size() != 0 && selectedNodes.size() < 63);
          // Initialize user data to zero
          memset(frcSendPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData, 0, 25 * sizeof(uint8_t));
          // Read from RAM address 0x04a0
          frcSendPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0] = 0xa0;
          frcSendPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[1] = 0x04;
          // Embedded OS Load Code command
          frcSendPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[2] = PNUM_OS;
          frcSendPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[3] = CMD_OS_LOAD_CODE;
          frcSendPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[4] = sizeof(TPerOSLoadCode_Request);
          TPerOSLoadCode_Request *perOsLoadCodeRequest = (TPerOSLoadCode_Request *)&frcSendPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[5];
          perOsLoadCodeRequest->Flags = 0x00;
          if (loadingAction == LoadingAction::Load)
            perOsLoadCodeRequest->Flags |= 0x01;
          if (loadingContentType == IOtaUploadService::LoadingContentType::Iqrf_plugin)
            perOsLoadCodeRequest->Flags |= 0x02;
          perOsLoadCodeRequest->Address = m_otaUploadParams.startMemAddr;
          perOsLoadCodeRequest->CheckSum = checksum;
          perOsLoadCodeRequest->Length = length;
          frcSendRequest.DataToBuffer(frcSendPacket.Buffer, sizeof(TDpaIFaceHeader) + 1 + 30 + 12);
          // Execute the DPA request
          m_exclusiveAccess->executeDpaTransactionRepeat(frcSendRequest, transResult, m_otaUploadParams.repeat);
          TRC_DEBUG("Result from CMD_FRC_SEND_SELECTIVE as string:" << PAR(transResult->getErrorString()));
          DpaMessage dpaResponse = transResult->getResponse();
          TRC_INFORMATION("CMD_FRC_SEND_SELECTIVE successful!");
          TRC_DEBUG(
            "DPA transaction: "
            << NAME_PAR(Peripheral type, frcSendRequest.PeripheralType())
            << NAME_PAR(Node address, frcSendRequest.NodeAddress())
            << NAME_PAR(Command, (int)frcSendRequest.PeripheralCommand())
          );
          // Check FRC status
          uint8_t frcStatus = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.Status;
          if (frcStatus > MAX_ADDRESS)
          {
            TRC_WARNING("Selective FRC Verify code failed." << NAME_PAR_HEX("Status", (int)frcStatus));
            THROW_EXC(std::logic_error, "Bad FRC status: " << PAR((int)frcStatus));
          }
          // Add FRC result
          uploadResult.addTransactionResult(transResult);
          // Process FRC data
          std::basic_string<uint8_t> frcData;
          frcData.append(&dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData[2], 54);
          // Get extra result
          if (selectedNodes.size() > 54)
          {
            DpaMessage frcExtraResult = getFrcExtraResult(uploadResult);
            frcData.append(frcExtraResult.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData, 9);
          }
          // Set Verify result
          uint8_t frcPDataIndex = 0;
          for (uint16_t addr : selectedNodes)
            uploadResult.setVerifyResultsMap(addr, frcData[frcPDataIndex++] == 0x02);
        }
        TRC_FUNCTION_LEAVE("");
      }
      catch (const std::exception& e)
      {
        if (!forced) {
          uploadResult.setStatus(transResult->getErrorCode(), e.what());
        }
        uploadResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //------------------------------------------------------
    // Load code into Nodes using acknowledged broadcast FRC
    //------------------------------------------------------
    void loadCodeBroadcast(const IOtaUploadService::LoadingContentType loadingContentType, const uint16_t length, const uint16_t checksum, UploadResult &uploadResult)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        DpaMessage frcSendRequest;
        DpaMessage::DpaPacket_t frcSendPacket;
        frcSendPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        frcSendPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
        frcSendPacket.DpaRequestPacket_t.PCMD = CMD_FRC_SEND;
        frcSendPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        // Initialize command to FRC_AcknowledgedBroadcastBits
        frcSendPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.FrcCommand = FRC_AcknowledgedBroadcastBits;
        // Initialize user data to zero
        memset(frcSendPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData, 0, 30 * sizeof(uint8_t));
        // Embedded OS Load Code command
        frcSendPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[0] = sizeof(TDpaIFaceHeader) - 1 * sizeof(uint8_t) + sizeof(TPerOSLoadCode_Request);
        frcSendPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[1] = PNUM_OS;
        frcSendPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[2] = CMD_OS_LOAD_CODE;
        uint16_t hwpId = m_otaUploadParams.hwpId;
        frcSendPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[3] = hwpId & 0xff;
        frcSendPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[4] = hwpId >> 0x08;
        TPerOSLoadCode_Request *perOsLoadCodeRequest = (TPerOSLoadCode_Request*)&frcSendPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[5];
        perOsLoadCodeRequest->Flags = 0x01;
        if (loadingContentType == IOtaUploadService::LoadingContentType::Iqrf_plugin)
          perOsLoadCodeRequest->Flags |= 0x02;
        perOsLoadCodeRequest->Address = m_otaUploadParams.startMemAddr;
        perOsLoadCodeRequest->CheckSum = checksum;
        perOsLoadCodeRequest->Length = length;
        frcSendRequest.DataToBuffer(frcSendPacket.Buffer, sizeof(TDpaIFaceHeader) + 1 + frcSendPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[0]);
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(frcSendRequest, transResult, m_otaUploadParams.repeat);
        TRC_DEBUG("Result from CMD_FRC_SEND as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("CMD_FRC_SEND successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, frcSendRequest.PeripheralType())
          << NAME_PAR(Node address, frcSendRequest.NodeAddress())
          << NAME_PAR(Command, (int)frcSendRequest.PeripheralCommand())
        );
        // Check FRC status
        uint8_t frcStatus = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.Status;
        if (frcStatus > MAX_ADDRESS)
        {
          TRC_WARNING("Send FRC CMD_OS_LOAD_CODE failed." << NAME_PAR_HEX("Status", (int)frcStatus));
          THROW_EXC(std::logic_error, "Bad FRC status: " << PAR((int)frcStatus));
        }
        // Add FRC result
        uploadResult.addTransactionResult(transResult);
        // Process FRC data
        std::basic_string<uint8_t> frcData;
        frcData.append(dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.FrcData, 55);
        // Get extra result
        std::basic_string<uint8_t> nodesList = uploadResult.getNodesList();
        if (nodesList.size() > 54)
        {
          DpaMessage frcExtraResult = getFrcExtraResult(uploadResult);
          frcData.append(frcExtraResult.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData, 9);
        }
        // Set Load Result
        for (uint8_t addr : nodesList)
        {
          uint8_t byteIndex = addr / 8;
          uint8_t bitIndex = addr % 8;
          uint8_t bitMask = 1 << bitIndex;
          bool loadResult = uploadResult.getVerifyResult(addr);
          if (loadResult)
            loadResult = frcData[byteIndex] & bitMask;
          uploadResult.setLoadResultsMap(addr, loadResult);
        }
        TRC_FUNCTION_LEAVE("");
      }
      catch (const std::exception& e)
      {
        uploadResult.setStatus(transResult->getErrorCode(), e.what());
        uploadResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //-------
    // Upload
    //-------
    void upload(UploadResult &uploadResult)
    {
      TRC_FUNCTION_ENTER("");
      try
      {
        std::string fileName;
        IOtaUploadService::LoadingContentType loadingContentType;
        LoadingAction loadingAction = uploadResult.getLoadingAction();
        std::unique_ptr<PreparedData> flashData;
        std::list<CodeBlock> eepromData;
        std::list<CodeBlock> eeepromData;
        bool uploadEeprom = false;
        bool uploadEeeprom = false;
        uint8_t eepromBottomAddr = 0x00;
        bool hasCompatibilityHeader = false;
        m_headerInfo = ihp::device::ModuleInfo();
        std::vector<std::string> headerOsTokens;
        m_devices.clear();

        // Prepare flash eeprom and eeepron data to upload
        try
        {
          // Process the file
          fileName = getFullFileName(m_uploadPath, m_otaUploadParams.fileName);

          try {
            loadingContentType = parseLoadingContentType(fileName);
          } catch (const std::exception &e) {
            uploadResult.setStatus(uploadFileProcessingError, e.what());
            THROW_EXC(std::logic_error, e.what());
          }

          // parse data
          try {
            if (loadingContentType == LoadingContentType::Hex) {
              IntelHexParser parser(fileName);
              flashData = std::make_unique<PreparedData>(PreparedData::fromHex(parser.getFlashData()));
              if (loadingAction == LoadingAction::Upload) {
                eepromData = parser.getEepromData();
                eeepromData = parser.getEeepromData();
              }
              hasCompatibilityHeader = parser.hasCompatibilityHeader();
              m_headerInfo = parser.getHeaderModuleInfo();
            } else {
              IqrfParser parser(fileName);
              flashData = std::make_unique<PreparedData>(PreparedData::fromIqrf(parser.getFlashData(), m_otaUploadParams.deviceAddress == BROADCAST_ADDRESS));
              m_headerInfo = parser.getHeaderModuleInfo();
              headerOsTokens = parser.getHeaderOs();
            }
          } catch (const std::exception &e) {
            uploadResult.setStatus(uploadFileProcessingError, e.what());
            THROW_EXC(std::logic_error, e.what());
          }

          // validate upload data
          if (loadingAction == LoadingAction::Upload) {
            // validate EEPROM data
            if (eepromData.empty() != true) {
              // Hex contains data for internal eeprom, upload eeprom data specified in request ?
              if (m_otaUploadParams.uploadEepromData == true) {
                // Yes - check internal eeprom address ([N]: 0x00-0xbf, [C]: 0x80-0xbf)
                if (m_otaUploadParams.deviceAddress == COORDINATOR_ADDRESS) {
                  eepromBottomAddr = 0x80;
                }
                for (CodeBlock block : eepromData) {
                  // Check eeeprom address is dedicated to user
                  if ((block.getStartAddr() < eepromBottomAddr) || (block.getEndAddr() > 0xbf)) {
                    std::stringstream strError;
                    strError << "Internal Eeprom area 0x" << std::hex << (block.getStartAddr()) << "-0x" << block.getEndAddr() << " is not dedicated to user.";
                    uploadResult.setStatus(invalidEepromAddress, strError.str());
                    THROW_EXC(std::logic_error, uploadResult.getStatusStr());
                  }
                }
                // OK - upload hex file eeprom content
                uploadEeprom = true;
              } else {
                // Hex file contains eeprom data, but uploadEepromData is false - upload stopped
                std::stringstream strError;
                strError << "Hex file contains eeprom data, uploadEepromData is false, upload stopped.";
                uploadResult.setStatus(eepromContentNotUploaded, strError.str());
                THROW_EXC(std::logic_error, uploadResult.getStatusStr());
              }
            }

            // validate EEEPROM data
            if (eeepromData.empty() != true) {
              // Hex contains data for internal eeprom, upload eeprom data ?
              if (m_otaUploadParams.uploadEeepromData == true) {
                // Check external eeprom address (0x0000-0x3fff)
                for (CodeBlock block : eeepromData) {
                  if (block.getEndAddr() > 0x3fff) {
                    std::stringstream strError;
                    strError << "External Eeprom area 0x" << std::hex << (block.getStartAddr()) << "-0x" << block.getEndAddr() << " is not dedicated to user.";
                    uploadResult.setStatus(invalidEepromAddress, strError.str());
                    THROW_EXC(std::logic_error, uploadResult.getStatusStr());
                  }

                  // Check eeeprom content is not in the same space as startMemAddr
                  if ((m_otaUploadParams.startMemAddr >= block.getStartAddr()) && (m_otaUploadParams.startMemAddr <= block.getEndAddr())) {
                    std::stringstream strError;
                    strError << "External Eeprom area 0x" << std::hex << (block.getStartAddr()) << "-0x" << block.getEndAddr() << " overlaps startMemAddr address.";
                    uploadResult.setStatus(invalidEepromAddress, strError.str());
                    THROW_EXC(std::logic_error, uploadResult.getStatusStr());
                  }
                }
                // OK - upload hex file eeeprom content
                uploadEeeprom = true;
              } else {
                std::stringstream strError;
                strError << "Hex file contains eeeprom data, uploadEeepromData is false, upload stopped.";
                uploadResult.setStatus(eepromContentNotUploaded, strError.str());
                THROW_EXC(std::logic_error, uploadResult.getStatusStr());
              }
            }
          }

          if (m_otaUploadParams.deviceAddress == BROADCAST_ADDRESS) {
            std::basic_string<uint8_t> bondedDevices = getBondedNodes(uploadResult);
            if (bondedDevices.size() == 0) {
              uploadResult.setStatus(noDevices, noDevicesStr);
              THROW_EXC(std::logic_error, uploadResult.getStatusStr());
            }
            auto nodes = getOnlineNodes(uploadResult);
            if (nodes.size() != bondedDevices.size()) {
              uploadResult.setStatus(deviceOffline, deviceOfflineStr);
              THROW_EXC(std::logic_error, uploadResult.getStatusStr());
            }
            // TODO
            // get online nodes, compare with bonded and requested (selectedNodes)
            // select online and requested
            // check compatibility
            // upload to compatible, return device compatibility/offline object in response
            // use selected nodes for upload, verify and load
            // add complete execution in one request (upload, verify, load), report success, offline, incompatible nodes at the end in response
            std::vector<uint8_t> osData = frcOsMcuData(uploadResult, bondedDevices, static_cast<uint16_t>(offsetof(TPerOSRead_Response, OsVersion)));
            for (uint8_t i = 0; i < bondedDevices.size(); i++) {
              uint8_t idx = 4*i;
              ihp::device::ModuleInfo node;
              node.osMajor = (osData[idx] & 0xF0) >> 4;
              node.osMinor = (osData[idx] & 0x0F);
              node.mcuType = osData[idx+1] & MCU_TYPE_BITS;
              node.trSeries = osData[idx+1] >> 4;
              node.osBuild = static_cast<uint16_t>(osData[idx+3]) << 8 | osData[idx+2];
              m_devices.insert(std::make_pair(bondedDevices[i], node));
            }
          } else {
            osRead(uploadResult);
          }
        }
        catch (const std::exception &e)
        {
          THROW_EXC(std::logic_error, e.what());
        }

        // check device compatibility
        for (auto &entry : m_devices) {
          bool compatible = true;
          uint8_t addr = entry.first;
          ihp::device::ModuleInfo device = entry.second;
          if (loadingContentType == LoadingContentType::Hex) {
            if (hasCompatibilityHeader) {
              if (m_headerInfo.mcuType != device.mcuType) {
                compatible = false;
              }
              if (m_headerInfo.trSeries != ihp::device::getTrFamily(device.mcuType, device.trSeries)) {
                compatible = false;
              }
              if (m_headerInfo.osMajor != device.osMajor) {
                compatible = false;
              }
              if (m_headerInfo.osMinor != device.osMinor) {
                compatible = false;
              }
            } else {
              ihp::device::TrFamily trFamily = ihp::device::getTrFamily(device.mcuType, device.trSeries);
              if (trFamily == ihp::device::TrFamily::UNKNOWN_FAMILY || trFamily == ihp::device::TrFamily::TR_7xG || trFamily == ihp::device::TrFamily::TR_8xG) {
                compatible = false;
              }
            }
          } else {
            if (m_headerInfo.mcuType != device.mcuType) {
              compatible = false;
            }
            if (m_headerInfo.trSeries != ihp::device::getTrFamily(device.mcuType, device.trSeries)) {
              compatible = false;
            }
            if (!ihp::iqrf::osCompatible(headerOsTokens, device)) {
              compatible = false;
            }
          }
          if (!compatible) {
            std::string error;
            if (m_otaUploadParams.deviceAddress == 255) {
              error = loadingContentType == LoadingContentType::Hex ? networkHexIncompatibleStr : networkIqrfIncompatibleStr;
            } else {
              error = loadingContentType == LoadingContentType::Hex ? deviceHexIncompatibleStr : deviceIqrfIncompatibleStr;
            }
            uploadResult.setStatus(incompatibleDevice, error);
            THROW_EXC(std::logic_error, uploadResult.getStatusStr());
          }
          uploadResult.setDeviceCompatibility(addr, compatible);
        }

        // Upload - write prepared data into external eeprom memory
        if (loadingAction == LoadingAction::Upload)
        {
          // Upload eeprom data
          if (uploadEeprom == true)
          {
            // Write data to internal eeprom
            for (CodeBlock block : eepromData)
            {
              uint8_t blockDataLen = (uint8_t)block.getLength();
              uint8_t address = (uint8_t)block.getStartAddr();
              uint8_t index = 0x00;
              do
              {
                uint8_t len = (uint8_t)(blockDataLen > 54 ? 54 : blockDataLen);
                std::basic_string<uint8_t> data;
                data.append(block.getCode(), index, len);
                writeInternalEeprom(uploadResult, address - eepromBottomAddr, data);
                blockDataLen -= len;
                index += len;
                address += len;
              } while (blockDataLen != 0);
            }
          }

          // Upload eeeprom data
          if (uploadEeeprom == true)
          {
            // Write data to external eeprom
            for (CodeBlock block : eeepromData)
            {
              uint16_t blockDataLen = block.getLength();
              uint16_t address = block.getStartAddr();
              uint16_t index = 0x00;
              do
              {
                uint8_t len = (uint8_t)(blockDataLen > 54 ? 54 : blockDataLen);
                std::basic_string<uint8_t> data;
                data.append(block.getCode(), index, len);
                writeExternalEeprom(uploadResult, address, data);
                blockDataLen -= len;
                index += len;
                address += len;
              } while (blockDataLen != 0);
            }
          }

          // Upload code to eeeprom
          writeDataToExtEEPROM(uploadResult, m_otaUploadParams.startMemAddr, flashData->getData());
        }

        // Verify (or load) action - check the external eeprom content
        if ((loadingAction == LoadingAction::Verify) || (loadingAction == LoadingAction::Load))
        {
          // Vefiry the external eeprom content
          if (m_otaUploadParams.deviceAddress != BROADCAST_ADDRESS)
          {
            if (!uploadResult.isCompatible((uint8_t)m_otaUploadParams.deviceAddress)) {
              std::string error = loadingContentType == LoadingContentType::Hex ? deviceHexIncompatibleStr : deviceIqrfIncompatibleStr;
              uploadResult.setStatus(incompatibleDevice, error);
              THROW_EXC(std::logic_error, uploadResult.getStatusStr());
            }
            // Unicast address
            loadCodeUnicast(LoadingAction::Verify, loadingContentType, flashData->getLength(), flashData->getChecksum(), uploadResult);
          }
          else
          {
            // Save actual FRC params
            IDpaTransaction2::FrcResponseTime frcResponseTime = m_iIqrfDpaService->getFrcResponseTime();
            // Verify the external eeprom memory content
            verifyCode(LoadingAction::Verify, loadingContentType, flashData->getLength(), flashData->getChecksum(), uploadResult);
            // Finally set FRC param back to initial value
            m_iIqrfDpaService->setFrcResponseTime(frcResponseTime);
            setFrcReponseTime(uploadResult, frcResponseTime);
          }
        }

        // Load action - load external eeprom content to flash
        if (loadingAction == LoadingAction::Load)
        {
          // Load the external eeprom content to flash
          if (m_otaUploadParams.deviceAddress != BROADCAST_ADDRESS) {
            if (!uploadResult.isCompatible((uint8_t)m_otaUploadParams.deviceAddress)) {
              std::string error = loadingContentType == LoadingContentType::Hex ? deviceHexIncompatibleStr : deviceIqrfIncompatibleStr;
              uploadResult.setStatus(incompatibleDevice, error);
              THROW_EXC(std::logic_error, uploadResult.getStatusStr());
            }
            loadCodeUnicast(LoadingAction::Load, loadingContentType, flashData->getLength(), flashData->getChecksum(), uploadResult);
          } else {
            loadCodeBroadcast(loadingContentType, flashData->getLength(), flashData->getChecksum(), uploadResult);
          }
        }

        TRC_FUNCTION_LEAVE("");
      }
      catch (std::exception& e)
      {
        CATCH_EXC_TRC_WAR(std::exception, e, e.what());
        TRC_FUNCTION_LEAVE("");
      }
    }

    //-----------------
    // Creates response
    //-----------------
    void createResponse(UploadResult &uploadResult)
    {
      TRC_FUNCTION_ENTER("");
      Document docUploadResult;

      // Set common parameters
      Pointer("/mType").Set(docUploadResult, m_msgType->m_type);
      Pointer("/data/msgId").Set(docUploadResult, m_comOtaUpload->getMsgId());

      // Device address
      Pointer("/data/rsp/deviceAddr").Set(docUploadResult, m_otaUploadParams.deviceAddress);

      // hwpId
      Pointer("/data/rsp/hwpId").Set(docUploadResult, m_otaUploadParams.hwpId);

      // Load action
      Pointer("/data/rsp/loadingAction").Set(docUploadResult, m_otaUploadParams.loadingAction);
      LoadingAction action = uploadResult.getLoadingAction();

      // Checking of error
      if (uploadResult.getStatus() == 0)
      {
        // OK
        if (action == LoadingAction::Upload)
          Pointer("/data/rsp/uploadResult").Set(docUploadResult, uploadResult.getUploadResult());

        if (action == LoadingAction::Verify || action == LoadingAction::Load)
        {
          // Array of objects
          Document::AllocatorType &allocator = docUploadResult.GetAllocator();
          rapidjson::Value verifyResult(kArrayType);
          std::map<uint16_t, bool> verifyResultMap = uploadResult.getVerifyResultsMap();
          for (std::map<uint16_t, bool>::iterator i = verifyResultMap.begin(); i != verifyResultMap.end(); ++i)
          {
            rapidjson::Value verifyResultItem(kObjectType);
            verifyResultItem.AddMember("address", i->first, allocator);
            verifyResultItem.AddMember("result", i->second, allocator);
            verifyResult.PushBack(verifyResultItem, allocator);
          }
          Pointer("/data/rsp/verifyResult").Set(docUploadResult, verifyResult);
        }

        if (action == LoadingAction::Load)
        {
          Document::AllocatorType &allocator = docUploadResult.GetAllocator();
          rapidjson::Value loadResult(kArrayType);
          std::map<uint16_t, bool> loadResultMap = uploadResult.getLoadResultsMap();
          for (std::map<uint16_t, bool>::iterator i = loadResultMap.begin(); i != loadResultMap.end(); ++i)
          {
            rapidjson::Value loadResultItem(kObjectType);
            loadResultItem.AddMember("address", i->first, allocator);
            loadResultItem.AddMember("result", i->second, allocator);
            loadResult.PushBack(loadResultItem, allocator);
          }
          Pointer("/data/rsp/loadResult").Set(docUploadResult, loadResult);
        }
      }
      else
      {
        // Error
        if (action == LoadingAction::Upload)
          Pointer("/data/rsp/uploadResult").Set(docUploadResult, false);
      }

      // Set raw fields, if verbose mode is active
      if (m_comOtaUpload->getVerbose() == true)
      {
        rapidjson::Value rawArray(kArrayType);
        Document::AllocatorType &allocator = docUploadResult.GetAllocator();

        while (uploadResult.isNextTransactionResult())
        {
          std::unique_ptr<IDpaTransactionResult2> transResult = uploadResult.consumeNextTransactionResult();
          rapidjson::Value rawObject(kObjectType);

          rawObject.AddMember(
            "request",
            encodeBinary(transResult->getRequest().DpaPacket().Buffer, transResult->getRequest().GetLength()),
            allocator);

          rawObject.AddMember(
            "requestTs",
            TimeConversion::encodeTimestamp(transResult->getRequestTs()),
            allocator);

          rawObject.AddMember(
            "confirmation",
            encodeBinary(transResult->getConfirmation().DpaPacket().Buffer, transResult->getConfirmation().GetLength()),
            allocator);

          rawObject.AddMember(
            "confirmationTs",
            TimeConversion::encodeTimestamp(transResult->getConfirmationTs()),
            allocator);

          rawObject.AddMember(
            "response",
            encodeBinary(transResult->getResponse().DpaPacket().Buffer, transResult->getResponse().GetLength()),
            allocator);

          rawObject.AddMember(
            "responseTs",
            TimeConversion::encodeTimestamp(transResult->getResponseTs()),
            allocator);

          // add object into array
          rawArray.PushBack(rawObject, allocator);
        }

        // add array into response document
        Pointer("/data/raw").Set(docUploadResult, rawArray);
      }

      // Set status
      Pointer("/data/status").Set(docUploadResult, uploadResult.getStatus());
      Pointer("/data/statusStr").Set(docUploadResult, uploadResult.getStatusStr());

      // Send message
      m_iMessagingSplitterService->sendMessage(*m_messagingId, std::move(docUploadResult));
      TRC_FUNCTION_LEAVE("");
    }

    //-------------------
    // Get full file name
    //-------------------
    std::string getFullFileName(const std::string &uploadPath, const std::string &fileName)
    {
      char fileSeparator;
#if defined(WIN32) || defined(_WIN32)
      fileSeparator = '\\';
#else
      fileSeparator = '/';
#endif
      std::string fullFileName = uploadPath;
      if (uploadPath[uploadPath.size() - 1] != fileSeparator)
      {
        fullFileName += fileSeparator;
      }
      fullFileName += fileName;

      return fullFileName;
    }

    //--------------------------
    // Creates and send response
    //--------------------------
    void createResponse(const int status, const std::string statusStr)
    {
      Document response;

      // Set common parameters
      Pointer("/mType").Set(response, m_msgType->m_type);
      Pointer("/data/msgId").Set(response, m_comOtaUpload->getMsgId());

      // Set status
      Pointer("/data/status").Set(response, status);
      Pointer("/data/statusStr").Set(response, statusStr);

      // Send message
      m_iMessagingSplitterService->sendMessage(*m_messagingId, std::move(response));
    }

    //---------------
    // Handle message
    //---------------
    void handleMsg(const std::string &messagingId, const IMessagingSplitterService::MsgType &msgType, rapidjson::Document doc)
    {
      TRC_FUNCTION_ENTER(PAR(messagingId) << NAME_PAR(mType, msgType.m_type) << NAME_PAR(major, msgType.m_major) << NAME_PAR(minor, msgType.m_minor) << NAME_PAR(micro, msgType.m_micro));

      // Unsupported type of request
      if (msgType.m_type != m_mTypeName_iqmeshNetworkOtaUpload)
        THROW_EXC(std::logic_error, "Unsupported message type: " << PAR(msgType.m_type));

      // Creating representation object
      ComIqmeshNetworkOtaUpload comOtaUpload(doc);
      m_msgType = &msgType;
      m_messagingId = &messagingId;
      m_comOtaUpload = &comOtaUpload;

      // If upload path (service's configuration parameter) is empty, return error message
      if (m_uploadPath.empty())
      {
        createResponse(emptyUploadPathError, "Empty upload patch.");
        TRC_FUNCTION_LEAVE("");
        return;
      }

      // Parsing and checking service parameters
      LoadingAction loadingAction = LoadingAction::Undefinded;
      try
      {
        m_otaUploadParams = comOtaUpload.getOtaUploadInputParams();

        if (m_otaUploadParams.loadingAction == "Upload")
          loadingAction = LoadingAction::Upload;

        if (m_otaUploadParams.loadingAction == "Verify")
          loadingAction = LoadingAction::Verify;

        if (m_otaUploadParams.loadingAction == "Load")
          loadingAction = LoadingAction::Load;

        if(loadingAction == LoadingAction::Undefinded)
          THROW_EXC(std::logic_error, "Unsupported loading action: " << m_otaUploadParams.loadingAction);

        // External eeprom area 0x0000-0x0300 is protected (could be used for Autoexec or IO Setup)
        if(m_otaUploadParams.startMemAddr < 0x0300 || m_otaUploadParams.startMemAddr > 0x3fff)
          THROW_EXC(std::logic_error, "Incorrect startMemAddr: " << m_otaUploadParams.startMemAddr << ". startMemAddr should be between 768 and 16383.");
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
        // Upload result
        UploadResult uploadResult(loadingAction);

        // Upload
        upload(uploadResult);

        // Create and send response
        createResponse(uploadResult);
      } catch (const std::exception& e) {
        m_exclusiveAccess.reset();
        THROW_EXC_TRC_WAR(std::logic_error, e.what());
      }

      // release exclusive access
      m_exclusiveAccess.reset();

      TRC_FUNCTION_LEAVE("");
    }

  public:
    void activate(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl
        << "************************************" << std::endl
        << "OtaUploadService instance activate" << std::endl
        << "************************************"
      );

      m_uploadPath = m_iLaunchService->getCacheDir();
      props->getMemberAsString("uploadPathSuffix", m_uploadPathSuffix);

      if (m_uploadPathSuffix.empty())
      {
        TRC_WARNING("Upload path suffix is empty, using default.");
        m_uploadPath += "/upload";
      }
      else
      {
        m_uploadPath += "/";
        m_uploadPath += m_uploadPathSuffix;
      }

      TRC_INFORMATION(PAR(m_uploadPath));

      // for the sake of register function parameters
      std::vector<std::string> supportedMsgTypes =
          {
              m_mTypeName_iqmeshNetworkOtaUpload};

      m_iMessagingSplitterService->registerFilteredMsgHandler(
          supportedMsgTypes,
          [&](const std::string &messagingId, const IMessagingSplitterService::MsgType &msgType, rapidjson::Document doc) {
            handleMsg(messagingId, msgType, std::move(doc));
          });

      TRC_FUNCTION_LEAVE("");
    }

    void deactivate()
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl
        << "**************************************" << std::endl
        << "OtaUploadService instance deactivate" << std::endl
        << "**************************************"
      );

      // for the sake of unregister function parameters
      std::vector<std::string> supportedMsgTypes = {m_mTypeName_iqmeshNetworkOtaUpload};
      m_iMessagingSplitterService->unregisterFilteredMsgHandler(supportedMsgTypes);

      TRC_FUNCTION_LEAVE("");
    }

    void modify(const shape::Properties *props)
    {
      (void)props;
    }

    void attachInterface(shape::ILaunchService *iface)
    {
      m_iLaunchService = iface;
    }

    void detachInterface(shape::ILaunchService *iface)
    {
      if (m_iLaunchService == iface)
      {
        m_iLaunchService = nullptr;
      }
    }

    void attachInterface(IIqrfDpaService *iface)
    {
      m_iIqrfDpaService = iface;
    }

    void detachInterface(IIqrfDpaService *iface)
    {
      if (m_iIqrfDpaService == iface)
      {
        m_iIqrfDpaService = nullptr;
      }
    }

    void attachInterface(IMessagingSplitterService *iface)
    {
      m_iMessagingSplitterService = iface;
    }

    void detachInterface(IMessagingSplitterService *iface)
    {
      if (m_iMessagingSplitterService == iface)
      {
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

  void OtaUploadService::attachInterface(shape::ILaunchService *iface)
  {
    m_imp->attachInterface(iface);
  }

  void OtaUploadService::detachInterface(shape::ILaunchService *iface)
  {
    m_imp->detachInterface(iface);
  }

  void OtaUploadService::attachInterface(iqrf::IIqrfDpaService *iface)
  {
    m_imp->attachInterface(iface);
  }

  void OtaUploadService::detachInterface(iqrf::IIqrfDpaService *iface)
  {
    m_imp->detachInterface(iface);
  }

  void OtaUploadService::attachInterface(iqrf::IMessagingSplitterService *iface)
  {
    m_imp->attachInterface(iface);
  }

  void OtaUploadService::detachInterface(iqrf::IMessagingSplitterService *iface)
  {
    m_imp->detachInterface(iface);
  }

  void OtaUploadService::attachInterface(shape::ITraceService *iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void OtaUploadService::detachInterface(shape::ITraceService *iface)
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
} // namespace iqrf
