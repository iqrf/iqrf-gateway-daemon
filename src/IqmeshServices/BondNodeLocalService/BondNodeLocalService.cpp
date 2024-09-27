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

#define IBondNodeLocalService_EXPORTS

#include "BondNodeLocalService.h"
#include "RawDpaEmbedOS.h"
#include "Trace.h"
#include "ComIqmeshNetworkBondNodeLocal.h"
#include "iqrf__BondNodeLocalService.hxx"
#include <list>
#include <cmath>
#include <thread>

TRC_INIT_MODULE(iqrf::BondNodeLocalService)

using namespace rapidjson;

namespace {
  static const int serviceError = 1000;
  static const int parsingRequestError = 1001;
  static const int exclusiveAccessError = 1002;
  static const int addressUsedError = 1003;
  static const int noFreeAddressError = 1004;
}

namespace iqrf {
  /// \class BondResult
  /// \brief Result of bonding of a node.
  class BondResult {
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

    // transaction results
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

    void setBondedAddr( const uint8_t addr ) {
      m_bondedAddr = addr;
    }

    // returns address of the newly bonded node
    uint8_t getBondedAddr() const { return m_bondedAddr; };

    void setBondedNodesNum( const uint8_t nodesNum ) {
      m_bondedNodesNum = nodesNum;
    }

    // returns number of bonded network nodes.
    uint8_t getBondedNodesNum() const { return m_bondedNodesNum; };

    // os read parsed
    const embed::os::RawDpaReadPtr& getOsRead() const
    {
      return m_osRead;
    }

    void setOsRead(embed::os::RawDpaReadPtr &osReadPtr) {
      m_osRead = std::move(osReadPtr);
    }

    uint16_t getOsBuild() const {
      return m_osBuild;
    }

    void setOsBuild(uint16_t osBuild) {
      m_osBuild = osBuild;
    }

    // sets HwpId of bonded node
    void setBondedNodeHwpId( const uint16_t hwpId ) {
      m_hwpId = hwpId;
    }

    // returns HwpId of bonded node
    uint16_t getBondedNodeHwpId() const {
      return m_hwpId;
    }

    // sets HwpId version of bonded node
    void setBondedNodeHwpIdVer(const uint16_t hwpIdVer) {
      m_hwpIdVer = hwpIdVer;
    }

    // returns HwpId version of bonded node
    uint16_t getBondedNodeHwpIdVer() const {
      return m_hwpIdVer;
    }

    std::string getManufacturer() {
      return m_manufacturer;
    }

    void setManufacturer(const std::string& manufacturer) {
      m_manufacturer = manufacturer;
    }

    std::string getProduct() {
      return m_product;
    }

    void setProduct(const std::string& product) {
      m_product = product;
    }

    std::list<std::string> getStandards() {
      return m_standards;
    }

