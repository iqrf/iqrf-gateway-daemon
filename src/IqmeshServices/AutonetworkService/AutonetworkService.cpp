#define IAutonetworkService_EXPORTS

#include "AutonetworkService.h"
#include "Trace.h"
#include "ComAutonetwork.h"
#include "iqrf__AutonetworkService.hxx"
#include <list>
#include <cmath>
#include <thread>
#include <bitset>
#include <chrono>

TRC_INIT_MODULE(iqrf::AutonetworkService);

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
  static const int SERVICE_ERROR_ALL_ADDRESS_ALLOCATED = SERVICE_ERROR + 19;
};


namespace iqrf {

  // Holds information about errors, which encounter during autonetwork run
  class AutonetworkError {
  public:
    // Type of error
    enum class Type {
      Internal,
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
      EmptyWaves,
      AllAddressAllocated
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
  class AutonetworkService::Imp {
  private:
    // parent object
    AutonetworkService & m_parent;

    // message type: autonetwork
    // for temporal reasons
    const std::string m_mTypeName_Autonetwork = "iqmeshNetwork_AutoNetwork";

    iqrf::IJsCacheService* m_iJsCacheService = nullptr;
    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    IIqrfDpaService* m_iIqrfDpaService = nullptr;
    std::unique_ptr<IIqrfDpaService::ExclusiveAccess> m_exclusiveAccess;

    uint8_t MAX_WAVES = MAX_ADDRESS;
    uint8_t MAX_EMPTY_WAVES = MAX_ADDRESS;
    uint8_t m_actionRetries = 0;

  public:
    Imp( AutonetworkService& parent ) : m_parent( parent )
    {
    }

    ~Imp()
    {
    }

    // Parses bit array of nodes into bitmap
    std::bitset<MAX_ADDRESS + 1> toNodesBitmap( const unsigned char* pData ) {
      std::bitset<MAX_ADDRESS + 1> nodesMap;

      for ( int byteId = 0; byteId < 29; byteId++ ) {
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

    // Check presence of Coordinator and OS peripherals on coordinator node
    void checkPresentCoordAndCoordOs( AutonetworkResult& autonetworkResult )
    {
      TRC_FUNCTION_ENTER( "" );
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage perEnumRequest;
        DpaMessage::DpaPacket_t perEnumPacket;
        perEnumPacket.DpaRequestPacket_t.NADR = 0x00;
        perEnumPacket.DpaRequestPacket_t.PNUM = 0xFF;
        perEnumPacket.DpaRequestPacket_t.PCMD = 0x3F;
        perEnumPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        perEnumRequest.DataToBuffer( perEnumPacket.Buffer, sizeof( TDpaIFaceHeader ) );
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat( perEnumRequest, transResult, m_actionRetries );
        TRC_DEBUG( "Result from Device Exploration transaction as string:" << PAR( transResult->getErrorString() ) );
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION( "Device exploration successful!" );
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR( Peripheral type, perEnumRequest.PeripheralType() )
          << NAME_PAR( Node address, perEnumRequest.NodeAddress() )
          << NAME_PAR( Command, (int)perEnumRequest.PeripheralCommand() )
        );
        // Parse response pdata
        uns8* respData = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData;
        uint8_t embPers = respData[3];
        // Check peripherals
        if ( ( embPers & 0x01 ) != 0x01 )
          THROW_EXC( std::logic_error, "Coordinator peripheral NOT found." );
        if ( ( embPers & 0x04 ) != 0x04 )
          THROW_EXC( std::logic_error, "Coordinator OS peripheral NOT found." );
        autonetworkResult.addTransactionResult( transResult );
        TRC_FUNCTION_LEAVE( "" );
      }
      catch ( std::exception& e )
      {
        AutonetworkError error( AutonetworkError::Type::NoCoordOrCoordOs, e.what() );
        autonetworkResult.setError( error );
        autonetworkResult.addTransactionResult( transResult );
        THROW_EXC( std::logic_error, e.what() );
      }
    }

    // Set FRC Param
    void setFrcParam( AutonetworkResult& autonetworkResult )
    {
      TRC_FUNCTION_ENTER( "" );
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
        setFrcParamPacket.DpaRequestPacket_t.DpaMessage.PerFrcSetParams_RequestResponse.FRCresponseTime = 0;
        setFrcParamRequest.DataToBuffer( setFrcParamPacket.Buffer, sizeof( TDpaIFaceHeader ) + sizeof( TPerFrcSetParams_RequestResponse ) );
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat( setFrcParamRequest, transResult, m_actionRetries );
        TRC_DEBUG( "Result from Set Hops transaction as string:" << PAR( transResult->getErrorString() ) );
        TRC_INFORMATION( "Set Hops successful!" );
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR( Peripheral type, setFrcParamRequest.PeripheralType() )
          << NAME_PAR( Node address, setFrcParamRequest.NodeAddress() )
          << NAME_PAR( Command, (int)setFrcParamRequest.PeripheralCommand() )
        );
        autonetworkResult.addTransactionResult( transResult );
        TRC_FUNCTION_LEAVE( "" );
      }
      catch ( std::exception& e )
      {
        AutonetworkError error( AutonetworkError::Type::SetHops, e.what() );
        autonetworkResult.setError( error );
        autonetworkResult.addTransactionResult( transResult );
        THROW_EXC( std::logic_error, e.what() );
      }
    }

