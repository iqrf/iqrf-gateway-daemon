#define IAutonetwork_EXPORTS

#include "Autonetwork.h"
#include "Trace.h"
#include "ComAutonetwork.h"
#include "iqrf__Autonetwork.hxx"
#include <list>
#include <cmath>
#include <thread> 
#include <bitset>
#include <chrono>

TRC_INIT_MODULE(iqrf::Autonetwork);

using namespace rapidjson;

namespace {

  // helper functions
  std::string encodeHexaNum_CapitalLetters(uint16_t from)
  {
    std::ostringstream os;
    os.fill('0'); os.width(4);
    os << std::hex << std::uppercase << (int)from;
    return os.str();
  }

  // maximum number of repeats
  static const uint8_t REPEAT_MAX = 3;

  // Default bonding mask. No masking effect.
  static const uint8_t DEFAULT_BONDING_MASK = 0;

  // values of result error codes
  // service general fail code - may and probably will be changed later in the future
  static const int SERVICE_ERROR = 1000;

  static const int SERVICE_ERROR_INTERNAL = SERVICE_ERROR + 1;
  static const int SERVICE_ERROR_NO_COORD_OR_COORD_OS = SERVICE_ERROR + 2;
  static const int SERVICE_ERROR_GET_ADDRESSING_INFO = SERVICE_ERROR + 3;
  static const int SERVICE_ERROR_GET_BONDED_NODES = SERVICE_ERROR + 4;
  static const int SERVICE_ERROR_GET_DISCOVERED_NODES = SERVICE_ERROR + 5;
  static const int SERVICE_ERROR_UNBONDED_NODES = SERVICE_ERROR + 6;
  static const int SERVICE_ERROR_SET_HOPS = SERVICE_ERROR + 7;
  static const int SERVICE_ERROR_SET_DPA_PARAMS = SERVICE_ERROR + 8;
  static const int SERVICE_ERROR_PREBOND = SERVICE_ERROR + 9;
  static const int SERVICE_ERROR_PREBONDED_ALIVE = SERVICE_ERROR + 10;
  static const int SERVICE_ERROR_PREBONDED_MEMORY_READ = SERVICE_ERROR + 11;
  static const int SERVICE_ERROR_AUTHORIZE_BOND = SERVICE_ERROR + 12;
  static const int SERVICE_ERROR_REMOVE_BOND = SERVICE_ERROR + 13;
  static const int SERVICE_ERROR_REMOVE_BOND_AND_RESTART = SERVICE_ERROR + 14;
  static const int SERVICE_ERROR_CHECK_NEW_NODES = SERVICE_ERROR + 15;
  static const int SERVICE_ERROR_REMOVE_BOND_AT_COORDINATOR = SERVICE_ERROR + 16;
  static const int SERVICE_ERROR_RUN_DISCOVERY = SERVICE_ERROR + 17;
  static const int SERVICE_ERROR_EMPTY_WAWES = SERVICE_ERROR + 18;
};


namespace iqrf {

  // Holds information about errors, which encounter during autonetwork run
  class AutonetworkError {
  public:
    // Type of error
    enum class Type {
      NoError,
      NoCoordOrCoordOs,
      GetAddressingInfo,
      GetBondedNodes,
      GetDiscoveredNodes,
      UnbondedNodes,
      SetHops,
      SetDpaParams,
      Prebond,
      PrebondedAlive,
      PrebondedMemoryRead,
      AuthorizeBond,
      RemoveBond,
      RemoveBondAndRestart,
      CheckNewNodes,
      RemoveBondAtCoordinator,
      RunDiscovery,
      EmptyWaves
    };

    AutonetworkError() : m_type( Type::NoError ), m_message( "" ) {};
    AutonetworkError( Type errorType ) : m_type( errorType ), m_message( "" ) {};
    AutonetworkError( Type errorType, const std::string& message ) : m_type( errorType ), m_message( message ) {};

    Type getType() const { return m_type; };
    std::string getMessage() const { return m_message; };