    void setStandards(const std::list<std::string> &standards) {
      m_standards = standards;
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
  class BondNodeLocalService::Imp {
  private:
    // parent object
    BondNodeLocalService & m_parent;

    // message type
    const std::string m_mTypeName_iqmeshNetworkBondNodeLocal = "iqmeshNetwork_BondNodeLocal";
    IJsCacheService* m_iJsCacheService = nullptr;
    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    IIqrfDpaService* m_iIqrfDpaService = nullptr;
    std::unique_ptr<IIqrfDpaService::ExclusiveAccess> m_exclusiveAccess;
    const MessagingInstance* m_messaging = nullptr;
    const IMessagingSplitterService::MsgType* m_msgType = nullptr;
    const ComIqmeshNetworkBondNodeLocal* m_comBondNode = nullptr;

    // Service input parameters
    TBondNodeInputParams m_bondNodeParams;

  public:
    explicit Imp( BondNodeLocalService& parent ) : m_parent( parent )
    {
    }

    ~Imp()
    {
    }

    void checkNodeAddr( const uint16_t nodeAddr )
    {
      if ( nodeAddr > 0xEF  ) {
        THROW_EXC(
          std::logic_error, "Node address outside of valid range. " << NAME_PAR_HEX( "Address", nodeAddr )
        );
      }
    }

    void checkBondedNodes(BondResult &bondResult) {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> result;
      uint8_t bondedArray[DPA_MAX_DATA_LENGTH] = {0};
      const uint16_t addr = m_bondNodeParams.deviceAddress;
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
        m_exclusiveAccess->executeDpaTransactionRepeat(bondedRequest, result, m_bondNodeParams.repeat);
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
        bondResult.addTransactionResultRef(result);
      } catch (const std::exception &e) {
        bondResult.setStatus(result->getErrorCode(), e.what());
        bondResult.addTransactionResultRef(result);
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
          bondResult.setStatus(noFreeAddressError, "No available address to assign to a new node found.");
          THROW_EXC(std::logic_error, bondResult.getStatusStr());
        }
      } else {
        if ((bondedArray[addr / 8] & (1 << (addr % 8))) != 0) {
          bondResult.setStatus(addressUsedError, "Requested address is already assigned to another device.");
          THROW_EXC(std::logic_error, bondResult.getStatusStr())
        }
      }
      TRC_FUNCTION_LEAVE("");
    }