    // Sets no LED indication and optimal timeslot
    void setNoLedAndOptimalTimeslot( AutonetworkResult& autonetworkResult )
    {
      TRC_FUNCTION_ENTER( "" );
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage setDpaParamsRequest;
        DpaMessage::DpaPacket_t setDpaParamsPacket;
        setDpaParamsPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        setDpaParamsPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
        setDpaParamsPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_SET_DPAPARAMS;
        setDpaParamsPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        setDpaParamsPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSetDpaParams_Request_Response.DpaParam = 0x00;
        setDpaParamsRequest.DataToBuffer( setDpaParamsPacket.Buffer, sizeof( TDpaIFaceHeader ) + sizeof( TPerCoordinatorSetDpaParams_Request_Response ) );
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat( setDpaParamsRequest, transResult, m_actionRetries );
        TRC_DEBUG( "Result from Set DPA params transaction as string:" << PAR( transResult->getErrorString() ) );
        TRC_INFORMATION( "Set DPA params successful!" );
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR( Peripheral type, setDpaParamsRequest.PeripheralType() )
          << NAME_PAR( Node address, setDpaParamsRequest.NodeAddress() )
          << NAME_PAR( Command, (int)setDpaParamsRequest.PeripheralCommand() )
        );
        autonetworkResult.addTransactionResult( transResult );
        TRC_FUNCTION_LEAVE( "" );
      }
      catch ( std::exception& e )
      {
        AutonetworkError error( AutonetworkError::Type::SetDpaParams, e.what() );
        autonetworkResult.setError( error );
        autonetworkResult.addTransactionResult( transResult );
        THROW_EXC( std::logic_error, e.what() );
      }
    }

    // Sets DPA hops to the number of routers
    void setDpaHopsToTheNumberOfRouters( AutonetworkResult& autonetworkResult )
    {
      TRC_FUNCTION_ENTER( "" );
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage setHopsRequest;
        DpaMessage::DpaPacket_t setHopsPacket;
        setHopsPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        setHopsPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
        setHopsPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_SET_HOPS;
        setHopsPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        setHopsPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSetHops_Request_Response.RequestHops = 0xff;
        setHopsPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSetHops_Request_Response.ResponseHops = 0xff;
        setHopsRequest.DataToBuffer( setHopsPacket.Buffer, sizeof( TDpaIFaceHeader ) + sizeof( TPerCoordinatorSetHops_Request_Response ) );
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat( setHopsRequest, transResult, m_actionRetries );
        TRC_DEBUG( "Result from Set Hops transaction as string:" << PAR( transResult->getErrorString() ) );
        TRC_INFORMATION( "Set Hops successful!" );
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR( Peripheral type, setHopsRequest.PeripheralType() )
          << NAME_PAR( Node address, setHopsRequest.NodeAddress() )
          << NAME_PAR( Command, (int)setHopsRequest.PeripheralCommand() )
        );
        autonetworkResult.addTransactionResult( transResult );
        TRC_FUNCTION_LEAVE( "" );
      }
      catch ( std::exception& e )
      {
        AutonetworkError error( AutonetworkError::Type::SetHops, e.what() );
        autonetworkResult.setError( error );
        autonetworkResult.addTransactionResult( transResult );
        THROW_EXC( std::logic_error, e.what() );
      }
    }

    // Returns addressing info 
    TPerCoordinatorAddrInfo_Response getAddressingInfo( AutonetworkResult& autonetworkResult )
    {
      TRC_FUNCTION_ENTER( "" );
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage addrInfoRequest;
        DpaMessage::DpaPacket_t addrInfoPacket;
        addrInfoPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        addrInfoPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
        addrInfoPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_ADDR_INFO;
        addrInfoPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        addrInfoRequest.DataToBuffer( addrInfoPacket.Buffer, sizeof( TDpaIFaceHeader ) );
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat( addrInfoRequest, transResult, m_actionRetries );
        TRC_DEBUG( "Result from Get addressing information transaction as string:" << PAR( transResult->getErrorString() ) );
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION( "Get addressing information successful!" );
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR( Peripheral type, addrInfoRequest.PeripheralType() )
          << NAME_PAR( Node address, addrInfoRequest.NodeAddress() )
          << NAME_PAR( Command, (int)addrInfoRequest.PeripheralCommand() )
        );
        autonetworkResult.addTransactionResult( transResult );
        TRC_FUNCTION_LEAVE( "" );
        return dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerCoordinatorAddrInfo_Response;
      }
      catch ( std::exception& e )
      {
        AutonetworkError error( AutonetworkError::Type::GetAddressingInfo, e.what() );
        autonetworkResult.setError( error );
        autonetworkResult.addTransactionResult( transResult );
        THROW_EXC( std::logic_error, e.what() );
      }
    }

    // Returns map of bonded nodes
    std::bitset<MAX_ADDRESS + 1> getBondedNodes( AutonetworkResult& autonetworkResult )
    {
      TRC_FUNCTION_ENTER( "" );
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
        getBondedNodesRequest.DataToBuffer( getBondedNodesPacket.Buffer, sizeof( TDpaIFaceHeader ) );
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat( getBondedNodesRequest, transResult, m_actionRetries );
        TRC_DEBUG( "Result from get bonded nodes transaction as string:" << PAR( transResult->getErrorString() ) );
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION( "Get bonded nodes successful!" );
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR( Peripheral type, getBondedNodesRequest.PeripheralType() )
          << NAME_PAR( Node address, getBondedNodesRequest.NodeAddress() )
          << NAME_PAR( Command, (int)getBondedNodesRequest.PeripheralCommand() )
        );
        // Get response data
        const unsigned char* pData = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData;
        autonetworkResult.addTransactionResult( transResult );
        TRC_FUNCTION_LEAVE( "" );
        return toNodesBitmap( pData );
      }
      catch ( std::exception& e )
      {
        AutonetworkError error( AutonetworkError::Type::GetBondedNodes, e.what() );
        autonetworkResult.setError( error );
        autonetworkResult.addTransactionResult( transResult );
        THROW_EXC( std::logic_error, e.what() );
      }
    }

    // Returns map of discovered nodes
    std::bitset<MAX_ADDRESS + 1> getDiscoveredNodes( AutonetworkResult& autonetworkResult )
    {
      TRC_FUNCTION_ENTER( "" );
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage getDiscoveredNodesRequest;
        DpaMessage::DpaPacket_t getDiscoveredNodesPacket;
        getDiscoveredNodesPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        getDiscoveredNodesPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
        getDiscoveredNodesPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_DISCOVERED_DEVICES;
        getDiscoveredNodesPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        getDiscoveredNodesRequest.DataToBuffer( getDiscoveredNodesPacket.Buffer, sizeof( TDpaIFaceHeader ) );
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat( getDiscoveredNodesRequest, transResult, m_actionRetries );
        TRC_DEBUG( "Result from Get discovered nodes transaction as string:" << PAR( transResult->getErrorString() ) );
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION( "Get discovered nodes successful!" );
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR( Peripheral type, getDiscoveredNodesRequest.PeripheralType() )
          << NAME_PAR( Node address, getDiscoveredNodesRequest.NodeAddress() )
          << NAME_PAR( Command, (int)getDiscoveredNodesRequest.PeripheralCommand() )
        );
        // Get response data
        const unsigned char* pData = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData;
        autonetworkResult.addTransactionResult( transResult );
        TRC_FUNCTION_LEAVE( "" );
        return toNodesBitmap( pData );
      }
      catch ( std::exception& e )
      {
        AutonetworkError error( AutonetworkError::Type::GetDiscoveredNodes, e.what() );
        autonetworkResult.setError( error );
        autonetworkResult.addTransactionResult( transResult );
        THROW_EXC( std::logic_error, e.what() );
      }
    }

    // Update node info
    void updateNodesInfo( AutonetworkResult& autonetworkResult, uint8_t& bondedNodesNr, std::bitset<MAX_ADDRESS + 1>& bondedNodes, uint8_t& discoveredNodesNr, std::bitset<MAX_ADDRESS + 1>& discoveredNodes )
    {
      TPerCoordinatorAddrInfo_Response addressingInfo = getAddressingInfo( autonetworkResult );

      bondedNodesNr = addressingInfo.DevNr;
      bondedNodes = getBondedNodes( autonetworkResult );

      discoveredNodes = getDiscoveredNodes( autonetworkResult );
      discoveredNodesNr = discoveredNodes.count();
    }

    // Returns comma-separated list of nodes, whose bits are set to 1 in the bitmap
    std::string toNodesListStr( const std::bitset<MAX_ADDRESS + 1>& nodes )
    {
      std::string nodesListStr;

      for ( uint8_t nodeAddr = 1; nodeAddr <= MAX_ADDRESS; nodeAddr++ ) {
        if ( nodes[nodeAddr] ) {
          if ( !nodesListStr.empty() ) {
            nodesListStr += ", ";
          }

          nodesListStr += nodeAddr;
        }
      }

      return nodesListStr;
    }

    // Check unbonded nodes
    bool checkUnbondedNodes( const std::bitset<MAX_ADDRESS + 1>& bondedNodes, const std::bitset<MAX_ADDRESS + 1>& discoveredNodes )
    {
      std::stringstream unbondedNodesStream;

      for ( uint8_t nodeAddr = 1; nodeAddr <= MAX_ADDRESS; nodeAddr++ ) {
        if ( !bondedNodes[nodeAddr] && discoveredNodes[nodeAddr] ) {
          unbondedNodesStream << nodeAddr << ", ";
        }
      }
      std::string unbondedNodesStr = unbondedNodesStream.str();
      if ( unbondedNodesStr.empty() ) {
        return true;
      }
      // Log unbonded nodes
      TRC_INFORMATION( "Nodes are discovered but NOT bonded. Discover the network!" << unbondedNodesStr );
      return false;
    }

    // SmartConnect
    void smartConnect( AutonetworkResult& autonetworkResult )
    {
      TRC_FUNCTION_ENTER( "" );
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage smartConnectRequest;
        DpaMessage::DpaPacket_t smartConnectPacket;
        smartConnectPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        smartConnectPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
        smartConnectPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_SMART_CONNECT;
        smartConnectPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        // Address
        smartConnectPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSmartConnect_Request.ReqAddr = TEMPORARY_ADDRESS;
        // Bonding test retries
        smartConnectPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSmartConnect_Request.BondingTestRetries = 0x00;
        // IBK - zeroes
        std::fill_n( smartConnectPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSmartConnect_Request.IBK, 16, 0 );
        // MID - zeroes
        std::fill_n( smartConnectPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSmartConnect_Request.MID, 4, 0 );
        // Set res0 to zero
        smartConnectPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSmartConnect_Request.reserved0[0x00] = 0x00;
        smartConnectPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSmartConnect_Request.reserved0[0x01] = 0x00;
        // Virtual Device Address - must be 0 like all other parameters see https://www.iqrf.org/DpaTechGuide/#3.2.19%20Smart%20Connect
        smartConnectPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSmartConnect_Request.VirtualDeviceAddress = 0x00;
        // Fill res1 with zeros
        std::fill_n( smartConnectPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSmartConnect_Request.reserved1, 9, 0 );
        // User data - zeroes
        std::fill_n( smartConnectPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSmartConnect_Request.UserData, 4, 0 );
        // Data to buffer
        smartConnectRequest.DataToBuffer( smartConnectPacket.Buffer, sizeof( TDpaIFaceHeader ) + sizeof( TPerCoordinatorSmartConnect_Request ) );
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat( smartConnectRequest, transResult, m_actionRetries );
        TRC_DEBUG( "Result from Smart Connect transaction as string:" << PAR( transResult->getErrorString() ) );
        TRC_INFORMATION( "Smart Connect ok!" );
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR( Peripheral type, smartConnectRequest.PeripheralType() )
          << NAME_PAR( Node address, smartConnectRequest.NodeAddress() )
          << NAME_PAR( Command, (int)smartConnectRequest.PeripheralCommand() )
        );
        autonetworkResult.addTransactionResult( transResult );
        TRC_FUNCTION_LEAVE( "" );
      }
      catch ( std::exception& e )
      {
        AutonetworkError error( AutonetworkError::Type::Prebond, e.what() );
        autonetworkResult.setError( error );
        autonetworkResult.addTransactionResult( transResult );
        THROW_EXC( std::logic_error, e.what() );
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

    // Returns prebonded nodes, which are alive
    std::vector<uint8_t> getPrebondedAliveNodes( AutonetworkResult& autonetworkResult, const uint8_t nodeSeed )
    {
      TRC_FUNCTION_ENTER( "" );
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage prebondedAliveRequest;
        DpaMessage::DpaPacket_t prebondedAlivePacket;
        prebondedAlivePacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        prebondedAlivePacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
        prebondedAlivePacket.DpaRequestPacket_t.PCMD = CMD_FRC_SEND;
        prebondedAlivePacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        // FRC Command
        prebondedAlivePacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.FrcCommand = FRC_PrebondedAlive;
        // Node seed
        prebondedAlivePacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[0x00] = nodeSeed;
        // 0x00
        prebondedAlivePacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[0x01] = 0;
        prebondedAliveRequest.DataToBuffer( prebondedAlivePacket.Buffer, sizeof( TDpaIFaceHeader ) + 3 );
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat( prebondedAliveRequest, transResult, m_actionRetries );
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_DEBUG( "Result from FRC Prebonded Alive transaction as string:" << PAR( transResult->getErrorString() ) );
        TRC_INFORMATION( "FRC Prebonded Alive successful!" );
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR( Peripheral type, prebondedAliveRequest.PeripheralType() )
          << NAME_PAR( Node address, prebondedAliveRequest.NodeAddress() )
          << NAME_PAR( Command, (int)prebondedAliveRequest.PeripheralCommand() )
        );
        // Data from FRC
        std::basic_string<uns8> frcData;
        // Check status
        uns8 status = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.Status;
        if ( ( status >= 0x00 ) && ( status <= 0xEF ) )
        {
          TRC_INFORMATION( "FRC Prebonded Alive status ok." << NAME_PAR_HEX( "Status", status ) );
          frcData.append( dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.FrcData, DPA_MAX_DATA_LENGTH - sizeof( uns8 ) );
          TRC_DEBUG( "Size of FRC data: " << PAR( frcData.size() ) );
          autonetworkResult.addTransactionResult( transResult );
        }
        else
        {
          TRC_WARNING( "FRC Prebonded Alive NOT ok." << NAME_PAR_HEX( "Status", status ) );
          AutonetworkError error( AutonetworkError::Type::PrebondedAlive, "Bad FRC status." );
          autonetworkResult.setError( error );
          THROW_EXC( std::logic_error, "Bad FRC status: " << PAR( status ) );
        }
        // Get extra results
        DpaMessage extraResultRequest;
        DpaMessage::DpaPacket_t extraResultPacket;
        extraResultPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        extraResultPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
        extraResultPacket.DpaRequestPacket_t.PCMD = CMD_FRC_EXTRARESULT;
        extraResultPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        extraResultRequest.DataToBuffer( extraResultPacket.Buffer, sizeof( TDpaIFaceHeader ) );
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat( extraResultRequest, transResult, m_actionRetries );
        TRC_DEBUG( "Result from FRC write config extra result transaction as string:" << PAR( transResult->getErrorString() ) );
        TRC_INFORMATION( "FRC write config extra result successful!" );
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR( Peripheral type, extraResultRequest.PeripheralType() )
          << NAME_PAR( Node address, extraResultRequest.NodeAddress() )
          << NAME_PAR( Command, (int)extraResultRequest.PeripheralCommand() )
        );
        frcData.append( dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData, 64 - frcData.size() );
        autonetworkResult.addTransactionResult( transResult );
        TRC_FUNCTION_LEAVE( "" );
        return toPrebondedAliveNodes( frcData );
      }
      catch ( std::exception& e )
      {
        AutonetworkError error( AutonetworkError::Type::PrebondedAlive, e.what() );
        autonetworkResult.setError( error );
        autonetworkResult.addTransactionResult( transResult );
        THROW_EXC( std::logic_error, e.what() );
      }
    }

    // Sets selected nodes to specified PData of FRC command
    void setFRCSelectedNodes(uns8* pData, const std::vector<uint8_t>& selectedNodes)
    {
      // Initialize to zero values
      memset(pData, 0, 30 * sizeof(uns8));
      for (uint16_t i : selectedNodes) 
      {
        uns8 byteIndex = i / 8;
        uns8 bitIndex = i % 8;
        pData[byteIndex] |= (0x01 << bitIndex);
      }
    }

    // Returns list of prebonded MIDS for specified prebonded alive nodes
    std::list<uint32_t> getPrebondedMIDs( AutonetworkResult& autonetworkResult, const std::vector<uint8_t>& prebondedAliveNodes, const uint8_t nodeSeed )
    {
      TRC_FUNCTION_ENTER( "" );
      std::list<uint32_t> prebondedMIDs;
      std::unique_ptr<IDpaTransactionResult2> transResult;
      for ( uint8_t offset = 0; offset < prebondedAliveNodes.size(); offset += 15 )
      {
        try
        {
          // Prepare DPA request
          DpaMessage prebondedMemoryRequest;
          DpaMessage::DpaPacket_t prebondedMemoryPacket;
          prebondedMemoryPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
          prebondedMemoryPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
          prebondedMemoryPacket.DpaRequestPacket_t.PCMD = CMD_FRC_SEND_SELECTIVE;
          prebondedMemoryPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
          // FRC Command
          prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.FrcCommand = FRC_PrebondedMemoryReadPlus1;
          // Selected nodes - prebonded alive nodes
          setFRCSelectedNodes( prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.SelectedNodes, prebondedAliveNodes );
          // Node seed
          prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x00] = nodeSeed;
          prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x01] = offset;
          // OS Read command
          prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x02] = 0xa0;
          prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x03] = 0x04;
          prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x04] = PNUM_OS;
          prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x05] = CMD_OS_READ;
          prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x06] = 0x00;
          prebondedMemoryRequest.DataToBuffer( prebondedMemoryPacket.Buffer, sizeof( TDpaIFaceHeader ) + 38 );
          // Execute the DPA request
          m_exclusiveAccess->executeDpaTransactionRepeat( prebondedMemoryRequest, transResult, m_actionRetries );
          TRC_DEBUG( "Result from FRC Prebonded Memory Read transaction as string:" << PAR( transResult->getErrorString() ) );
          DpaMessage dpaResponse = transResult->getResponse();
          TRC_INFORMATION( "FRC FRC Prebonded Memory Read successful!" );
          TRC_DEBUG(
            "DPA transaction: "
            << NAME_PAR( Peripheral type, prebondedMemoryRequest.PeripheralType() )
            << NAME_PAR( Node address, prebondedMemoryRequest.NodeAddress() )
            << NAME_PAR( Command, (int)prebondedMemoryRequest.PeripheralCommand() )
          );
          // Data from FRC
          std::basic_string<uns8> mids1;
          // Check status
          uns8 status = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.Status;
          if ( status < 0xFD ) 
          {
            TRC_INFORMATION( "FRC Prebonded Memory Read status ok." << NAME_PAR_HEX( "Status", (int)status ) );
            mids1.append(
              dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.FrcData,
              DPA_MAX_DATA_LENGTH - sizeof( uns8 )
            );
            TRC_DEBUG( "Size of FRC data: " << PAR( mids1.size() ) );
          }
          else
          {
            TRC_WARNING( "FRC Prebonded Memory Read NOT ok." << NAME_PAR_HEX( "Status", (int)status ) );
            AutonetworkError error( AutonetworkError::Type::PrebondedMemoryRead, "Bad FRC status." );
            autonetworkResult.setError( error );
            THROW_EXC( std::logic_error, "Bad FRC status: " << PAR( (int)status ) );
          }
          // Add FRC result
          autonetworkResult.addTransactionResult( transResult );

          // Get extra results
          DpaMessage extraResultRequest;
          DpaMessage::DpaPacket_t extraResultPacket;
          extraResultPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
          extraResultPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
          extraResultPacket.DpaRequestPacket_t.PCMD = CMD_FRC_EXTRARESULT;
          extraResultPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
          extraResultRequest.DataToBuffer( extraResultPacket.Buffer, sizeof( TDpaIFaceHeader ) );
          // Execute the DPA request
          m_exclusiveAccess->executeDpaTransactionRepeat( extraResultRequest, transResult, m_actionRetries );
          TRC_DEBUG( "Result from FRC write config extra result transaction as string:" << PAR( transResult->getErrorString() ) );
          dpaResponse = transResult->getResponse();
          TRC_INFORMATION( "FRC write config extra result successful!" );
          TRC_DEBUG(
            "DPA transaction: "
            << NAME_PAR( Peripheral type, extraResultRequest.PeripheralType() )
            << NAME_PAR( Node address, extraResultRequest.NodeAddress() )
            << NAME_PAR( Command, (int)extraResultRequest.PeripheralCommand() )
          );
          // Add FRC extra result
          autonetworkResult.addTransactionResult( transResult );
          std::basic_string<uns8> mids2;
          mids2.append( dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData, 64 - mids1.size() );
          std::basic_string<uint8_t> mids;
          mids.append( mids1 );
          mids.append( mids2 );
          std::stringstream midsSstream;
          for ( int midIndex = 0; midIndex <= mids.size() - 4; midIndex += 4 )
          {
            uint32_t mid = 0;
            mid = mids[midIndex + 0];
            mid += mids[midIndex + 1] << 8;
            mid += mids[midIndex + 2] << 16;
            mid += mids[midIndex + 3] << 24;

            if ( mid != 0 )
            {
              // FRC_PrebondedMemoryReadPlus1 returns +1
              mid--;
              prebondedMIDs.push_back( mid );
              midsSstream << std::hex << std::uppercase << mid;
              midsSstream << ",";
            }
          }
          TRC_INFORMATION( "Prebonded MIDS: " << midsSstream.str() );
        }
        catch ( std::exception& e )
        {
          AutonetworkError error( AutonetworkError::Type::PrebondedMemoryRead, e.what() );
          autonetworkResult.setError( error );
          autonetworkResult.addTransactionResult( transResult );
          THROW_EXC( std::logic_error, e.what() );
        }
      }
      TRC_FUNCTION_LEAVE( "" );
      return prebondedMIDs;
    }

    // Returns next free address 
    uint8_t getNextFreeAddr( const std::bitset<MAX_ADDRESS + 1>& bondedNodes, const uint8_t fromAddr )
    {
      uint8_t origAddr = fromAddr;
      uint8_t checkAddr = fromAddr;

      for ( ; ; )
      {
        if ( ++checkAddr > MAX_ADDRESS )
          checkAddr = 1;

        if ( bondedNodes[checkAddr] == 0 )
          return checkAddr;

        if ( checkAddr == origAddr )
          THROW_EXC( std::logic_error, "NextFreeAddr: no free address" );
      }
    }

    // Bond authorization
    TPerCoordinatorAuthorizeBond_Response authorizeBond( AutonetworkResult& autonetworkResult, const uint8_t reqAddr, const uint32_t mid )
    {
      TRC_FUNCTION_ENTER( "" );
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage authorizeBondRequest;
        DpaMessage::DpaPacket_t authorizeBondPacket;
        authorizeBondPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        authorizeBondPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
        authorizeBondPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_AUTHORIZE_BOND;
        authorizeBondPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        // Requested address
        authorizeBondPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorAuthorizeBond_Request.ReqAddr = reqAddr;
        // MID to authorize
        authorizeBondPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorAuthorizeBond_Request.MID[0x00] = mid & 0xff;
        authorizeBondPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorAuthorizeBond_Request.MID[0x01] = ( mid >> 8 ) & 0xff;
        authorizeBondPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorAuthorizeBond_Request.MID[0x02] = ( mid >> 16 ) & 0xff;
        authorizeBondPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorAuthorizeBond_Request.MID[0x03] = ( mid >> 24 ) & 0xff;
        // Data to buffer
        authorizeBondRequest.DataToBuffer( authorizeBondPacket.Buffer, sizeof( TDpaIFaceHeader ) + sizeof( TPerCoordinatorAuthorizeBond_Request ) );
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat( authorizeBondRequest, transResult, m_actionRetries );
        TRC_DEBUG( "Result from Authorize Bond transaction as string:" << PAR( transResult->getErrorString() ) );
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION( "Authorize Bond ok!" );
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR( Peripheral type, authorizeBondRequest.PeripheralType() )
          << NAME_PAR( Node address, authorizeBondRequest.NodeAddress() )
          << NAME_PAR( Command, (int)authorizeBondRequest.PeripheralCommand() )
        );
        std::unique_ptr<IDpaTransactionResult2> transResult;
        TRC_FUNCTION_LEAVE( "" );
        return dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerCoordinatorAuthorizeBond_Response;
      }
      catch ( std::exception& e )
      {
        AutonetworkError error( AutonetworkError::Type::AuthorizeBond, e.what() );
        autonetworkResult.setError( error );
        autonetworkResult.addTransactionResult( transResult );
        THROW_EXC( std::logic_error, e.what() );
      }
    }

    // Authorize MIDs
    void authorizeMIDs(
      AutonetworkResult& autonetworkResult,
      const std::list<uint32_t>& prebondedMIDs,
      uint8_t& bondedNodesNr,
      std::bitset<MAX_ADDRESS + 1>& bondedNodes,
      uint8_t& discoveredNodesNr,
      std::bitset<MAX_ADDRESS + 1>& discoveredNodes,
      uint8_t& nextAddr,
      std::map<uint8_t, uint32_t>& authorizedNodes
    )
    {
      TRC_FUNCTION_ENTER( "" );
      for ( uint32_t moduleId : prebondedMIDs )
      {
        nextAddr = getNextFreeAddr( bondedNodes, nextAddr );
        uint8_t newAddr = 0xff;
        try
        {
          TPerCoordinatorAuthorizeBond_Response response = authorizeBond( autonetworkResult, nextAddr, moduleId );
          newAddr = response.BondAddr;
          uint8_t newDevicesCount = response.DevNr;
          TRC_INFORMATION(
            "Authorizing node: " << PAR( moduleId ) << ", address: " << PAR( (int)newAddr )
            << ", devices count: " << PAR( (int)newDevicesCount )
          );
          authorizedNodes.insert( std::pair<uint8_t, uint32_t>( newAddr, moduleId ) );
          updateNodesInfo( autonetworkResult, bondedNodesNr, bondedNodes, discoveredNodesNr, discoveredNodes );
        }
        catch ( std::exception& ex )
        {
          TRC_WARNING( "Authorizing node " << PAR( moduleId ) << " error" );
          try
          {
            removeBondAtCoordinator( autonetworkResult, nextAddr );
          }
          catch ( std::exception& ex )
          {
            TRC_WARNING( "Error remove bond: " << PAR( nextAddr ) );
          }
        }
      }

      TRC_FUNCTION_LEAVE( "" );
    }

    // Checks new nodes
    std::vector<uint8_t> checkNewNodes( AutonetworkResult& autonetworkResult, uint8_t& frcStatusCheck )
    {
      TRC_FUNCTION_ENTER( "" );
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage checkNewNodesRequest;
        DpaMessage::DpaPacket_t checkNewNodesPacket;
        checkNewNodesPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        checkNewNodesPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
        checkNewNodesPacket.DpaRequestPacket_t.PCMD = CMD_FRC_SEND;
        checkNewNodesPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        // FRC command - Ping
        checkNewNodesPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.FrcCommand = FRC_Ping;
        // User data
        checkNewNodesPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[0x00] = 0x00;
        checkNewNodesPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[0x01] = 0x00;
        // Data to buffer
        checkNewNodesRequest.DataToBuffer( checkNewNodesPacket.Buffer, sizeof( TDpaIFaceHeader ) + 3 );
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat( checkNewNodesRequest, transResult, m_actionRetries );
        TRC_DEBUG( "Result from Check new nodes transaction as string:" << PAR( transResult->getErrorString() ) );
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION( "Check new nodes ok!" );
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR( Peripheral type, checkNewNodesRequest.PeripheralType() )
          << NAME_PAR( Node address, checkNewNodesRequest.NodeAddress() )
          << NAME_PAR( Command, (int)checkNewNodesRequest.PeripheralCommand() )
        );
        TPerFrcSend_Response response = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response;
        frcStatusCheck = response.Status;
        autonetworkResult.addTransactionResult( transResult );        
        std::vector<uint8_t> frcDataVector( response.FrcData, response.FrcData + DPA_MAX_DATA_LENGTH - sizeof( uns8 ) );
        TRC_FUNCTION_LEAVE( "" );
        return frcDataVector;
      }
      catch ( std::exception& e )
      {
        AutonetworkError error( AutonetworkError::Type::CheckNewNodes, e.what() );
        autonetworkResult.setError( error );
        autonetworkResult.addTransactionResult( transResult );
        THROW_EXC( std::logic_error, e.what() );
      }
    }

    // Removes new nodes, which not responded to control FRC (Ping)
    void removeNotRespondedNewNodes( AutonetworkResult& autonetworkResult, const std::vector<uint8_t>& notRespondedNewNodes )
    {
      TRC_FUNCTION_ENTER( "" );
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage frcAckBroadcastRequest;
        DpaMessage::DpaPacket_t frcAckBroadcastPacket;
        frcAckBroadcastPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        frcAckBroadcastPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
        frcAckBroadcastPacket.DpaRequestPacket_t.PCMD = CMD_FRC_SEND_SELECTIVE;
        frcAckBroadcastPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        // FRC - Acknowledge Broadcast - Bits
        frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.FrcCommand = FRC_AcknowledgedBroadcastBits;
        // Put selected nodes
        setFRCSelectedNodes( frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.SelectedNodes, notRespondedNewNodes );
        // Clear UserData
        memset( (void*)frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData, sizeof( frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData ), 0x00 );
        // Request length
        uint8_t requestLength = sizeof( TDpaIFaceHeader );
        requestLength += sizeof( frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.FrcCommand );
        requestLength += sizeof( frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.SelectedNodes );
        // Get DPA version
        IIqrfDpaService::CoordinatorParameters coordParams = m_iIqrfDpaService->getCoordinatorParameters();
        if ( coordParams.dpaVerWord >= 0x0400)
        {
          // DPA >= 0x0400 - send Remove bond command to Node peripheral
          frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x00] = 0x05;
          frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x01] = PNUM_NODE;
          frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x02] = CMD_NODE_REMOVE_BOND;
          frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x03] = HWPID_DoNotCheck >> 0x08;
          frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x04] = HWPID_DoNotCheck & 0xff;
        }
        else
        {
          // DPA < 0x0400 - send Batch command (Remove bond + restart)
          frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x00] = 16;
          frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x01] = PNUM_OS;
          frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x02] = CMD_OS_BATCH;
          frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x03] = HWPID_DoNotCheck >> 0x08;;
          frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x04] = HWPID_DoNotCheck & 0xff;
          // Remove bond
          frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x05] = 5;
          frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x06] = PNUM_NODE;
          frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x07] = CMD_NODE_REMOVE_BOND;
          frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x08] = HWPID_DoNotCheck >> 0x08;
          frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x09] = HWPID_DoNotCheck & 0xff;
          // Restart
          frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x0a] = 5;
          frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x0b] = PNUM_OS;
          frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x0c] = CMD_OS_RESTART;
          frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x0d] = HWPID_DoNotCheck >> 0x08;
          frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x0e] = HWPID_DoNotCheck & 0xff;
          // End of batch
          frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x0f] = 0;
        }
        requestLength += frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x00];
        // Data to buffer
        frcAckBroadcastRequest.DataToBuffer( frcAckBroadcastPacket.Buffer, requestLength );
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat( frcAckBroadcastRequest, transResult, m_actionRetries );
        TRC_DEBUG( "Result from Remove bond and restart (SELECTIVE BROADCAST BATCH) transaction as string:" << PAR( transResult->getErrorString() ) );
        TRC_INFORMATION( "Selective BATCH Remove bond and restart ok!" );
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR( Peripheral type, frcAckBroadcastRequest.PeripheralType() )
          << NAME_PAR( Node address, frcAckBroadcastRequest.NodeAddress() )
          << NAME_PAR( Command, (int)frcAckBroadcastRequest.PeripheralCommand() )
        );
        autonetworkResult.addTransactionResult( transResult );
        TRC_FUNCTION_LEAVE( "" );
      }
      catch ( std::exception& e )
      {
        AutonetworkError error( AutonetworkError::Type::RemoveBondAndRestart, e.what() );
        autonetworkResult.setError( error );
        autonetworkResult.addTransactionResult( transResult );
        THROW_EXC( std::logic_error, e.what() );
      }
    }

    // Removes specified node address at the coordinator side
    void removeBondAtCoordinator( AutonetworkResult& autonetworkResult, const uint8_t addrToRemove )
    {
      TRC_FUNCTION_ENTER( "" );
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage removeBondAtCoordinatorRequest;
        DpaMessage::DpaPacket_t removeBondAtCoordinatorPacket;
        removeBondAtCoordinatorPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        removeBondAtCoordinatorPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
        removeBondAtCoordinatorPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_REMOVE_BOND;
        removeBondAtCoordinatorPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        // BondAddr
        removeBondAtCoordinatorPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorRemoveBond_Request.BondAddr = addrToRemove;
        // Data to buffer
        removeBondAtCoordinatorRequest.DataToBuffer( removeBondAtCoordinatorPacket.Buffer, sizeof( TDpaIFaceHeader ) + sizeof( TPerCoordinatorRemoveBond_Request ) );
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat( removeBondAtCoordinatorRequest, transResult, m_actionRetries );
        TRC_DEBUG( "Result from Remove bond at Coordinator transaction as string:" << PAR( transResult->getErrorString() ) );
        TRC_INFORMATION( "Remove bond and restart ok!" );
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR( Peripheral type, removeBondAtCoordinatorRequest.PeripheralType() )
          << NAME_PAR( Node address, removeBondAtCoordinatorRequest.NodeAddress() )
          << NAME_PAR( Command, (int)removeBondAtCoordinatorRequest.PeripheralCommand() )
        );
        autonetworkResult.addTransactionResult( transResult );
        TRC_FUNCTION_LEAVE( "" );
      }
      catch ( std::exception& e )
      {
        AutonetworkError error( AutonetworkError::Type::RemoveBondAtCoordinator, e.what() );
        autonetworkResult.setError( error );
        autonetworkResult.addTransactionResult( transResult );
        THROW_EXC( std::logic_error, e.what() );
      }
    }

    // Run discovery process
    void runDiscovery( AutonetworkResult& autonetworkResult, const uint8_t txPower, uint8_t&  discoveredNodesCnt )
    {
      TRC_FUNCTION_ENTER( "" );
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        DpaMessage runDiscoveryRequest;
        DpaMessage::DpaPacket_t runDiscoveryPacket;
        runDiscoveryPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        runDiscoveryPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
        runDiscoveryPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_DISCOVERY;
        runDiscoveryPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        // TX power
        runDiscoveryPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorDiscovery_Request.TxPower = txPower;
        // Max address
        runDiscoveryPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorDiscovery_Request.MaxAddr = 0x00;
        runDiscoveryRequest.DataToBuffer( runDiscoveryPacket.Buffer, sizeof( TDpaIFaceHeader ) + sizeof( TPerCoordinatorDiscovery_Request ) );
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat( runDiscoveryRequest, transResult, m_actionRetries );
        TRC_INFORMATION( "Run discovery ok!" );
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR( Peripheral type, runDiscoveryRequest.PeripheralType() )
          << NAME_PAR( Node address, runDiscoveryRequest.NodeAddress() )
          << NAME_PAR( Command, (int)runDiscoveryRequest.PeripheralCommand() )
        );
        TRC_DEBUG( "Result from Run discovery transaction as string:" << PAR( transResult->getErrorString() ) );
        TPerCoordinatorDiscovery_Response response = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerCoordinatorDiscovery_Response;
        discoveredNodesCnt = response.DiscNr;
        autonetworkResult.addTransactionResult( transResult );
        TRC_FUNCTION_LEAVE( "" );
      }
      catch ( std::exception& e )
      {
        AutonetworkError error( AutonetworkError::Type::RunDiscovery, e.what() );
        autonetworkResult.setError( error );
        autonetworkResult.addTransactionResult( transResult );
        THROW_EXC( std::logic_error, e.what() );
      }
    }

    void setFreeNodes( std::bitset<MAX_ADDRESS + 1>& bondedNodes, const std::vector<uint8_t>& notRespondedNewNodes )
    {
      for ( uint8_t addrToFree : notRespondedNewNodes ) {
        bondedNodes.reset( addrToFree );
      }
    }

    // Process the autonetwork algorithm
    void runAutonetwork(
      const uint8_t actionRetries,
      const uint8_t discoveryTxPower,
      const bool discoveryBeforeStart,
      const uint8_t waves,
      const uint8_t emptyWaves,
      const ComAutonetwork& comAutonetwork,
      const IMessagingSplitterService::MsgType& msgType,
      const std::string& messagingId
    )
    {
      TRC_FUNCTION_ENTER( "" );
      AutonetworkResult autonetworkResult;
      m_actionRetries = actionRetries;
      autonetworkResult.setLastWave( false );

      std::bitset<MAX_ADDRESS + 1> bondedNodes;
      uint8_t bondedNodesNr = 0;

      std::bitset<MAX_ADDRESS + 1> discoveredNodes;
      uint8_t discoveredNodesNr = 0;

      // Nodes, which successfully responded to FRC check -> for to be available in the error response
      std::vector<AutonetworkResult::NewNode> respondedNewNodes;

      try
      {
        // Check, if Coordinator and OS peripherals are present at coordinator's node
        checkPresentCoordAndCoordOs( autonetworkResult );
        TRC_INFORMATION( "Initial network check." );

        // Update network info
        updateNodesInfo( autonetworkResult, bondedNodesNr, bondedNodes, discoveredNodesNr, discoveredNodes );
        TRC_INFORMATION( NAME_PAR( Bonded nodes, toNodesListStr( bondedNodes ) ) );
        TRC_INFORMATION( NAME_PAR( Discovered nodes, toNodesListStr( discoveredNodes ) ) );
        // Check max address
        if ( bondedNodes == MAX_ADDRESS )
        {
          TRC_INFORMATION( "All available network addresses are already allocated." )
          AutonetworkError error( AutonetworkError::Type::AllAddressAllocated, "All available network addresses are already allocated." );
          autonetworkResult.setError( error );
          THROW_EXC( std::logic_error, "All available network addresses are already allocated." );
        }

        // Set FRC param to 0x00
        setFrcParam( autonetworkResult );
        TRC_INFORMATION( "Set FRC Param 0x00" );

        // Set DPA param to 0x00
        setNoLedAndOptimalTimeslot( autonetworkResult );
        TRC_INFORMATION( "No LED indication and use of optimal time slot length" );

        // Set DPA Hops Param to 0xff, 0xff
        setDpaHopsToTheNumberOfRouters( autonetworkResult );
        TRC_INFORMATION( "Number of hops set to the number of routers" );

        // Start autonetwork 
        TRC_INFORMATION( "Automatic network construction in progress" );
        uint8_t origNodesCount = bondedNodesNr;
        int round = 1;
        int emptyRounds = 0;
        uint8_t nextAddr = MAX_ADDRESS;
        using std::chrono::system_clock;

        // Main cycle
        for ( ; ( bondedNodesNr != MAX_ADDRESS ) && ( round <= waves ); round++ )
        {
          TRC_INFORMATION( NAME_PAR( Orig nodes count, (int)origNodesCount ) );
          TRC_INFORMATION( NAME_PAR( Round, round ) );

          time_t now = system_clock::to_time_t( system_clock::now() );
          TRC_INFORMATION( NAME_PAR( Start time, ctime( &now ) ) );

          // Run Discovery before start 
          if ( ( round == 1 ) && ( discoveryBeforeStart == true ) && ( bondedNodesNr > 0 ) )
          {
            uns8 discoveredNodes;
            runDiscovery( autonetworkResult, discoveryTxPower, discoveredNodes );
          }

          // Preset params
          autonetworkResult.setWave( round );
          autonetworkResult.setNodesNr( bondedNodesNr );
          respondedNewNodes.clear();

          // SmartConnect
          TRC_INFORMATION( "SmartConnect" );
          smartConnect( autonetworkResult );

          // Get pre-bonded alive noded
          uint8_t virtFrcId = (uint8_t)( 1 + round % 255 );
          std::vector<uint8_t> prebondedAliveNodes;
          prebondedAliveNodes = getPrebondedAliveNodes( autonetworkResult, virtFrcId );

          // Check empty wave
          if ( prebondedAliveNodes.empty() )
          {
            if ( ++emptyRounds == emptyWaves )
            {
              TRC_INFORMATION( "Maximum number of consecutive empty waves reached." );
              AutonetworkError error( AutonetworkError::Type::EmptyWaves, "Maximum number of consecutive empty waves reached 1." );
              autonetworkResult.setError( error );
              goto SendResponse;
            }

            // Send NOT last results
            Document responseDoc = createResponse( comAutonetwork.getMsgId(), msgType, autonetworkResult, comAutonetwork, respondedNewNodes );
            m_iMessagingSplitterService->sendMessage( messagingId, std::move( responseDoc ) );

            // Clear new nodes for the next wave
            autonetworkResult.clearNewNodes();
            continue;
          }
          TRC_INFORMATION( NAME_PAR( Prebonded alive nodes, prebondedAliveNodes.size() ) );

          // Get list of prebonded MIDs
          std::list<uint32_t> prebondedMIDs;
          prebondedMIDs = getPrebondedMIDs( autonetworkResult, prebondedAliveNodes, virtFrcId );
          // Authorize MIDs
          std::map<uint8_t, uint32_t> newNodes;
          authorizeMIDs( autonetworkResult, prebondedMIDs, bondedNodesNr, bondedNodes, discoveredNodesNr, discoveredNodes, nextAddr, newNodes );

          // Check new nodes
          if ( newNodes.size() == 0 )
          {
            if ( ++emptyRounds == emptyWaves )
            {
              TRC_INFORMATION( "Maximum number of consecutive empty waves reached." );
              AutonetworkError error( AutonetworkError::Type::EmptyWaves, "Maximum number of consecutive empty waves reached 2." );
              autonetworkResult.setError( error );
              goto SendResponse;
            }

            // Send NOT last results
            Document responseDoc = createResponse( comAutonetwork.getMsgId(), msgType, autonetworkResult, comAutonetwork, respondedNewNodes );
            m_iMessagingSplitterService->sendMessage( messagingId, std::move( responseDoc ) );

            // Clear new nodes for the next wave
            autonetworkResult.clearNewNodes();
            continue;
          }

          // Ping new nodes
          TRC_INFORMATION( "Running FRC to check new nodes" );
          uint8_t frcStatusCheck = 0xFE;
          std::vector<uint8_t> frcDataCheck;
          frcDataCheck = checkNewNodes( autonetworkResult, frcStatusCheck );
          std::vector<uint8_t> notRespondedNewNodes;
          if ( frcStatusCheck >= 0xFE )
          {
            TRC_WARNING( "FRC to check new nodes failed." )
          }
          else
          {
            std::map<uint8_t, uint32_t> newNodesCopy = newNodes;
            for ( std::pair<uint8_t, uint32_t> authorizedNode : newNodesCopy )
            {
              if ( !( ( ( frcDataCheck[0 + authorizedNode.first / 8] >> ( authorizedNode.first % 8 ) ) & 0x01 ) == 0x00 ) )
              {
                AutonetworkResult::NewNode respNode = { authorizedNode.first, authorizedNode.second };
                respondedNewNodes.push_back( respNode );
              }
              else
              {
                notRespondedNewNodes.push_back( authorizedNode.first );
                // Wait for sure - it is still valid ?
                std::this_thread::sleep_for( std::chrono::microseconds( ( bondedNodesNr + 1 ) * ( 2 * ( MIN_TIMESLOT + 10 ) ) ) );
                removeBondAtCoordinator( autonetworkResult, authorizedNode.first );
                // Delete removed node from newNodes
                newNodes.erase( authorizedNode.first );
              }
            }

            // Remove not responded nodes
            if ( !notRespondedNewNodes.empty() )
            {
              removeNotRespondedNewNodes( autonetworkResult, notRespondedNewNodes );
              // Return not responded nodes into free nodes available for bond
              setFreeNodes( bondedNodes, notRespondedNewNodes );
              notRespondedNewNodes.clear();
            }
          }

          // No new nodes - go to next iteration
          if ( newNodes.size() == 0 )
          {
            updateNodesInfo( autonetworkResult, bondedNodesNr, bondedNodes, discoveredNodesNr, discoveredNodes );
            if ( ++emptyRounds == emptyWaves )
            {
              TRC_INFORMATION( "Maximum number of consecutive empty waves reached." )
                AutonetworkError error( AutonetworkError::Type::EmptyWaves, "Maximum number of consecutive empty waves reached 3." );
              autonetworkResult.setError( error );
              goto SendResponse;
            }
            // Send NOT last results
            Document responseDoc = createResponse( comAutonetwork.getMsgId(), msgType, autonetworkResult, comAutonetwork, respondedNewNodes );
            m_iMessagingSplitterService->sendMessage( messagingId, std::move( responseDoc ) );

            // Clear new nodes for the next wave
            autonetworkResult.clearNewNodes();
            continue;
          }

          // Consecutive empty rounds
          emptyRounds = 0;

          TRC_INFORMATION( "Running discovery" );
          try
          {
            uint8_t discoveredNodesCnt = 0;
            runDiscovery( autonetworkResult, discoveryTxPower, discoveredNodesCnt );
            TRC_INFORMATION( NAME_PAR( Discovered nodes, (int)discoveredNodesCnt ) );
          }
          catch ( std::exception& ex )
          {
            TRC_WARNING( "Running discovery failed." )
          }

          TRC_INFORMATION( "Waiting for coordinator to finish discovery" );
          updateNodesInfo( autonetworkResult, bondedNodesNr, bondedNodes, discoveredNodesNr, discoveredNodes );

          for ( std::pair<uint8_t, uint32_t> newNode : newNodes )
            autonetworkResult.putNewNode( newNode.first, newNode.second );

          // Last iteration ?
          if ( ( bondedNodesNr == MAX_ADDRESS ) || ( round == waves ) )
            goto SendResponse;

          // Send NOT last results
          Document responseDoc = createResponse( comAutonetwork.getMsgId(), msgType, autonetworkResult, comAutonetwork, respondedNewNodes );
          m_iMessagingSplitterService->sendMessage( messagingId, std::move( responseDoc ) );

          // Clear new nodes for the next wave
          autonetworkResult.clearNewNodes();
        }
      }
      catch ( std::exception& ex )
      {
        TRC_WARNING( "Error during algorithm run: " << ex.what() );
        AutonetworkError error( AutonetworkError::Type::Internal, ex.what() );
        autonetworkResult.setError( error );
        if ( autonetworkResult.getError().getType() == AutonetworkError::Type::NoError )
        {
          AutonetworkError error( AutonetworkError::Type::Internal, ex.what() );
          autonetworkResult.setError( error );
        }
      }

      // Creating and sending of message
SendResponse:
      autonetworkResult.setLastWave( true );
      Document responseDoc = createResponse( comAutonetwork.getMsgId(), msgType, autonetworkResult, comAutonetwork, respondedNewNodes );
      m_iMessagingSplitterService->sendMessage( messagingId, std::move( responseDoc ) );
      TRC_FUNCTION_LEAVE( "" );
    }

    // Creates error response about service general fail   
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
      const ComAutonetwork& comAutonetwork,
      const std::vector<AutonetworkResult::NewNode>& respondedNewNodes
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
          case AutonetworkError::Type::Internal:
            status = SERVICE_ERROR_INTERNAL;
            break;

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

          case AutonetworkError::Type::EmptyWaves:
            status = SERVICE_ERROR_EMPTY_WAWES;
            rapidjson::Pointer("/data/rsp/wave").Set(response, autonetworkResult.getWave());
            rapidjson::Pointer("/data/rsp/nodesNr").Set(response, autonetworkResult.getNodesNr());
            rapidjson::Pointer("/data/rsp/newNodesNr").Set(response, autonetworkResult.getNewNodesNr());
            // last wave indication
            rapidjson::Pointer("/data/rsp/lastWave").Set(response, autonetworkResult.isLastWave());
            break;

          case AutonetworkError::Type::AllAddressAllocated:
            status = SERVICE_ERROR_ALL_ADDRESS_ALLOCATED;
            break;

          default:
            status = SERVICE_ERROR;
            break; 
        }

        // add newly added nodes - even in the error response
        if ((!respondedNewNodes.empty()) && (status != SERVICE_ERROR_EMPTY_WAWES)) 
        {
          // rsp object
          rapidjson::Pointer("/data/rsp/wave").Set(response, autonetworkResult.getWave());
          rapidjson::Pointer("/data/rsp/nodesNr").Set(response, autonetworkResult.getNodesNr());
          rapidjson::Pointer("/data/rsp/newNodesNr").Set(response, respondedNewNodes.size());

          rapidjson::Value newNodesJsonArray(kArrayType);
          Document::AllocatorType& allocator = response.GetAllocator();
          for (AutonetworkResult::NewNode newNode : respondedNewNodes) {
            rapidjson::Value newNodeObject(kObjectType);

            std::stringstream stream;
            stream << std::hex << newNode.MID;

            newNodeObject.AddMember("mid", stream.str(), allocator);
            newNodeObject.AddMember("address", newNode.address, allocator);
            newNodesJsonArray.PushBack(newNodeObject, allocator);
          }
          Pointer("/data/rsp/newNodes").Set(response, newNodesJsonArray);
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

        std::stringstream stream;
        stream << std::hex << newNode.MID;

        newNodeObject.AddMember("mid", stream.str(), allocator);
        newNodeObject.AddMember("address", newNode.address, allocator);
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

      // Unsupported type of request
      if ( msgType.m_type != m_mTypeName_Autonetwork )
        THROW_EXC( std::logic_error, "Unsupported message type: " << PAR( msgType.m_type ) );

      // Creating representation object
      ComAutonetwork comAutonetwork( doc );

      // Service input parameters
      uint8_t actionRetries;
      uint8_t discoveryTxPower;
      bool discoveryBeforeStart;
      uint8_t waves;
      uint8_t emptyWaves;
      bool returnVerbose = false;

      // Parsing and checking service parameters
      try
      {
        actionRetries = comAutonetwork.getActionRetries();
        discoveryTxPower = comAutonetwork.getDiscoveryTxPower();
        discoveryBeforeStart = comAutonetwork.getDiscoveryBeforeStart();

        if (!comAutonetwork.isSetWaves())
          THROW_EXC(std::logic_error, "waves not set");        
        waves = parseAndCheckWaves(comAutonetwork.getWaves());

        if (!comAutonetwork.isSetEmptyWaves()) 
          THROW_EXC(std::logic_error, "emptyWaves not set");        
        emptyWaves = parseAndCheckEmptyWaves(comAutonetwork.getEmptyWaves());

        returnVerbose = comAutonetwork.getVerbose();
      }
      // parsing and checking service parameters failed 
      catch ( std::exception& ex ) 
      {
        Document failResponse = createCheckParamsFailedResponse( comAutonetwork.getMsgId(), msgType, ex.what() );
        m_iMessagingSplitterService->sendMessage( messagingId, std::move( failResponse ) );
        TRC_FUNCTION_LEAVE( "" );
        return;
      }

      // Try to establish exclusive access
      try
      {
        m_exclusiveAccess = m_iIqrfDpaService->getExclusiveAccess();
      }
      catch (std::exception &e)
      {
        const char* errorStr = e.what();
        TRC_WARNING("Error while establishing exclusive DPA access: " << PAR(errorStr));
        Document failResponse = getExclusiveAccessFailedResponse(comAutonetwork.getMsgId(), msgType, errorStr);
        m_iMessagingSplitterService->sendMessage(messagingId, std::move(failResponse));
        TRC_FUNCTION_LEAVE("");
        return;
      }

      // Run the Autonetwork process
      runAutonetwork( actionRetries, discoveryTxPower, discoveryBeforeStart, waves, emptyWaves, comAutonetwork, msgType, messagingId );

      // Release exclusive access
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


  AutonetworkService::AutonetworkService()
  {
    m_imp = shape_new Imp( *this );
  }

  AutonetworkService::~AutonetworkService()
  {
    delete m_imp;
  }


  void AutonetworkService::attachInterface( iqrf::IIqrfDpaService* iface )
  {
    m_imp->attachInterface( iface );
  }

  void AutonetworkService::detachInterface( iqrf::IIqrfDpaService* iface )
  {
    m_imp->detachInterface( iface );
  }

  void AutonetworkService::attachInterface( iqrf::IJsCacheService* iface )
  {
    m_imp->attachInterface( iface );
  }

  void AutonetworkService::detachInterface( iqrf::IJsCacheService* iface )
  {
    m_imp->detachInterface( iface );
  }

  void AutonetworkService::attachInterface( iqrf::IMessagingSplitterService* iface )
  {
    m_imp->attachInterface( iface );
  }

  void AutonetworkService::detachInterface( iqrf::IMessagingSplitterService* iface )
  {
    m_imp->detachInterface( iface );
  }

  void AutonetworkService::attachInterface( shape::ITraceService* iface )
  {
    shape::Tracer::get().addTracerService( iface );
  }

  void AutonetworkService::detachInterface( shape::ITraceService* iface )
  {
    shape::Tracer::get().removeTracerService( iface );
  }


  void AutonetworkService::activate( const shape::Properties *props )
  {
    m_imp->activate( props );
  }

  void AutonetworkService::deactivate()
  {
    m_imp->deactivate();
  }

  void AutonetworkService::modify( const shape::Properties *props )
  {
    m_imp->modify( props );
  }

}
