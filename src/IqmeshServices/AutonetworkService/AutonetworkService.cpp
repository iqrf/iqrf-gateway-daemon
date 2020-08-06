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
  // ToDo Timeout step
  const int TIMEOUT_STEP = 500;
  const int TIMEOUT_REPEAT = 2000;

  // Default bonding mask. No masking effect.
  static const uint8_t DEFAULT_BONDING_MASK = 0;

  // values of result error codes
  // service general fail code - may and probably will be changed later in the future
  static const int SERVICE_ERROR = 1000;
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
      PrebondedMemoryCompare,
      AuthorizeBond,
      RemoveBond,
      RemoveBondAndRestart,
      CheckNewNodes,
      RemoveBondAtCoordinator,
      RunDiscovery,
      AllAddressAllocated,
      ValidateBonds,
      IncorrectStopCondition,
      TooManyNodesFound
    };

    AutonetworkError() : m_type( Type::NoError ), m_message( "ok" ) {};
    AutonetworkError( Type errorType ) : m_type( errorType ), m_message( "" ) {};
    AutonetworkError( Type errorType, const std::string& message ) : m_type( errorType ), m_message( message ) {};

    Type getType() const { return m_type; };
    std::string getMessage() const { return m_message; };

    AutonetworkError(const AutonetworkError& other) {
      m_type = other.getType();
      m_message = other.getMessage();
    }

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

  // Result of AutonetworkResult algorithm
  class AutonetworkResult {
  public:
    // Information related to node newly added into the network
    struct NewNode {
      uint8_t address;
      uint32_t MID;
    };

  private:
    AutonetworkError m_error;
    std::vector<NewNode> m_newNodes;

    // Transaction results
    std::list<std::unique_ptr<IDpaTransactionResult2>> m_transResults;

  public:
    AutonetworkError getError() const { return m_error; };

    void setError( const AutonetworkError& error ) {
      m_error = error;
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
    
    // Adds transaction result into the list of results
    void addTransactionResult( std::unique_ptr<IDpaTransactionResult2>& transResult ) {
      m_transResults.push_back( std::move( transResult ) );
    }

    bool isNextTransactionResult() {
      return ( m_transResults.size() > 0 );
    }

    // Consumes the first element in the transaction results list
    std::unique_ptr<IDpaTransactionResult2> consumeNextTransactionResult() {
      std::list<std::unique_ptr<IDpaTransactionResult2>>::iterator iter = m_transResults.begin();
      std::unique_ptr<IDpaTransactionResult2> tranResult = std::move( *iter );
      m_transResults.pop_front();
      return tranResult;
    }
  };

  // Implementation class
  class AutonetworkService::Imp 
  {
  private:

    // Node authorization error definition
    enum class TAuthorizeErr
    {
      eNo,
      eMIDFiltering,
      eHWPIDFiltering,
      eFRC,
      eAddress,
      eNetworkNum,
      eNodeBonded
    };

    // waveState codes definition
    enum class TWaveStateCode
    {
      discoveryBeforeStart,
      smartConnect,
      checkPrebondedAlive,
      readingDPAVersion,
      readPrebondedMID,
      readPrebondedHWPID,
      enumeration,
      authorize,
      ping,
      removeNotResponded,
      discovery,
      stopOnMaxNumWaves,
      stopOnNumberOfTotalNodes,
      stopOnMaxEmptyWaves,
      stopOnNumberOfNewNodes,
      abortOnTooManyNodesFound,
      abortOnAllAddresseAllocated,
      waveFinished,
      cannotStartProcessMaxAddress,
      cannotStartProcessTotalNodesNr,
      cannotStartProcessNewNodesNr,
    };

    // MID union
    typedef union
    {
      uint8_t bytes[sizeof( uint32_t )];
      uint32_t value;
    }TMID;

    // Prebonded node
    typedef struct
    {
      uint8_t node;
      TMID mid;
      uint8_t addrBond;
      uint16_t HWPID;
      uint16_t HWPIDVer;
      bool supportMultipleAuth;
      bool authorize;
      TAuthorizeErr authorizeErr;
    }TPrebondedNode;

    // Network node 
    typedef struct
    {
      uint8_t address;
      TMID mid;
      uint16_t HWPID;
      uint16_t HWPIDVer;
      bool bonded;
      bool discovered;
      bool online;
    }TNode;

    // Autonetwork process paramaters
    typedef struct
    {
      // Bonded nodes map
      std::bitset<MAX_ADDRESS + 1> bondedNodes;
      uint8_t initialBondedNodesNr, bondedNodesNr;
      // Discovered nodes map
      std::bitset<MAX_ADDRESS + 1> discoveredNodes;
      uint8_t discoveredNodesNr;
      // Nodes
      std::vector<AutonetworkResult::NewNode> respondedNewNodes;
      // Duplicit MIDs
      std::vector<uint8_t> duplicitMID;
      // Prebonded nodes map
      std::map<uint8_t, TPrebondedNode> prebondedNodes;
      // Network nodes map
      std::map<uint8_t, TNode> networkNodes;
      // FRC param value
      uint8_t FrcResponseTime;
      // DPA param value
      uint8_t DpaParam;
      // TX and RX hops
      uint8_t RequestHops, ResponseHops;
      uint8_t countWaves, countEmpty, countNewNodes, countWaveNewNodes;
      TWaveStateCode waveStateCode;
      int progress;
      int progressStep;
    }TAutonetworkProcessParams;

    // Parent object
    AutonetworkService & m_parent;

    // Service input parameters
    TAutonetworkInputParams antwInputParams;

    // Service process parameters
    TAutonetworkProcessParams antwProcessParams;

    // Message type
    const std::string m_mTypeName_Autonetwork = "iqmeshNetwork_AutoNetwork";
    IIqrfInfo* m_iIqrfInfo = nullptr;
    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    IIqrfDpaService* m_iIqrfDpaService = nullptr;
    std::unique_ptr<IIqrfDpaService::ExclusiveAccess> m_exclusiveAccess;
    const std::string* m_messagingId = nullptr;
    const IMessagingSplitterService::MsgType* m_msgType = nullptr;
    const ComAutonetwork* m_comAutonetwork = nullptr;

    uint8_t MAX_WAVES = MAX_ADDRESS;
    uint8_t MAX_EMPTY_WAVES = MAX_ADDRESS;

    uint8_t mZero = 0;

  public:
    Imp( AutonetworkService& parent ) : m_parent( parent )
    {
    }

    ~Imp()
    {
    }

    // Parses bit array of nodes into bitmap
    std::bitset<MAX_ADDRESS + 1> toNodesBitmap( const unsigned char* pData )
    {
      std::bitset<MAX_ADDRESS + 1> nodesMap;
      for ( uint8_t nodeAddr = 0; nodeAddr <= MAX_ADDRESS; nodeAddr++ )
        nodesMap[nodeAddr] = ( pData[nodeAddr / 8] & ( 1 << ( nodeAddr % 8 ) ) ) != 0;

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
        perEnumPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        perEnumPacket.DpaRequestPacket_t.PNUM = PNUM_ENUMERATION;
        perEnumPacket.DpaRequestPacket_t.PCMD = CMD_GET_PER_INFO;
        perEnumPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        perEnumRequest.DataToBuffer( perEnumPacket.Buffer, sizeof( TDpaIFaceHeader ) );
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat( perEnumRequest, transResult, antwInputParams.actionRetries );
        TRC_DEBUG( "Result from Device Exploration transaction as string:" << PAR( transResult->getErrorString() ) );
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION( "Device exploration successful!" );
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR( Peripheral type, perEnumRequest.PeripheralType() )
          << NAME_PAR( Node address, perEnumRequest.NodeAddress() )
          << NAME_PAR( Command, (int)perEnumRequest.PeripheralCommand() )
        );
        // Check Coordinator and OS peripherals
        if ( ( dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.EnumPeripheralsAnswer.EmbeddedPers[PNUM_COORDINATOR / 8] & ( 1 << PNUM_COORDINATOR ) ) != ( 1 << PNUM_COORDINATOR ) )
          THROW_EXC( std::logic_error, "Coordinator peripheral NOT found." );
        if ( ( dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.EnumPeripheralsAnswer.EmbeddedPers[PNUM_OS / 8] & ( 1 << PNUM_OS ) ) != ( 1 << PNUM_OS ) )
          THROW_EXC( std::logic_error, "OS peripheral NOT found." );
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

    // Set FRC response time
    uint8_t setFrcReponseTime( AutonetworkResult& autonetworkResult, uint8_t FRCresponseTime )
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
        setFrcParamPacket.DpaRequestPacket_t.DpaMessage.PerFrcSetParams_RequestResponse.FRCresponseTime = FRCresponseTime;
        setFrcParamRequest.DataToBuffer( setFrcParamPacket.Buffer, sizeof( TDpaIFaceHeader ) + sizeof( TPerFrcSetParams_RequestResponse ) );
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat( setFrcParamRequest, transResult, antwInputParams.actionRetries );
        TRC_DEBUG( "Result from Set Hops transaction as string:" << PAR( transResult->getErrorString() ) );
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION( "Set Hops successful!" );
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR( Peripheral type, setFrcParamRequest.PeripheralType() )
          << NAME_PAR( Node address, setFrcParamRequest.NodeAddress() )
          << NAME_PAR( Command, (int)setFrcParamRequest.PeripheralCommand() )
        );
        autonetworkResult.addTransactionResult( transResult );
        TRC_FUNCTION_LEAVE( "" );
        return dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSetParams_RequestResponse.FRCresponseTime;
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
    uint8_t setNoLedAndOptimalTimeslot( AutonetworkResult& autonetworkResult, uint8_t DpaParam )
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
        setDpaParamsPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSetDpaParams_Request_Response.DpaParam = DpaParam;
        setDpaParamsRequest.DataToBuffer( setDpaParamsPacket.Buffer, sizeof( TDpaIFaceHeader ) + sizeof( TPerCoordinatorSetDpaParams_Request_Response ) );
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat( setDpaParamsRequest, transResult, antwInputParams.actionRetries );
        TRC_DEBUG( "Result from Set DPA params transaction as string:" << PAR( transResult->getErrorString() ) );
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION( "Set DPA params successful!" );
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR( Peripheral type, setDpaParamsRequest.PeripheralType() )
          << NAME_PAR( Node address, setDpaParamsRequest.NodeAddress() )
          << NAME_PAR( Command, (int)setDpaParamsRequest.PeripheralCommand() )
        );
        autonetworkResult.addTransactionResult( transResult );
        TRC_FUNCTION_LEAVE( "" );
        return dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerCoordinatorSetDpaParams_Request_Response.DpaParam;
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
    TPerCoordinatorSetHops_Request_Response setDpaHopsToTheNumberOfRouters( AutonetworkResult& autonetworkResult, uint8_t RequestHops, uint8_t ResponseHops )
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
        setHopsPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSetHops_Request_Response.RequestHops = RequestHops;
        setHopsPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSetHops_Request_Response.ResponseHops = ResponseHops;
        setHopsRequest.DataToBuffer( setHopsPacket.Buffer, sizeof( TDpaIFaceHeader ) + sizeof( TPerCoordinatorSetHops_Request_Response ) );
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat( setHopsRequest, transResult, antwInputParams.actionRetries );
        TRC_DEBUG( "Result from Set Hops transaction as string:" << PAR( transResult->getErrorString() ) );
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION( "Set Hops successful!" );
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR( Peripheral type, setHopsRequest.PeripheralType() )
          << NAME_PAR( Node address, setHopsRequest.NodeAddress() )
          << NAME_PAR( Command, (int)setHopsRequest.PeripheralCommand() )
        );
        autonetworkResult.addTransactionResult( transResult );
        TRC_FUNCTION_LEAVE( "" );        
        return dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerCoordinatorSetHops_Request_Response;
      }
      catch ( std::exception& e )
      {
        AutonetworkError error( AutonetworkError::Type::SetHops, e.what() );
        autonetworkResult.setError( error );
        autonetworkResult.addTransactionResult( transResult );
        THROW_EXC( std::logic_error, e.what() );
      }
    }

    // Read from coordinator extended eeprom
    std::basic_string<uint8_t> readCoordXMemory( AutonetworkResult& autonetworkResult, uint16_t address, uint8_t length )
    {
      TRC_FUNCTION_ENTER( "" );
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage XMemoryReadRequest;
        DpaMessage::DpaPacket_t XMemoryReadPacket;
        XMemoryReadPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        XMemoryReadPacket.DpaRequestPacket_t.PNUM = PNUM_EEEPROM;
        XMemoryReadPacket.DpaRequestPacket_t.PCMD = CMD_EEEPROM_XREAD;
        XMemoryReadPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        // Set address and length 
        XMemoryReadPacket.DpaRequestPacket_t.DpaMessage.XMemoryRequest.Address = address;
        XMemoryReadPacket.DpaRequestPacket_t.DpaMessage.XMemoryRequest.ReadWrite.Read.Length = length;
        // Data to buffer
        XMemoryReadRequest.DataToBuffer( XMemoryReadPacket.Buffer, sizeof( TDpaIFaceHeader ) + sizeof( uint16_t ) + sizeof( uint8_t ) );
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat( XMemoryReadRequest, transResult, antwInputParams.actionRetries );
        TRC_DEBUG( "Result from XMemoryRequest transaction as string:" << PAR( transResult->getErrorString() ) );
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION( "Read XMemoryRequest successful!" );
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR( Peripheral type, XMemoryReadRequest.PeripheralType() )
          << NAME_PAR( Node address, XMemoryReadRequest.NodeAddress() )
          << NAME_PAR( Command, (int)XMemoryReadRequest.PeripheralCommand() )
        );
        autonetworkResult.addTransactionResult( transResult );
        // Get response data
        std::basic_string<uint8_t> XMemoryData;
        XMemoryData.append( dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData, length );
        TRC_FUNCTION_LEAVE( "" );
        return XMemoryData;
      }
      catch ( std::exception& e )
      {
        AutonetworkError error( AutonetworkError::Type::GetDiscoveredNodes, e.what() );
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
        m_exclusiveAccess->executeDpaTransactionRepeat( addrInfoRequest, transResult, antwInputParams.actionRetries );
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
        m_exclusiveAccess->executeDpaTransactionRepeat( getBondedNodesRequest, transResult, antwInputParams.actionRetries );
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
        autonetworkResult.addTransactionResult( transResult );
        TRC_FUNCTION_LEAVE( "" );
        return toNodesBitmap( dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData );
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
        m_exclusiveAccess->executeDpaTransactionRepeat( getDiscoveredNodesRequest, transResult, antwInputParams.actionRetries );
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
        autonetworkResult.addTransactionResult( transResult );
        TRC_FUNCTION_LEAVE( "" );
        return toNodesBitmap( dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData );
      }
      catch ( std::exception& e )
      {
        AutonetworkError error( AutonetworkError::Type::GetDiscoveredNodes, e.what() );
        autonetworkResult.setError( error );
        autonetworkResult.addTransactionResult( transResult );
        THROW_EXC( std::logic_error, e.what() );
      }
    }

    // Update network info
    void updateNetworkInfo( AutonetworkResult& autonetworkResult )
    {
      // Get addressing info
      TPerCoordinatorAddrInfo_Response addressingInfo = getAddressingInfo( autonetworkResult );
      // Bonded nodes
      antwProcessParams.bondedNodesNr = addressingInfo.DevNr;
      antwProcessParams.bondedNodes = getBondedNodes( autonetworkResult );
      // Discovered nodes
      antwProcessParams.discoveredNodes = getDiscoveredNodes( autonetworkResult );
      antwProcessParams.discoveredNodesNr = (uint8_t)antwProcessParams.discoveredNodes.count();
      // Clear discoveredNodes bitmap and discoveredNodesNr if no node is bonded
      if ( antwProcessParams.bondedNodesNr == 0 )
      {
        antwProcessParams.discoveredNodes.reset();
        antwProcessParams.discoveredNodesNr = 0;
      }

      // Update networkNodes structure      
      for ( uint8_t addr = 1; addr <= MAX_ADDRESS; addr++ )
      {
        // Node bonded ?
        antwProcessParams.networkNodes[addr].bonded = antwProcessParams.bondedNodes[addr];
        if ( antwProcessParams.networkNodes[addr].bonded == true)
        {
          // Yes, node MID known ?
          if ( antwProcessParams.networkNodes[addr].mid.value == 0 )
          {
            // No, read MID from Coordinator eeprom
            uint16_t address = 0x4000 + addr * 0x08;
            std::basic_string<uint8_t> mid = readCoordXMemory( autonetworkResult, address, sizeof( TMID ) );
            antwProcessParams.networkNodes[addr].mid.bytes[0] = mid[0];
            antwProcessParams.networkNodes[addr].mid.bytes[1] = mid[1];
            antwProcessParams.networkNodes[addr].mid.bytes[2] = mid[2];
            antwProcessParams.networkNodes[addr].mid.bytes[3] = mid[3];
            antwProcessParams.networkNodes[addr].discovered = antwProcessParams.discoveredNodes[addr];
          }
        }
        else
        {
          // No, node isn't bondes, clear discovered flag and MID
          antwProcessParams.networkNodes[addr].discovered = false;
          antwProcessParams.networkNodes[addr].mid.value = 0;
        }
      }
    }

    // Returns comma-separated list of nodes, whose bits are set to 1 in the bitmap
    std::string toNodesListStr( const std::bitset<MAX_ADDRESS + 1>& nodes )
    {
      std::string nodesListStr;
      for ( uint8_t nodeAddr = 1; nodeAddr <= MAX_ADDRESS; nodeAddr++ )
      {
        if ( nodes[nodeAddr] && !nodesListStr.empty() ) {
          nodesListStr += ", ";
        }
        nodesListStr += std::to_string((int)nodeAddr);
      }

      return nodesListStr;
    }

    // Check unbonded nodes
    bool checkUnbondedNodes( const std::bitset<MAX_ADDRESS + 1>& bondedNodes, const std::bitset<MAX_ADDRESS + 1>& discoveredNodes )
    {
      std::stringstream unbondedNodesStream;

      for ( uint8_t nodeAddr = 1; nodeAddr <= MAX_ADDRESS; nodeAddr++ )
        if ( ( bondedNodes[nodeAddr] == false ) && ( discoveredNodes[nodeAddr] == true ) )
          unbondedNodesStream << nodeAddr << ", ";

      std::string unbondedNodesStr = unbondedNodesStream.str();
      if ( unbondedNodesStr.empty() )
        return true;

      // Log unbonded nodes
      TRC_INFORMATION( "Nodes are discovered but NOT bonded. Discover the network!" << unbondedNodesStr );
      return false;
    }

    // SmartConnect
    TPerCoordinatorBondNodeSmartConnect_Response smartConnect( AutonetworkResult& autonetworkResult )
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
        // Optimized bonding ?
        IIqrfDpaService::CoordinatorParameters coordParams = m_iIqrfDpaService->getCoordinatorParameters();
        if ( ( coordParams.dpaVerWord >= 0x0414 ) && ( antwInputParams.overlappingNetworks.networks != 0 ) && ( antwInputParams.overlappingNetworks.network != 0 ) )
        {
          // Optimize bonging (applied for DPA >= 0x414)
          smartConnectPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSmartConnect_Request.MID[0] = antwInputParams.overlappingNetworks.network - 1;
          smartConnectPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSmartConnect_Request.MID[1] = antwInputParams.overlappingNetworks.networks;
          smartConnectPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSmartConnect_Request.MID[2] = 0xff;
          smartConnectPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSmartConnect_Request.MID[3] = 0xff;
        }
        // Set res0 to zero
        smartConnectPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSmartConnect_Request.reserved0[0x00] = 0x00;
        smartConnectPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSmartConnect_Request.reserved0[0x01] = 0x00;
        // Virtual Device Address - must equal 0xFF if not used.
        smartConnectPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSmartConnect_Request.VirtualDeviceAddress = 0xff;
        // Fill res1 with zeros
        std::fill_n( smartConnectPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSmartConnect_Request.reserved1, 9, 0 );
        // User data - zeroes
        std::fill_n( smartConnectPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSmartConnect_Request.UserData, 4, 0 );
        // Data to buffer
        smartConnectRequest.DataToBuffer( smartConnectPacket.Buffer, sizeof( TDpaIFaceHeader ) + sizeof( TPerCoordinatorSmartConnect_Request ) );
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat( smartConnectRequest, transResult, antwInputParams.actionRetries );
        TRC_DEBUG( "Result from Smart Connect transaction as string:" << PAR( transResult->getErrorString() ) );
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION( "Smart Connect successful!" );
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR( Peripheral type, smartConnectRequest.PeripheralType() )
          << NAME_PAR( Node address, smartConnectRequest.NodeAddress() )
          << NAME_PAR( Command, (int)smartConnectRequest.PeripheralCommand() )
        );
        autonetworkResult.addTransactionResult( transResult );
        TRC_FUNCTION_LEAVE( "" );
        return dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerCoordinatorBondNodeSmartConnect_Response;
      }
      catch ( std::exception& e )
      {
        AutonetworkError error( AutonetworkError::Type::Prebond, e.what() );
        autonetworkResult.setError( error );
        autonetworkResult.addTransactionResult( transResult );
        THROW_EXC( std::logic_error, e.what() );
      }
    }

    // Returns prebonded alive nodes
    std::basic_string<uint8_t> FrcPrebondedAliveNodes( AutonetworkResult& autonetworkResult, const uint8_t nodeSeed )
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
        m_exclusiveAccess->executeDpaTransactionRepeat( prebondedAliveRequest, transResult, antwInputParams.actionRetries );
        TRC_DEBUG( "Result from FRC Prebonded Alive transaction as string:" << PAR( transResult->getErrorString() ) );
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION( "FRC Prebonded Alive successful!" );
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR( Peripheral type, prebondedAliveRequest.PeripheralType() )
          << NAME_PAR( Node address, prebondedAliveRequest.NodeAddress() )
          << NAME_PAR( Command, (int)prebondedAliveRequest.PeripheralCommand() )
        );
        autonetworkResult.addTransactionResult( transResult );
        // Check FRC status
        uint8_t status = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.Status;
        if ( status < 0xfd )
        {
          // Add FRC result
          TRC_INFORMATION( "FRC Prebonded Alive status OK." << NAME_PAR_HEX( "Status", (int)status ) );         
          // Get list of nodes responded FRC_PrebondedAlive
          std::basic_string<uint8_t> prebondedNodes;
          prebondedNodes.clear();
          // Check FRC data - bit0
          for ( uint8_t nodeAddr = 1; nodeAddr <= MAX_ADDRESS; nodeAddr++ )
            if ( dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.FrcData[nodeAddr / 8] & ( 1 << ( nodeAddr % 8 ) ) )
              prebondedNodes.push_back( nodeAddr );
          TRC_FUNCTION_LEAVE( "" );
          return prebondedNodes;
        }
        else
        {
          TRC_WARNING( "FRC Prebonded Alive NOK!" << NAME_PAR_HEX( "Status", (int)status ) );
          THROW_EXC( std::logic_error, "Bad FRC status: " << PAR( (int)status ) );
        }
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
    void setFRCSelectedNodes(uint8_t* pData, const std::basic_string<uint8_t>& selectedNodes)
    {
      // Initialize to zero values
      memset(pData, 0, 30 * sizeof( uint8_t ));
      for (uint8_t i : selectedNodes) 
        pData[i / 0x08] |= (0x01 << (i % 8));
    }

    // FRC_PrebondedMemoryRead4BPlus1 (used to read MIDs and HWPID)
    std::basic_string<uint8_t> FrcPrebondedMemoryRead4BPlus1( AutonetworkResult& autonetworkResult, const std::basic_string<uint8_t>& prebondedNodes, const uint8_t nodeSeed, const uint8_t offset, const uint16_t address, const uint8_t PNUM, const uint8_t PCMD )
    {
      TRC_FUNCTION_ENTER( "" );
      std::unique_ptr<IDpaTransactionResult2> transResult;
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
        prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.FrcCommand = FRC_PrebondedMemoryRead4BPlus1;
        // Selected nodes - prebonded alive nodes
        setFRCSelectedNodes( prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.SelectedNodes, prebondedNodes );
        // Node seed, offset
        prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x00] = nodeSeed;
        prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x01] = offset;
        // OS Read command
        prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x02] = address & 0xff;
        prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x03] = address >> 0x08;
        prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x04] = PNUM;
        prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x05] = PCMD;
        prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x06] = 0x00;
        prebondedMemoryRequest.DataToBuffer( prebondedMemoryPacket.Buffer, sizeof( TDpaIFaceHeader ) + 38 );
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat( prebondedMemoryRequest, transResult, antwInputParams.actionRetries );
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
        std::basic_string<uint8_t> prebondedMemoryData;
        // Check status
        uint8_t status = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.Status;
        if ( status < 0xfd )
        {
          TRC_INFORMATION( "FRC Prebonded Memory Read status ok." << NAME_PAR_HEX( "Status", (int)status ) );
          prebondedMemoryData.append( dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.FrcData + sizeof(TMID), 51 );
          TRC_DEBUG( "Size of FRC data: " << PAR( prebondedMemoryData.size() ) );
        }
        else
        {
          TRC_WARNING( "FRC Prebonded Memory Read NOT ok." << NAME_PAR_HEX( "Status", (int)status ) );
          THROW_EXC( std::logic_error, "Bad FRC status: " << PAR( (int)status ) );
        }
        // Add FRC result
        autonetworkResult.addTransactionResult( transResult );

        // Read FRC extra result (if needed)
        if ( prebondedNodes.size() > 12 )
        {
          // Read FRC extra results
          DpaMessage extraResultRequest;
          DpaMessage::DpaPacket_t extraResultPacket;
          extraResultPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
          extraResultPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
          extraResultPacket.DpaRequestPacket_t.PCMD = CMD_FRC_EXTRARESULT;
          extraResultPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
          extraResultRequest.DataToBuffer( extraResultPacket.Buffer, sizeof( TDpaIFaceHeader ) );
          // Execute the DPA request
          m_exclusiveAccess->executeDpaTransactionRepeat( extraResultRequest, transResult, antwInputParams.actionRetries );
          TRC_DEBUG( "Result from FRC CMD_FRC_EXTRARESULT transaction as string:" << PAR( transResult->getErrorString() ) );
          dpaResponse = transResult->getResponse();
          TRC_INFORMATION( "FRC CMD_FRC_EXTRARESULT successful!" );
          TRC_DEBUG(
            "DPA transaction: "
            << NAME_PAR( Peripheral type, extraResultRequest.PeripheralType() )
            << NAME_PAR( Node address, extraResultRequest.NodeAddress() )
            << NAME_PAR( Command, (int)extraResultRequest.PeripheralCommand() )
          );
          // Append FRC data
          prebondedMemoryData.append( dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData, 9 );
          // Add FRC extra result
          autonetworkResult.addTransactionResult( transResult );
        }        
        TRC_FUNCTION_LEAVE( "" );
        return prebondedMemoryData;
      }
      catch ( std::exception& e )
      {
        AutonetworkError error( AutonetworkError::Type::PrebondedMemoryRead, e.what() );
        autonetworkResult.setError( error );
        autonetworkResult.addTransactionResult( transResult );
        THROW_EXC( std::logic_error, e.what() );
      }
    }

    // FRC_PrebondedMemoryCompare2B
    std::basic_string<uint8_t> FrcPrebondedMemoryCompare2B( AutonetworkResult& autonetworkResult, const std::basic_string<uint8_t>& prebondedNodes, const uint8_t nodeSeed, const uint16_t valueToCompare, const uint16_t address, const uint8_t PNUM, const uint8_t PCMD )
    {
      TRC_FUNCTION_ENTER( "" );
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage prebondedMemoryRequest;
        DpaMessage::DpaPacket_t prebondedMemoryPacket;
        prebondedMemoryPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        prebondedMemoryPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
        prebondedMemoryPacket.DpaRequestPacket_t.PCMD = CMD_FRC_SEND_SELECTIVE;
        prebondedMemoryPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        // FRC Command - https://doc.iqrf.org/DpaTechGuide/414/
        prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.FrcCommand = FRC_PrebondedMemoryCompare2B;
        // Selected nodes - prebonded alive nodes
        setFRCSelectedNodes( prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.SelectedNodes, prebondedNodes );
        // Node seed
        prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x00] = nodeSeed;
        // Zero
        prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x01] = 0x00;
        // Flags 
        prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x02] = 0x01;
        // Value to compare
        prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x03] = valueToCompare & 0xff;
        prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x04] = valueToCompare >> 0x08;
        // 
        prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x05] = address & 0xff;
        prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x06] = address >> 0x08;
        prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x07] = PNUM;
        prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x08] = PCMD;
        prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x09] = 0x00;
        prebondedMemoryRequest.DataToBuffer( prebondedMemoryPacket.Buffer, sizeof( TDpaIFaceHeader ) + 41 );
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat( prebondedMemoryRequest, transResult, antwInputParams.actionRetries );
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
        std::basic_string<uint8_t> prebondedMemoryData;
        // Check status
        uint8_t status = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.Status;
        if ( status < 0xFD )
        {
          TRC_INFORMATION( "FRC Prebonded Memory Read status ok." << NAME_PAR_HEX( "Status", (int)status ) );
          prebondedMemoryData.append( dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.FrcData, 55 );
          TRC_DEBUG( "Size of FRC data: " << PAR( prebondedMemoryData.size() ) );
        }
        else
        {
          TRC_WARNING( "FRC Prebonded Memory Read NOT ok." << NAME_PAR_HEX( "Status", (int)status ) );
          THROW_EXC( std::logic_error, "Bad FRC status: " << PAR( (int)status ) );
        }
        // Add FRC result
        autonetworkResult.addTransactionResult( transResult );

        // Read FRC extra results
        DpaMessage extraResultRequest;
        DpaMessage::DpaPacket_t extraResultPacket;
        extraResultPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        extraResultPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
        extraResultPacket.DpaRequestPacket_t.PCMD = CMD_FRC_EXTRARESULT;
        extraResultPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        extraResultRequest.DataToBuffer( extraResultPacket.Buffer, sizeof( TDpaIFaceHeader ) );
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat( extraResultRequest, transResult, antwInputParams.actionRetries );
        TRC_DEBUG( "Result from FRC CMD_FRC_EXTRARESULT transaction as string:" << PAR( transResult->getErrorString() ) );
        dpaResponse = transResult->getResponse();
        TRC_INFORMATION( "FRC CMD_FRC_EXTRARESULT successful!" );
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR( Peripheral type, extraResultRequest.PeripheralType() )
          << NAME_PAR( Node address, extraResultRequest.NodeAddress() )
          << NAME_PAR( Command, (int)extraResultRequest.PeripheralCommand() )
        );
        // Append FRC data
        prebondedMemoryData.append( dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData, 7 );
        // Add FRC extra result
        autonetworkResult.addTransactionResult( transResult );
        TRC_FUNCTION_LEAVE( "" );
        return prebondedMemoryData;
      }
      catch ( std::exception& e )
      {
        AutonetworkError error( AutonetworkError::Type::PrebondedMemoryRead, e.what() );
        autonetworkResult.setError( error );
        autonetworkResult.addTransactionResult( transResult );
        THROW_EXC( std::logic_error, e.what() );
      }
    }

    // Bond authorization
    TPerCoordinatorAuthorizeBond_Response authorizeBond( AutonetworkResult& autonetworkResult, std::basic_string<TPrebondedNode> &nodes )
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
        // Add nodes
        uint8_t index = 0;
        for ( TPrebondedNode node : nodes )
        {
          // Requested address
          authorizeBondPacket.DpaRequestPacket_t.DpaMessage.Request.PData[index++] = node.addrBond;
          // MID to authorize
          authorizeBondPacket.DpaRequestPacket_t.DpaMessage.Request.PData[index++] = node.mid.bytes[0x00];
          authorizeBondPacket.DpaRequestPacket_t.DpaMessage.Request.PData[index++] = node.mid.bytes[0x01];
          authorizeBondPacket.DpaRequestPacket_t.DpaMessage.Request.PData[index++] = node.mid.bytes[0x02];
          authorizeBondPacket.DpaRequestPacket_t.DpaMessage.Request.PData[index++] = node.mid.bytes[0x03];
        }
        // Data to buffer
        authorizeBondRequest.DataToBuffer( authorizeBondPacket.Buffer, sizeof( TDpaIFaceHeader ) + index );
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat( authorizeBondRequest, transResult, antwInputParams.actionRetries );
        TRC_DEBUG( "Result from Authorize Bond transaction as string:" << PAR( transResult->getErrorString() ) );
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION( "Authorize Bond ok!" );
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR( Peripheral type, authorizeBondRequest.PeripheralType() )
          << NAME_PAR( Node address, authorizeBondRequest.NodeAddress() )
          << NAME_PAR( Command, (int)authorizeBondRequest.PeripheralCommand() )
        );
        // Add FRC extra result
        autonetworkResult.addTransactionResult( transResult );
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

    // Ping new nodes
    TPerFrcSend_Response FrcPingNodes( AutonetworkResult& autonetworkResult )
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
        m_exclusiveAccess->executeDpaTransactionRepeat( checkNewNodesRequest, transResult, antwInputParams.actionRetries );
        TRC_DEBUG( "Result from Check new nodes transaction as string:" << PAR( transResult->getErrorString() ) );
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION( "Check new nodes ok!" );
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR( Peripheral type, checkNewNodesRequest.PeripheralType() )
          << NAME_PAR( Node address, checkNewNodesRequest.NodeAddress() )
          << NAME_PAR( Command, (int)checkNewNodesRequest.PeripheralCommand() )
        );
        autonetworkResult.addTransactionResult( transResult );
        // Check FRC status
        uint8_t status = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.Status;
        if ( status <= 0xEF )
        {
          TRC_INFORMATION( "FRC_Ping: status OK." << NAME_PAR_HEX( "Status", (int)status ) );
          TRC_FUNCTION_LEAVE( "" );
          return dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response;
        }
        else
        {
          TRC_WARNING( "FRC_Ping: status NOK!" << NAME_PAR_HEX( "Status", (int)status ) );
          THROW_EXC( std::logic_error, "Bad FRC status: " << PAR( (int)status ) );
        }
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
    TPerFrcSend_Response removeNotRespondedNewNodes( AutonetworkResult& autonetworkResult, const std::basic_string<uint8_t>& notRespondedNewNodes )
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
        memset( (void*)frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData, sizeof( frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData ), mZero );
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
          frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x00] = 15;
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
        m_exclusiveAccess->executeDpaTransactionRepeat( frcAckBroadcastRequest, transResult, antwInputParams.actionRetries );
        TRC_DEBUG( "Result from Remove bond and restart (SELECTIVE BROADCAST BATCH) transaction as string:" << PAR( transResult->getErrorString() ) );
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION( "Selective BATCH Remove bond and restart ok!" );
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR( Peripheral type, frcAckBroadcastRequest.PeripheralType() )
          << NAME_PAR( Node address, frcAckBroadcastRequest.NodeAddress() )
          << NAME_PAR( Command, (int)frcAckBroadcastRequest.PeripheralCommand() )
        );
        autonetworkResult.addTransactionResult( transResult );
        // Check FRC status
        uint8_t status = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.Status;
        if ( status <= 0xEF )
        {
          TRC_INFORMATION( "FRC Prebonded Alive status OK." << NAME_PAR_HEX( "Status", (int)status ) );
          TRC_FUNCTION_LEAVE( "" );
          return dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response;
        }
        else
        {
          TRC_WARNING( "FRC Prebonded Alive status NOK!" << NAME_PAR_HEX( "Status", (int)status ) );
          THROW_EXC( std::logic_error, "Bad FRC status: " << PAR( (int)status ) );
        }
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
    void removeBondAtCoordinator( AutonetworkResult& autonetworkResult, const uint8_t nodeAddress )
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
        removeBondAtCoordinatorPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorRemoveBond_Request.BondAddr = nodeAddress;
        // Data to buffer
        removeBondAtCoordinatorRequest.DataToBuffer( removeBondAtCoordinatorPacket.Buffer, sizeof( TDpaIFaceHeader ) + sizeof( TPerCoordinatorRemoveBond_Request ) );
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat( removeBondAtCoordinatorRequest, transResult, antwInputParams.actionRetries );
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
    uint8_t runDiscovery( AutonetworkResult& autonetworkResult, const uint8_t txPower )
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
        // Data to buffer
        runDiscoveryRequest.DataToBuffer( runDiscoveryPacket.Buffer, sizeof( TDpaIFaceHeader ) + sizeof( TPerCoordinatorDiscovery_Request ) );
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat( runDiscoveryRequest, transResult, antwInputParams.actionRetries );
        TRC_INFORMATION( "Run discovery ok!" );
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR( Peripheral type, runDiscoveryRequest.PeripheralType() )
          << NAME_PAR( Node address, runDiscoveryRequest.NodeAddress() )
          << NAME_PAR( Command, (int)runDiscoveryRequest.PeripheralCommand() )
        );
        TRC_DEBUG( "Result from Run discovery transaction as string:" << PAR( transResult->getErrorString() ) );
        autonetworkResult.addTransactionResult( transResult );
        TRC_FUNCTION_LEAVE( "" );
        return  dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerCoordinatorDiscovery_Response.DiscNr;
      }
      catch ( std::exception& e )
      {
        AutonetworkError error( AutonetworkError::Type::RunDiscovery, e.what() );
        autonetworkResult.setError( error );
        autonetworkResult.addTransactionResult( transResult );
        THROW_EXC( std::logic_error, e.what() );
      }
    }

    // Clear duplicit MID
    void clearDuplicitMID( AutonetworkResult& autonetworkResult )
    {
      TRC_FUNCTION_ENTER( "" );
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Any duplicitMIDs ?
        if ( antwProcessParams.duplicitMID.empty() == false )
        {
          DpaMessage validateBondRequest;
          DpaMessage::DpaPacket_t validateBondPacket;
          validateBondPacket.DpaRequestPacket_t.NADR = BROADCAST_ADDRESS;
          validateBondPacket.DpaRequestPacket_t.PNUM = PNUM_NODE;
          validateBondPacket.DpaRequestPacket_t.PCMD = CMD_NODE_VALIDATE_BONDS;
          validateBondPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
          uint8_t index = 0;
          for ( uint8_t address = 1; address <= MAX_ADDRESS; address++ )
          {
            auto node = std::find( antwProcessParams.duplicitMID.begin(), antwProcessParams.duplicitMID.end(), address );
            if ( node != antwProcessParams.duplicitMID.end() )
            {
              validateBondPacket.DpaRequestPacket_t.DpaMessage.PerNodeValidateBonds_Request.Bonds[index].Address = address;
              if ( antwProcessParams.networkNodes[address].bonded == true )
              {
                validateBondPacket.DpaRequestPacket_t.DpaMessage.PerNodeValidateBonds_Request.Bonds[index].MID[0x00] = antwProcessParams.networkNodes[address].mid.bytes[0x00];
                validateBondPacket.DpaRequestPacket_t.DpaMessage.PerNodeValidateBonds_Request.Bonds[index].MID[0x01] = antwProcessParams.networkNodes[address].mid.bytes[0x01];
                validateBondPacket.DpaRequestPacket_t.DpaMessage.PerNodeValidateBonds_Request.Bonds[index].MID[0x02] = antwProcessParams.networkNodes[address].mid.bytes[0x02];
                validateBondPacket.DpaRequestPacket_t.DpaMessage.PerNodeValidateBonds_Request.Bonds[index].MID[0x03] = antwProcessParams.networkNodes[address].mid.bytes[0x03];
                antwProcessParams.duplicitMID.erase( node );
              }
              else
              {
                validateBondPacket.DpaRequestPacket_t.DpaMessage.PerNodeValidateBonds_Request.Bonds[index].MID[0x00] = 0;
                validateBondPacket.DpaRequestPacket_t.DpaMessage.PerNodeValidateBonds_Request.Bonds[index].MID[0x01] = 0;
                validateBondPacket.DpaRequestPacket_t.DpaMessage.PerNodeValidateBonds_Request.Bonds[index].MID[0x02] = 0;
                validateBondPacket.DpaRequestPacket_t.DpaMessage.PerNodeValidateBonds_Request.Bonds[index].MID[0x03] = 0;
              }
              index++;
            }

            if ( ( index == 11 ) || ( address == MAX_ADDRESS ) )
            {
              if ( ( index != 11 ) && ( address == MAX_ADDRESS ) )
              {
                validateBondPacket.DpaRequestPacket_t.DpaMessage.PerNodeValidateBonds_Request.Bonds[index].Address = TEMPORARY_ADDRESS;
                validateBondPacket.DpaRequestPacket_t.DpaMessage.PerNodeValidateBonds_Request.Bonds[index].MID[0x00] = 0;
                validateBondPacket.DpaRequestPacket_t.DpaMessage.PerNodeValidateBonds_Request.Bonds[index].MID[0x01] = 0;
                validateBondPacket.DpaRequestPacket_t.DpaMessage.PerNodeValidateBonds_Request.Bonds[index].MID[0x02] = 0;
                validateBondPacket.DpaRequestPacket_t.DpaMessage.PerNodeValidateBonds_Request.Bonds[index].MID[0x03] = 0;
                index++;
              }

              // Data to buffer
              validateBondRequest.DataToBuffer( validateBondPacket.Buffer, sizeof( TDpaIFaceHeader ) + index * sizeof( TPerNodeValidateBondsItem ) );
              // Execute the DPA request
              m_exclusiveAccess->executeDpaTransactionRepeat( validateBondRequest, transResult, antwInputParams.actionRetries );
              TRC_INFORMATION( "CMD_NODE_VALIDATE_BONDS ok!" );
              DpaMessage dpaResponse = transResult->getResponse();
              TRC_DEBUG(
                "DPA transaction: "
                << NAME_PAR( Peripheral type, validateBondRequest.PeripheralType() )
                << NAME_PAR( Node address, validateBondRequest.NodeAddress() )
                << NAME_PAR( Command, (int)validateBondRequest.PeripheralCommand() )
              );
              index = 0;
            }
          }
        }
        TRC_FUNCTION_LEAVE( "" );
      }
      catch ( std::exception& e )
      {
        AutonetworkError error( AutonetworkError::Type::ValidateBonds, e.what() );
        autonetworkResult.setError( error );
        autonetworkResult.addTransactionResult( transResult );
        THROW_EXC( std::logic_error, e.what() );
      }
    }

    // Unbond nodes with temporary address
    void unbondTemporaryAddress( AutonetworkResult& autonetworkResult )
    {
      TRC_FUNCTION_ENTER( "" );
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        DpaMessage validateBondRequest;
        DpaMessage::DpaPacket_t validateBondPacket;
        validateBondPacket.DpaRequestPacket_t.NADR = BROADCAST_ADDRESS;
        validateBondPacket.DpaRequestPacket_t.PNUM = PNUM_NODE;
        validateBondPacket.DpaRequestPacket_t.PCMD = CMD_NODE_VALIDATE_BONDS;
        validateBondPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        validateBondPacket.DpaRequestPacket_t.DpaMessage.PerNodeValidateBonds_Request.Bonds[0x00].Address = TEMPORARY_ADDRESS;
        validateBondPacket.DpaRequestPacket_t.DpaMessage.PerNodeValidateBonds_Request.Bonds[0x00].MID[0x00] = 0x00;
        validateBondPacket.DpaRequestPacket_t.DpaMessage.PerNodeValidateBonds_Request.Bonds[0x00].MID[0x01] = 0x00;
        validateBondPacket.DpaRequestPacket_t.DpaMessage.PerNodeValidateBonds_Request.Bonds[0x00].MID[0x02] = 0x00;
        validateBondPacket.DpaRequestPacket_t.DpaMessage.PerNodeValidateBonds_Request.Bonds[0x00].MID[0x03] = 0x00;
        // Data to buffer
        validateBondRequest.DataToBuffer( validateBondPacket.Buffer, sizeof( TDpaIFaceHeader ) + sizeof( TPerNodeValidateBondsItem ) );
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat( validateBondRequest, transResult, antwInputParams.actionRetries );
        TRC_INFORMATION( "CMD_NODE_VALIDATE_BONDS ok!" );
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR( Peripheral type, validateBondRequest.PeripheralType() )
          << NAME_PAR( Node address, validateBondRequest.NodeAddress() )
          << NAME_PAR( Command, (int)validateBondRequest.PeripheralCommand() )
        );
        TRC_FUNCTION_LEAVE( "" );
      }
      catch ( std::exception& e )
      {
        AutonetworkError error( AutonetworkError::Type::ValidateBonds, e.what() );
        autonetworkResult.setError( error );
        autonetworkResult.addTransactionResult( transResult );
        THROW_EXC( std::logic_error, e.what() );
      }
    }

    // Authorize control
    bool authorizeControl( uint32_t MID, uint16_t HWPID, uint8_t& bondAddr, TAuthorizeErr& authorizeErr )
    {
      bondAddr = 0;
      authorizeErr = TAuthorizeErr::eNo;

      // Check duplicit MID
      for ( uint8_t address = 1; address <= MAX_ADDRESS; address++ )
      {
        if ( antwProcessParams.networkNodes[address].mid.value == MID )
        {
          TRC_WARNING( "authorizeControl: duplicit MID found " << PAR( (int)antwProcessParams.networkNodes[address].mid.value ) );
          authorizeErr = TAuthorizeErr::eNodeBonded;
          bondAddr = address;
          return false;
        }
      }

      // Overlapping networks
      IIqrfDpaService::CoordinatorParameters coordParams = m_iIqrfDpaService->getCoordinatorParameters();
      if ( ( coordParams.dpaVerWord < 0x0414 ) && ( antwInputParams.overlappingNetworks.networks != 0 ) && ( antwInputParams.overlappingNetworks.network != 0 ) )
      {
        // Applied only when DPA at [C] is < 0x414
        uint32_t rem = MID % antwInputParams.overlappingNetworks.networks;
        if ( rem != (uint32_t)( antwInputParams.overlappingNetworks.network - 1 ) )
        {
          authorizeErr = TAuthorizeErr::eNetworkNum;
          return false;
        }
      }

      // HWPID filtering ?
      if ( antwInputParams.hwpidFiltering.empty() == false )
      {
        // Check HWPID
        if ( std::find( antwInputParams.hwpidFiltering.begin(), antwInputParams.hwpidFiltering.end(), HWPID ) == antwInputParams.hwpidFiltering.end() )
        {
          authorizeErr = TAuthorizeErr::eHWPIDFiltering;
          return false;
        }
      }

      // Select free address to bond
      for ( uint8_t addr = 1; addr <= MAX_ADDRESS; addr++ )
      {
        // Node bonded ?
        if ( antwProcessParams.networkNodes[addr].bonded == false )
        {
          // No, use address 
          antwProcessParams.networkNodes[addr].bonded = true;
          bondAddr = addr;
          return true;
        }
      }

      // No free address
      authorizeErr = TAuthorizeErr::eAddress;
      return false;
    }

    // Get string description of the wave state
    const std::string getWaveState( void )
    {
      std::string strWaveState = "";
      switch ( antwProcessParams.waveStateCode )
      {
        case TWaveStateCode::discoveryBeforeStart:
          strWaveState = "Running discovery before start.";
          break;
        case TWaveStateCode::smartConnect:
          strWaveState = "Prebonding Nodes.";
          break;
        case TWaveStateCode::checkPrebondedAlive:
          strWaveState = "Looking for prebonded Nodes.";
          break;
        case TWaveStateCode::readingDPAVersion:
          strWaveState = "Reading DPA version of prebonded Nodes.";
          break;
        case TWaveStateCode::readPrebondedMID:
          strWaveState = "Reading MIDs of prebonded Nodes.";
          break;
        case TWaveStateCode::readPrebondedHWPID:
          strWaveState = "Reading HWPID of prebonded Nodes.";
          break;
        case TWaveStateCode::enumeration:
          strWaveState = "Enumerating authorized Nodes.";
          break;
        case TWaveStateCode::authorize:
          strWaveState = "Authorizing Nodes.";
          break;
        case TWaveStateCode::ping:
          strWaveState = "Running FRC to check new Nodes.";
          break;
        case TWaveStateCode::removeNotResponded:
          strWaveState = "Removing not responded Nodes.";
          break;
        case TWaveStateCode::discovery:
          strWaveState = "Running discovery.";
          break;
        case TWaveStateCode::stopOnMaxNumWaves:
          strWaveState = "Maximum number of waves reached.";
          break;
        case TWaveStateCode::stopOnNumberOfTotalNodes:
          strWaveState = "Number of total nodes bonded into network.";
          break;
        case TWaveStateCode::stopOnMaxEmptyWaves:
          strWaveState = "Maximum number of consecutive empty waves reached.";
          break;
        case TWaveStateCode::stopOnNumberOfNewNodes:
          strWaveState = "Number of new nodes bonded into network.";
          break;
        case TWaveStateCode::abortOnTooManyNodesFound:
          strWaveState = "Too many nodes found - Autonetwork process aborted.";
          break;
        case TWaveStateCode::abortOnAllAddresseAllocated:
          strWaveState = "All available network addresses are already allocated - Autonetwork process aborted.";
          break;
        case TWaveStateCode::waveFinished:
          strWaveState = "Wave finished.";
          break;     
        case TWaveStateCode::cannotStartProcessMaxAddress:
          strWaveState = "The AutoNetwork process cannot be started because all available network addresses are already allocated.";
          break;
        case TWaveStateCode::cannotStartProcessTotalNodesNr:
          strWaveState = "The AutoNetwork process cannot be started because the number of total nodes is equal or lower than the size of the existing network.";
          break;
        case TWaveStateCode::cannotStartProcessNewNodesNr:
          strWaveState = "The AutoNetwork process cannot be started because the number of existing nodes plus number of new nodes exceeds the maximum network size.";
          break;
        default:
          THROW_EXC( std::logic_error, "Unknown waveStateCode." );
      }

      return strWaveState;
    }

    // Send autonetwok algorithm state
    void sendWaveState( void )
    {
      Document waveState;
      // Set common parameters
      Pointer( "/mType" ).Set( waveState, m_msgType->m_type );
      Pointer( "/data/msgId" ).Set( waveState, m_comAutonetwork->getMsgId() );
      // Fill response
      rapidjson::Pointer( "/data/rsp/wave" ).Set( waveState, antwProcessParams.countWaves );
      rapidjson::Pointer( "/data/rsp/waveStateCode" ).Set( waveState, (int)antwProcessParams.waveStateCode );
      rapidjson::Pointer( "/data/rsp/progress" ).Set( waveState, (int)antwProcessParams.progress );
      if ( m_comAutonetwork->getVerbose() == true )
        rapidjson::Pointer( "/data/rsp/waveState" ).Set( waveState, getWaveState() );
      // Set status
      Pointer( "/data/status" ).Set( waveState, 0 );
      Pointer( "/data/statusStr" ).Set( waveState, "ok" );
      // Send message      
      m_iMessagingSplitterService->sendMessage( *m_messagingId, std::move( waveState ) );

      // Update progress
      if( antwProcessParams.progress < 100)
        antwProcessParams.progress += ( 100 / antwProcessParams.progressStep );
    }

    // Send wave result
    bool sendWaveResult( AutonetworkResult& autonetworkResult )
    {
      antwProcessParams.progress = 100;
      if ( antwProcessParams.waveStateCode == TWaveStateCode::waveFinished )
      {
        // Maximum waves reached ?
        if ( ( antwInputParams.stopConditions.waves != 0 ) && ( antwProcessParams.countWaves == antwInputParams.stopConditions.waves ) )
        {
          TRC_INFORMATION( "Maximum number of waves reached." );
          antwProcessParams.waveStateCode = TWaveStateCode::stopOnMaxNumWaves;
        }

        // Maximum empty waves reached ?
        if ( ( antwInputParams.stopConditions.emptyWaves != 0 ) && ( antwProcessParams.countEmpty >= antwInputParams.stopConditions.emptyWaves ) )
        {
          TRC_INFORMATION( "Maximum number of consecutive empty waves reached." );
          antwProcessParams.waveStateCode = TWaveStateCode::stopOnMaxEmptyWaves;
        }

        // Number of new nodes bonded into network ?
        if ( ( antwInputParams.stopConditions.numberOfNewNodes != 0 ) && ( antwProcessParams.countNewNodes >= antwInputParams.stopConditions.numberOfNewNodes ) )
        {
          TRC_INFORMATION( "Number of new nodes bonded into network." );
          antwProcessParams.waveStateCode = TWaveStateCode::stopOnNumberOfNewNodes;
        }

        // Number of total nodes bonded into network ?
        if ( ( antwInputParams.stopConditions.numberOfTotalNodes != 0 ) && ( antwProcessParams.bondedNodesNr >= antwInputParams.stopConditions.numberOfTotalNodes ) )
        {
          TRC_INFORMATION( "Number of total nodes bonded into network." );
          antwProcessParams.waveStateCode = TWaveStateCode::stopOnNumberOfTotalNodes;
        }

        // Check max address
        if ( antwProcessParams.bondedNodes == MAX_ADDRESS )
        {
          TRC_INFORMATION( "All available network addresses are already allocated - Autonetwork process aborted." );
          antwProcessParams.waveStateCode = TWaveStateCode::abortOnAllAddresseAllocated;
        }
      }

      bool stopCondReached = antwProcessParams.waveStateCode != TWaveStateCode::waveFinished;
      Document waveResult;
      // Set common parameters
      Pointer( "/mType" ).Set( waveResult, m_msgType->m_type );
      Pointer( "/data/msgId" ).Set( waveResult, m_comAutonetwork->getMsgId() );

      // Add wave result
      rapidjson::Pointer( "/data/rsp/wave" ).Set( waveResult, antwProcessParams.countWaves );
      rapidjson::Pointer( "/data/rsp/nodesNr" ).Set( waveResult, antwProcessParams.bondedNodesNr );
      rapidjson::Pointer( "/data/rsp/newNodesNr" ).Set( waveResult, antwProcessParams.countWaveNewNodes );
      rapidjson::Pointer( "/data/rsp/waveStateCode" ).Set( waveResult, (int)antwProcessParams.waveStateCode );
      rapidjson::Pointer( "/data/rsp/progress" ).Set( waveResult, (int)antwProcessParams.progress );
      if ( m_comAutonetwork->getVerbose() == true )
        rapidjson::Pointer( "/data/rsp/waveState" ).Set( waveResult, getWaveState() );
      rapidjson::Pointer( "/data/rsp/lastWave" ).Set( waveResult, stopCondReached );
      if ( antwProcessParams.respondedNewNodes.empty() == false )
      {
        // Rsp object
        rapidjson::Pointer( "/data/rsp/wave" ).Set( waveResult, antwProcessParams.countWaves );
        rapidjson::Pointer( "/data/rsp/nodesNr" ).Set( waveResult, antwProcessParams.bondedNodesNr );
        rapidjson::Pointer( "/data/rsp/newNodesNr" ).Set( waveResult, antwProcessParams.countWaveNewNodes );
        rapidjson::Value newNodesJsonArray( kArrayType );
        Document::AllocatorType& allocator = waveResult.GetAllocator();
        for ( AutonetworkResult::NewNode newNode : antwProcessParams.respondedNewNodes )
        {
          rapidjson::Value newNodeObject( kObjectType );
          std::stringstream stream;
          stream << std::hex << newNode.MID;
          newNodeObject.AddMember( "mid", stream.str(), allocator );
          newNodeObject.AddMember( "address", newNode.address, allocator );
          newNodesJsonArray.PushBack( newNodeObject, allocator );
        }
        Pointer( "/data/rsp/newNodes" ).Set( waveResult, newNodesJsonArray );
      }

      // Set status
      AutonetworkError error = autonetworkResult.getError();
      int status = 0;
      if ( error.getType() != AutonetworkError::Type::NoError )
        status = SERVICE_ERROR + (int)error.getType();

      // Set raw fields, if verbose mode is active
      if ( m_comAutonetwork->getVerbose() == true )
      {
        rapidjson::Value rawArray( kArrayType );
        Document::AllocatorType& allocator = waveResult.GetAllocator();

        while ( autonetworkResult.isNextTransactionResult() ) {
          std::unique_ptr<IDpaTransactionResult2> transResult = autonetworkResult.consumeNextTransactionResult();
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

        // Add array into response document
        Pointer( "/data/raw" ).Set( waveResult, rawArray );
      }

      // Set status
      Pointer( "/data/status" ).Set( waveResult, status );
      Pointer( "/data/statusStr" ).Set( waveResult, error.getMessage() );

      // Send message      
      m_iMessagingSplitterService->sendMessage( *m_messagingId, std::move( waveResult ) );
      
      return stopCondReached;
    }

    // Process the autonetwork algorithm
    void runAutonetwork( void )
    {
      TRC_FUNCTION_ENTER( "" );

      // Autonetwork result
      AutonetworkResult autonetworkResult;
      // List of new nodes passed to IqrfInfo when AN finishes
      std::map<int, embed::node::BriefInfo> newNodes;

      try
      {
        // Check, if Coordinator and OS peripherals are present at coordinator's
        checkPresentCoordAndCoordOs( autonetworkResult );
        TRC_INFORMATION( "Initial network check." );

        // Initialize networkNodes structure
        antwProcessParams.networkNodes.clear();
        TNode node;
        node.mid.value = 0;
        node.discovered = false;
        node.bonded = false;
        node.online = false;
        for ( uint8_t addr = 0; addr <= MAX_ADDRESS; addr++ )
        {
          node.address = addr;
          antwProcessParams.networkNodes[addr] = node;
        }

        // Update network info
        updateNetworkInfo( autonetworkResult );
        TRC_INFORMATION( NAME_PAR( Bonded nodes, toNodesListStr( antwProcessParams.bondedNodes ) ) );
        TRC_INFORMATION( NAME_PAR( Discovered nodes, toNodesListStr( antwProcessParams.discoveredNodes ) ) );

        // Initialize process params
        antwProcessParams.countNewNodes = 0;
        antwProcessParams.countWaves = 1;
        antwProcessParams.countEmpty = 0;
        newNodes.clear();
        antwProcessParams.countWaveNewNodes = 0;
        antwProcessParams.respondedNewNodes.clear();

        // Check max address
        if ( antwProcessParams.bondedNodes == MAX_ADDRESS )
        {
          TRC_INFORMATION( "The AutoNetwork process cannot be started because all available network addresses are already allocated." )
          antwProcessParams.waveStateCode = TWaveStateCode::cannotStartProcessMaxAddress;
          sendWaveResult( autonetworkResult );
          TRC_FUNCTION_LEAVE( "" );
          return;
        }

        // Check stop conditions - number of total nodes
        if ( ( antwInputParams.stopConditions.numberOfTotalNodes != 0 ) && ( antwProcessParams.bondedNodesNr >= antwInputParams.stopConditions.numberOfTotalNodes ) )
        {
          TRC_INFORMATION( "The AutoNetwork process cannot be started because the number of total nodes is equal or lower than the size of the existing network." );
          antwProcessParams.waveStateCode = TWaveStateCode::cannotStartProcessTotalNodesNr;
          sendWaveResult( autonetworkResult );
          TRC_FUNCTION_LEAVE( "" );
          return;
        }

        // Check stop conditions - number of new nodes          
        if ( ( antwInputParams.stopConditions.numberOfNewNodes != 0 ) && ( ( antwInputParams.stopConditions.numberOfNewNodes + antwProcessParams.bondedNodesNr ) > MAX_ADDRESS ) )
        {
          TRC_INFORMATION( "The AutoNetwork process cannot be started because the number of existing nodes plus number of new nodes exceeds the maximum network size." );
          antwProcessParams.waveStateCode = TWaveStateCode::cannotStartProcessNewNodesNr;
          sendWaveResult( autonetworkResult );
          TRC_FUNCTION_LEAVE( "" );
          return;
        }

        // Set FRC param to 0, store previous value
        antwProcessParams.FrcResponseTime = 0;
        antwProcessParams.FrcResponseTime = setFrcReponseTime( autonetworkResult, antwProcessParams.FrcResponseTime );
        TRC_INFORMATION( "Set FRC Response time to 0x00 (_FRC_RESPONSE_TIME_40_MS)" );

        // Set DPA param to 0, store previous value
        antwProcessParams.DpaParam = 0;
        antwProcessParams.DpaParam = setNoLedAndOptimalTimeslot( autonetworkResult, antwProcessParams.DpaParam );
        TRC_INFORMATION( "No LED indication and use of optimal time slot length" );

        // Set DPA Hops Param to 0xff, 0xff and store previous values
        antwProcessParams.RequestHops = 0xff;
        antwProcessParams.ResponseHops = 0xff;
        TPerCoordinatorSetHops_Request_Response response = setDpaHopsToTheNumberOfRouters( autonetworkResult, antwProcessParams.RequestHops, antwProcessParams.ResponseHops );
        antwProcessParams.RequestHops = response.RequestHops;
        antwProcessParams.ResponseHops = response.ResponseHops;
        TRC_INFORMATION( "Number of hops set to the number of routers" );

        // Start autonetwork 
        TRC_INFORMATION( "Automatic network construction in progress" );
        antwProcessParams.countWaves = 0;
        antwProcessParams.initialBondedNodesNr = antwProcessParams.bondedNodesNr;
        using std::chrono::system_clock;
        bool waveRun = true;
        std::basic_string<uint8_t> FrcSelect, FrcOnlineNodes, FrcOfflineNodes, FrcSupportMultipleAuth;
        uint8_t retryAction, countDiscNodes = antwProcessParams.discoveredNodesNr;
        uint8_t maxStep, step;
        bool stepBreak;

        // Initialize random seed
        std::srand( std::time( nullptr ) );
        uint8_t nodeSeed = (uint8_t)std::rand();

        // Calculate progress step per wave
        antwProcessParams.progressStep = 6;
        if ( antwInputParams.skipDiscoveryEachWave == false )
          antwProcessParams.progressStep++;
        if ( antwInputParams.hwpidFiltering.empty() == false )
          antwProcessParams.progressStep++;
        // Get DPA version
        // DPA >= 4.14 ?
        IIqrfDpaService::CoordinatorParameters coordParams = m_iIqrfDpaService->getCoordinatorParameters();
        if ( coordParams.dpaVerWord >= 0x0414 )
          antwProcessParams.progressStep++;

        // Main loop
        while ( waveRun )
        {
          // Increment nodeSeed and countWaves
          nodeSeed++;
          antwProcessParams.countWaves++;

          // Clear progress
          antwProcessParams.progress = 0;

          // Run Discovery before start ?
          if ( ( antwProcessParams.countWaves == 1 ) && ( antwInputParams.discoveryBeforeStart == true ) && ( antwProcessParams.bondedNodesNr > 0 ) )
          {
            TRC_INFORMATION( "Running Discovery before start." );
            antwProcessParams.waveStateCode = TWaveStateCode::discoveryBeforeStart;
            sendWaveState();
            runDiscovery( autonetworkResult, antwInputParams.discoveryTxPower );
          }

          // SmartConnect
          antwProcessParams.countWaveNewNodes = 0;
          antwProcessParams.respondedNewNodes.clear();
          TRC_INFORMATION( "SmartConnect." );
          antwProcessParams.waveStateCode = TWaveStateCode::smartConnect;
          sendWaveState();
          smartConnect( autonetworkResult );

          // ToDo
          std::this_thread::sleep_for( std::chrono::milliseconds( TIMEOUT_STEP ) );

          // Get pre-bonded alive nodes
          TRC_INFORMATION( "Reading prebonded alive nodes." );
          antwProcessParams.waveStateCode = TWaveStateCode::checkPrebondedAlive;
          sendWaveState();
          FrcSelect = FrcPrebondedAliveNodes( autonetworkResult, nodeSeed );

          // Clear prebondedNodes map
          antwProcessParams.prebondedNodes.clear();
          // Clear new nodes for the next wave
          autonetworkResult.clearNewNodes();

          // Empty wave ?
          if ( FrcSelect.size() == 0 )
          {
            // Clear duplicit MIDs
            clearDuplicitMID( autonetworkResult );
            // Increase empty wave counter
            antwProcessParams.countEmpty++;
            // Send result
            antwProcessParams.waveStateCode = TWaveStateCode::waveFinished;
            if ( sendWaveResult( autonetworkResult ) == true )
              break;
            else
              continue;
          }

          // Abort the autonetwork process when requested number of nodes (total/new) is found
          if ( antwInputParams.stopConditions.abortOnTooManyNodesFound == true )
          {
            // Check number of total nodes
            if ( ( antwInputParams.stopConditions.numberOfTotalNodes != 0 ) && ( antwProcessParams.bondedNodesNr + FrcSelect.size() > antwInputParams.stopConditions.numberOfTotalNodes ) )
            {
              TRC_INFORMATION( "Too many nodes found - Autonetwork process aborted." );
              antwProcessParams.waveStateCode = TWaveStateCode::abortOnTooManyNodesFound;
              sendWaveResult( autonetworkResult );
              break;
            }

            // Check number of new nodes
            if ( ( antwInputParams.stopConditions.numberOfNewNodes != 0 ) && ( antwProcessParams.countNewNodes + FrcSelect.size() > antwInputParams.stopConditions.numberOfNewNodes ) )
            {
              TRC_INFORMATION( "Too many nodes found - Autonetwork process aborted." );
              antwProcessParams.waveStateCode = TWaveStateCode::abortOnTooManyNodesFound;
              sendWaveResult( autonetworkResult );
              break;
            }
          }

          // ToDo
          std::this_thread::sleep_for( std::chrono::milliseconds( TIMEOUT_STEP ) );
         
          // DPA >= 4.14 ?
          if ( ( coordParams.dpaVerWord >= 0x0414 ) && ( FrcSelect.size() > 1 ) )
          {
            // Check the DPA version of prebonded nodes is >= 0x0414
            TRC_INFORMATION( "Reading prebonded alive nodes." );
            antwProcessParams.waveStateCode = TWaveStateCode::readingDPAVersion;
            sendWaveState();
            std::basic_string<uint8_t> frcData = FrcPrebondedMemoryCompare2B( autonetworkResult, FrcSelect, nodeSeed, 0x0414, 0x04a0, PNUM_ENUMERATION, CMD_GET_PER_INFO );
            for ( uint8_t node : FrcSelect )
            {
              // The condition was met ?
              if ( ( ( frcData[node / 8] & ( 1 << ( node % 8 ) ) ) == 0 ) && ( ( frcData[32 + node / 8] & ( 1 << ( node % 8 ) ) ) != 0 ) )
                FrcSupportMultipleAuth.push_back( node );
            }
            // ToDo
            std::this_thread::sleep_for( std::chrono::milliseconds( TIMEOUT_STEP ) );
          }

          // Read MIDs of prebonded alive nodes
          TRC_INFORMATION( NAME_PAR( Prebonded alive nodes, FrcSelect.size() ) );
          antwProcessParams.waveStateCode = TWaveStateCode::readPrebondedMID;
          sendWaveState();
          uint8_t offset = 0x00;
          maxStep = 0x00;
          stepBreak = false;
          do
          {
            // Prebonded memory read plus 1 - read MIDs
            std::basic_string<uint8_t> prebondedMemoryData = FrcPrebondedMemoryRead4BPlus1( autonetworkResult, FrcSelect, nodeSeed, offset, 0x04a0, PNUM_OS, CMD_OS_READ );
            uint8_t i = 0;
            do
            {
              // TPrebondedNode structure
              TPrebondedNode node;
              node.authorize = false;
              node.authorizeErr = TAuthorizeErr::eNo;
              node.node = FrcSelect[( i / 4 ) + offset];
              node.supportMultipleAuth = std::find( FrcSupportMultipleAuth.begin(), FrcSupportMultipleAuth.end(), node.node ) != FrcSupportMultipleAuth.end();
              node.mid.bytes[0] = prebondedMemoryData[i];
              node.mid.bytes[1] = prebondedMemoryData[i + 1];
              node.mid.bytes[2] = prebondedMemoryData[i + 2];
              node.mid.bytes[3] = prebondedMemoryData[i + 3];
              // Node responded to prebondedMemoryRead plus 1 ?
              if ( node.mid.value != 0 )
              {
                // Yes, decrease MID
                node.mid.value--;

                // Check duplicit MID in antwProcessParams.prebondedNodes
                if ( antwProcessParams.prebondedNodes.size() != 0 )
                {
                  // Compare current node MID with all other prebonded nodes MID
                  for ( auto n : antwProcessParams.prebondedNodes )
                  {
                    // Duplicit MID ?
                    if ( ( n.second.mid.value == node.mid.value ) && ( n.second.authorize == true ) )
                    {
                      // Duplicit nodes must not be authorized
                      TRC_WARNING( "Reading prebonded MID: Duplicit MID " << PAR( node.mid.value ) << " detected." );
                      node.authorizeErr = TAuthorizeErr::eFRC;
                      n.second.authorizeErr = TAuthorizeErr::eFRC;
                      n.second.authorize = false;
                      maxStep--;
                      break;
                    }
                  }
                }

                // HWPID filtering active ?
                if ( ( antwInputParams.hwpidFiltering.empty() == true ) && ( node.authorizeErr == TAuthorizeErr::eNo ) )
                {
                  // Authorize control
                  node.authorize = authorizeControl( node.mid.value, 0, node.addrBond, node.authorizeErr );
                  if ( node.authorize == true )
                  {
                    maxStep++;
                    if ( maxStep + antwProcessParams.bondedNodesNr >= MAX_ADDRESS )
                      stepBreak = true;

                    // Check number of total nodes
                    if ( ( antwInputParams.stopConditions.numberOfTotalNodes != 0 ) && ( maxStep + antwProcessParams.bondedNodesNr >= antwInputParams.stopConditions.numberOfTotalNodes ) )
                      stepBreak = true;

                    // Check number of new nodes
                    if ( ( antwInputParams.stopConditions.numberOfNewNodes != 0 ) && ( maxStep + antwProcessParams.countNewNodes >= antwInputParams.stopConditions.numberOfNewNodes ) )
                      stepBreak = true;
                  }
                }
              }
              else
              {
                // Node didn't respond to prebondedMemoryRead plus 1 
                node.authorizeErr = TAuthorizeErr::eFRC;
                TRC_WARNING( "Reading prebonded MID: Node " << PAR( (int)node.node ) << " doesn't respond to FRC." );
              }

              //  Add (or set) node to prebondedNodes map
              antwProcessParams.prebondedNodes[node.node] = node;

              // Next Node
              i += sizeof( TMID );
            } while ( ( i < 60 ) && ( FrcSelect.size() > antwProcessParams.prebondedNodes.size() ) && ( stepBreak == false ) );
            offset += 15;
          } while ( FrcSelect.size() > antwProcessParams.prebondedNodes.size() && ( stepBreak == false ) );

          // ToDo
          std::this_thread::sleep_for( std::chrono::milliseconds( TIMEOUT_STEP ) );

          // Read HWPID of prebonded alive nodes if HWP filtering requested
          if ( antwInputParams.hwpidFiltering.empty() == false )
          {
            // Check prebonded nodes
            for ( auto n : antwProcessParams.prebondedNodes )
            {
              // MID reading error (or duplicit MID) ?
              if ( n.second.authorizeErr == TAuthorizeErr::eFRC )
              {
                // Yes, remove node from FrcSelect vector - don't read HWPID
                auto nFrcSelect( std::find( FrcSelect.begin(), FrcSelect.end(), n.first ) );
                if ( nFrcSelect != FrcSelect.end() )
                  FrcSelect.erase( nFrcSelect );
              }
            }
            // Read HWPID of prebonded alive nodes
            TRC_INFORMATION( "Reading HWPID of prebonded alive nodes." );
            antwProcessParams.waveStateCode = TWaveStateCode::readPrebondedHWPID;
            sendWaveState();
            offset = 0x00;
            uint8_t prebondedNodesCount = 0;
            maxStep = 0x00;
            stepBreak = false;
            do
            {
              // Prebonded memory read plus 1 - read HWPID and HWPVer
              std::basic_string<uint8_t> prebondedMemoryData = FrcPrebondedMemoryRead4BPlus1( autonetworkResult, FrcSelect, nodeSeed, offset, 0x04a7, PNUM_ENUMERATION, CMD_GET_PER_INFO );
              uint8_t i = 0;
              do
              {
                uint8_t addr = FrcSelect[( i / 4 ) + offset];
                auto n = antwProcessParams.prebondedNodes.find( addr );
                if ( n != antwProcessParams.prebondedNodes.end() )
                {
                  // Node responded to prebondedMemoryRead plus 1 ?
                  TPrebondedNode node = n->second;
                  node.authorize = false;
                  uint32_t HWPID_HWPVer = prebondedMemoryData[i];
                  HWPID_HWPVer |= ( prebondedMemoryData[i + 1] << 8 );
                  HWPID_HWPVer |= ( prebondedMemoryData[i + 2] << 16 );
                  HWPID_HWPVer |= ( prebondedMemoryData[i + 3] << 24 );
                  if ( HWPID_HWPVer != 0 )
                  {
                    // Yes, decrease HWPID_HWPVer
                    HWPID_HWPVer--;
                    node.HWPID = HWPID_HWPVer & 0xffff;
                    node.HWPIDVer = HWPID_HWPVer >> 16;
                    // Authorize control
                    node.authorize = authorizeControl( node.mid.value, node.HWPID, node.addrBond, node.authorizeErr );
                    if ( node.authorize == true )
                    {
                      maxStep++;
                      if ( maxStep + antwProcessParams.bondedNodesNr >= MAX_ADDRESS )
                        stepBreak = true;

                      // Check number of total nodes
                      if ( ( antwInputParams.stopConditions.numberOfTotalNodes != 0 ) && ( maxStep + antwProcessParams.bondedNodesNr >= antwInputParams.stopConditions.numberOfTotalNodes ) )
                        stepBreak = true;

                      // Check number of new nodes
                      if ( ( antwInputParams.stopConditions.numberOfNewNodes != 0 ) && ( maxStep + antwProcessParams.countNewNodes >= antwInputParams.stopConditions.numberOfNewNodes ) )
                        stepBreak = true;
                    }
                  }
                  else
                  {
                    // Node didn't respond to prebondedMemoryRead plus 1 
                    node.authorizeErr = TAuthorizeErr::eFRC;
                    TRC_WARNING( "Reading prebonded HWPID: Node " << PAR( (int)node.node ) << " doesn't respond to FRC." );
                  }

                  // Add (or set) node to prebondedNodes map
                  antwProcessParams.prebondedNodes[node.node] = node;
                }

                // Next Node
                i += 2 * sizeof( uint16_t );
              } while ( ( i < 60 ) && ( FrcSelect.size() > ++prebondedNodesCount ) && ( stepBreak == false ) );
              offset += 15;
            } while ( ( FrcSelect.size() > prebondedNodesCount ) && ( stepBreak == false ) );
          }

          // ToDo
          std::this_thread::sleep_for( std::chrono::milliseconds( TIMEOUT_STEP ) );

          // Authorize prebonded alive nodes
          FrcSelect.clear();
          step = 0x00;
          TRC_INFORMATION( "Authorizing prebonded alive nodes." );
          antwProcessParams.waveStateCode = TWaveStateCode::authorize;
          sendWaveState();

          // Multiple authorization for DPA >= 4.14 ?
          if ( coordParams.dpaVerWord >= 0x0414 )
          {
            std::basic_string<TPrebondedNode> authrozireNodes;
            authrozireNodes.clear();
            uint8_t index = 0;
            for ( std::pair<uint8_t, TPrebondedNode> node : antwProcessParams.prebondedNodes )
            {
              if ( antwProcessParams.bondedNodesNr + antwProcessParams.countWaveNewNodes >= MAX_ADDRESS )
              {
                TRC_INFORMATION( "All available network addresses are already allocated." );
                break;
              }

              // Node passed the authorizeControl (is intended to authorize) and supports multiple authorization ?
              if ( ( node.second.authorize == true ) && ( node.second.supportMultipleAuth == true ) )
              {
                // Yes, add node to multiple authorization list
                authrozireNodes.push_back( node.second );
                // Add authorized node to FrcSelect
                FrcSelect.push_back( node.second.addrBond );
                // Actualize networkNodes 
                antwProcessParams.networkNodes[node.second.addrBond].bonded = true;
                antwProcessParams.networkNodes[node.second.addrBond].discovered = false;
                antwProcessParams.networkNodes[node.second.addrBond].mid.value = node.second.mid.value;
                antwProcessParams.networkNodes[node.second.addrBond].HWPID = node.second.HWPID;
                antwProcessParams.networkNodes[node.second.addrBond].HWPIDVer = node.second.HWPIDVer;
                antwProcessParams.countWaveNewNodes++;
                antwProcessParams.countNewNodes++;
                // Increase number of authorized nodes
                step++;
              }

              // Multiple authorization    
              index++;
              if ( ( authrozireNodes.size() == 11 ) || ( step >= maxStep ) || ( index == antwProcessParams.prebondedNodes.size() ) )
              {
                // Any nodes in the list ?
                if ( authrozireNodes.size() != 0 )
                {
                  retryAction = antwInputParams.actionRetries + 1;
                  do
                  {
                    try
                    {
                      authorizeBond( autonetworkResult, authrozireNodes );
                      authrozireNodes.clear();
                      break;
                    }
                    catch ( std::exception& ex )
                    {
                      TRC_WARNING( "Authorizing node " << PAR( node.second.mid.value ) << " error: " << ex.what() );
                    }
                  } while ( --retryAction != 0 );
                }
              }

              // Check number of authorized nodes
              if ( step >= maxStep )
                break;
            }
          }
         
          // Single authorization for [N] with DPA < 0x0414
          for ( std::pair<uint8_t, TPrebondedNode> node : antwProcessParams.prebondedNodes )
          {
            if ( antwProcessParams.bondedNodesNr + antwProcessParams.countWaveNewNodes >= MAX_ADDRESS )
            {
              TRC_INFORMATION( "All available network addresses are already allocated." );
              break;
            }

            // Check number of authorized nodes
            if ( step >= maxStep )
              break;

            // Node passed the authorizeControl (is intended to authorize) ?
            if ( ( node.second.authorize == true ) && ( node.second.supportMultipleAuth == false ) )
            {
              // Yes, authorize the node
              retryAction = antwInputParams.actionRetries + 1;
              do
              {
                try
                {
                  // Authorize nodes with DPA < 4.14 one by one
                  std::basic_string<TPrebondedNode> authrozireNodes;
                  authrozireNodes.clear();
                  authrozireNodes.push_back( node.second );
                  TPerCoordinatorAuthorizeBond_Response response = authorizeBond( autonetworkResult, authrozireNodes );
                  // Add authorized node to FrcSelect
                  FrcSelect.push_back( response.BondAddr );
                  // Actualize networkNodes 
                  antwProcessParams.networkNodes[response.BondAddr].bonded = true;
                  antwProcessParams.networkNodes[response.BondAddr].discovered = false;
                  antwProcessParams.networkNodes[response.BondAddr].mid.value = node.second.mid.value;
                  antwProcessParams.networkNodes[response.BondAddr].HWPID = node.second.HWPID;
                  antwProcessParams.networkNodes[response.BondAddr].HWPIDVer = node.second.HWPIDVer;
                  antwProcessParams.countWaveNewNodes++;
                  antwProcessParams.countNewNodes++;
                  // Increase number of authorized nodes
                  step++;
                  break;
                }
                catch ( std::exception& ex )
                {
                  TRC_WARNING( "Authorizing node " << PAR( node.second.mid.value ) << " error: " << ex.what() );
                }
              } while ( --retryAction != 0 );
            }
          }

          // ToDo
          std::this_thread::sleep_for( std::chrono::milliseconds( TIMEOUT_STEP ) );

          // TestCase - overit chovani clearDuplicitMID
          if ( FrcSelect.size() == 0 )
          {
            try
            {
              clearDuplicitMID( autonetworkResult );
            }
            catch ( std::exception& ex )
            {
              TRC_WARNING( "Clear Duplicit MID error: " << ex.what() );
            }
            antwProcessParams.countEmpty++;
            // Send result
            antwProcessParams.waveStateCode = TWaveStateCode::waveFinished;
            if ( sendWaveResult( autonetworkResult ) == true )
              break;
            else
              continue;
          }
          else
            antwProcessParams.countEmpty = 0;

          // Ping nodes
          // TestCase - v prubehu ping odpojit uspesne autorizovany [N], overit, ze se odbonduje
          // TestCase - behem jednotlivych cyklu ping [N] odpojovat/zapojovat, overit, jak probehne odbondovani
          TRC_INFORMATION( "Pinging nodes." );
          antwProcessParams.waveStateCode = TWaveStateCode::ping;
          sendWaveState();
          FrcOnlineNodes.clear();
          retryAction = antwInputParams.actionRetries + 1;
          while ( ( FrcSelect.size() != 0 ) && ( retryAction-- != 0 ) )
          {
            try
            {
              // Add dealy at next retries
              if ( retryAction != antwInputParams.actionRetries )
              {
                // ToDo
                std::this_thread::sleep_for( std::chrono::milliseconds( TIMEOUT_REPEAT ) );
              }

              // FRC_Ping
              TPerFrcSend_Response response = FrcPingNodes( autonetworkResult );
              // Clear FrcPingOfflineNodes
              FrcOfflineNodes.clear();
              // Check the response             
              for ( uint8_t address = 1; address <= MAX_ADDRESS; address++ )
              {
                // Node bonded ?
                if ( antwProcessParams.networkNodes[address].bonded == true )
                {
                  // Node acknowledged to FRC_Ping (Bit0 is set) ?
                  bool nodeOnline = ( response.FrcData[address / 8] & (uint8_t)( 1 << ( address % 8 ) ) ) != 0;
                  antwProcessParams.networkNodes[address].online = nodeOnline;
                  // Is node in authorized nodes list 
                  auto node = std::find( FrcSelect.begin(), FrcSelect.end(), address );
                  bool nodeInAuthList = node != FrcSelect.end();
                  // Is node in online nodes list 
                  bool nodeInOnlineList = std::find( FrcOnlineNodes.begin(), FrcOnlineNodes.end(), address ) != FrcOnlineNodes.end();
                  // Node is online and is in authorized nodes list ?
                  if ( ( nodeOnline == true ) && ( nodeInAuthList == true ) )
                  {
                    // Remove the node from FrcSelect (authorized nodes list)
                    FrcSelect.erase( node );
                    // Add node to FrcOnlineNodes list
                    FrcOnlineNodes.push_back( address );
                    antwProcessParams.respondedNewNodes.push_back( { address, antwProcessParams.networkNodes[address].mid.value } );
                  }
                  else
                  {
                    // Add offline nodes to FrcOfflineNodes list
                    if ( ( nodeOnline == false ) && ( nodeInAuthList == false ) && ( nodeInOnlineList == false ) )
                    {
                      // Add node to FrcOnlineNodes list
                      FrcOfflineNodes.push_back( address );
                      TRC_WARNING( "FRC_Ping: Node " << PAR( (int)address ) << " is offline." );
                    }
                    else
                    {
                      // If node is online a subsequent FRC, mark it as online
                      antwProcessParams.networkNodes[address].online = true;
                    }
                  }
                }
              }
            }
            catch ( std::exception& ex )
            {
              TRC_WARNING( "FRC_Ping: error: " << ex.what() );
            }
          }

          // ToDo
          std::this_thread::sleep_for( std::chrono::milliseconds( TIMEOUT_STEP ) );

          // Remove not responded nodes
          retryAction = antwInputParams.actionRetries + 1;
          while ( ( FrcSelect.size() != 0 ) && ( retryAction-- != 0 ) )
          {
            // Add dealy at next retries
            if ( retryAction != antwInputParams.actionRetries )
            {
              // ToDo
              std::this_thread::sleep_for( std::chrono::milliseconds( TIMEOUT_REPEAT ) );
            }

            TRC_INFORMATION( "Removing not responded nodes." );
            antwProcessParams.waveStateCode = TWaveStateCode::removeNotResponded;
            sendWaveState();
            // FRC_AcknowledgedBroadcastBits remove bond (for DPA < 0x0400 - send Batch command Remove bond + Restart)
            TPerFrcSend_Response response = removeNotRespondedNewNodes( autonetworkResult, FrcSelect );
            // Check the nodes contained in FrcSelect list acknowledged FRC_AcknowledgedBroadcastBits
            for ( uint8_t address = 1; address < MAX_ADDRESS; address++ )
            {
              auto node = std::find( FrcSelect.begin(), FrcSelect.end(), address );
              if ( node != FrcSelect.end() )
              {
                // Bit0 is set ?
                if ( ( response.FrcData[address / 8] & (uint8_t)( 1 << ( address % 8 ) ) ) != 0 )
                {
                  // Yes, remove node at [C] side too
                  try
                  {
                    removeBondAtCoordinator( autonetworkResult, address );
                    // Remove the node from FrcSelect
                    FrcSelect.erase( node );
                    // Actualize networkNodes 
                    antwProcessParams.networkNodes[address].bonded = false;
                    antwProcessParams.networkNodes[address].discovered = false;
                    antwProcessParams.networkNodes[address].mid.value = 0;
                    antwProcessParams.countWaveNewNodes--;
                    antwProcessParams.countNewNodes--;
                    TRC_INFORMATION( "Removing Node " << PAR( (int)address ) << " at [C]." );
                  }
                  catch ( std::exception& ex )
                  {
                    TRC_WARNING( "Removing the bond " << PAR( (int)address ) << " at [C] error: " << ex.what() );
                  }
                }
                else
                {
                  TRC_WARNING( "Remove not responded Nodes: Node " << PAR( (int)address ) << " doesn't respond to FRC." );
                }
              }
            }
          }

          // ToDo
          std::this_thread::sleep_for( std::chrono::milliseconds( TIMEOUT_STEP ) );

          // Unbond node at coordinator only ?
          // TestCase - overit odbondovani na strane [C] (zamerne nasimulovat FrcSelect.size() != 0)
          if ( FrcSelect.size() != 0 )
          {
            TRC_INFORMATION( "Unbonding Nodes only at Coordinator." );
            for ( uint8_t address = 1; address < MAX_ADDRESS; address++ )
            {
              auto node = std::find( FrcSelect.begin(), FrcSelect.end(), address );
              if ( node != FrcSelect.end() )
              {
                // Insert duplicit node to duplicitMID 
                if ( std::find( antwProcessParams.duplicitMID.begin(), antwProcessParams.duplicitMID.end(), address ) != antwProcessParams.duplicitMID.end() )
                  antwProcessParams.duplicitMID.push_back( address );
                try
                {
                  // Remove node at [C] side only
                  removeBondAtCoordinator( autonetworkResult, address );
                  // Remove the node from FrcSelect
                  FrcSelect.erase( node );
                  // Actualize networkNodes 
                  antwProcessParams.networkNodes[address].bonded = false;
                  antwProcessParams.networkNodes[address].discovered = false;
                  antwProcessParams.networkNodes[address].mid.value = 0;
                  antwProcessParams.countWaveNewNodes--;
                  antwProcessParams.countNewNodes--;
                }
                catch ( std::exception& ex )
                {
                  TRC_WARNING( "Removing the bond " << PAR( (int)address ) << " at [C] error: " << ex.what() );
                }
              }
            }
          }

          // ToDo
          std::this_thread::sleep_for( std::chrono::milliseconds( TIMEOUT_STEP ) );

          // Clear duplicit MIDs
          // TestCase - overit chovani clearDuplicitMID
          clearDuplicitMID( autonetworkResult );

          // Skip discovery in each wave ?
          if ( antwInputParams.skipDiscoveryEachWave == false )
          {
            if ( antwProcessParams.countWaveNewNodes != 0 )
            {
              retryAction = antwInputParams.actionRetries + 1;
              do
              {
                TRC_INFORMATION( "Running discovery." );
                antwProcessParams.waveStateCode = TWaveStateCode::discovery;
                sendWaveState();
                try
                {
                  uint8_t discNodes = runDiscovery( autonetworkResult, antwInputParams.discoveryTxPower );
                  if ( countDiscNodes <= discNodes )
                    break;
                }
                catch ( std::exception& ex )
                {
                  TRC_WARNING( "Discovery failed: " << ex.what() );
                }
              } while ( --retryAction != 0 );

              updateNetworkInfo( autonetworkResult );
              countDiscNodes = antwProcessParams.discoveredNodesNr;
            }
          }
          else
            updateNetworkInfo( autonetworkResult );

          // New nodes bonded ?
          if ( antwProcessParams.countWaveNewNodes != 0 )
          {
            // Copy new nodes to response
            for ( auto node : antwProcessParams.respondedNewNodes )
            {
              autonetworkResult.putNewNode( node.address, node.MID );
              // HWPID filtering requested ?
              if ( antwInputParams.hwpidFiltering.empty() == true )
              {
                // No, pass only MIDs
                newNodes.insert( std::make_pair( node.address, embed::node::BriefInfo( node.MID ) ) );
              }
              else
              {
                // Yes, pass MID, HWPID and HWPID version
                newNodes.insert( std::make_pair( node.address, embed::node::BriefInfo( node.MID, antwProcessParams.networkNodes[node.address].HWPID, antwProcessParams.networkNodes[node.address].HWPIDVer ) ) );
              }
            }
          }

          // Send result
          antwProcessParams.waveStateCode = TWaveStateCode::waveFinished;
          if ( sendWaveResult( autonetworkResult ) == true )
            break;
        }
      }
      catch ( std::exception& ex )
      {
        TRC_WARNING( "Error during algorithm run: " << ex.what() );
        // Send result
        sendWaveResult( autonetworkResult );
      }

      // Unbond temporary address, set initial FRC param, DPA param and DPA Hops param
      try
      {
        // Unbond temporary address
        unbondTemporaryAddress( autonetworkResult );
        // Set initial FRC param
        if ( antwProcessParams.FrcResponseTime != 0 )
          antwProcessParams.FrcResponseTime = setFrcReponseTime( autonetworkResult, antwProcessParams.FrcResponseTime );
        // Set initial DPA param
        if ( antwProcessParams.DpaParam != 0 )
          antwProcessParams.DpaParam = setNoLedAndOptimalTimeslot( autonetworkResult, antwProcessParams.DpaParam );
        // Set initial DPA Hops param
        if ( ( antwProcessParams.RequestHops != 0xff ) || ( antwProcessParams.ResponseHops != 0xff ) )
          setDpaHopsToTheNumberOfRouters( autonetworkResult, antwProcessParams.RequestHops, antwProcessParams.ResponseHops );
      }
      catch ( std::exception& ex )
      {
        // Set error
        TRC_WARNING( "Error during algorithm run: " << ex.what() );
      }

      // SQLDB add nodes
      try
      {
        // SQLDB - predat MID, pokud je aktivni filtrovani HWPID, predat take HWPID a HWPIDVer
        m_iIqrfInfo->insertNodes( newNodes );
      }
      catch ( std::exception& ex )
      {
        TRC_ERROR( "Error inserting nodes to DB: " << ex.what() );
      }

      TRC_FUNCTION_LEAVE( "" );
    }

    // Process request
    void handleMsg( const std::string& messagingId, const IMessagingSplitterService::MsgType& msgType, rapidjson::Document doc )
    {
      TRC_FUNCTION_ENTER( PAR( messagingId ) << NAME_PAR( mType, msgType.m_type ) << NAME_PAR( major, msgType.m_major ) << NAME_PAR( minor, msgType.m_minor ) << NAME_PAR( micro, msgType.m_micro ) );

      // Unsupported type of request
      if ( msgType.m_type != m_mTypeName_Autonetwork )
        THROW_EXC( std::logic_error, "Unsupported message type: " << PAR( msgType.m_type ) );

      // Create representation object
      ComAutonetwork comAutonetwork( doc );

      // Parsing and checking service parameters
      try
      {
        antwInputParams = comAutonetwork.getAutonetworkParams();
      }
      catch ( std::exception& e )
      {
        const char* errorStr = e.what();
        TRC_WARNING( "Error while parsing service input parameters: " << PAR( errorStr ) );
        // Create error response
        Document response;
        Pointer( "/mType" ).Set( response, msgType.m_type );
        Pointer( "/data/msgId" ).Set( response, comAutonetwork.getMsgId() );
        // Set result
        Pointer( "/data/status" ).Set( response, SERVICE_ERROR );
        Pointer( "/data/statusStr" ).Set( response, errorStr );
        m_iMessagingSplitterService->sendMessage( messagingId, std::move( response ) );

        TRC_FUNCTION_LEAVE( "" );
        return;
      }

      // Try to establish exclusive access
      try
      {
        m_exclusiveAccess = m_iIqrfDpaService->getExclusiveAccess();
      }
      catch ( std::exception &e )
      {
        const char* errorStr = e.what();
        TRC_WARNING( "Error while establishing exclusive DPA access: " << PAR( errorStr ) );
        // Create error response
        Document response;
        Pointer( "/mType" ).Set( response, msgType.m_type );
        Pointer( "/data/msgId" ).Set( response, comAutonetwork.getMsgId() );
        // Set result
        Pointer( "/data/status" ).Set( response, SERVICE_ERROR );
        Pointer( "/data/statusStr" ).Set( response, errorStr );
        m_iMessagingSplitterService->sendMessage( messagingId, std::move( response ) );

        TRC_FUNCTION_LEAVE( "" );
        return;
      }
       
      // Run autonetwork
      m_msgType = &msgType;
      m_messagingId = &messagingId;
      m_comAutonetwork = &comAutonetwork;
      runAutonetwork();
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

      (void)props;

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

    void attachInterface(IIqrfInfo* iface)
    {
      m_iIqrfInfo = iface;
    }

    void detachInterface(IIqrfInfo* iface)
    {
      if (m_iIqrfInfo == iface) {
        m_iIqrfInfo = nullptr;
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

  void AutonetworkService::attachInterface(IIqrfInfo* iface)
  {
    m_imp->attachInterface(iface);
  }

  void AutonetworkService::detachInterface(IIqrfInfo* iface)
  {
    m_imp->detachInterface(iface);
  }

  void AutonetworkService::attachInterface( IIqrfDpaService* iface )
  {
    m_imp->attachInterface( iface );
  }

  void AutonetworkService::detachInterface( IIqrfDpaService* iface )
  {
    m_imp->detachInterface( iface );
  }

  void AutonetworkService::attachInterface( IMessagingSplitterService* iface )
  {
    m_imp->attachInterface( iface );
  }

  void AutonetworkService::detachInterface( IMessagingSplitterService* iface )
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