    //----------
    // Bond node
    //----------
    void doBondNode(BondResult& bondResult)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        DpaMessage bondNodeRequest;
        DpaMessage::DpaPacket_t bondNodePacket;
        bondNodePacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        bondNodePacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
        bondNodePacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_BOND_NODE;
        bondNodePacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        bondNodePacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorBondNode_Request.ReqAddr = (uint8_t)m_bondNodeParams.deviceAddress;
        bondNodePacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorBondNode_Request.BondNode.Current.BondingTestRetries = (uint8_t)m_bondNodeParams.bondingTestRetries;
        bondNodeRequest.DataToBuffer(bondNodePacket.Buffer, sizeof(TDpaIFaceHeader) + sizeof(TPerCoordinatorBondNode_Request));
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(bondNodeRequest, transResult, m_bondNodeParams.repeat);
        TRC_DEBUG("Result from CMD_COORDINATOR_BOND_NODE as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("CMD_COORDINATOR_BOND_NODE successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, bondNodeRequest.PeripheralType())
          << NAME_PAR(Node address, bondNodeRequest.NodeAddress())
          << NAME_PAR(Command, (int)bondNodeRequest.PeripheralCommand())
        );
        // Getting bond data
        TPerCoordinatorBondNodeSmartConnect_Response respData = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerCoordinatorBondNodeSmartConnect_Response;
        bondResult.setBondedAddr(respData.BondAddr);
        bondResult.setBondedNodesNum(respData.DevNr);
        bondResult.addTransactionResultRef(transResult);
        TRC_FUNCTION_LEAVE("");
      }
      catch (const std::exception& e)
      {
        bondResult.setStatus(transResult->getErrorCode(), e.what());
        bondResult.addTransactionResultRef(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //---------------------------
    // Get peripheral information
    //---------------------------
    void getPerInfo(BondResult& bondResult)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage getPerInfoRequest;
        DpaMessage::DpaPacket_t getPerInfoPacket;
        getPerInfoPacket.DpaRequestPacket_t.NADR = bondResult.getBondedAddr();
        getPerInfoPacket.DpaRequestPacket_t.PNUM = PNUM_ENUMERATION;
        getPerInfoPacket.DpaRequestPacket_t.PCMD = CMD_GET_PER_INFO;
        getPerInfoPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        getPerInfoRequest.DataToBuffer(getPerInfoPacket.Buffer, sizeof(TDpaIFaceHeader));
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(getPerInfoRequest, transResult, m_bondNodeParams.repeat);
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
        bondResult.setEnumPer(enumPerAnswer);
        bondResult.addTransactionResultRef(transResult);
        TRC_FUNCTION_LEAVE("");
      }
      catch (std::exception& e)
      {
        bondResult.setStatus(transResult->getErrorCode(), e.what());
        bondResult.addTransactionResultRef(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //-----------------------------------------
    // Reads OS info about smart connected node
    //-----------------------------------------
    void osRead(BondResult& bondResult)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<embed::os::RawDpaRead> osReadPtr(shape_new embed::os::RawDpaRead(bondResult.getBondedAddr()));
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        m_exclusiveAccess->executeDpaTransactionRepeat(osReadPtr->getRequest(), transResult, m_bondNodeParams.repeat);
        osReadPtr->processDpaTransactionResult(std::move(transResult));
        TRC_DEBUG("Result from OS read transaction as string:" << PAR(osReadPtr->getResult()->getErrorString()));
        bondResult.setOsBuild((uint16_t)osReadPtr->getOsBuild());
        bondResult.setHwpId(osReadPtr->getHwpid());
        bondResult.addTransactionResult(osReadPtr->getResultMove());
        bondResult.setOsRead(osReadPtr);
        TRC_INFORMATION("OS read successful!");
        TRC_FUNCTION_LEAVE("");
      }
      catch (const std::exception &e)
      {
        bondResult.setStatus(transResult->getErrorCode(), e.what());
        bondResult.addTransactionResultRef(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //----------
    // Bond node
    //----------
    void bondNode(BondResult& bondResult)
    {
      TRC_FUNCTION_ENTER("");
      try
      {
        // Check for available nodes
        checkBondedNodes(bondResult);
        // SmartConnect request
        doBondNode(bondResult);

        // Delay after successful bonding
        std::this_thread::sleep_for(std::chrono::milliseconds(250));

        // Enumerate newly bonded node
        getPerInfo(bondResult);

        // Read OS of new node
        osRead(bondResult);

        std::shared_ptr<IJsCacheService::Manufacturer> manufacturer = m_iJsCacheService->getManufacturer(bondResult.getHwpId());
        if (manufacturer != nullptr)
        {
          bondResult.setManufacturer(manufacturer->m_name);
        }

        std::shared_ptr<IJsCacheService::Product> product = m_iJsCacheService->getProduct(bondResult.getHwpId());
        if (product != nullptr)
        {
          bondResult.setProduct(product->m_name);
        }

        std::string osBuildStr;
        {
          std::ostringstream os;
          os.fill('0');
          os << std::hex << std::uppercase << std::setw(4) << (int)bondResult.getOsBuild();
          osBuildStr = os.str();
        }

        std::shared_ptr<IJsCacheService::Package> package = m_iJsCacheService->getPackage(
          bondResult.getHwpId(),
          bondResult.getHwpIdVersion(),
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
          bondResult.setStandards(standards);
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
      TRC_FUNCTION_LEAVE("");
    }

    //--------------------------
    // Creates and send response
    //--------------------------
    void createResponse(BondResult& bondResult)
    {
      Document response;

      // set common parameters
      Pointer("/mType").Set(response, m_msgType->m_type);
      Pointer("/data/msgId").Set(response, m_comBondNode->getMsgId());

      // Set status
      int status = bondResult.getStatus();
      if (status == 0)
      {
        Pointer("/data/rsp/assignedAddr").Set(response, bondResult.getBondedAddr());
        Pointer("/data/rsp/nodesNr").Set(response, bondResult.getBondedNodesNum());
        Pointer("/data/rsp/hwpId").Set(response, bondResult.getHwpId());
        Pointer("/data/rsp/manufacturer").Set(response, bondResult.getManufacturer());
        Pointer("/data/rsp/product").Set(response, bondResult.getProduct());

        // rsp object
        Pointer("/data/rsp/assignedAddr").Set(response, bondResult.getBondedAddr());
        Pointer("/data/rsp/nodesNr").Set(response, bondResult.getBondedNodesNum());

        // standards - array of strings
        rapidjson::Value standardsJsonArray(kArrayType);
        Document::AllocatorType& allocator = response.GetAllocator();
        for (std::string standard : bondResult.getStandards())
        {
          rapidjson::Value standardJsonString;
          standardJsonString.SetString(standard.c_str(), (SizeType)standard.length(), allocator);
          standardsJsonArray.PushBack(standardJsonString, allocator);
        }
        Pointer("/data/rsp/standards").Set(response, standardsJsonArray);

        rapidjson::Pointer("/data/rsp/osRead/mid").Set(response, bondResult.getOsRead()->getMidAsString());
        rapidjson::Pointer("/data/rsp/osRead/osVersion").Set(response, bondResult.getOsRead()->getOsVersionAsString());
        rapidjson::Pointer("/data/rsp/osRead/trMcuType/value").Set(response, bondResult.getOsRead()->getTrMcuType());
        rapidjson::Pointer("/data/rsp/osRead/trMcuType/trType").Set(response, bondResult.getOsRead()->getTrTypeAsString());
        rapidjson::Pointer("/data/rsp/osRead/trMcuType/fccCertified").Set(response, bondResult.getOsRead()->isFccCertified());
        rapidjson::Pointer("/data/rsp/osRead/trMcuType/mcuType").Set(response, bondResult.getOsRead()->getTrMcuTypeAsString());

        rapidjson::Pointer("/data/rsp/osRead/osBuild").Set(response, bondResult.getOsRead()->getOsBuildAsString());

        // RSSI [dBm]
        rapidjson::Pointer("/data/rsp/osRead/rssi").Set(response, bondResult.getOsRead()->getRssiAsString());

        // Supply voltage [V]
        rapidjson::Pointer("/data/rsp/osRead/supplyVoltage").Set(response, bondResult.getOsRead()->getSupplyVoltageAsString());

        // Flags
        rapidjson::Pointer("/data/rsp/osRead/flags/value").Set(response, bondResult.getOsRead()->getFlags());
        if (bondResult.getOsRead()->getDpaVer() >= 0x0417) {
          rapidjson::Pointer("/data/rsp/osRead/flags/insufficientOsVersion").Set(response, bondResult.getOsRead()->isInsufficientOs());
        } else {
          rapidjson::Pointer("/data/rsp/osRead/flags/insufficientOsBuild").Set(response, bondResult.getOsRead()->isInsufficientOs());
        }
        rapidjson::Pointer("/data/rsp/osRead/flags/interfaceType").Set(response, bondResult.getOsRead()->getInterfaceAsString());
        rapidjson::Pointer("/data/rsp/osRead/flags/dpaHandlerDetected").Set(response, bondResult.getOsRead()->isDpaHandlerDetected());
        rapidjson::Pointer("/data/rsp/osRead/flags/dpaHandlerNotDetectedButEnabled").Set(response, bondResult.getOsRead()->isDpaHandlerNotDetectedButEnabled());
        rapidjson::Pointer("/data/rsp/osRead/flags/noInterfaceSupported").Set(response, bondResult.getOsRead()->isNoInterfaceSupported());
        if (bondResult.getOsRead()->getDpaVer() >= 0x0413)
          rapidjson::Pointer("/data/rsp/osRead/flags/iqrfOsChanged").Set(response, bondResult.getOsRead()->isIqrfOsChanges());
        if (bondResult.getOsRead()->getDpaVer() >= 0x0416)
          rapidjson::Pointer("/data/rsp/osRead/flags/frcAggregationEnabled").Set(response, bondResult.getOsRead()->isFrcAggregationEnabled());

        // Slot limits
        rapidjson::Pointer("/data/rsp/osRead/slotLimits/value").Set(response, bondResult.getOsRead()->getSlotLimits());
        rapidjson::Pointer("/data/rsp/osRead/slotLimits/shortestTimeslot").Set(response, bondResult.getOsRead()->getShortestTimeSlotAsString());
        rapidjson::Pointer("/data/rsp/osRead/slotLimits/longestTimeslot").Set(response, bondResult.getOsRead()->getLongestTimeSlotAsString());

        if (bondResult.getOsRead()->is410Compliant())
        {
          // dpaVer, perNr
          Pointer("/data/rsp/osRead/dpaVer").Set(response, bondResult.getOsRead()->getDpaVerAsString());
          Pointer("/data/rsp/osRead/perNr").Set(response, bondResult.getOsRead()->getPerNr());

          Document::AllocatorType& allocator = response.GetAllocator();

          // embPers
          rapidjson::Value embPersJsonArray(kArrayType);
          for (std::set<int>::iterator it = bondResult.getOsRead()->getEmbedPer().begin(); it != bondResult.getOsRead()->getEmbedPer().end(); ++it)
          {
            embPersJsonArray.PushBack(*it, allocator);
          }
          Pointer("/data/rsp/osRead/embPers").Set(response, embPersJsonArray);

          // hwpId
          Pointer("/data/rsp/osRead/hwpId").Set(response, bondResult.getOsRead()->getHwpid());

          // hwpIdVer
          Pointer("/data/rsp/osRead/hwpIdVer").Set(response, bondResult.getOsRead()->getHwpidVer());

          // flags - int value
          Pointer("/data/rsp/osRead/enumFlags/value").Set(response, bondResult.getOsRead()->getFlags());

          // flags - parsed
          bool stdModeSupported = (bondResult.getOsRead()->getFlags() & 0b1) == 0b1;
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
          if (bondResult.getOsRead()->getDpaVer() >= 0x0400)
          {
            bool stdAndLpModeNetwork = (bondResult.getOsRead()->getFlags() & 0b100) == 0b100;
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
          for (std::set<int>::iterator it = bondResult.getOsRead()->getUserPer().begin(); it != bondResult.getOsRead()->getUserPer().end(); ++it)
          {
            userPerJsonArray.PushBack(*it, allocator);
          }
          Pointer("/data/rsp/osRead/userPers").Set(response, userPerJsonArray);
        }
      }

      // Set raw fields, if verbose mode is active
      if (m_comBondNode->getVerbose())
      {
        rapidjson::Value rawArray(kArrayType);
        Document::AllocatorType& allocator = response.GetAllocator();
        while (bondResult.isNextTransactionResult())
        {
          std::unique_ptr<IDpaTransactionResult2> transResult = bondResult.consumeNextTransactionResult();
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
      Pointer("/data/statusStr").Set(response, bondResult.getStatusStr());

      // Send message
      m_iMessagingSplitterService->sendMessage(*m_messaging, std::move(response));
    }

    //--------------------------
    // Creates and send response
    //--------------------------
    void createResponse(const int status, const std::string statusStr)
    {
      Document response;

      // Set common parameters
      Pointer("/mType").Set(response, m_msgType->m_type);
      Pointer("/data/msgId").Set(response, m_comBondNode->getMsgId());

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
        NAME_PAR( mType, msgType.m_type ) <<
        NAME_PAR( major, msgType.m_major ) <<
        NAME_PAR( minor, msgType.m_minor ) <<
        NAME_PAR( micro, msgType.m_micro )
      );

      // unsupported type of request
      if ( msgType.m_type != m_mTypeName_iqmeshNetworkBondNodeLocal )
        THROW_EXC( std::logic_error, "Unsupported message type: " << PAR( msgType.m_type ) );

      // Creating representation object
      ComIqmeshNetworkBondNodeLocal comBondNode(doc);
      m_msgType = &msgType;
      m_messaging = &messaging;
      m_comBondNode = &comBondNode;

      // Parsing and checking service parameters
      try
      {
        m_bondNodeParams = comBondNode.getBondNodeInputParams();
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
        // Bond node result
        BondResult bondResult;

        // Bond node
        bondNode(bondResult);

        // Create and send response
        createResponse(bondResult);
      } catch (const std::exception& e) {
        m_exclusiveAccess.reset();
        THROW_EXC_TRC_WAR(std::logic_error, e.what());
      }

      // release exclusive access
      m_exclusiveAccess.reset();
      TRC_FUNCTION_LEAVE("");
    }

    void activate( const shape::Properties *props )
    {
      TRC_FUNCTION_ENTER( "" );
      TRC_INFORMATION( std::endl <<
                       "************************************" << std::endl <<
                       "BondNodeLocalService instance activate" << std::endl <<
                       "************************************"
      );

      (void)props;

      // for the sake of register function parameters
      std::vector<std::string> supportedMsgTypes =
      {
        m_mTypeName_iqmeshNetworkBondNodeLocal
      };

      m_iMessagingSplitterService->registerFilteredMsgHandler(
        supportedMsgTypes,
        [&](const MessagingInstance &messaging, const IMessagingSplitterService::MsgType & msgType, rapidjson::Document doc){
          handleMsg(messaging, msgType, std::move(doc));
        }
      );

      TRC_FUNCTION_LEAVE( "" )
    }

    void deactivate()
    {
      TRC_FUNCTION_ENTER( "" );
      TRC_INFORMATION( std::endl <<
                       "************************************" << std::endl <<
                       "BondNodeLocalService instance deactivate" << std::endl <<
                       "************************************"
      );

      std::vector<std::string> supportedMsgTypes =
      {
        m_mTypeName_iqmeshNetworkBondNodeLocal
      };

      m_iMessagingSplitterService->unregisterFilteredMsgHandler( supportedMsgTypes );

      TRC_FUNCTION_LEAVE( "" );
    }

    void modify( const shape::Properties *props )
    {
      (void)props;
    }

    void attachInterface( IIqrfDpaService* iface )
    {
      m_iIqrfDpaService = iface;
    }

    void detachInterface( IIqrfDpaService* iface )
    {
      if ( m_iIqrfDpaService == iface ) {
        m_iIqrfDpaService = nullptr;
      }
    }

    void attachInterface( IJsCacheService* iface )
    {
      m_iJsCacheService = iface;
    }

    void detachInterface( IJsCacheService* iface )
    {
      if ( m_iJsCacheService == iface ) {
        m_iJsCacheService = nullptr;
      }
    }

    void attachInterface( IMessagingSplitterService* iface )
    {
      m_iMessagingSplitterService = iface;
    }

    void detachInterface( IMessagingSplitterService* iface )
    {
      if ( m_iMessagingSplitterService == iface ) {
        m_iMessagingSplitterService = nullptr;
      }
    }

  };


  BondNodeLocalService::BondNodeLocalService()
  {
    m_imp = shape_new Imp( *this );
  }

  BondNodeLocalService::~BondNodeLocalService()
  {
    delete m_imp;
  }


  void BondNodeLocalService::attachInterface( iqrf::IIqrfDpaService* iface )
  {
    m_imp->attachInterface( iface );
  }

  void BondNodeLocalService::detachInterface( iqrf::IIqrfDpaService* iface )
  {
    m_imp->detachInterface( iface );
  }

  void BondNodeLocalService::attachInterface( iqrf::IJsCacheService* iface )
  {
    m_imp->attachInterface( iface );
  }

  void BondNodeLocalService::detachInterface( iqrf::IJsCacheService* iface )
  {
    m_imp->detachInterface( iface );
  }

  void BondNodeLocalService::attachInterface( iqrf::IMessagingSplitterService* iface )
  {
    m_imp->attachInterface( iface );
  }

  void BondNodeLocalService::detachInterface( iqrf::IMessagingSplitterService* iface )
  {
    m_imp->detachInterface( iface );
  }

  void BondNodeLocalService::attachInterface( shape::ITraceService* iface )
  {
    shape::Tracer::get().addTracerService( iface );
  }

  void BondNodeLocalService::detachInterface( shape::ITraceService* iface )
  {
    shape::Tracer::get().removeTracerService( iface );
  }


  void BondNodeLocalService::activate( const shape::Properties *props )
  {
    m_imp->activate( props );
  }

  void BondNodeLocalService::deactivate()
  {
    m_imp->deactivate();
  }

  void BondNodeLocalService::modify( const shape::Properties *props )
  {
    m_imp->modify( props );
  }

}