    AutonetworkError& operator=( const AutonetworkError& error ) {
      if ( this == &error ) {
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


  /// \class BondResult
  /// \brief Result of bonding of a node.
  class AutonetworkResult {
  public:
    // information related to node newly added into the network
    struct NewNode {
      uint8_t address;
      uint32_t MID;
    };

  private:
    AutonetworkError m_error;

    uint8_t m_wave;
    uint8_t m_nodesNr;
    uint8_t m_newNodesNr;
    std::vector<NewNode> m_newNodes;
    bool m_lastWave = false;

    // transaction results
    std::list<std::unique_ptr<IDpaTransactionResult2>> m_transResults;

  public:
    AutonetworkError getError() const { return m_error; };

    void setError( const AutonetworkError& error ) {
      m_error = error;
    }

    void setWave(uint8_t wave) {
      m_wave = wave;
    }

    uint8_t getWave() {
      return m_wave;
    }
    
    void setNodesNr(uint8_t nodesNr) {
      m_nodesNr = nodesNr;
    }
    
    uint8_t getNodesNr() {
      return m_nodesNr;
    }
    
    void setNewNodesNr(uint8_t newNodesNr) {
      m_newNodesNr = newNodesNr;
    }

    uint8_t getNewNodesNr() {
      return m_newNodes.size();
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

    
    void setLastWave(bool lastWave) {
      m_lastWave = lastWave;
    }

    bool isLastWave() {
      return m_lastWave;
    }


    // adds transaction result into the list of results
    void addTransactionResult( std::unique_ptr<IDpaTransactionResult2>& transResult ) {
      m_transResults.push_back( std::move( transResult ) );
    }

    bool isNextTransactionResult() {
      return ( m_transResults.size() > 0 );
    }

    // consumes the first element in the transaction results list
    std::unique_ptr<IDpaTransactionResult2> consumeNextTransactionResult() {
      std::list<std::unique_ptr<IDpaTransactionResult2>>::iterator iter = m_transResults.begin();
      std::unique_ptr<IDpaTransactionResult2> tranResult = std::move( *iter );
      m_transResults.pop_front();
      return std::move( tranResult );
    }

  };



  // implementation class
  class Autonetwork::Imp {
  private:
    // parent object
    Autonetwork & m_parent;

    // message type: autonetwork
    // for temporal reasons
    const std::string m_mTypeName_Autonetwork = "iqmeshNetwork_Autonetwork";

    iqrf::IJsCacheService* m_iJsCacheService = nullptr;
    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    IIqrfDpaService* m_iIqrfDpaService = nullptr;
    std::unique_ptr<IIqrfDpaService::ExclusiveAccess> m_exclusiveAccess;

    uint8_t MAX_WAVES = MAX_ADDRESS;
    uint8_t MAX_EMPTY_WAVES = MAX_ADDRESS;

  public:
    Imp( Autonetwork& parent ) : m_parent( parent )
    {
    }

    ~Imp()
    {
    }

    void checkNodeAddr( const uint16_t nodeAddr )
    {
      if ( ( nodeAddr < 0 ) || ( nodeAddr > 0xEF ) ) {
        THROW_EXC(
          std::logic_error, "Node address outside of valid range. " << NAME_PAR_HEX( "Address", nodeAddr )
        );
      }
    }

    // parses bit array of nodes into bitmap
    std::bitset<MAX_ADDRESS> toNodesBitmap( const unsigned char* pData ) {
      std::bitset<MAX_ADDRESS> nodesMap;

      for ( int byteId = 0; byteId < 32; byteId++ ) {
        uint8_t bitComp = 1;

        for ( int bitId = 0; bitId < 8; bitId++ ) {
          int bitPos = byteId * 8 + bitId;

          if ( ( pData[byteId] & bitComp ) == bitComp ) {
            nodesMap.set(bitPos, true);
          }
          else {
            nodesMap.set(bitPos, false);
          }

          bitComp <<= 1;
        }
      }

      return nodesMap;
    }


    // check presence of Coordinator and OS peripherals on coordinator node
    void checkPresentCoordAndCoordOs(AutonetworkResult& autonetworkResult)
    {
      TRC_FUNCTION_ENTER("");

      DpaMessage addrInfoRequest;
      DpaMessage::DpaPacket_t perEnumPacket;
      perEnumPacket.DpaRequestPacket_t.NADR = 0x00;
      perEnumPacket.DpaRequestPacket_t.PNUM = 0xFF;
      perEnumPacket.DpaRequestPacket_t.PCMD = 0x3F;
      perEnumPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
      addrInfoRequest.DataToBuffer(perEnumPacket.Buffer, sizeof(TDpaIFaceHeader));

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> perEnumTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      try {
        perEnumTransaction = m_exclusiveAccess->executeDpaTransaction(addrInfoRequest);
        transResult = perEnumTransaction->get();
      }
      catch (std::exception& e) {
        TRC_WARNING("DPA transaction error : " << e.what());

        AutonetworkError error(AutonetworkError::Type::NoCoordOrCoordOs, e.what());
        autonetworkResult.setError(error);

        TRC_FUNCTION_LEAVE("");
        return;
      }

      TRC_DEBUG("Result from Device Exploration transaction as string:" << PAR(transResult->getErrorString()));

      IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

      // because of the move-semantics
      DpaMessage dpaResponse = transResult->getResponse();
      autonetworkResult.addTransactionResult(transResult);

      if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
        TRC_INFORMATION("Device exploration successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(addrInfoRequest.PeripheralType(), addrInfoRequest.NodeAddress())
          << PAR(addrInfoRequest.PeripheralCommand())
        );

        // parsing response pdata
        uns8* respData = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData;
        uint8_t embPers = respData[3];

        // checking peripherals
        if (!(embPers & 0x01 == 0x01)) {
          AutonetworkError error(AutonetworkError::Type::NoCoordOrCoordOs, "Coordinator peripheral NOT found.");
          autonetworkResult.setError(error);
        }

        if (!(embPers & 0x04 == 0x04)) {
          AutonetworkError error(AutonetworkError::Type::NoCoordOrCoordOs, "Coordinator OS peripheral NOT found.");
          autonetworkResult.setError(error);
        }

        TRC_FUNCTION_LEAVE("");
        return;
      }

      // transaction error
      if (errorCode < 0) {
        TRC_WARNING("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

        AutonetworkError error(AutonetworkError::Type::NoCoordOrCoordOs, "Transaction error.");
        autonetworkResult.setError(error);

        TRC_FUNCTION_LEAVE("");
        return;
      }

      // DPA error
      TRC_WARNING("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

      AutonetworkError error(AutonetworkError::Type::NoCoordOrCoordOs, "Dpa error.");
      autonetworkResult.setError(error);

      TRC_FUNCTION_LEAVE("");
    }

    // returns addressing info 
    TPerCoordinatorAddrInfo_Response getAddressingInfo(AutonetworkResult& autonetworkResult)
    {
      // get addesssing info
      TRC_FUNCTION_ENTER("");

      DpaMessage addrInfoRequest;
      DpaMessage::DpaPacket_t addrInfoPacket;
      addrInfoPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      addrInfoPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
      addrInfoPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_ADDR_INFO;
      addrInfoPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
      addrInfoRequest.DataToBuffer(addrInfoPacket.Buffer, sizeof(TDpaIFaceHeader));

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> addrInfoTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      try {
        addrInfoTransaction = m_exclusiveAccess->executeDpaTransaction(addrInfoRequest);
        transResult = addrInfoTransaction->get();
      }
      catch (std::exception& e) {
        TRC_WARNING("DPA transaction error : " << e.what());

        AutonetworkError error(AutonetworkError::Type::GetAddressingInfo, e.what());
        autonetworkResult.setError(error);

        THROW_EXC(std::logic_error, "DPA transaction error: " << e.what());
      }

      TRC_DEBUG("Result from Get addressing information transaction as string:" << PAR(transResult->getErrorString()));

      IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

      // because of the move-semantics
      DpaMessage dpaResponse = transResult->getResponse();
      autonetworkResult.addTransactionResult(transResult);

      if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
        TRC_INFORMATION("Get addressing information successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(addrInfoRequest.PeripheralType(), addrInfoRequest.NodeAddress())
          << PAR(addrInfoRequest.PeripheralCommand())
        );

        TRC_FUNCTION_LEAVE("");
        return dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerCoordinatorAddrInfo_Response;
      }

      // transaction error
      if (errorCode < 0) {
        TRC_WARNING("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

        AutonetworkError error(AutonetworkError::Type::GetAddressingInfo, "Transaction error.");
        autonetworkResult.setError(error);

        THROW_EXC(std::logic_error, "Transaction error." << NAME_PAR_HEX("Error code", errorCode));
      }

      // DPA error
      TRC_WARNING("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

      AutonetworkError error(AutonetworkError::Type::GetAddressingInfo, "Dpa error.");
      autonetworkResult.setError(error);

      THROW_EXC(std::logic_error, "Dpa error." << NAME_PAR_HEX("Error code", errorCode));
    }

    // returns map of bonded nodes
    std::bitset<MAX_ADDRESS> getBondedNodes(AutonetworkResult& autonetworkResult)
    {
      TRC_FUNCTION_ENTER("");

      DpaMessage getBondedNodesRequest;
      DpaMessage::DpaPacket_t getBondedNodesPacket;
      getBondedNodesPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      getBondedNodesPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
      getBondedNodesPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_BONDED_DEVICES;
      getBondedNodesPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;

      getBondedNodesRequest.DataToBuffer(getBondedNodesPacket.Buffer, sizeof(TDpaIFaceHeader));

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> getBondedNodesTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      try {          
        getBondedNodesTransaction = m_exclusiveAccess->executeDpaTransaction(getBondedNodesRequest);
        transResult = getBondedNodesTransaction->get();
      }
      catch (std::exception& e) {
        TRC_WARNING("DPA transaction error : " << e.what());

        AutonetworkError error(AutonetworkError::Type::GetBondedNodes, e.what());
        autonetworkResult.setError(error);

        THROW_EXC(std::logic_error, "DPA transaction error.");
      }

      TRC_DEBUG("Result from get bonded nodes transaction as string:" << PAR(transResult->getErrorString()));

      IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

      // because of the move-semantics
      DpaMessage dpaResponse = transResult->getResponse();
      autonetworkResult.addTransactionResult(transResult);

      if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
        TRC_INFORMATION("Get bonded nodes successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(getBondedNodesRequest.PeripheralType(), getBondedNodesRequest.NodeAddress())
          << PAR(getBondedNodesRequest.PeripheralCommand())
        );

        // get response data
        const unsigned char* pData = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData;

        TRC_FUNCTION_LEAVE("");
        return toNodesBitmap(pData);
      }

      // transaction error
      if (errorCode < 0) {
        TRC_WARNING("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

        AutonetworkError error(AutonetworkError::Type::GetBondedNodes, "Transaction error.");
        autonetworkResult.setError(error);

        THROW_EXC(std::logic_error, "Transaction error. " << NAME_PAR_HEX("Error code", errorCode));
      }
      else {
        // DPA error
        TRC_WARNING("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

        AutonetworkError error(AutonetworkError::Type::GetBondedNodes, "Dpa error.");
        autonetworkResult.setError(error);

        THROW_EXC(std::logic_error, "DPA error. " << NAME_PAR_HEX("Error code", errorCode));
      }
    }

    // returns map of discovered nodes
    std::bitset<MAX_ADDRESS> getDiscoveredNodes(AutonetworkResult& autonetworkResult)
    {
      TRC_FUNCTION_ENTER("");

      DpaMessage getDiscoveredNodesRequest;
      DpaMessage::DpaPacket_t getDiscoveredNodesPacket;
      getDiscoveredNodesPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      getDiscoveredNodesPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
      getDiscoveredNodesPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_DISCOVERED_DEVICES;
      getDiscoveredNodesPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;

      getDiscoveredNodesRequest.DataToBuffer(getDiscoveredNodesPacket.Buffer, sizeof(TDpaIFaceHeader));

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> getDiscoveredNodesTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      
      try {
        getDiscoveredNodesTransaction = m_exclusiveAccess->executeDpaTransaction(getDiscoveredNodesRequest);
        transResult = getDiscoveredNodesTransaction->get();
      }
      catch (std::exception& e) {
        TRC_WARNING("DPA transaction error : " << e.what());

        AutonetworkError error(AutonetworkError::Type::GetDiscoveredNodes, e.what());
        autonetworkResult.setError(error);

        THROW_EXC(std::logic_error, "DPA transaction error: " << e.what());
      }

      TRC_DEBUG("Result from Get discovered nodes transaction as string:" << PAR(transResult->getErrorString()));

      IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

      // because of the move-semantics
      DpaMessage dpaResponse = transResult->getResponse();
      autonetworkResult.addTransactionResult(transResult);

      if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
        TRC_INFORMATION("Get discovered nodes successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(getDiscoveredNodesRequest.PeripheralType(), getDiscoveredNodesRequest.NodeAddress())
          << PAR(getDiscoveredNodesRequest.PeripheralCommand())
        );

        // get response data
        const unsigned char* pData = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData;

        TRC_FUNCTION_LEAVE("");
        return toNodesBitmap(pData);
      }

      // transaction error
      if (errorCode < 0) {
        TRC_WARNING("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

        AutonetworkError error(AutonetworkError::Type::GetDiscoveredNodes, "Transaction error.");
        autonetworkResult.setError(error);

        THROW_EXC(std::logic_error, "Transaction error. " << NAME_PAR_HEX("Error code", errorCode));
      }
      else {
        // DPA error
        TRC_WARNING("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

        AutonetworkError error(AutonetworkError::Type::GetDiscoveredNodes, "Dpa error.");
        autonetworkResult.setError(error);

        THROW_EXC(std::logic_error, "DPA error. " << NAME_PAR_HEX("Error code", errorCode));
      }
    }

    void updateNodesInfo(
      AutonetworkResult& autonetworkResult,
      uint8_t& bondedNodesNr,
      std::bitset<MAX_ADDRESS>& bondedNodes,
      uint8_t& discoveredNodesNr, 
      std::bitset<MAX_ADDRESS>& discoveredNodes
    )
    {
      TPerCoordinatorAddrInfo_Response addressingInfo = getAddressingInfo(autonetworkResult);
      
      bondedNodesNr = addressingInfo.DevNr;
      bondedNodes = getBondedNodes(autonetworkResult);

      discoveredNodes = getDiscoveredNodes(autonetworkResult);
      discoveredNodesNr = discoveredNodes.count();
    }

    // returns comma-separated list of nodes, whose bits are set to 1 in the bitmap
    std::string toNodesListStr(const std::bitset<MAX_ADDRESS>& nodes)
    {
      std::string nodesListStr;

      for (uint8_t nodeAddr = 1; nodeAddr <= MAX_ADDRESS; nodeAddr++) {
        if (nodes[nodeAddr]) {
          if (!nodesListStr.empty()) {
            nodesListStr += ", ";
          }

          nodesListStr += nodeAddr;
        }
      }

      return nodesListStr;
    }


    bool checkUnbondedNodes(
      const std::bitset<32>& bondedNodes,
      const std::bitset<32>& discoveredNodes
    ) 
    {
      std::stringstream unbondedNodesStream;
      
      for (uint8_t nodeAddr = 1; nodeAddr <= MAX_ADDRESS; nodeAddr++) {
        if (!bondedNodes[nodeAddr] && discoveredNodes[nodeAddr]) {
          unbondedNodesStream << nodeAddr << ", ";
        }
      }

      std::string unbondedNodesStr = unbondedNodesStream.str();

      if (unbondedNodesStr.empty()) {
        return true;
      }

      // log unbonded nodes
      TRC_INFORMATION("Nodes are discovered but NOT bonded. Discover the network!" << unbondedNodesStr);
      return false;
    }

    // sets DPA hops to the number of routers
    void setDpaHopsToTheNumberOfRouters(AutonetworkResult& autonetworkResult)
    {
      TRC_FUNCTION_ENTER("");

      DpaMessage setHopsRequest;
      DpaMessage::DpaPacket_t setHopsPacket;
      setHopsPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      setHopsPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
      setHopsPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_SET_HOPS;
      setHopsPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;

      uns8* pData = setHopsPacket.DpaRequestPacket_t.DpaMessage.Request.PData;
      pData[0] = 0xFF;
      pData[1] = 0xFF;

      setHopsRequest.DataToBuffer(setHopsPacket.Buffer, sizeof(TDpaIFaceHeader) + 2);

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> setHopsTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      try {
        setHopsTransaction = m_exclusiveAccess->executeDpaTransaction(setHopsRequest);
        transResult = setHopsTransaction->get();
      }
      catch (std::exception& e) {
        TRC_WARNING("Transaction error. " << e.what());

        AutonetworkError error(AutonetworkError::Type::SetHops, e.what());
        autonetworkResult.setError(error);

        THROW_EXC(std::logic_error, "DPA transaction error : " << e.what());
      }

      TRC_DEBUG("Result from Set Hops transaction as string:" << PAR(transResult->getErrorString()));

      IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

      // because of the move-semantics
      DpaMessage dpaResponse = transResult->getResponse();
      autonetworkResult.addTransactionResult(transResult);

      if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
        TRC_INFORMATION("Set Hops successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(setHopsRequest.PeripheralType(), setHopsRequest.NodeAddress())
          << PAR(setHopsRequest.PeripheralCommand())
        );

        TRC_FUNCTION_LEAVE("");
        return;
      }

      // transaction error
      if (errorCode < 0) {
        TRC_WARNING("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

        AutonetworkError error(AutonetworkError::Type::SetHops, "Transaction error.");
        autonetworkResult.setError(error);

        THROW_EXC(std::logic_error, "Transaction error. " << NAME_PAR_HEX("Error code", errorCode));
      }
      else {
        TRC_WARNING("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

        AutonetworkError error(AutonetworkError::Type::SetHops, "DPA error.");
        autonetworkResult.setError(error);

        THROW_EXC(std::logic_error, "DPA error. " << NAME_PAR_HEX("Error code", errorCode));
      }
    }

    // sets no LED indication and optimal timeslot
    void setNoLedAndOptimalTimeslot(AutonetworkResult& autonetworkResult)
    {
      TRC_FUNCTION_ENTER("");

      DpaMessage setDpaParamsRequest;
      DpaMessage::DpaPacket_t setDpaParamsPacket;
      setDpaParamsPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      setDpaParamsPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
      setDpaParamsPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_SET_DPAPARAMS;
      setDpaParamsPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;

      uns8* pData = setDpaParamsPacket.DpaRequestPacket_t.DpaMessage.Request.PData;
      pData[0] = 0;

      setDpaParamsRequest.DataToBuffer(setDpaParamsPacket.Buffer, sizeof(TDpaIFaceHeader) + 1);

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> setDpaParamsTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      try {
        setDpaParamsTransaction = m_exclusiveAccess->executeDpaTransaction(setDpaParamsRequest);
        transResult = setDpaParamsTransaction->get();
      }
      catch (std::exception& e) {
        TRC_WARNING("DPA transaction error : " << e.what());

        AutonetworkError error(AutonetworkError::Type::SetDpaParams, e.what());
        autonetworkResult.setError(error);

        THROW_EXC(std::logic_error, "DPA transaction error : " << e.what());
      }

      TRC_DEBUG("Result from Set DPA params transaction as string:" << PAR(transResult->getErrorString()));

      IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

      // because of the move-semantics
      DpaMessage dpaResponse = transResult->getResponse();
      autonetworkResult.addTransactionResult(transResult);

      if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
        TRC_INFORMATION("Set DPA params successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(setDpaParamsRequest.PeripheralType(), setDpaParamsRequest.NodeAddress())
          << PAR(setDpaParamsRequest.PeripheralCommand())
        );

        TRC_FUNCTION_LEAVE("");
        return;
      }

      // transaction error
      if (errorCode < 0) {
        TRC_WARNING("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

        AutonetworkError error(AutonetworkError::Type::SetDpaParams, "Transaction error.");
        autonetworkResult.setError(error);

        THROW_EXC(std::logic_error, "Transaction error. " << NAME_PAR_HEX("Error code", errorCode));
      }
      else {
        // DPA error
        TRC_WARNING("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

        AutonetworkError error(AutonetworkError::Type::SetDpaParams, "DPA error.");
        autonetworkResult.setError(error);

        THROW_EXC(std::logic_error, "DPA error. " << NAME_PAR_HEX("Error code", errorCode));
      }
    }

    // sets no LED indication and optimal timeslot
    void prebond(AutonetworkResult& autonetworkResult)
    {
      TRC_FUNCTION_ENTER("");

      DpaMessage smartConnectRequest;
      DpaMessage::DpaPacket_t smartConnectPacket;
      smartConnectPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      smartConnectPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
      smartConnectPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_SMART_CONNECT;
      smartConnectPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;

      // Set pData fields
      uns8* pData = smartConnectPacket.DpaRequestPacket_t.DpaMessage.Request.PData;

      // address
      pData[0] = TEMPORARY_ADDRESS;
      
      // bonding test retries
      pData[1] = 0;

      // IBK - zeroes
      std::fill_n(pData + 2, 16, 0);

      // MID - zeroes
      std::fill_n(pData + 18, 4, 0);

      // Set res0 to zero
      pData[22] = 0x00;
      pData[23] = 0x00;

      // Virtual Device Address - NOT USED
      pData[24] = 0xFF;

      // fill res1 with zeros
      std::fill_n(pData + 25, 9, 0);
      
      // user data - zeroes
      std::fill_n(pData + 34, 4, 0);

      // Data to buffer
      smartConnectRequest.DataToBuffer(smartConnectPacket.Buffer, sizeof(TDpaIFaceHeader) + 38);

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> smartConnectTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

    
      try {
        smartConnectTransaction = m_exclusiveAccess->executeDpaTransaction(smartConnectRequest);
        transResult = smartConnectTransaction->get();
      }
      catch (std::exception& e) {
        TRC_WARNING("DPA transaction error : " << e.what());

        AutonetworkError error(AutonetworkError::Type::Prebond, e.what());
        autonetworkResult.setError(error);

        TRC_FUNCTION_LEAVE("");
        return;
      }

      TRC_DEBUG("Result from Smart Connect transaction as string:" << PAR(transResult->getErrorString()));

      IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

      // because of the move-semantics
      DpaMessage dpaResponse = transResult->getResponse();
      autonetworkResult.addTransactionResult(transResult);

      if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
        TRC_INFORMATION("Smart Connect ok!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(smartConnectRequest.PeripheralType(), smartConnectRequest.NodeAddress())
          << PAR(smartConnectRequest.PeripheralCommand())
        );

        TRC_FUNCTION_LEAVE("");
        return;
      }

        // transaction error
        if (errorCode < 0) {
          TRC_WARNING("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

          AutonetworkError error(AutonetworkError::Type::Prebond, "Transaction error.");
          autonetworkResult.setError(error);

          TRC_FUNCTION_LEAVE("");
          return;
        } else {
          // DPA error
          TRC_WARNING("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

          AutonetworkError error(AutonetworkError::Type::Prebond, "Dpa error.");
          autonetworkResult.setError(error);

          TRC_FUNCTION_LEAVE("");
      }
    }

    std::vector<uint8_t> toPrebondedAliveNodes(const std::basic_string<uns8>& frcData)
    {
      std::vector<uint8_t> aliveNodes;

      uint16_t nodeId = 0;

      for (int byteId = 0; byteId <= 29; byteId++) {
        int bitComp = 1;
        for (int bitId = 0; bitId < 8; bitId++) {
          uint8_t bit0 = ((frcData[byteId] & bitComp) == bitComp) ? 1 : 0;

          if (bit0 == 1) {
            aliveNodes.push_back(nodeId);
          }

          nodeId++;
          bitComp *= 2;
        }
      }

      return aliveNodes;
    }

    // returns prebonded nodes, which are alive
    std::vector<uint8_t> getPrebondedAliveNodes(
      AutonetworkResult& autonetworkResult,
      const uint8_t nodeSeed
    )
    {
      TRC_FUNCTION_ENTER("");

      DpaMessage prebondedAliveRequest;
      DpaMessage::DpaPacket_t prebondedAlivePacket;
      prebondedAlivePacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      prebondedAlivePacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
      prebondedAlivePacket.DpaRequestPacket_t.PCMD = CMD_FRC_SEND;
      prebondedAlivePacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;

      // Set pData fields
      uns8* pData = prebondedAlivePacket.DpaRequestPacket_t.DpaMessage.Request.PData;

      // FRC Command
      pData[0] = FRC_PrebondedAlive;

      // node seed
      pData[1] = nodeSeed;
      pData[2] = 0;

      prebondedAliveRequest.DataToBuffer(prebondedAlivePacket.Buffer, sizeof(TDpaIFaceHeader) + 2);

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> prebondedAliveTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      try {
        prebondedAliveTransaction = m_exclusiveAccess->executeDpaTransaction(prebondedAliveRequest);
        transResult = prebondedAliveTransaction->get();
      }
      catch (std::exception& e) {
        TRC_WARNING("DPA transaction error : " << e.what());

        AutonetworkError error(AutonetworkError::Type::PrebondedAlive, e.what());
        autonetworkResult.setError(error);

        THROW_EXC(std::logic_error, "DPA transaction error:" << e.what());
      }

      TRC_DEBUG("Result from FRC Prebonded Alive transaction as string:" << PAR(transResult->getErrorString()));


      // data from FRC
      std::basic_string<uns8> frcData;

      IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

      // because of the move-semantics
      DpaMessage dpaResponse = transResult->getResponse();
      autonetworkResult.addTransactionResult(transResult);

      if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
        TRC_INFORMATION("FRC Prebonded Alive successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(prebondedAliveRequest.PeripheralType(), prebondedAliveRequest.NodeAddress())
          << PAR(prebondedAliveRequest.PeripheralCommand())
        );

        // check status
        uns8 status = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.Status;
        if ((status >= 0x00) && (status <= 0xEF)) {
          TRC_INFORMATION("FRC Prebonded Alive status ok." << NAME_PAR_HEX("Status", status));
          frcData.append(
            dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.FrcData,
            DPA_MAX_DATA_LENGTH - sizeof(uns8)
          );
          TRC_DEBUG("Size of FRC data: " << PAR(frcData.size()));
        }
        else {
          TRC_WARNING("FRC Prebonded Alive NOT ok." << NAME_PAR_HEX("Status", status));

          AutonetworkError error(AutonetworkError::Type::PrebondedAlive, "Bad FRC status.");
          autonetworkResult.setError(error);
            
          THROW_EXC(std::logic_error, "Bad FRC status: " << PAR(status) );
        }
      }
      else {
        // transaction error
        if (errorCode < 0) {
          TRC_WARNING("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

          AutonetworkError error(AutonetworkError::Type::PrebondedAlive, "Transaction error.");
          autonetworkResult.setError(error);

          THROW_EXC(std::logic_error, "Transaction error." << NAME_PAR("Error code", errorCode));
        }

        // DPA error
        TRC_WARNING("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

        AutonetworkError error(AutonetworkError::Type::PrebondedAlive, "Dpa error.");
        autonetworkResult.setError(error);

        THROW_EXC(std::logic_error, "DPA error." << NAME_PAR("Error code", errorCode))
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
        extraResultTransaction = m_exclusiveAccess->executeDpaTransaction(extraResultRequest, 0);
        transResult = extraResultTransaction->get();
      }
      catch (std::exception& e) {
        TRC_WARNING("DPA transaction error : " << e.what());

        AutonetworkError error(AutonetworkError::Type::PrebondedAlive, e.what());
        autonetworkResult.setError(error);
          
        THROW_EXC(std::logic_error, "Transaction error:" << e.what());
      }

      TRC_DEBUG("Result from FRC write config extra result transaction as string:" << PAR(transResult->getErrorString()));

      errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

      // because of the move-semantics
      dpaResponse = transResult->getResponse();
      autonetworkResult.addTransactionResult(transResult);

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
            
          AutonetworkError error(AutonetworkError::Type::PrebondedAlive, "Transaction error.");
          autonetworkResult.setError(error);
            
          THROW_EXC(std::logic_error, "Transaction error. " << NAME_PAR_HEX("Error code", errorCode));
        } // DPA error
        else {
          TRC_WARNING("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

          AutonetworkError error(AutonetworkError::Type::PrebondedAlive, "Transaction error.");
          autonetworkResult.setError(error);
            
          THROW_EXC(std::logic_error, "DPA error. " << NAME_PAR_HEX("Error code", errorCode));
        }
      }

      TRC_FUNCTION_LEAVE("");
      return toPrebondedAliveNodes(frcData);
    }

    // sets selected nodes to specified PData of FRC command
    void setFRCSelectedNodes(uns8* pData, const std::vector<uint8_t>& selectedNodes)
    {
      // initialize to zero values
      memset(pData, 1, 30 * sizeof(uns8));

      for (uint16_t i : selectedNodes) {
        uns8 byteIndex = i / 8;
        uns8 bitIndex = i % 8;
        pData[1 + byteIndex] |= (uns8)pow(2, bitIndex);
      }
    }

    // returns list of prebonded MIDS for specified prebonded alive nodes
    std::list<uint32_t> getPrebondedMIDs(
      AutonetworkResult& autonetworkResult,
      const std::vector<uint8_t>& prebondedAliveNodes,
      const uint8_t nodeSeed
    )
    {
      std::list<uint32_t> prebondedMIDs;

      for (uint8_t offset = 0; offset < prebondedAliveNodes.size(); offset += 15) 
      {
        DpaMessage prebondedMemoryRequest;
        DpaMessage::DpaPacket_t prebondedMemoryPacket;
        prebondedMemoryPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        prebondedMemoryPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
        prebondedMemoryPacket.DpaRequestPacket_t.PCMD = CMD_FRC_SEND_SELECTIVE;
        prebondedMemoryPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;

        // Set pData fields
        uns8* pData = prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.Request.PData;

        // FRC Command
        pData[0] = FRC_MemoryReadPlus1;

        // selected nodes - prebonded alive nodes
        setFRCSelectedNodes(pData, prebondedAliveNodes);

        // node seed
        pData[31] = nodeSeed;

        // offset
        pData[32] = offset;

        // OS READ command
        pData[33] = 0x00;
        pData[34] = 0x00;
        pData[35] = PNUM_OS;
        pData[36] = CMD_OS_READ;
        pData[37] = 0x00;

        prebondedMemoryRequest.DataToBuffer(prebondedMemoryPacket.Buffer, sizeof(TDpaIFaceHeader) + 38);

        // issue the DPA request
        std::shared_ptr<IDpaTransaction2> prebondedMemoryTransaction;
        std::unique_ptr<IDpaTransactionResult2> transResult;

        try {
          prebondedMemoryTransaction = m_exclusiveAccess->executeDpaTransaction(prebondedMemoryRequest);
          transResult = prebondedMemoryTransaction->get();
        }
        catch (std::exception& e) {
          TRC_WARNING("DPA transaction error : " << e.what());

          AutonetworkError error(AutonetworkError::Type::PrebondedMemoryRead, e.what());
          autonetworkResult.setError(error);

          THROW_EXC(std::logic_error, "DPA transaction error:" << e.what());
        }

        TRC_DEBUG("Result from FRC Prebonded Memory Read transaction as string:" << PAR(transResult->getErrorString()));
        
        // data from FRC
        std::basic_string<uns8> mids1;

        IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

        // because of the move-semantics
        DpaMessage dpaResponse = transResult->getResponse();
        autonetworkResult.addTransactionResult(transResult);

        if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
          TRC_INFORMATION("FRC FRC Prebonded Memory Read successful!");
          TRC_DEBUG(
            "DPA transaction: "
            << NAME_PAR(prebondedMemoryRequest.PeripheralType(), prebondedMemoryRequest.NodeAddress())
            << PAR(prebondedMemoryRequest.PeripheralCommand())
          );

          // check status
          uns8 status = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.Status;
          if ((status < 0xFD) ) {
            TRC_INFORMATION("FRC FRC Prebonded Memory Read status ok." << NAME_PAR_HEX("Status", status));
            mids1.append(
              dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.FrcData,
              DPA_MAX_DATA_LENGTH - sizeof(uns8)
            );
            TRC_DEBUG("Size of FRC data: " << PAR(mids1.size()));
          }
          else {
            TRC_WARNING("FRC FRC Prebonded Memory Read NOT ok." << NAME_PAR_HEX("Status", status));

            AutonetworkError error(AutonetworkError::Type::PrebondedMemoryRead, "Bad FRC status.");
            autonetworkResult.setError(error);

            THROW_EXC(std::logic_error, "Bad FRC status: " << PAR(status));
          }
        }
        else {
          // transaction error
          if (errorCode < 0) {
            TRC_WARNING("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

            AutonetworkError error(AutonetworkError::Type::PrebondedMemoryRead, "Transaction error.");
            autonetworkResult.setError(error);

            THROW_EXC(std::logic_error, "Transaction error." << NAME_PAR_HEX("Error code", errorCode));
          }
          else {
            // DPA error
            TRC_WARNING("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

            AutonetworkError error(AutonetworkError::Type::PrebondedMemoryRead, "Dpa error.");
            autonetworkResult.setError(error);

            THROW_EXC(std::logic_error, "DPA error." << NAME_PAR_HEX("Error code", errorCode))
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
          extraResultTransaction = m_exclusiveAccess->executeDpaTransaction(extraResultRequest, 0);
          transResult = extraResultTransaction->get();
        }
        catch (std::exception& e) {
          TRC_WARNING("DPA transaction error : " << e.what());

          AutonetworkError error(AutonetworkError::Type::PrebondedMemoryRead, e.what());
          autonetworkResult.setError(error);

          THROW_EXC(std::logic_error, "DPa transaction error:" << e.what());
        }

        TRC_DEBUG("Result from FRC write config extra result transaction as string:" << PAR(transResult->getErrorString()));

        errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

        std::basic_string<uns8> mids2;

        // because of the move-semantics
        dpaResponse = transResult->getResponse();
        autonetworkResult.addTransactionResult(transResult);

        if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
          TRC_INFORMATION("FRC write config extra result successful!");
          TRC_DEBUG(
            "DPA transaction: "
            << NAME_PAR(extraResultRequest.PeripheralType(), extraResultRequest.NodeAddress())
            << PAR(extraResultRequest.PeripheralCommand())
          );

          mids2.append(
            dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData,
            64 - mids1.size()
          );
        }
        else {
          // transaction error
          if (errorCode < 0) {
            TRC_WARNING("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

            AutonetworkError error(AutonetworkError::Type::PrebondedMemoryRead, "Transaction error.");
            autonetworkResult.setError(error);

            THROW_EXC(std::logic_error, "Transaction error. " << NAME_PAR_HEX("Error code", errorCode));
          } // DPA error
          else {
            TRC_WARNING("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

            AutonetworkError error(AutonetworkError::Type::PrebondedMemoryRead, "Transaction error.");
            autonetworkResult.setError(error);

            THROW_EXC(std::logic_error, "DPA error. " << NAME_PAR_HEX("Error code", errorCode));
          }
        }

        std::basic_string<uint8_t> mids;
        mids.append(mids1);
        mids.append(mids2);

        std::stringstream midsSstream;

        for (int midIndex = 0; midIndex <= mids.size() - 4; midIndex += 4)
        {
          uint32_t mid = 0;
          mid = mids[midIndex + 0];
          mid += mids[midIndex + 1] << 8;
          mid += mids[midIndex + 2] << 16;
          mid += mids[midIndex + 3] << 24;

          if (mid != 0)
          {
            // FRC_PrebondedMemoryReadPlus1 returns +1
            mid--;
            prebondedMIDs.push_back(mid);

            midsSstream << ",";
            midsSstream << std::hex << std::uppercase << mid;
          }
        }

        TRC_INFORMATION("Prebonded MIDS: " << midsSstream.str());
      }

      return prebondedMIDs;
    }



    // returns next free address
    uint8_t getNextFreeAddr(
      const std::bitset<MAX_ADDRESS>& bondedNodes,
      const uint8_t fromAddr
    ) 
    {
      uint8_t origAddr = fromAddr;
      uint8_t checkAddr = fromAddr;

      for (; ; )
      {
        if (++checkAddr > MAX_ADDRESS) {
          checkAddr = 1;
        }

        if (!bondedNodes[checkAddr]) {
          return checkAddr;
        }

        if (checkAddr == origAddr) {
          THROW_EXC(std::logic_error, "NextFreeAddr: no free address");
        }
      }
    }

    // do bond authorization
    TPerCoordinatorAuthorizeBond_Response authorizeBond(
      AutonetworkResult& autonetworkResult,
      const uint8_t reqAddr,
      const uint32_t mid
    )
    {
      TRC_FUNCTION_ENTER("");

      DpaMessage authorizeBondRequest;
      DpaMessage::DpaPacket_t authorizeBondPacket;
      authorizeBondPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      authorizeBondPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
      authorizeBondPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_AUTHORIZE_BOND;
      authorizeBondPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;

      // Set pData fields
      uns8* pData = authorizeBondPacket.DpaRequestPacket_t.DpaMessage.Request.PData;

      // requested address
      pData[0] = reqAddr;

      // MID
      pData[1] = 0;
      pData[2] = 0;
      pData[3] = 0;
      pData[4] = 0;

      // Data to buffer
      authorizeBondRequest.DataToBuffer(authorizeBondPacket.Buffer, sizeof(TDpaIFaceHeader) + 5);

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> authorizeBondTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      try {
        authorizeBondTransaction = m_exclusiveAccess->executeDpaTransaction(authorizeBondRequest);
        transResult = authorizeBondTransaction->get();
      }
      catch (std::exception& e) {
        TRC_WARNING("DPA transaction error : " << e.what());

        AutonetworkError error(AutonetworkError::Type::AuthorizeBond, e.what());
        autonetworkResult.setError(error);

        THROW_EXC(std::logic_error, "DPA transaction error : " << e.what());
      }

      TRC_DEBUG("Result from Authorize Bond transaction as string:" << PAR(transResult->getErrorString()));

      IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

      // because of the move-semantics
      DpaMessage dpaResponse = transResult->getResponse();
      autonetworkResult.addTransactionResult(transResult);

      if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
        TRC_INFORMATION("Authorize Bond ok!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(authorizeBondRequest.PeripheralType(), authorizeBondRequest.NodeAddress())
          << PAR(authorizeBondRequest.PeripheralCommand())
        );

        TRC_FUNCTION_LEAVE("");
        return dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerCoordinatorAuthorizeBond_Response;
      }

      // transaction error
      if (errorCode < 0) {
        TRC_WARNING("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

        AutonetworkError error(AutonetworkError::Type::AuthorizeBond, "Transaction error.");
        autonetworkResult.setError(error);

        THROW_EXC(std::logic_error, "Transaction error. " << NAME_PAR_HEX("Error code", errorCode));
      }
      else {
        // DPA error
        TRC_WARNING("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

        AutonetworkError error(AutonetworkError::Type::AuthorizeBond, "DPA error.");
        autonetworkResult.setError(error);

        THROW_EXC(std::logic_error, "DPA error. " << NAME_PAR_HEX("Error code", errorCode));
      }
    }

    // remove specified bonded address at the side of coordinator
    void removeBond(AutonetworkResult& autonetworkResult, const uint8_t bondedAdd)
    {
      TRC_FUNCTION_ENTER("");

      DpaMessage removeBondRequest;
      DpaMessage::DpaPacket_t removeBondPacket;
      removeBondPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      removeBondPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
      removeBondPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_REMOVE_BOND;
      removeBondPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;

      // Set pData fields
      uns8* pData = removeBondPacket.DpaRequestPacket_t.DpaMessage.Request.PData;

      // bonded address
      pData[0] = bondedAdd;

      // Data to buffer
      removeBondRequest.DataToBuffer(removeBondPacket.Buffer, sizeof(TDpaIFaceHeader) + 1);

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> removeBondTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      try {
        removeBondTransaction = m_exclusiveAccess->executeDpaTransaction(removeBondRequest);
        transResult = removeBondTransaction->get();
      }
      catch (std::exception& e) {
        TRC_WARNING("DPA transaction error : " << e.what());

        AutonetworkError error(AutonetworkError::Type::RemoveBond, e.what());
        autonetworkResult.setError(error);

        THROW_EXC(std::logic_error, "DPA transaction error : " << e.what());
      }

      TRC_DEBUG("Result from Remove Bond transaction as string:" << PAR(transResult->getErrorString()));

      IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

      // because of the move-semantics
      DpaMessage dpaResponse = transResult->getResponse();
      autonetworkResult.addTransactionResult(transResult);

      if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
        TRC_INFORMATION("Remove Bond ok!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(removeBondRequest.PeripheralType(), removeBondRequest.NodeAddress())
          << PAR(removeBondRequest.PeripheralCommand())
        );

        TRC_FUNCTION_LEAVE("");
        return;
      }

      // transaction error
      if (errorCode < 0) {
        TRC_WARNING("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

        AutonetworkError error(AutonetworkError::Type::RemoveBond, "Transaction error.");
        autonetworkResult.setError(error);

        THROW_EXC(std::logic_error, "Transaction error. " << NAME_PAR_HEX("Error code", errorCode));
      }
      else {
        TRC_WARNING("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

        AutonetworkError error(AutonetworkError::Type::RemoveBond, "DPA error");
        autonetworkResult.setError(error);

        THROW_EXC(std::logic_error, "DPA error. " << NAME_PAR_HEX("Error code", errorCode));
      }
    }

    void authorizeMIDs(
      AutonetworkResult& autonetworkResult,
      const std::list<uint32_t>& prebondedMIDs, 
      uint8_t& bondedNodesNr,
      std::bitset<MAX_ADDRESS>& bondedNodes,
      uint8_t& discoveredNodesNr,
      std::bitset<MAX_ADDRESS>& discoveredNodes,
      uint8_t& nextAddr,
      std::map<uint8_t, uint32_t>& authorizedNodes
    )
    {
      TRC_FUNCTION_ENTER("");
      
      for (uint32_t moduleId : prebondedMIDs)
      {
        nextAddr = getNextFreeAddr(bondedNodes, nextAddr);

        uint8_t newAddr = 0xff;
        bool authorizeOK = false;

        try
        {
          TPerCoordinatorAuthorizeBond_Response response 
            = authorizeBond(autonetworkResult, nextAddr, moduleId);

          newAddr = response.BondAddr;
          uint8_t newDevicesCount = response.DevNr;

          TRC_INFORMATION(
            "Authorizing node: " << PAR(moduleId) << ", address: " <<  PAR(newAddr) 
            << ", devices count: " << PAR(newDevicesCount)
            );

          authorizeOK = true;
        }
        catch (std::exception& ex)
        {
          TRC_WARNING("Authorizing node " << PAR(moduleId) <<  " error");
        }

        if (authorizeOK) {
          authorizedNodes.insert(std::pair<uint8_t, uint32_t>(newAddr, moduleId));
          updateNodesInfo(autonetworkResult, bondedNodesNr, bondedNodes, discoveredNodesNr, discoveredNodes);
        }
        else {
          try
          {
            removeBond(autonetworkResult, newAddr);
          }
          catch (std::exception& ex)
          {
            TRC_WARNING("Error remove bond: " << PAR(newAddr));
          }
        }
      }

      TRC_FUNCTION_LEAVE("");
    }

    // checks new nodes
    std::vector<uint8_t> checkNewNodes(
      AutonetworkResult& autonetworkResult,
      uint8_t& frcStatusCheck
    )
    {
      TRC_FUNCTION_ENTER("");

      DpaMessage checkNewNodesRequest;
      DpaMessage::DpaPacket_t checkNewNodesPacket;
      checkNewNodesPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      checkNewNodesPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
      checkNewNodesPacket.DpaRequestPacket_t.PCMD = CMD_FRC_SEND;
      checkNewNodesPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;

      // Set pData fields
      uns8* pData = checkNewNodesPacket.DpaRequestPacket_t.DpaMessage.Request.PData;

      // FRC command - Prebonding
      pData[0] = FRC_Prebonding;

      // user data
      pData[1] = 0x01;
      pData[2] = 0x00;

      // Data to buffer
      checkNewNodesRequest.DataToBuffer(checkNewNodesPacket.Buffer, sizeof(TDpaIFaceHeader) + 3);

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> checkNewNodesTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      try {
        checkNewNodesTransaction = m_exclusiveAccess->executeDpaTransaction(checkNewNodesRequest);
        transResult = checkNewNodesTransaction->get();
      }
      catch (std::exception& e) {
        TRC_WARNING("DPA transaction error : " << e.what());

        AutonetworkError error(AutonetworkError::Type::CheckNewNodes, e.what());
        autonetworkResult.setError(error);

        THROW_EXC(std::logic_error, "DPA transaction error : " << e.what());
      }

      TRC_DEBUG("Result from Check new nodes transaction as string:" << PAR(transResult->getErrorString()));

      IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

      // because of the move-semantics
      DpaMessage dpaResponse = transResult->getResponse();
      autonetworkResult.addTransactionResult(transResult);

      if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
        TRC_INFORMATION("Check new nodes ok!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(checkNewNodesRequest.PeripheralType(), checkNewNodesRequest.NodeAddress())
          << PAR(checkNewNodesRequest.PeripheralCommand())
        );

        TPerFrcSend_Response response = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response;
        frcStatusCheck = response.Status;

        TRC_FUNCTION_LEAVE("");

        std::vector<uint8_t> frcDataVector(response.FrcData, response.FrcData + DPA_MAX_DATA_LENGTH - sizeof(uns8));
        return frcDataVector;
      }

      // transaction error
      if (errorCode < 0) {
        TRC_WARNING("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

        AutonetworkError error(AutonetworkError::Type::CheckNewNodes, "Transaction error");
        autonetworkResult.setError(error);

        THROW_EXC(std::logic_error, "Transaction error. " << NAME_PAR_HEX("Error code", errorCode));
      }
      else {
        // DPA error
        TRC_WARNING("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

        AutonetworkError error(AutonetworkError::Type::CheckNewNodes, "DPA error");
        autonetworkResult.setError(error);

        THROW_EXC(std::logic_error, "DPA error. " << NAME_PAR_HEX("Error code", errorCode));
      }
    }

    // removes bond at the node side and restarts OS
    void removeBondAndRestart(AutonetworkResult& autonetworkResult, const uint8_t nodeAddr)
    {
      TRC_FUNCTION_ENTER("");

      DpaMessage removeBondAndRestartRequest;
      DpaMessage::DpaPacket_t removeBondAndRestartPacket;
      removeBondAndRestartPacket.DpaRequestPacket_t.NADR = nodeAddr;
      removeBondAndRestartPacket.DpaRequestPacket_t.PNUM = PNUM_OS;
      removeBondAndRestartPacket.DpaRequestPacket_t.PCMD = CMD_OS_BATCH;
      removeBondAndRestartPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;

      // Set pData fields
      uns8* pData = removeBondAndRestartPacket.DpaRequestPacket_t.DpaMessage.Request.PData;

      // remove bond at node side
      pData[0] = PNUM_NODE;
      pData[1] = CMD_NODE_REMOVE_BOND;
      pData[2] = 0xFF;
      pData[3] = 0xFF;

      // restart OS
      pData[4] = PNUM_OS;
      pData[5] = CMD_OS_RESTART;
      pData[6] = 0xFF;
      pData[7] = 0xFF;

      // end of BATCH
      pData[8] = 0x00;

      // Data to buffer
      removeBondAndRestartRequest.DataToBuffer(removeBondAndRestartPacket.Buffer, sizeof(TDpaIFaceHeader) + 9);

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> removeBondAndRestartTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      try {
        removeBondAndRestartTransaction = m_exclusiveAccess->executeDpaTransaction(removeBondAndRestartRequest);
        transResult = removeBondAndRestartTransaction->get();
      }
      catch (std::exception& e) {
        TRC_WARNING("DPA transaction error : " << e.what());

        AutonetworkError error(AutonetworkError::Type::RemoveBondAndRestart, e.what());
        autonetworkResult.setError(error);

        THROW_EXC(std::logic_error, "DPA transaction error : " << e.what());
      }

      TRC_DEBUG("Result from Remove bond and restart (BATCH) transaction as string:" << PAR(transResult->getErrorString()));

      IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

      // because of the move-semantics
      DpaMessage dpaResponse = transResult->getResponse();
      autonetworkResult.addTransactionResult(transResult);

      if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
        TRC_INFORMATION("Remove bond and restart ok!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(removeBondAndRestartRequest.PeripheralType(), removeBondAndRestartRequest.NodeAddress())
          << PAR(removeBondAndRestartRequest.PeripheralCommand())
        );

        TRC_FUNCTION_LEAVE("");
        return;
      }

      // transaction error
      if (errorCode < 0) {
        TRC_WARNING("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

        AutonetworkError error(AutonetworkError::Type::RemoveBondAndRestart, "Transaction error.");
        autonetworkResult.setError(error);

        THROW_EXC(std::logic_error, "Transaction error. " << NAME_PAR_HEX("Error code", errorCode));
      }
      else {
        // DPA error
        TRC_WARNING("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

        AutonetworkError error(AutonetworkError::Type::RemoveBondAndRestart, "DPA error.");
        autonetworkResult.setError(error);

        THROW_EXC(std::logic_error, "DPA error. " << NAME_PAR_HEX("Error code", errorCode));
      }
    }

    // removes specified node address at the coordinator side
    void removeBondAtCoordinator(AutonetworkResult& autonetworkResult, const uint8_t addrToRemove)
    {
      TRC_FUNCTION_ENTER("");

      DpaMessage removeBondAtCoordinatorRequest;
      DpaMessage::DpaPacket_t removeBondAtCoordinatorPacket;
      removeBondAtCoordinatorPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      removeBondAtCoordinatorPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
      removeBondAtCoordinatorPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_REMOVE_BOND;
      removeBondAtCoordinatorPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;

      // Set pData fields
      uns8* pData = removeBondAtCoordinatorPacket.DpaRequestPacket_t.DpaMessage.Request.PData;

      // address to remove
      pData[0] = addrToRemove;

      // Data to buffer
      removeBondAtCoordinatorRequest.DataToBuffer(removeBondAtCoordinatorPacket.Buffer, sizeof(TDpaIFaceHeader) + 1);

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> removeBondAtCoordinatorTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      try {
        removeBondAtCoordinatorTransaction = m_exclusiveAccess->executeDpaTransaction(removeBondAtCoordinatorRequest);
        transResult = removeBondAtCoordinatorTransaction->get();
      }
      catch (std::exception& e) {
        TRC_WARNING("DPA transaction error : " << e.what());

        AutonetworkError error(AutonetworkError::Type::RemoveBondAtCoordinator, e.what());
        autonetworkResult.setError(error);

        THROW_EXC(std::logic_error, "DPA transaction error : " << e.what());
      }

      TRC_DEBUG("Result from Remove bond at Coordinator transaction as string:" << PAR(transResult->getErrorString()));

      IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

      // because of the move-semantics
      DpaMessage dpaResponse = transResult->getResponse();
      autonetworkResult.addTransactionResult(transResult);

      if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
        TRC_INFORMATION("Remove bond and restart ok!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(removeBondAtCoordinatorRequest.PeripheralType(), removeBondAtCoordinatorRequest.NodeAddress())
          << PAR(removeBondAtCoordinatorRequest.PeripheralCommand())
        );

        TRC_FUNCTION_LEAVE("");
        return;
      }

      // transaction error
      if (errorCode < 0) {
        TRC_WARNING("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

        AutonetworkError error(AutonetworkError::Type::RemoveBondAtCoordinator, "Transaction error.");
        autonetworkResult.setError(error);

        THROW_EXC(std::logic_error, "Transaction error. " << NAME_PAR_HEX("Error code", errorCode));
      }
      else {
        // DPA error
        TRC_WARNING("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

        AutonetworkError error(AutonetworkError::Type::RemoveBondAtCoordinator, "DPA error.");
        autonetworkResult.setError(error);

        THROW_EXC(std::logic_error, "DPA error. " << NAME_PAR_HEX("Error code", errorCode));
      }
    }

    // runs discovery process
    void runDiscovery(
      AutonetworkResult& autonetworkResult, 
      const uint8_t txPower, 
      uint8_t&  discoveredNodesCnt
    )
    {
      TRC_FUNCTION_ENTER("");

      DpaMessage runDiscoveryRequest;
      DpaMessage::DpaPacket_t runDiscoveryPacket;
      runDiscoveryPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      runDiscoveryPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
      runDiscoveryPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_DISCOVERY;
      runDiscoveryPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;

      // Set pData fields
      uns8* pData = runDiscoveryPacket.DpaRequestPacket_t.DpaMessage.Request.PData;

      // TX power
      pData[0] = txPower;

      // Max address
      pData[1] = 0;

      // Data to buffer
      runDiscoveryRequest.DataToBuffer(runDiscoveryPacket.Buffer, sizeof(TDpaIFaceHeader) + 2);

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> runDiscoveryTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      try {
        runDiscoveryTransaction = m_exclusiveAccess->executeDpaTransaction(runDiscoveryRequest);
        transResult = runDiscoveryTransaction->get();
      }
      catch (std::exception& e) {
        TRC_WARNING("DPA transaction error : " << e.what());

        AutonetworkError error(AutonetworkError::Type::RunDiscovery, e.what());
        autonetworkResult.setError(error);

        THROW_EXC(std::logic_error, "DPA transaction error : " << e.what());
      }

      TRC_DEBUG("Result from Run discovery transaction as string:" << PAR(transResult->getErrorString()));

      IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

      // because of the move-semantics
      DpaMessage dpaResponse = transResult->getResponse();
      autonetworkResult.addTransactionResult(transResult);

      if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
        TRC_INFORMATION("Run discovery ok!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(runDiscoveryRequest.PeripheralType(), runDiscoveryRequest.NodeAddress())
          << PAR(runDiscoveryRequest.PeripheralCommand())
        );

        TPerCoordinatorDiscovery_Response response = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerCoordinatorDiscovery_Response;

        discoveredNodesCnt = response.DiscNr;

        TRC_FUNCTION_LEAVE("");
        return;
      }

      // transaction error
      if (errorCode < 0) {
        TRC_WARNING("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

        AutonetworkError error(AutonetworkError::Type::RunDiscovery, "Transaction error.");
        autonetworkResult.setError(error);

        THROW_EXC(std::logic_error, "Transaction error. " << NAME_PAR_HEX("Error code", errorCode));
      }
      else {
        // DPA error
        TRC_WARNING("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

        AutonetworkError error(AutonetworkError::Type::RunDiscovery, "DPA error.");
        autonetworkResult.setError(error);

        THROW_EXC(std::logic_error, "DPA error. " << NAME_PAR_HEX("Error code", errorCode));
      }
    }

    // processes the autonetwork algorithm
    void runAutonetwork(
      const uint8_t waves, 
      const uint8_t emptyWaves,
      const uint8_t discoveryTxPower,
      const ComAutonetwork& comAutonetwork,
      const IMessagingSplitterService::MsgType& msgType,
      const std::string& messagingId
    )
    {
      TRC_FUNCTION_ENTER( "" );

      AutonetworkResult autonetworkResult;

      autonetworkResult.setLastWave(false);

      std::bitset<MAX_ADDRESS> bondedNodes;
      uint8_t bondedNodesNr = 0;

      std::bitset<MAX_ADDRESS> discoveredNodes;
      uint8_t discoveredNodesNr = 0;

      // check, if Coordinator and OS peripherals are present at coordinator's node
      checkPresentCoordAndCoordOs(autonetworkResult);

      if (autonetworkResult.getError().getType() != AutonetworkError::Type::NoError) {
        goto SendResponse;
      }

      TRC_INFORMATION("Initial network check");

      try {
        updateNodesInfo(autonetworkResult, bondedNodesNr, bondedNodes, discoveredNodesNr, discoveredNodes);

        TRC_INFORMATION("Bonded nodes: " << toNodesListStr(bondedNodes));
        TRC_INFORMATION("Discovered nodes: " << toNodesListStr(discoveredNodes));

        // checks, if there are some nodes, which are discovered but NOT bonded
        if (!checkUnbondedNodes(bondedNodesNr, discoveredNodesNr))
        {
          AutonetworkError error(AutonetworkError::Type::UnbondedNodes, "Unbonded nodes.");
          autonetworkResult.setError(error);

          goto SendResponse;
        }

        setDpaHopsToTheNumberOfRouters(autonetworkResult);
        TRC_INFORMATION("Number of hops set to the number of routers");


        setNoLedAndOptimalTimeslot(autonetworkResult);
        TRC_INFORMATION("No LED indication and use of optimal time slot length");


        TRC_INFORMATION("Automatic network construction in progress");

        uint8_t origNodesCount = bondedNodesNr;
        int round = 1;
        int emptyRounds = 0;
        uint8_t nextAddr = MAX_ADDRESS;

        autonetworkResult.setNodesNr(bondedNodesNr);

        using std::chrono::system_clock;

        // main cycle
        for (; (bondedNodesNr != MAX_ADDRESS) && (round <= waves); round++)
        {
          TRC_INFORMATION("Orig nodes count: " << origNodesCount);
          TRC_INFORMATION("Round: " << round);

          time_t now = system_clock::to_time_t(system_clock::now());
          TRC_INFORMATION("Start time: " << PAR(ctime(&now)));

          autonetworkResult.setWave(round);

          TRC_INFORMATION("Prebonding");
          prebond(autonetworkResult);


          uint8_t virtFrcId = (uint8_t)(1 + round % 255);
          std::vector<uint8_t> prebondedAliveNodes;
          prebondedAliveNodes = getPrebondedAliveNodes(autonetworkResult, virtFrcId);
          

          // next iteration
          if (prebondedAliveNodes.empty()) {
            continue;
          }
          TRC_INFORMATION("Prebonded alive nodes: " << PAR(prebondedAliveNodes.size()));

          std::list<uint32_t> prebondedMIDs;
          prebondedMIDs = getPrebondedMIDs(autonetworkResult, prebondedAliveNodes, virtFrcId);
          

          // authorize MIDs
          std::list<uint8_t> removedNodes;
          std::map<uint8_t, uint32_t> newNodes;
          authorizeMIDs(
            autonetworkResult, 
            prebondedMIDs,
            bondedNodesNr,
            bondedNodes, 
            discoveredNodesNr,
            discoveredNodes,
            nextAddr,
            newNodes
          );
          

          // no new nodes - go to the next iteration
          if (newNodes.size() == 0) {
            emptyRounds++;

            if (emptyRounds == emptyWaves) {
              TRC_INFORMATION("Maximum number of consecutive empty waves reached.")
              
              AutonetworkError error(AutonetworkError::Type::EmptyWaves, "Maximum number of consecutive empty waves reached.");
              autonetworkResult.setError(error);

              goto SendResponse;
            }

            continue;
          }

          TRC_INFORMATION("Running FRC to check new nodes");

          uint8_t frcStatusCheck = 0xFE;
          std::vector<uint8_t> frcDataCheck;
          frcDataCheck = checkNewNodes(autonetworkResult, frcStatusCheck);
          
          if (frcStatusCheck >= 0xFE) {
            TRC_WARNING("FRC to check new nodes failed.")
          }
          else {
            for (std::pair<uint8_t, uint32_t> authorizedNode : newNodes) {
              if (!(((frcDataCheck[0 + authorizedNode.first / 8] >> (authorizedNode.first % 8)) & 0x01) == 0x00)) {
                continue;
              }

              TRC_INFORMATION("Removing bond: " << PAR(authorizedNode.first));
              try {
                removedNodes.push_back(authorizedNode.first);
                removeBondAndRestart(autonetworkResult, authorizedNode.first);
              }
              catch (std::exception& ex) {
                TRC_WARNING("Error removing bond: " << PAR(authorizedNode.first));
              }

              // Wait for sure
              std::this_thread::sleep_for(std::chrono::microseconds((bondedNodesNr + 1) * (2 * (MIN_TIMESLOT + 10))));

              removeBondAtCoordinator(autonetworkResult, authorizedNode.first);

              // delete removed nodes from newNodes
              for (uint8_t removedNode : removedNodes) {
                newNodes.erase(removedNode);
              }
            }
          }

          // no new nodes - go to next iteration
          if (newNodes.size() == 0) {
            updateNodesInfo(autonetworkResult, bondedNodesNr, bondedNodes, discoveredNodesNr, discoveredNodes);
            emptyRounds++;

            if (emptyRounds == emptyWaves) {
              TRC_INFORMATION("Maximum number of consecutive empty waves reached.")
              
              AutonetworkError error(AutonetworkError::Type::EmptyWaves, "Maximum number of consecutive empty waves reached.");
              autonetworkResult.setError(error);

              goto SendResponse;
            }

            continue;
          }

          // consecutive empty rounds
          emptyRounds = 0;

          TRC_INFORMATION("Running discovery");
          try {
            uint8_t discoveredNodesCnt = 0;

            runDiscovery(autonetworkResult, discoveryTxPower, discoveredNodesCnt);
            TRC_INFORMATION("Discovered nodes: " << PAR(discoveredNodesCnt));
          }
          catch (std::exception& ex) {
            TRC_WARNING("Running discovery failed.")
          }

          // how to implement waiting to finnish discovery process?
          // FINAL DECISION: we will not implement breaking of autonetwork service
          TRC_INFORMATION("Waiting for coordinator to finish discovery");

          updateNodesInfo(autonetworkResult, bondedNodesNr, bondedNodes, discoveredNodesNr, discoveredNodes);

          for (std::pair<uint8_t, uint32_t> newNode : newNodes) {
            autonetworkResult.putNewNode(newNode.first, newNode.second);
          }

          // last iteration
          if ((bondedNodesNr == MAX_ADDRESS) || (round == waves)) {
            goto SendResponse;
          }

          // send NOT last results
          Document responseDoc = createResponse(comAutonetwork.getMsgId(), msgType, autonetworkResult, comAutonetwork);
          m_iMessagingSplitterService->sendMessage(messagingId, std::move(responseDoc));

          // clear new nodes for the next wave
          autonetworkResult.clearNewNodes();
        }
      }
      catch (std::exception& ex) {
        TRC_WARNING("Error during algorithm run: " << ex.what());
      }

// creating and sending of message
    SendResponse:
      autonetworkResult.setLastWave(true);
      Document responseDoc = createResponse(comAutonetwork.getMsgId(), msgType, autonetworkResult, comAutonetwork);
      m_iMessagingSplitterService->sendMessage(messagingId, std::move(responseDoc));

      TRC_FUNCTION_LEAVE("");
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
      Pointer( "/mType" ).Set( response, msgType.m_type );
      Pointer( "/data/msgId" ).Set( response, msgId );

      // set result
      Pointer( "/data/status" ).Set( response, SERVICE_ERROR );
      Pointer( "/data/statusStr" ).Set( response, errorMsg );

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
    void setVerboseData( rapidjson::Document& response, AutonetworkResult& bondResult )
    {
      rapidjson::Value rawArray( kArrayType );
      Document::AllocatorType& allocator = response.GetAllocator();

      while ( bondResult.isNextTransactionResult() ) {
        std::unique_ptr<IDpaTransactionResult2> transResult = bondResult.consumeNextTransactionResult();
        rapidjson::Value rawObject( kObjectType );

        rawObject.AddMember(
          "request",
          encodeBinary( transResult->getRequest().DpaPacket().Buffer, transResult->getRequest().GetLength() ),
          allocator
        );

        rawObject.AddMember(
          "requestTs",
          encodeTimestamp( transResult->getRequestTs() ),
          allocator
        );

        rawObject.AddMember(
          "confirmation",
          encodeBinary( transResult->getConfirmation().DpaPacket().Buffer, transResult->getConfirmation().GetLength() ),
          allocator
        );

        rawObject.AddMember(
          "confirmationTs",
          encodeTimestamp( transResult->getConfirmationTs() ),
          allocator
        );

        rawObject.AddMember(
          "response",
          encodeBinary( transResult->getResponse().DpaPacket().Buffer, transResult->getResponse().GetLength() ),
          allocator
        );

        rawObject.AddMember(
          "responseTs",
          encodeTimestamp( transResult->getResponseTs() ),
          allocator
        );

        // add object into array
        rawArray.PushBack( rawObject, allocator );
      }

      // add array into response document
      Pointer( "/data/raw" ).Set( response, rawArray );
    }

    // creates response on the basis of bond result
    Document createResponse(
      const std::string& msgId,
      const IMessagingSplitterService::MsgType& msgType,
      AutonetworkResult& autonetworkResult,
      const ComAutonetwork& comAutonetwork
    )
    {
      Document response;

      // set common parameters
      Pointer( "/mType" ).Set( response, msgType.m_type );
      Pointer( "/data/msgId" ).Set( response, msgId );

      // checking of error
      AutonetworkError error = autonetworkResult.getError();

      if ( error.getType() != AutonetworkError::Type::NoError ) 
      {
        int status = SERVICE_ERROR;

        switch ( error.getType() ) {
          case AutonetworkError::Type::NoCoordOrCoordOs:
            status = SERVICE_ERROR_NO_COORD_OR_COORD_OS;
            break;

          case AutonetworkError::Type::GetAddressingInfo:
            status = SERVICE_ERROR_GET_ADDRESSING_INFO;
            break;

          case AutonetworkError::Type::GetBondedNodes:
            status = SERVICE_ERROR_GET_BONDED_NODES;
            break;

          case AutonetworkError::Type::GetDiscoveredNodes:
            status = SERVICE_ERROR_GET_DISCOVERED_NODES;
            break;
          
          case AutonetworkError::Type::UnbondedNodes:
            status = SERVICE_ERROR_UNBONDED_NODES;
            break;
          
          case AutonetworkError::Type::SetHops:
            status = SERVICE_ERROR_SET_HOPS;
            break;
        
          case AutonetworkError::Type::SetDpaParams:
            status = SERVICE_ERROR_SET_DPA_PARAMS;
            break;
          
          case AutonetworkError::Type::Prebond:
            status = SERVICE_ERROR_PREBOND;
            break;

          case AutonetworkError::Type::PrebondedAlive:
            status = SERVICE_ERROR_PREBONDED_ALIVE;
            break;
        
          case AutonetworkError::Type::PrebondedMemoryRead:
            status = SERVICE_ERROR_PREBONDED_MEMORY_READ;
            break;

          case AutonetworkError::Type::AuthorizeBond:
            status = SERVICE_ERROR_AUTHORIZE_BOND;
            break;

          case AutonetworkError::Type::RemoveBond:
            status = SERVICE_ERROR_REMOVE_BOND;
            break;
        
          case AutonetworkError::Type::RemoveBondAndRestart:
            status = SERVICE_ERROR_REMOVE_BOND_AND_RESTART;
            break;
        
          case AutonetworkError::Type::CheckNewNodes:
            status = SERVICE_ERROR_CHECK_NEW_NODES;
            break;
          
          case AutonetworkError::Type::RemoveBondAtCoordinator:
            status = SERVICE_ERROR_REMOVE_BOND_AT_COORDINATOR;
            break;
          
          case AutonetworkError::Type::RunDiscovery:
            status = SERVICE_ERROR_RUN_DISCOVERY;
            break;

          default:
            status = SERVICE_ERROR;
            break; 
        }

        // set raw fields, if verbose mode is active
        if ( comAutonetwork.getVerbose() ) {
          setVerboseData( response, autonetworkResult );
        }

        Pointer("/data/status").Set(response, status);
        Pointer("/data/statusStr").Set(response, error.getMessage());

        return response;
      }

      // no errors

      // rsp object
      rapidjson::Pointer( "/data/rsp/wave" ).Set( response, autonetworkResult.getWave() );
      rapidjson::Pointer( "/data/rsp/nodesNr" ).Set( response, autonetworkResult.getNodesNr() );
      rapidjson::Pointer("/data/rsp/newNodesNr").Set(response, autonetworkResult.getNewNodesNr());

      rapidjson::Value newNodesJsonArray(kArrayType);
      Document::AllocatorType& allocator = response.GetAllocator();
      for (AutonetworkResult::NewNode newNode : autonetworkResult.getNewNodes()) {
        rapidjson::Value newNodeObject(kObjectType);
        newNodeObject.AddMember("address", newNode.address, allocator);
        newNodeObject.AddMember("mid", newNode.MID, allocator);
        newNodesJsonArray.PushBack(newNodeObject, allocator);
      }
      Pointer("/data/rsp/newNodes").Set(response, newNodesJsonArray);
      
      // last wave indication
      rapidjson::Pointer("/data/rsp/lastWave").Set(response, autonetworkResult.isLastWave());

      // set raw fields, if verbose mode is active
      if ( comAutonetwork.getVerbose() ) {
        setVerboseData( response, autonetworkResult );
      }

      // status
      Pointer("/data/status").Set(response, 0);
      Pointer("/data/statusStr").Set(response, "ok");

      return response;
    }

    uint8_t parseAndCheckWaves(const int waves) {
      if ((waves < 0x00) || (waves > MAX_WAVES)) {
        THROW_EXC(
          std::out_of_range, "Waves outside of valid range. " << NAME_PAR_HEX("waves", waves)
        );
      }
      return waves;
    }

    uint8_t parseAndCheckEmptyWaves(const int emptyWaves) {
      if ((emptyWaves < 0x00) || (emptyWaves > MAX_EMPTY_WAVES)) {
        THROW_EXC(
          std::out_of_range, "emptyWaves outside of valid range. " << NAME_PAR_HEX("emptyWaves", emptyWaves)
        );
      }
      return emptyWaves;
    }

    uint8_t parseAndCheckDiscoveryTxPower(const int discoveryTxPower) {
      if ((discoveryTxPower < 0x00) || (discoveryTxPower > 0x07)) {
        THROW_EXC(
          std::out_of_range, "Discovery Tx power outside of valid range. " << NAME_PAR_HEX("discoveryTxPower", discoveryTxPower)
        );
      }
      return discoveryTxPower;
    }


    void handleMsg(
      const std::string& messagingId,
      const IMessagingSplitterService::MsgType& msgType,
      rapidjson::Document doc
    )
    {
      TRC_FUNCTION_ENTER(
        PAR( messagingId ) <<
        NAME_PAR( mType, msgType.m_type ) <<
        NAME_PAR( major, msgType.m_major ) <<
        NAME_PAR( minor, msgType.m_minor ) <<
        NAME_PAR( micro, msgType.m_micro )
      );

      // unsupported type of request
      if ( msgType.m_type != m_mTypeName_Autonetwork ) {
        THROW_EXC( std::logic_error, "Unsupported message type: " << PAR( msgType.m_type ) );
      }

      // creating representation object
      ComAutonetwork comAutonetwork( doc );

      // service input parameters
      uint8_t waves = 0;
      uint8_t emptyWaves = 0;
      uint8_t discoveryTxPower = 0;
      bool returnVerbose = false;

      // parsing and checking service parameters
      try {
        if (!comAutonetwork.isSetWaves()) {
          THROW_EXC(std::logic_error, "waves not set");
        }
        waves = parseAndCheckWaves(comAutonetwork.getWaves());

        if (!comAutonetwork.isSetEmptyWaves()) {
          THROW_EXC(std::logic_error, "emptyWaves not set");
        }
        emptyWaves = parseAndCheckEmptyWaves(comAutonetwork.getEmptyWaves());

        discoveryTxPower = parseAndCheckDiscoveryTxPower(comAutonetwork.getDiscoveryTxPower());

        returnVerbose = comAutonetwork.getVerbose();
      }
      // parsing and checking service parameters failed 
      catch ( std::exception& ex ) {
        Document failResponse = createCheckParamsFailedResponse( comAutonetwork.getMsgId(), msgType, ex.what() );
        m_iMessagingSplitterService->sendMessage( messagingId, std::move( failResponse ) );

        TRC_FUNCTION_LEAVE( "" );
        return;
      }

      // try to establish exclusive access
      try {
        m_exclusiveAccess = m_iIqrfDpaService->getExclusiveAccess();
      }
      catch (std::exception &e) {
        const char* errorStr = e.what();
        TRC_WARNING("Error while establishing exclusive DPA access: " << PAR(errorStr));

        Document failResponse = getExclusiveAccessFailedResponse(comAutonetwork.getMsgId(), msgType, errorStr);
        m_iMessagingSplitterService->sendMessage(messagingId, std::move(failResponse));

        TRC_FUNCTION_LEAVE("");
        return;
      }

      runAutonetwork(
        waves, emptyWaves, discoveryTxPower,
        comAutonetwork, msgType, messagingId
      );

      // release exclusive access
      m_exclusiveAccess.reset();

      TRC_FUNCTION_LEAVE( "" );
    }

    void activate( const shape::Properties *props )
    {
      TRC_FUNCTION_ENTER( "" );
      TRC_INFORMATION( std::endl <<
                       "************************************" << std::endl <<
                       "Autonetwork instance activate" << std::endl <<
                       "************************************"
      );

      // for the sake of register function parameters 
      std::vector<std::string> supportedMsgTypes =
      {
        m_mTypeName_Autonetwork
      };


      m_iMessagingSplitterService->registerFilteredMsgHandler(
        supportedMsgTypes,
        [&]( const std::string & messagingId, const IMessagingSplitterService::MsgType & msgType, rapidjson::Document doc )
      {
        handleMsg( messagingId, msgType, std::move( doc ) );
      } );

      TRC_FUNCTION_LEAVE( "" )
    }

    void deactivate()
    {
      TRC_FUNCTION_ENTER( "" );
      TRC_INFORMATION( std::endl <<
                       "************************************" << std::endl <<
                       "Autonetwork instance deactivate" << std::endl <<
                       "************************************"
      );

      std::vector<std::string> supportedMsgTypes =
      {
        m_mTypeName_Autonetwork
      };

      m_iMessagingSplitterService->unregisterFilteredMsgHandler( supportedMsgTypes );

      TRC_FUNCTION_LEAVE( "" );
    }

    void modify( const shape::Properties *props )
    {
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


  Autonetwork::Autonetwork()
  {
    m_imp = shape_new Imp( *this );
  }

  Autonetwork::~Autonetwork()
  {
    delete m_imp;
  }


  void Autonetwork::attachInterface( iqrf::IIqrfDpaService* iface )
  {
    m_imp->attachInterface( iface );
  }

  void Autonetwork::detachInterface( iqrf::IIqrfDpaService* iface )
  {
    m_imp->detachInterface( iface );
  }

  void Autonetwork::attachInterface( iqrf::IJsCacheService* iface )
  {
    m_imp->attachInterface( iface );
  }

  void Autonetwork::detachInterface( iqrf::IJsCacheService* iface )
  {
    m_imp->detachInterface( iface );
  }

  void Autonetwork::attachInterface( iqrf::IMessagingSplitterService* iface )
  {
    m_imp->attachInterface( iface );
  }

  void Autonetwork::detachInterface( iqrf::IMessagingSplitterService* iface )
  {
    m_imp->detachInterface( iface );
  }

  void Autonetwork::attachInterface( shape::ITraceService* iface )
  {
    shape::Tracer::get().addTracerService( iface );
  }

  void Autonetwork::detachInterface( shape::ITraceService* iface )
  {
    shape::Tracer::get().removeTracerService( iface );
  }


  void Autonetwork::activate( const shape::Properties *props )
  {
    m_imp->activate( props );
  }

  void Autonetwork::deactivate()
  {
    m_imp->deactivate();
  }

  void Autonetwork::modify( const shape::Properties *props )
  {
    m_imp->modify( props );
  }

}
