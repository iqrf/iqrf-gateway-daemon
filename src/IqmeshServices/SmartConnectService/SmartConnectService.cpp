/**
 * Copyright 2015-2024 IQRF Tech s.r.o.
 * Copyright 2019-2024 MICRORISC s.r.o.
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

#define ISmartConnectService_EXPORTS

#include "SmartConnectService.h"
#include "RawDpaEmbedOS.h"
#include "Trace.h"
#include "ComIqmeshNetworkSmartConnect.h"
#include "IqrfCodeDecoder.h"
#include "iqrf__SmartConnectService.hxx"
#include <list>
#include <math.h>
#include <thread>

TRC_INIT_MODULE(iqrf::SmartConnectService)

using namespace rapidjson;

namespace {

  // maximum number of repeats
  static const uint8_t REPEAT_MAX = 3;

  // length of Individual Bonding key [in bytes]
  static const uint8_t IBK_LEN = 16;

  // length of MID
  static const uint8_t MID_LEN = 4;

  // length of user data
  static const uint8_t USER_DATA_LEN = 4;

  // minimal required DPA version
  static const uint16_t DPA_MIN_REQ_VERSION = 0x0303;

  static const int serviceError = 1000;
  static const int parsingRequestError = 1001;
  static const int exclusiveAccessError = 1002;
  static const int addressUsedError = 1003;
  static const int noFreeAddressError = 1004;
}

namespace iqrf {
  // Holds information about result of smart connect
  class SmartConnectResult
  {
  private:
    // Status
    int m_status = 0;
    std::string m_statusStr = "ok";

    uint16_t m_hwpId;
    uint16_t m_hwpIdVer;
    uint8_t m_bondedAddr;
    uint8_t m_bondedNodesNum;
    std::string m_manufacturer = "";
    std::string m_product = "";
    std::list<std::string> m_standards = { "" };

    // OS read response data
    TEnumPeripheralsAnswer m_enumPer;
    embed::os::RawDpaReadPtr m_osRead;
    uint16_t m_osBuild;

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

    uint16_t getHwpId() const { return m_hwpId; };
    void setHwpId(uint16_t hwpId) {
      m_hwpId = hwpId;
    }

    uint16_t getHwpIdVersion() const { return m_hwpIdVer; };
    void setHwpIdVersion(uint16_t hwpIdVer) {
      m_hwpIdVer = hwpIdVer;
    }

    uint8_t getBondedAddr() const { return m_bondedAddr; };
    void setBondedAddr(uint8_t bondedAddr) {
      m_bondedAddr = bondedAddr;
    }

    uint8_t getBondedNodesNum() const { return m_bondedNodesNum; };
    void setBondedNodesNum(uint8_t bondedNodesNum) {
      m_bondedNodesNum = bondedNodesNum;
    }

    std::string getManufacturer() const { return m_manufacturer; };
    void setManufacturer(const std::string& manufacturer) {
      m_manufacturer = manufacturer;
    }

    std::string getProduct() const { return m_product; };
    void setProduct(const std::string& product) {
      m_product = product;
    }

    std::list<std::string> getStandards() const { return m_standards; };
    void setStandards(std::list<std::string> standards) {
      m_standards = standards;
    }

    const embed::os::RawDpaReadPtr& getOsRead() const { return m_osRead; };
    void setOsRead(embed::os::RawDpaReadPtr &osReadPtr) {
      m_osRead = std::move(osReadPtr);
    }

    uint16_t getOsBuild() const { return m_osBuild; };
    void setOsBuild(uint16_t osBuild) {
      m_osBuild = osBuild;
    }

    TEnumPeripheralsAnswer getEnumPer() const { return m_enumPer; };
    void setEnumPer(TEnumPeripheralsAnswer enumPer) {
      m_enumPer = enumPer;
    }

    // Adds transaction result into the list of results
    void addTransactionResult(std::unique_ptr<IDpaTransactionResult2> transResult)
    {
      m_transResults.push_back(std::move(transResult));
    }

    // Adds transaction result into the list of results
    void addTransactionResultRef(std::unique_ptr<IDpaTransactionResult2> &transResult)
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

  // implementation class
  class SmartConnectService::Imp {
  private:
    // parent object
    SmartConnectService& m_parent;

    // message type
    const std::string m_mTypeName_iqmeshNetworkSmartConnect = "iqmeshNetwork_SmartConnect";
    IJsCacheService* m_iJsCacheService = nullptr;
    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    IIqrfDpaService* m_iIqrfDpaService = nullptr;
    std::unique_ptr<IIqrfDpaService::ExclusiveAccess> m_exclusiveAccess;
    const MessagingInstance* m_messaging = nullptr;
    const IMessagingSplitterService::MsgType* m_msgType = nullptr;
    const ComIqmeshNetworkSmartConnect* m_comSmartConnect = nullptr;

    // Service input parameters
    TSmartConnectInputParams m_smartConnectParams;

  public:
    explicit Imp(SmartConnectService& parent) : m_parent(parent)
    {
    }

    ~Imp()
    {
    }

  private:
    void checkBondedNodes(SmartConnectResult &smartConnectResult) {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> result;
      uint8_t bondedArray[DPA_MAX_DATA_LENGTH] = {0};
      const uint16_t addr = m_smartConnectParams.deviceAddress;
      try {
        // Build packet and request
        DpaMessage bondedRequest;
        DpaMessage::DpaPacket_t bondedPacket;
        bondedPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        bondedPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
        bondedPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_BONDED_DEVICES;
        bondedPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        bondedRequest.DataToBuffer(bondedPacket.Buffer, sizeof(TDpaIFaceHeader));
        // Execute DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(bondedRequest, result, m_smartConnectParams.repeat);
        TRC_DEBUG("Result from CMD_COORDINATOR_BONDED_DEVICES as string: " << PAR(result->getErrorString()));
        // Process bonded response and check for available address
        DpaMessage bondedResponse = result->getResponse();
        TRC_INFORMATION("CMD_COORDINATOR_BONDED_DEVICES successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(NADR, bondedRequest.NodeAddress())
          << NAME_PAR(PNUM, bondedRequest.PeripheralType())
          << NAME_PAR(PCMD, (int)bondedRequest.PeripheralCommand())
        );
        uint8_t *pdata = bondedResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData;
        for (uint8_t i = 0; i < DPA_MAX_DATA_LENGTH; ++i) {
          bondedArray[i] = pdata[i];
        }
        smartConnectResult.addTransactionResultRef(result);
      } catch (const std::exception &e) {
        smartConnectResult.setStatus(result->getErrorCode(), e.what());
        smartConnectResult.addTransactionResultRef(result);
        THROW_EXC(std::logic_error, e.what());
      }
      if (addr == 0) {
        bool freeAddr = false;
        for (uint8_t i = 0; i <= MAX_ADDRESS; ++i) {
          if (!(bondedArray[i / 8] & (1 << (i % 8)))) {
            freeAddr = true;
            break;
          }
        }
        if (!freeAddr) {
          smartConnectResult.setStatus(noFreeAddressError, "No available address to assign to a new node found.");
          THROW_EXC(std::logic_error, smartConnectResult.getStatusStr());
        }
      } else {
        if ((bondedArray[addr / 8] & (1 << (addr % 8))) != 0) {
          smartConnectResult.setStatus(addressUsedError, "Requested address is already assigned to another device.");
          THROW_EXC(std::logic_error, smartConnectResult.getStatusStr())
        }
      }
      TRC_FUNCTION_LEAVE("");
    }

    //-------------
    // SmartConnect
    //-------------
    void doSmartConnect(SmartConnectResult& smartConnectResult)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        DpaMessage smartConnectRequest;
        DpaMessage::DpaPacket_t smartConnectPacket;
        smartConnectPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        smartConnectPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
        smartConnectPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_SMART_CONNECT;
        smartConnectPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        // Set PerCoordinatorSmartConnect_Request
        smartConnectPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSmartConnect_Request.ReqAddr = (uint8_t)m_smartConnectParams.deviceAddress;
        smartConnectPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSmartConnect_Request.BondingTestRetries = (uint8_t)m_smartConnectParams.bondingRetries;
        // Copy IBK
        std::copy(m_smartConnectParams.IBK.begin(), m_smartConnectParams.IBK.end(), smartConnectPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSmartConnect_Request.IBK);
        // Copy MID
        std::basic_string<uint8_t> reversed_mid = m_smartConnectParams.MID;
        std::reverse(reversed_mid.begin(), reversed_mid.end());
        std::copy(reversed_mid.begin(), reversed_mid.end(), smartConnectPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSmartConnect_Request.MID);
        // Set reserved0 to zero
        smartConnectPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSmartConnect_Request.reserved0 = 0x00;
        // VirtualDeviceAddress
        smartConnectPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSmartConnect_Request.VirtualDeviceAddress = 0xff;
        // Fill reserved1 with zeros
        for (size_t i = 0x00; i < sizeof(smartConnectPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSmartConnect_Request.reserved1); i++)
          smartConnectPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSmartConnect_Request.reserved1[i] = 0x00;
        // Copy UserData
        std::copy(m_smartConnectParams.userData.begin(), m_smartConnectParams.userData.end(), smartConnectPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSmartConnect_Request.UserData);
        // Data to buffer
        smartConnectRequest.DataToBuffer(smartConnectPacket.Buffer, sizeof(TDpaIFaceHeader) + sizeof(smartConnectPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSmartConnect_Request));
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(smartConnectRequest, transResult, m_smartConnectParams.repeat);
        TRC_DEBUG("Result from CMD_COORDINATOR_SMART_CONNECT as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("CMD_COORDINATOR_SMART_CONNECT successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, smartConnectRequest.PeripheralType())
          << NAME_PAR(Node address, smartConnectRequest.NodeAddress())
          << NAME_PAR(Command, (int)smartConnectRequest.PeripheralCommand())
        );
        // Add transaction
        smartConnectResult.addTransactionResultRef(transResult);
        // Parsing response pdata
        smartConnectResult.setHwpId(dpaResponse.DpaPacket().DpaResponsePacket_t.HWPID);
        smartConnectResult.setBondedAddr(dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerCoordinatorBondNodeSmartConnect_Response.BondAddr);
        smartConnectResult.setBondedNodesNum(dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerCoordinatorBondNodeSmartConnect_Response.DevNr);
        TRC_FUNCTION_LEAVE("");
      }
      catch (const std::exception& e)
      {
        smartConnectResult.setStatus(transResult->getErrorCode(), e.what());
        smartConnectResult.addTransactionResultRef(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //---------------------------
    // Get peripheral information
    //---------------------------
    void getPerInfo(SmartConnectResult& smartConnectResult)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage getPerInfoRequest;
        DpaMessage::DpaPacket_t getPerInfoPacket;
        getPerInfoPacket.DpaRequestPacket_t.NADR = smartConnectResult.getBondedAddr();
        getPerInfoPacket.DpaRequestPacket_t.PNUM = PNUM_ENUMERATION;
        getPerInfoPacket.DpaRequestPacket_t.PCMD = CMD_GET_PER_INFO;
        getPerInfoPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        getPerInfoRequest.DataToBuffer(getPerInfoPacket.Buffer, sizeof(TDpaIFaceHeader));
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(getPerInfoRequest, transResult, m_smartConnectParams.repeat);
        TRC_DEBUG("Result from PNUM_ENUMERATION as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("Device PNUM_ENUMERATION successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, getPerInfoRequest.PeripheralType())
          << NAME_PAR(Node address, getPerInfoRequest.NodeAddress())
          << NAME_PAR(Command, (int)getPerInfoRequest.PeripheralCommand())
        );
        TEnumPeripheralsAnswer enumPerAnswer = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.EnumPeripheralsAnswer;
        smartConnectResult.setEnumPer(enumPerAnswer);
        smartConnectResult.addTransactionResultRef(transResult);
        TRC_FUNCTION_LEAVE("");
      }
      catch (std::exception& e)
      {
        smartConnectResult.setStatus(transResult->getErrorCode(), e.what());
        smartConnectResult.addTransactionResultRef(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //-----------------------------------------
    // Reads OS info about smart connected node
    //-----------------------------------------
    void osRead(SmartConnectResult& smartConnectResult)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<embed::os::RawDpaRead> osReadPtr(shape_new embed::os::RawDpaRead(smartConnectResult.getBondedAddr()));
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        m_exclusiveAccess->executeDpaTransactionRepeat(osReadPtr->getRequest(), transResult, m_smartConnectParams.repeat);
        osReadPtr->processDpaTransactionResult(std::move(transResult));
        TRC_DEBUG("Result from OS read transaction as string:" << PAR(osReadPtr->getResult()->getErrorString()));
        smartConnectResult.setOsBuild((uint16_t)osReadPtr->getOsBuild());
        smartConnectResult.setHwpId(osReadPtr->getHwpid());
        smartConnectResult.addTransactionResult(osReadPtr->getResultMove());
        smartConnectResult.setOsRead(osReadPtr);
        TRC_INFORMATION("OS read successful!");
        TRC_FUNCTION_LEAVE("");
      }
      catch (const std::exception &e)
      {
        smartConnectResult.setStatus(transResult->getErrorCode(), e.what());
        smartConnectResult.addTransactionResultRef(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //-------------
    // SmartConnect
    //-------------
    void smartConnect(SmartConnectResult &smartConnectResult)
    {
      TRC_FUNCTION_ENTER("");
      try
      {
        // check, if there is at minimal DPA ver. 3.03 at the coordinator
        uint16_t dpaVersion = 0;
        IIqrfDpaService::CoordinatorParameters coordParams = m_iIqrfDpaService->getCoordinatorParameters();
        dpaVersion = coordParams.dpaVerWord;
        if (dpaVersion < DPA_MIN_REQ_VERSION) {
          THROW_EXC(std::logic_error, "Old version of DPA: " << PAR(dpaVersion));
        }

        checkBondedNodes(smartConnectResult);

        // SmartConnect request
        doSmartConnect(smartConnectResult);

        // Delay after successful bonding
        std::this_thread::sleep_for(std::chrono::milliseconds(250));

        // Enumerate newly bonded node
        getPerInfo(smartConnectResult);

        // Read OS of new node
        osRead(smartConnectResult);

        std::shared_ptr<IJsCacheService::Manufacturer> manufacturer = m_iJsCacheService->getManufacturer(smartConnectResult.getHwpId());
        if (manufacturer != nullptr)
        {
          smartConnectResult.setManufacturer(manufacturer->m_name);
        }

        std::shared_ptr<IJsCacheService::Product> product = m_iJsCacheService->getProduct(smartConnectResult.getHwpId());
        if (product != nullptr)
        {
          smartConnectResult.setProduct(product->m_name);
        }

        std::string osBuildStr;
        {
          std::ostringstream os;
          os.fill('0');
          os << std::hex << std::uppercase << std::setw(4) << (int)smartConnectResult.getOsBuild();
          osBuildStr = os.str();
        }

        std::shared_ptr<IJsCacheService::Package> package = m_iJsCacheService->getPackage(
          smartConnectResult.getHwpId(),
          smartConnectResult.getHwpIdVersion(),
          osBuildStr,
          m_iIqrfDpaService->getCoordinatorParameters().dpaVerWordAsStr
        );
        if (package != nullptr)
        {
          std::list<std::string> standards;
          for (const IJsCacheService::StdDriver & driver : package->m_stdDriverVect)
          {
            standards.push_back(driver.getName());
          }
          smartConnectResult.setStandards(standards);
        }
        else
        {
          TRC_INFORMATION("Package not found");
        }
      }
      catch (std::exception& e)
      {
        CATCH_EXC_TRC_WAR(std::exception, e, e.what());
      }
      TRC_FUNCTION_LEAVE( "" );
    }

    //--------------------------
    // Creates and send response
    //--------------------------
    void createResponse(SmartConnectResult& smartConnectResult)
    {
      Document response;

      // set common parameters
      Pointer("/mType").Set(response, m_msgType->m_type);
      Pointer("/data/msgId").Set(response, m_comSmartConnect->getMsgId());

      // Set status
      int status = smartConnectResult.getStatus();
      if (status == 0)
      {
        Pointer("/data/rsp/assignedAddr").Set(response, smartConnectResult.getBondedAddr());
        Pointer("/data/rsp/nodesNr").Set(response, smartConnectResult.getBondedNodesNum());
        Pointer("/data/rsp/hwpId").Set(response, smartConnectResult.getHwpId());
        Pointer("/data/rsp/manufacturer").Set(response, smartConnectResult.getManufacturer());
        Pointer("/data/rsp/product").Set(response, smartConnectResult.getProduct());

        // rsp object
        Pointer("/data/rsp/assignedAddr").Set(response, smartConnectResult.getBondedAddr());
        Pointer("/data/rsp/nodesNr").Set(response, smartConnectResult.getBondedNodesNum());

        // standards - array of strings
        rapidjson::Value standardsJsonArray(kArrayType);
        Document::AllocatorType& allocator = response.GetAllocator();
        for (std::string standard : smartConnectResult.getStandards())
        {
          rapidjson::Value standardJsonString;
          standardJsonString.SetString(standard.c_str(), (SizeType)standard.length(), allocator);
          standardsJsonArray.PushBack(standardJsonString, allocator);
        }
        Pointer("/data/rsp/standards").Set(response, standardsJsonArray);

        rapidjson::Pointer("/data/rsp/osRead/mid").Set(response, smartConnectResult.getOsRead()->getMidAsString());
        rapidjson::Pointer("/data/rsp/osRead/osVersion").Set(response, smartConnectResult.getOsRead()->getOsVersionAsString());
        rapidjson::Pointer("/data/rsp/osRead/trMcuType/value").Set(response, smartConnectResult.getOsRead()->getTrMcuType());
        rapidjson::Pointer("/data/rsp/osRead/trMcuType/trType").Set(response, smartConnectResult.getOsRead()->getTrTypeAsString());
        rapidjson::Pointer("/data/rsp/osRead/trMcuType/fccCertified").Set(response, smartConnectResult.getOsRead()->isFccCertified());
        rapidjson::Pointer("/data/rsp/osRead/trMcuType/mcuType").Set(response, smartConnectResult.getOsRead()->getTrMcuTypeAsString());

        rapidjson::Pointer("/data/rsp/osRead/osBuild").Set(response, smartConnectResult.getOsRead()->getOsBuildAsString());

        // RSSI [dBm]
        rapidjson::Pointer("/data/rsp/osRead/rssi").Set(response, smartConnectResult.getOsRead()->getRssiAsString());

        // Supply voltage [V]
        rapidjson::Pointer("/data/rsp/osRead/supplyVoltage").Set(response, smartConnectResult.getOsRead()->getSupplyVoltageAsString());

        // Flags
        rapidjson::Pointer("/data/rsp/osRead/flags/value").Set(response, smartConnectResult.getOsRead()->getFlags());
        if (smartConnectResult.getOsRead()->getDpaVer() >= 0x0417) {
          rapidjson::Pointer("/data/rsp/osRead/flags/insufficientOsVersion").Set(response, smartConnectResult.getOsRead()->isInsufficientOs());
        } else {
          rapidjson::Pointer("/data/rsp/osRead/flags/insufficientOsBuild").Set(response, smartConnectResult.getOsRead()->isInsufficientOs());
        }
        rapidjson::Pointer("/data/rsp/osRead/flags/interfaceType").Set(response, smartConnectResult.getOsRead()->getInterfaceAsString());
        rapidjson::Pointer("/data/rsp/osRead/flags/dpaHandlerDetected").Set(response, smartConnectResult.getOsRead()->isDpaHandlerDetected());
        rapidjson::Pointer("/data/rsp/osRead/flags/dpaHandlerNotDetectedButEnabled").Set(response, smartConnectResult.getOsRead()->isDpaHandlerNotDetectedButEnabled());
        rapidjson::Pointer("/data/rsp/osRead/flags/noInterfaceSupported").Set(response, smartConnectResult.getOsRead()->isNoInterfaceSupported());
        if (smartConnectResult.getOsRead()->getDpaVer() >= 0x0413)
          rapidjson::Pointer("/data/rsp/osRead/flags/iqrfOsChanged").Set(response, smartConnectResult.getOsRead()->isIqrfOsChanges());
        if (smartConnectResult.getOsRead()->getDpaVer() >= 0x0416)
          rapidjson::Pointer("/data/rsp/osRead/flags/frcAggregationEnabled").Set(response, smartConnectResult.getOsRead()->isFrcAggregationEnabled());

        // Slot limits
        rapidjson::Pointer("/data/rsp/osRead/slotLimits/value").Set(response, smartConnectResult.getOsRead()->getSlotLimits());
        rapidjson::Pointer("/data/rsp/osRead/slotLimits/shortestTimeslot").Set(response, smartConnectResult.getOsRead()->getShortestTimeSlotAsString());
        rapidjson::Pointer("/data/rsp/osRead/slotLimits/longestTimeslot").Set(response, smartConnectResult.getOsRead()->getLongestTimeSlotAsString());

        if (smartConnectResult.getOsRead()->is410Compliant())
        {
          // dpaVer, perNr
          Pointer("/data/rsp/osRead/dpaVer").Set(response, smartConnectResult.getOsRead()->getDpaVerAsString());
          Pointer("/data/rsp/osRead/perNr").Set(response, smartConnectResult.getOsRead()->getPerNr());

          Document::AllocatorType& allocator = response.GetAllocator();

          // embPers
          rapidjson::Value embPersJsonArray(kArrayType);
          for (std::set<int>::iterator it = smartConnectResult.getOsRead()->getEmbedPer().begin(); it != smartConnectResult.getOsRead()->getEmbedPer().end(); ++it)
          {
            embPersJsonArray.PushBack(*it, allocator);
          }
          Pointer("/data/rsp/osRead/embPers").Set(response, embPersJsonArray);

          // hwpId
          Pointer("/data/rsp/osRead/hwpId").Set(response, smartConnectResult.getOsRead()->getHwpid());

          // hwpIdVer
          Pointer("/data/rsp/osRead/hwpIdVer").Set(response, smartConnectResult.getOsRead()->getHwpidVer());

          // flags - int value
          Pointer("/data/rsp/osRead/enumFlags/value").Set(response, smartConnectResult.getOsRead()->getFlags());

          // flags - parsed
          bool stdModeSupported = (smartConnectResult.getOsRead()->getFlags() & 0b1) == 0b1;
          if (stdModeSupported)
          {
            Pointer("/data/rsp/osRead/enumFlags/rfModeStd").Set(response, true);
            Pointer("/data/rsp/osRead/enumFlags/rfModeLp").Set(response, false);
          }
          else
          {
            Pointer("/data/rsp/osRead/enumFlags/rfModeStd").Set(response, false);
            Pointer("/data/rsp/osRead/enumFlags/rfModeLp").Set(response, true);
          }

          // STD+LP network is running, otherwise STD network.
          if (smartConnectResult.getOsRead()->getDpaVer() >= 0x0400)
          {
            bool stdAndLpModeNetwork = (smartConnectResult.getOsRead()->getFlags() & 0b100) == 0b100;
            if (stdAndLpModeNetwork)
            {
              Pointer("/data/rsp/osRead/enumFlags/stdAndLpNetwork").Set(response, true);
            }
            else
            {
              Pointer("/data/rsp/osRead/enumFlags/stdAndLpNetwork").Set(response, false);
            }
          }

          // UserPers
          rapidjson::Value userPerJsonArray(kArrayType);
          for (std::set<int>::iterator it = smartConnectResult.getOsRead()->getUserPer().begin(); it != smartConnectResult.getOsRead()->getUserPer().end(); ++it)
          {
            userPerJsonArray.PushBack(*it, allocator);
          }
          Pointer("/data/rsp/osRead/userPers").Set(response, userPerJsonArray);
        }
      }

      // Set raw fields, if verbose mode is active
      if (m_comSmartConnect->getVerbose())
      {
        rapidjson::Value rawArray(kArrayType);
        Document::AllocatorType& allocator = response.GetAllocator();
        while (smartConnectResult.isNextTransactionResult())
        {
          std::unique_ptr<IDpaTransactionResult2> transResult = smartConnectResult.consumeNextTransactionResult();
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
          // Add object into array
          rawArray.PushBack(rawObject, allocator);
        }
        // Add array into response document
        Pointer("/data/raw").Set(response, rawArray);
      }

      // Set status
      Pointer("/data/status").Set(response, status);
      Pointer("/data/statusStr").Set(response, smartConnectResult.getStatusStr());

      // Send message
      m_iMessagingSplitterService->sendMessage(*m_messaging, std::move(response));
    }

    // creates and return default user data
    std::basic_string<uint8_t> createDefaultUserData() {
      std::basic_string<uint8_t> defaultUserData;

      for (int i = 0; i < USER_DATA_LEN; i++) {
        defaultUserData.push_back(0);
      }
      return defaultUserData;
    }

    // creates string of HEX characters for specified byte stream
    std::string getHexaString(const std::basic_string<uint8_t>& byteStream)
    {
      std::ostringstream os;

      for ( const uint8_t byte : byteStream ) {
        os << std::setfill('0') << std::setw(2) << std::hex << (int)byte;
        os << " ";
      }
      return os.str();
    }

    // creates string of HEX characters for specified byte stream
    std::string getHexaString(const uint16_t val)
    {
      std::ostringstream os;

      os << std::setfill('0') << std::setw(2) << std::hex << (int)((val >> 8) & 0xFF);
      os << " ";
      os << std::setfill('0') << std::setw(2) << std::hex << (int)(val & 0xFF);

      return os.str();
    }

    // returns reversed byte sequence to the one in the parameter
    std::basic_string<uint8_t> getReversedBytes(const std::basic_string<uint8_t>& bytes)
    {
      std::basic_string<uint8_t> reversedBytes(bytes);
      std::reverse(reversedBytes.begin(), reversedBytes.end());
      return reversedBytes;
    }

    //--------------------------
    // Creates and send response
    //--------------------------
    void createResponse(const int status, const std::string statusStr)
    {
      Document response;

      // Set common parameters
      Pointer("/mType").Set(response, m_msgType->m_type);
      Pointer("/data/msgId").Set(response, m_comSmartConnect->getMsgId());

      // Set status
      Pointer("/data/status").Set(response, status);
      Pointer("/data/statusStr").Set(response, statusStr);

      // Send message
      m_iMessagingSplitterService->sendMessage(*m_messaging, std::move(response));
    }

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
      if (msgType.m_type != m_mTypeName_iqmeshNetworkSmartConnect)
        THROW_EXC(std::logic_error, "Unsupported message type: " << PAR(msgType.m_type));

      // Creating representation object
      ComIqmeshNetworkSmartConnect comSmartConnect(doc);
      m_msgType = &msgType;
      m_messaging = &messaging;
      m_comSmartConnect = &comSmartConnect;

      // Parsing and checking service parameters
      try
      {
        m_smartConnectParams = comSmartConnect.getSmartConnectInputParams();
        IqrfCodeDecoder::decode(m_smartConnectParams.smartConnectCode);
        m_smartConnectParams.MID = IqrfCodeDecoder::getMid();
        m_smartConnectParams.IBK = IqrfCodeDecoder::getIbk();
        m_smartConnectParams.hwpId = IqrfCodeDecoder::getHwpId();
        // Logs decoded values
        TRC_INFORMATION("IQRFCode decoded values: ");
        TRC_INFORMATION("MID: " << PAR(getHexaString(m_smartConnectParams.MID)));
        TRC_INFORMATION("IBK: " << PAR(getHexaString(m_smartConnectParams.IBK)));
        TRC_INFORMATION("HWP ID: " << PAR(getHexaString(m_smartConnectParams.hwpId)));
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
        // SmartConnect result
        SmartConnectResult smartConnectResult;

        // SmartConnect
        smartConnect(smartConnectResult);

        // Create and send response
        createResponse(smartConnectResult);
      } catch (std::exception& e) {
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
      TRC_INFORMATION(std::endl <<
        "************************************" << std::endl <<
        "SmartConnectService instance activate" << std::endl <<
        "************************************"
      );

      (void)props;

      // for the sake of register function parameters
      std::vector<std::string> m_filters =
      {
        "iqmeshNetwork_SmartConnect"
      };

      m_iMessagingSplitterService->registerFilteredMsgHandler(
        m_filters,
        [&](const MessagingInstance &messaging, const IMessagingSplitterService::MsgType & msgType, rapidjson::Document doc)
      {
        handleMsg(messaging, msgType, std::move(doc));
      });


      TRC_FUNCTION_LEAVE("");
    }

    void deactivate()
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "**************************************" << std::endl <<
        "SmartConnectService instance deactivate" << std::endl <<
        "**************************************"
      );

      // for the sake of unregister function parameters
      std::vector<std::string> m_filters =
      {
        m_mTypeName_iqmeshNetworkSmartConnect
      };

      m_iMessagingSplitterService->unregisterFilteredMsgHandler(m_filters);

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

    void attachInterface(IJsCacheService* iface)
    {
      m_iJsCacheService = iface;
    }

    void detachInterface(IJsCacheService* iface)
    {
      if (m_iJsCacheService == iface) {
        m_iJsCacheService = nullptr;
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



  SmartConnectService::SmartConnectService()
  {
    m_imp = shape_new Imp(*this);
  }

  SmartConnectService::~SmartConnectService()
  {
    delete m_imp;
  }


  void SmartConnectService::attachInterface(iqrf::IIqrfDpaService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void SmartConnectService::detachInterface(iqrf::IIqrfDpaService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void SmartConnectService::attachInterface(iqrf::IMessagingSplitterService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void SmartConnectService::detachInterface(iqrf::IMessagingSplitterService* iface)
  {
    m_imp->detachInterface(iface);
  }


  void SmartConnectService::attachInterface(iqrf::IJsCacheService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void SmartConnectService::detachInterface(iqrf::IJsCacheService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void SmartConnectService::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void SmartConnectService::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }


  void SmartConnectService::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void SmartConnectService::deactivate()
  {
    m_imp->deactivate();
  }

  void SmartConnectService::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
  }
}
