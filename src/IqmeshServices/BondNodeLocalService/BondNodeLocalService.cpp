#define IBondNodeLocalService_EXPORTS

#include "BondNodeLocalService.h"
#include "RawDpaEmbedOS.h"
#include "Trace.h"
#include "ComIqmeshNetworkBondNodeLocal.h"
#include "iqrf__BondNodeLocalService.hxx"
#include <list>
#include <cmath>
#include <thread> 

TRC_INIT_MODULE(iqrf::BondNodeLocalService);

using namespace rapidjson;

namespace {

  // maximum number of repeats
  static const uint8_t REPEAT_MAX = 3;

  // Default bonding mask. No masking effect.
  static const uint8_t DEFAULT_BONDING_MASK = 0;

  // values of result error codes
  // service general fail code - may and probably will be changed later in the future
  static const int SERVICE_ERROR = 1000;

  static const int SERVICE_ERROR_INTERNAL = SERVICE_ERROR + 1;

  static const int SERVICE_ERROR_GET_BONDED_NODES = SERVICE_ERROR + 2;
  static const int SERVICE_ERROR_ALREADY_BONDED = SERVICE_ERROR + 3;
  static const int SERVICE_ERROR_NO_FREE_SPACE = SERVICE_ERROR + 4;
  static const int SERVICE_ERROR_BOND_FAILED = SERVICE_ERROR + 5;
  static const int SERVICE_ERROR_ABOVE_ADDRESS_LIMIT = SERVICE_ERROR + 6;
  static const int SERVICE_ERROR_PING_FAILED = SERVICE_ERROR + 7;
  static const int SERVICE_ERROR_PING_INTERNAL_ERROR = SERVICE_ERROR + 8;
  static const int SERVICE_ERROR_HWPID_VERSION = SERVICE_ERROR + 9;
};


namespace iqrf {

  // Holds information about errors, which encounter during local bond service run
  class BondError {
  public:
    // Type of error
    enum class Type {
      NoError,
      GetBondedNodes,
      AlreadyBonded,
      NoFreeSpace,
      BondError,
      AboveAddressLimit,
      PingFailed,
      InternalError,
      HwpIdVersion
    };

    BondError() : m_type( Type::NoError ), m_message( "" ) {};
    BondError( Type errorType ) : m_type( errorType ), m_message( "" ) {};
    BondError( Type errorType, const std::string& message ) : m_type( errorType ), m_message( message ) {};

    Type getType() const { return m_type; };
    std::string getMessage() const { return m_message; };

    BondError& operator=( const BondError& error ) {
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
  class BondResult {
  private:
    BondError m_error;
    uint8_t m_bondedAddr;
    uint8_t m_bondedNodesNum;
    uint16_t m_bondedNodeHwpId;
    uint16_t m_bondedNodeHwpIdVer;
    
    embed::os::RawDpaReadPtr m_osRead;
    uint16_t m_osBuild;

    std::string m_manufacturer = "";
    std::string m_product = "";
    std::list<std::string> m_standards = { "" };

    // transaction results
    std::list<std::unique_ptr<IDpaTransactionResult2>> m_transResults;

  public:
    BondError getError() const { return m_error; };

    void setError( const BondError& error ) {
      m_error = error;
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
      m_bondedNodeHwpId = hwpId;
    }

    // returns HwpId of bonded node
    uint16_t getBondedNodeHwpId() const {
      return m_bondedNodeHwpId;
    }

    // sets HwpId version of bonded node
    void setBondedNodeHwpIdVer(const uint16_t hwpIdVer) {
      m_bondedNodeHwpIdVer = hwpIdVer;
    }

    // returns HwpId version of bonded node
    uint16_t getBondedNodeHwpIdVer() const {
      return m_bondedNodeHwpIdVer;
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

    void setStandards(std::list<std::string> standards) {
      m_standards = standards;
    }


    // adds transaction result into the list of results
    void addTransactionResult( std::unique_ptr<IDpaTransactionResult2> transResult ) {
      if( transResult != nullptr )
        m_transResults.push_back( std::move( transResult ) );
    }

    // adds transaction result into the list of results
    void addTransactionResultRef(std::unique_ptr<IDpaTransactionResult2> &transResult)
    {
      if( transResult != nullptr )
        m_transResults.push_back(std::move(transResult));
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
  class BondNodeLocalService::Imp {
  private:
    // parent object
    BondNodeLocalService & m_parent;

    // message type: network management bond node local
    // for temporal reasons
    const std::string m_mTypeName_iqmeshNetworkBondNodeLocal = "iqmeshNetwork_BondNodeLocal";

    iqrf::IJsCacheService* m_iJsCacheService = nullptr;
    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    IIqrfDpaService* m_iIqrfDpaService = nullptr;
    std::unique_ptr<IIqrfDpaService::ExclusiveAccess> m_exclusiveAccess;

    // number of repeats
    uint8_t m_repeat = 1;

    // if is set Verbose mode
    bool m_returnVerbose = false;


  public:
    Imp( BondNodeLocalService& parent ) : m_parent( parent )
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

    // tries to bonds node and returns result
    void _bondNode( BondResult& bondResult, const uint8_t nodeAddr, const uint8_t bondingMask, const uint8_t bondingTestRetries )
    {
      TRC_FUNCTION_ENTER( "" );

      DpaMessage bondNodeRequest;
      DpaMessage::DpaPacket_t bondNodePacket;
      bondNodePacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      bondNodePacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
      bondNodePacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_BOND_NODE;
      bondNodePacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
      bondNodePacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorBondNode_Request.ReqAddr = uint8_t( nodeAddr & 0xFF );
      // Put bondingTestRetries for DPA >= 4.00, bondingMask for DPA < 4.00
      if ( m_iIqrfDpaService->getCoordinatorParameters().dpaVerWord >= 0x400 )
        bondNodePacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorBondNode_Request.BondNode.Current.BondingTestRetries = bondingTestRetries;
      else
        bondNodePacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorBondNode_Request.BondNode.Previous.BondingMask = bondingMask;
      bondNodeRequest.DataToBuffer( bondNodePacket.Buffer, sizeof( TDpaIFaceHeader ) + 2 );

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> bondNodeTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      try {
        //bondNodeTransaction = m_iIqrfDpaService->executeDpaTransaction( bondNodeRequest );
        bondNodeTransaction = m_exclusiveAccess->executeDpaTransaction(bondNodeRequest);
        transResult = bondNodeTransaction->get();
      }
      catch ( std::exception& e ) {
        TRC_WARNING( "DPA transaction error : " << e.what() );

        BondError error( BondError::Type::BondError, e.what() );
        bondResult.setError( error );

        TRC_FUNCTION_LEAVE( "" );
        return;
      }

      TRC_DEBUG( "Result from bond node transaction as string:" << PAR( transResult->getErrorString() ) );

      IDpaTransactionResult2::ErrorCode errorCode = ( IDpaTransactionResult2::ErrorCode )transResult->getErrorCode();

      // because of the move-semantics
      DpaMessage dpaResponse = transResult->getResponse();
      bondResult.addTransactionResultRef( transResult );

      if ( errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK ) {
        TRC_INFORMATION( "Bond node successful!" );
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR( bondNodeRequest.PeripheralType(), bondNodeRequest.NodeAddress() )
          << PAR( bondNodeRequest.PeripheralCommand() )
        );

        // getting bond data
        TPerCoordinatorBondNodeSmartConnect_Response respData
          = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerCoordinatorBondNodeSmartConnect_Response;

        bondResult.setBondedAddr( respData.BondAddr );
        bondResult.setBondedNodesNum( respData.DevNr );

        TRC_FUNCTION_LEAVE( "" );
        return;
      }

      // transaction error
      if ( errorCode < 0 ) {
        TRC_WARNING( "Transaction error. " << NAME_PAR_HEX( "Error code", errorCode ) );

        BondError error( BondError::Type::BondError, "Transaction error." );
        bondResult.setError( error );
      }
      else {
        // DPA error
        TRC_WARNING( "DPA error. " << NAME_PAR_HEX( "Error code", errorCode ) );

        BondError error( BondError::Type::BondError, "Dpa error." );
        bondResult.setError( error );
      }

      TRC_FUNCTION_LEAVE( "" );
      return;
    }

    // pings specified node using OS::Read command
    void pingNode( BondResult& bondResult )
    {
      TRC_FUNCTION_ENTER( "" );

      std::unique_ptr<embed::os::RawDpaRead> osReadPtr( shape_new embed::os::RawDpaRead(bondResult.getBondedAddr()) );
      std::unique_ptr<IDpaTransactionResult2> transResultPtr;

      try
      {
        m_exclusiveAccess->executeDpaTransactionRepeat( osReadPtr->getRequest(), transResultPtr, m_repeat );
        osReadPtr->processDpaTransactionResult( std::move(transResultPtr) );
        TRC_DEBUG("Result from OS read transaction as string:" << PAR( osReadPtr->getResult()->getErrorString()) );
        bondResult.setOsBuild( osReadPtr->getOsBuild() );
        bondResult.setBondedNodeHwpId( osReadPtr->getHwpid() );
        bondResult.addTransactionResult( osReadPtr->getResultMove() );
        bondResult.setOsRead( osReadPtr );
        TRC_INFORMATION( "OS read successful!" );
      }
      catch (std::exception &e) {
        BondError error(BondError::Type::PingFailed, e.what());
        bondResult.setError(error);
      }

      TRC_FUNCTION_LEAVE("");
    }

    // removes specified address from coordinator's list of bonded addresses
    void removeBondedNode( BondResult& bondResult )
    {
      TRC_FUNCTION_ENTER( "" );

      DpaMessage removeBondRequest;
      DpaMessage::DpaPacket_t removeBondPacket;
      removeBondPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      removeBondPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
      removeBondPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_REMOVE_BOND;
      removeBondPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;

      uns8* pData = removeBondPacket.DpaRequestPacket_t.DpaMessage.Request.PData;
      pData[0] = uns8(bondResult.getBondedAddr() & 0xFF);

      removeBondRequest.DataToBuffer( removeBondPacket.Buffer, sizeof( TDpaIFaceHeader ) + 1 );

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> removeBondTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      for ( int rep = 0; rep <= m_repeat; rep++ ) {
        try {
          //removeBondTransaction = m_iIqrfDpaService->executeDpaTransaction( removeBondRequest );
          removeBondTransaction = m_exclusiveAccess->executeDpaTransaction(removeBondRequest);
          transResult = removeBondTransaction->get();
        }
        catch ( std::exception& e ) {
          TRC_WARNING( "DPA transaction error : " << e.what() );
          THROW_EXC( std::logic_error, "Could not remove bond." );
        }

        TRC_DEBUG( "Result from remove bond transaction as string:" << PAR( transResult->getErrorString() ) );

        IDpaTransactionResult2::ErrorCode errorCode = ( IDpaTransactionResult2::ErrorCode )transResult->getErrorCode();

        // because of the move-semantics
        DpaMessage dpaResponse = transResult->getResponse();
        bondResult.addTransactionResultRef( transResult );

        if ( errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK ) {
          TRC_INFORMATION( "Remove node bond done!" );
          TRC_DEBUG(
            "DPA transaction: "
            << NAME_PAR( removeBondRequest.PeripheralType(), removeBondRequest.NodeAddress() )
            << PAR( removeBondRequest.PeripheralCommand() )
          );

          TRC_FUNCTION_LEAVE( "" );
          return;
        }

        // transaction error
        if ( errorCode < 0 ) {
          TRC_WARNING( "Transaction error. " << NAME_PAR_HEX( "Error code", errorCode ) );

          if ( rep < m_repeat ) {
            continue;
          }
        } // DPA error
        else {
          TRC_WARNING( "DPA error. " << NAME_PAR_HEX( "Error code", errorCode ) );

          if ( rep < m_repeat ) {
            continue;
          }
        }
      }

      TRC_FUNCTION_LEAVE( "" );
      return;
    }

    // parses bit array of bonded nodes
    std::list<uint8_t> parseBondedNodes( const unsigned char* pData ) {
      std::list<uint8_t> bondedNodes;

      // maximal bonded node number
      const uint8_t MAX_BONDED_NODE_NUMBER = 0xEF;
      const uint8_t MAX_BYTES_USED = (uint8_t)ceil( MAX_BONDED_NODE_NUMBER / 8.0 );

      for ( int byteId = 0; byteId < 32; byteId++ ) {
        if ( byteId >= MAX_BYTES_USED ) {
          break;
        }

        if ( pData[byteId] == 0 ) {
          continue;
        }

        uint8_t bitComp = 1;
        for ( int bitId = 0; bitId < 8; bitId++ ) {
          if ( ( pData[byteId] & bitComp ) == bitComp ) {
            bondedNodes.push_back( byteId * 8 + bitId );
          }
          bitComp <<= 1;
        }
      }

      return bondedNodes;
    }

    // returns list of bonded nodes
    std::list<uint8_t> getBondedNodes( BondResult& bondResult ) {
      TRC_FUNCTION_ENTER( "" );

      DpaMessage getBondedNodesRequest;
      DpaMessage::DpaPacket_t getBondedNodesPacket;
      getBondedNodesPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      getBondedNodesPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
      getBondedNodesPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_BONDED_DEVICES;
      getBondedNodesPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;

      getBondedNodesRequest.DataToBuffer( getBondedNodesPacket.Buffer, sizeof( TDpaIFaceHeader ) );

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> getBondedNodesTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      for ( int rep = 0; rep <= m_repeat; rep++ ) {
        try {
          //getBondedNodesTransaction = m_iIqrfDpaService->executeDpaTransaction( getBondedNodesRequest );
          getBondedNodesTransaction = m_exclusiveAccess->executeDpaTransaction(getBondedNodesRequest);
          transResult = getBondedNodesTransaction->get();
        }
        catch ( std::exception& e ) {
          TRC_WARNING( "DPA transaction error : " << e.what() );

          if ( rep < m_repeat ) {
            continue;
          }

          BondError error( BondError::Type::GetBondedNodes, e.what() );
          bondResult.setError( error );

          THROW_EXC( std::logic_error, "Could not get bonded nodes." );
        }

        TRC_DEBUG( "Result from get bonded nodes transaction as string:" << PAR( transResult->getErrorString() ) );

        IDpaTransactionResult2::ErrorCode errorCode = ( IDpaTransactionResult2::ErrorCode )transResult->getErrorCode();

        // because of the move-semantics
        DpaMessage dpaResponse = transResult->getResponse();
        bondResult.addTransactionResultRef( transResult );

        if ( errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK ) {
          TRC_INFORMATION( "Get bonded nodes successful!" );
          TRC_DEBUG(
            "DPA transaction: "
            << NAME_PAR( getBondedNodesRequest.PeripheralType(), getBondedNodesRequest.NodeAddress() )
            << PAR( getBondedNodesRequest.PeripheralCommand() )
          );

          // get response data
          const unsigned char* pData = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData;

          TRC_FUNCTION_LEAVE( "" );
          return parseBondedNodes( pData );
        }

        // transaction error
        if ( errorCode < 0 ) {
          TRC_WARNING( "Transaction error. " << NAME_PAR_HEX( "Error code", errorCode ) );

          if ( rep < m_repeat ) {
            continue;
          }

          BondError error( BondError::Type::GetBondedNodes, "Transaction error." );
          bondResult.setError( error );
        }
        else {
          // DPA error
          TRC_WARNING( "DPA error. " << NAME_PAR_HEX( "Error code", errorCode ) );

          if ( rep < m_repeat ) {
            continue;
          }

          BondError error( BondError::Type::GetBondedNodes, "Dpa error." );
          bondResult.setError( error );
        }
      }

      THROW_EXC( std::logic_error, "Could not get bonded nodes." );
    }

    // indicates, whether the specified node is already bonded
    bool isBonded( const std::list<uint8_t>& bondedNodes, const uint8_t nodeAddr )
    {
      std::list<uint8_t>::const_iterator it = std::find( bondedNodes.begin(), bondedNodes.end(), nodeAddr );
      bool result = it != bondedNodes.end();
      std::string n = std::to_string( nodeAddr );
      n += result ? " true" : " false";
      TRC_DEBUG( n );      
      return ( result );
    }

    // gets HWP ID version of specified node address and sets it into the result
    void getHwpIdVersion(BondResult& bondResult, const uint16_t nodeAddr)
    {
      TRC_FUNCTION_ENTER("");

      DpaMessage perEnumRequest;
      DpaMessage::DpaPacket_t perEnumPacket;
      perEnumPacket.DpaRequestPacket_t.NADR = nodeAddr;
      perEnumPacket.DpaRequestPacket_t.PNUM = 0xFF;
      perEnumPacket.DpaRequestPacket_t.PCMD = 0x3F;
      perEnumPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
      perEnumRequest.DataToBuffer(perEnumPacket.Buffer, sizeof(TDpaIFaceHeader));

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> perEnumTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      for (int rep = 0; rep <= m_repeat; rep++) {
        try {
          perEnumTransaction = m_exclusiveAccess->executeDpaTransaction(perEnumRequest);
          transResult = perEnumTransaction->get();
        }
        catch (std::exception& e) {
          TRC_WARNING("DPA transaction error : " << e.what());

          if (rep < m_repeat) {
            continue;
          }

          BondError error(BondError::Type::HwpIdVersion, e.what());
          bondResult.setError(error);

          TRC_FUNCTION_LEAVE("");
          return;
        }

        TRC_DEBUG("Result from smart connect transaction as string:" << PAR(transResult->getErrorString()));

        IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

        // because of the move-semantics
        DpaMessage dpaResponse = transResult->getResponse();
        bondResult.addTransactionResultRef( transResult );

        if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
          TRC_INFORMATION("Device exploration successful!");
          TRC_DEBUG(
            "DPA transaction: "
            << NAME_PAR(perEnumRequest.PeripheralType(), perEnumRequest.NodeAddress())
            << PAR(perEnumRequest.PeripheralCommand())
          );

          // parsing response pdata
          uns8* respData = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData;
          uint8_t minorHwpIdVer = respData[9];
          uint8_t majorHwpIdVer = respData[10];

          bondResult.setBondedNodeHwpIdVer(minorHwpIdVer + (majorHwpIdVer << 8));

          TRC_FUNCTION_LEAVE("");
          return;
        }

        // transaction error
        if (errorCode < 0) {
          TRC_WARNING("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

          if (rep < m_repeat) {
            continue;
          }

          BondError error(BondError::Type::HwpIdVersion, "Transaction error.");
          bondResult.setError(error);

          TRC_FUNCTION_LEAVE("");
          return;
        }

        // DPA error
        TRC_WARNING("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

        if (rep < m_repeat) {
          continue;
        }

        BondError error(BondError::Type::HwpIdVersion, "Dpa error.");
        bondResult.setError(error);

        TRC_FUNCTION_LEAVE("");
      }
    }

    // Bond the requested node
    BondResult bondNode( const uint8_t nodeAddr, const uint8_t bondingMask, const uint8_t bondingTestRetries )
    {
      TRC_FUNCTION_ENTER( "" );

      BondResult bondResult;

      std::list<uint8_t> bondedNodes;
      try {
        // get bonded nodes to check it against address to bond
        bondedNodes = getBondedNodes( bondResult );        
      }
      catch ( std::exception& ex ) {
        TRC_FUNCTION_LEAVE( "" );
        return bondResult;
      }

      // node is already bonded
      if ( isBonded( bondedNodes, nodeAddr ) ) {
        BondError error( BondError::Type::AlreadyBonded, "Address already bonded." );
        bondResult.setError( error );

        TRC_FUNCTION_LEAVE( "" );
        return bondResult;
      }

      // all nodes are already bonded
      if ( bondedNodes.size() >= 239 ) {
        BondError error( BondError::Type::NoFreeSpace, "No free space." );
        bondResult.setError( error );

        TRC_FUNCTION_LEAVE( "" );
        return bondResult;
      }

      // bond a node
      _bondNode( bondResult, nodeAddr, bondingMask, bondingTestRetries );

      // bonding node failed
      if ( bondResult.getError().getType() != BondError::Type::NoError ) {
        return bondResult;
      }

      // Delay after successful bonding
      std::this_thread::sleep_for( std::chrono::milliseconds( 250 ) );

      // ping newly bonded node and stores its OS Read info into the result
      pingNode( bondResult );
      if ( bondResult.getError().getType() != BondError::Type::NoError ) 
      {
        // if ping failed, remove bonded node from the coordinator's list
        removeBondedNode( bondResult );

        // and return Ping Failed error
        return bondResult;
      }


      // manufacturer name and product name
      const IJsCacheService::Manufacturer* manufacturer = m_iJsCacheService->getManufacturer(bondResult.getBondedNodeHwpId());
      if (manufacturer != nullptr) {
        bondResult.setManufacturer(manufacturer->m_name);
      }
      
      const IJsCacheService::Product* product = m_iJsCacheService->getProduct(bondResult.getBondedNodeHwpId());
      if (product != nullptr) {
        bondResult.setProduct(product->m_name);
      }

      //uint8_t osVersion = bondResult.getOsRead()[4];
      //std::string osVersionStr = std::to_string((osVersion >> 4) & 0xFF) + "." + std::to_string(osVersion & 0x0F);
      std::string osBuildStr;
      {
        std::ostringstream os;
        os.fill('0');
        os << std::hex << std::uppercase << std::setw(4) << (int)bondResult.getOsBuild();
        osBuildStr = os.str();
      }

      //IIqrfDpaService::CoordinatorParameters coordParams = m_iIqrfDpaService->getCoordinatorParameters();
      //uint16_t dpaVersion = (coordParams.dpaVerMajor << 8) + coordParams.dpaVerMinor;
      //std::string dpaVersionStr = std::to_string((dpaVersion >> 8) & 0xFF) + "." + std::to_string(dpaVersion & 0xFF);

      // getting hwpId version of the newly bonded node
      getHwpIdVersion(bondResult, bondResult.getBondedAddr());

      // if there was an error, return
      if (bondResult.getError().getType() != BondError::Type::NoError) {
        return bondResult;
      }

      const IJsCacheService::Package* package = m_iJsCacheService->getPackage(
        bondResult.getBondedNodeHwpId(),
        bondResult.getBondedNodeHwpIdVer(),
        osBuildStr, //TODO m_iIqrfDpaService->getCoordinatorParameters().osBuild ?
        m_iIqrfDpaService->getCoordinatorParameters().dpaVerWordAsStr
      );
      if (package != nullptr) {
        std::list<std::string> standards;
        for (const IJsCacheService::StdDriver* driver : package->m_stdDriverVect) {
          standards.push_back(driver->getName());
        }
        bondResult.setStandards(standards);
      }
      else {
        TRC_INFORMATION("Package not found");
      }


      return bondResult;
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
    void setVerboseData( rapidjson::Document& response, BondResult& bondResult )
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

    // sets OS Read data to specified JSON message
    void setOsReadSection( const iqrf::embed::os::RawDpaReadPtr& osReadObject, Document& response ) 
    {
      rapidjson::Pointer( "/data/rsp/osRead/mid" ).Set( response, osReadObject->getMidAsString() );
      rapidjson::Pointer( "/data/rsp/osRead/osVersion" ).Set( response, osReadObject->getOsVersionAsString() );

      rapidjson::Pointer( "/data/rsp/osRead/trMcuType/value" ).Set( response, osReadObject->getTrMcuType() );
      rapidjson::Pointer( "/data/rsp/osRead/trMcuType/trType" ).Set( response, osReadObject->getTrTypeAsString() );
      rapidjson::Pointer( "/data/rsp/osRead/trMcuType/fccCertified" ).Set( response, osReadObject->isFccCertified() );
      rapidjson::Pointer( "/data/rsp/osRead/trMcuType/mcuType" ).Set( response, osReadObject->getTrMcuTypeAsString() );

      rapidjson::Pointer( "/data/rsp/osRead/osBuild" ).Set( response, osReadObject->getOsBuildAsString() );

      // RSSI [dBm]
      rapidjson::Pointer( "/data/rsp/osRead/rssi" ).Set( response, osReadObject->getRssiAsString() );

      // Supply voltage [V]
      rapidjson::Pointer( "/data/rsp/osRead/supplyVoltage" ).Set( response, osReadObject->getSupplyVoltageAsString() );

      // Flags
      rapidjson::Pointer( "/data/rsp/osRead/flags/value" ).Set( response, osReadObject->getFlags() );
      rapidjson::Pointer( "/data/rsp/osRead/flags/insufficientOsBuild" ).Set( response, osReadObject->isInsufficientOsBuild() );
      rapidjson::Pointer( "/data/rsp/osRead/flags/interfaceType" ).Set( response, osReadObject->getInterfaceAsString() );
      rapidjson::Pointer( "/data/rsp/osRead/flags/dpaHandlerDetected" ).Set( response, osReadObject->isDpaHandlerDetected() );
      rapidjson::Pointer( "/data/rsp/osRead/flags/dpaHandlerNotDetectedButEnabled" ).Set( response, osReadObject->isDpaHandlerNotDetectedButEnabled() );
      rapidjson::Pointer( "/data/rsp/osRead/flags/noInterfaceSupported" ).Set( response, osReadObject->isNoInterfaceSupported() );

      // Slot limits
      rapidjson::Pointer( "/data/rsp/osRead/slotLimits/value" ).Set( response, osReadObject->getSlotLimits() );
      rapidjson::Pointer( "/data/rsp/osRead/slotLimits/shortestTimeslot" ).Set( response, osReadObject->getShortestTimeSlotAsString() );
      rapidjson::Pointer( "/data/rsp/osRead/slotLimits/longestTimeslot" ).Set( response, osReadObject->getLongestTimeSlotAsString() );

      if ( osReadObject->is410Compliant() )
      {
        // dpaVer, perNr
        Pointer("/data/rsp/osRead/dpaVer").Set(response, osReadObject->getDpaVerAsHexaString());
        Pointer("/data/rsp/osRead/perNr").Set(response, osReadObject->getPerNr());

        Document::AllocatorType& allocator = response.GetAllocator();

        // embPers
        rapidjson::Value embPersJsonArray(kArrayType);
        for (std::set<int>::iterator it = osReadObject->getEmbedPer().begin(); it != osReadObject->getEmbedPer().end(); it++)
        {
          embPersJsonArray.PushBack(*it, allocator);
        }
        Pointer("/data/rsp/osRead/embPers").Set(response, embPersJsonArray);

        // hwpId
        Pointer("/data/rsp/osRead/hwpId").Set(response, osReadObject->getHwpid());

        // hwpIdVer
        Pointer("/data/rsp/osRead/hwpIdVer").Set(response, osReadObject->getHwpidVer());

        // flags - int value
        Pointer("/data/rsp/osRead/enumFlags/value").Set(response, osReadObject->getFlags());

        // flags - parsed
        bool stdModeSupported = ((osReadObject->getFlags() & 0b1) == 0b1) ? true : false;
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
        if (osReadObject->getDpaVer() >= 0x0400)
        {
          bool stdAndLpModeNetwork = ((osReadObject->getFlags() & 0b100) == 0b100) ? true : false;
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
        for (std::set<int>::iterator it = osReadObject->getUserPer().begin(); it != osReadObject->getUserPer().end(); it++)
        {
          userPerJsonArray.PushBack(*it, allocator);
        }
        Pointer("/data/rsp/osRead/userPers").Set(response, userPerJsonArray);
      }
    }

    // creates response on the basis of bond result
    Document createResponse(
      const std::string& msgId,
      const IMessagingSplitterService::MsgType& msgType,
      BondResult& bondResult,
      const ComIqmeshNetworkBondNodeLocal& comBondNodeLocal
    )
    {
      Document response;

      // set common parameters
      Pointer( "/mType" ).Set( response, msgType.m_type );
      Pointer( "/data/msgId" ).Set( response, msgId );

      // checking of error
      BondError error = bondResult.getError();

      if ( error.getType() != BondError::Type::NoError ) 
      {
        int status = SERVICE_ERROR;

        switch ( error.getType() ) {
          case BondError::Type::GetBondedNodes:
            status = SERVICE_ERROR_GET_BONDED_NODES;
            break;

          case BondError::Type::AlreadyBonded:
            status = SERVICE_ERROR_ALREADY_BONDED;
            break;

          case BondError::Type::NoFreeSpace:
            status = SERVICE_ERROR_NO_FREE_SPACE;
            break;

          case BondError::Type::BondError:
            status = SERVICE_ERROR_BOND_FAILED;
            break;

          case BondError::Type::PingFailed:
            rapidjson::Pointer("/data/rsp/assignedAddr").Set(response, bondResult.getBondedAddr());
            rapidjson::Pointer("/data/rsp/nodesNr").Set(response, bondResult.getBondedNodesNum());
            status = SERVICE_ERROR_PING_FAILED;
            break;

          case BondError::Type::HwpIdVersion:
            Pointer("/data/rsp/assignedAddr").Set(response, bondResult.getBondedAddr());
            Pointer("/data/rsp/nodesNr").Set(response, bondResult.getBondedNodesNum());
            Pointer("/data/rsp/hwpId").Set(response, bondResult.getBondedNodeHwpId());
            Pointer("/data/rsp/manufacturer").Set(response, bondResult.getManufacturer());
            Pointer("/data/rsp/product").Set(response, bondResult.getProduct());
            status = SERVICE_ERROR_HWPID_VERSION;
            break;

          default:
            status = SERVICE_ERROR;
            break;
        }

        // set raw fields, if verbose mode is active
        if ( comBondNodeLocal.getVerbose() ) {
          setVerboseData( response, bondResult );
        }

        Pointer("/data/status").Set(response, status);
        Pointer("/data/statusStr").Set(response, error.getMessage());

        return response;
      }

      // no errors

      // rsp object
      rapidjson::Pointer( "/data/rsp/assignedAddr" ).Set( response, bondResult.getBondedAddr() );
      rapidjson::Pointer( "/data/rsp/nodesNr" ).Set( response, bondResult.getBondedNodesNum() );
      
      rapidjson::Pointer("/data/rsp/hwpId").Set(response, bondResult.getBondedNodeHwpId());
      rapidjson::Pointer("/data/rsp/manufacturer").Set(response, bondResult.getManufacturer());
      rapidjson::Pointer("/data/rsp/product").Set(response, bondResult.getProduct());

      // standards - array of strings
      rapidjson::Value standardsJsonArray(kArrayType);
      Document::AllocatorType& allocator = response.GetAllocator();
      for (std::string standard : bondResult.getStandards()) {
        rapidjson::Value standardJsonString;
        standardJsonString.SetString(standard.c_str(), standard.length(), allocator);
        standardsJsonArray.PushBack(standardJsonString, allocator);
      }
      Pointer("/data/rsp/standards").Set(response, standardsJsonArray);

      // osRead object
      setOsReadSection(bondResult.getOsRead(), response);

      // set raw fields, if verbose mode is active
      if ( comBondNodeLocal.getVerbose() ) {
        setVerboseData( response, bondResult );
      }

      // status
      Pointer("/data/status").Set(response, 0);
      Pointer("/data/statusStr").Set(response, "ok");

      return response;
    }

    uint8_t parseAndCheckRepeat( const int repeat ) {
      if ( repeat < 0 ) {
        TRC_WARNING( "repeat cannot be less than 0. It will be set to 0." );
        return 0;
      }

      if ( repeat > 0xFF ) {
        TRC_WARNING( "repeat exceeds maximum. It will be trimmed to maximum of: " << PAR( REPEAT_MAX ) );
        return REPEAT_MAX;
      }

      return repeat;
    }

    uint8_t parseAndCheckDeviceAddr( const int devAddr ) {
      if ( ( devAddr < 0 ) || ( devAddr > 0xEF ) ) {
        THROW_EXC(
          std::out_of_range, "Device address outside of valid range. " << NAME_PAR_HEX( "deviceAddr", devAddr )
        );
      }
      return devAddr;
    }

    uint8_t parseAndCheckBondingMask( const int bondingMask ) {
      switch ( bondingMask ) {
        case 0x00:
        case 0x01:
        case 0x03:
        case 0x07:
        case 0x0f:
        case 0x1f:
        case 0x3f:
        case 0x7f:
        case 0xff:
          return bondingMask;
        default:
          THROW_EXC(
            std::out_of_range, "Bonding mask outside of recommended range. " << NAME_PAR_HEX( "bondingMask", bondingMask )
          );
      }
    }

    uint8_t parseAndCheckBondingTestRetries( int bondingTestRetries ) {
      if ( ( bondingTestRetries < 0 ) || ( bondingTestRetries > 0xFF ) ) {
        THROW_EXC(
          std::out_of_range, "bondingTestRetries outside of valid range. " << NAME_PAR_HEX( "bondingTestRetries", bondingTestRetries )
        );
      }
      return bondingTestRetries;
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
      if ( msgType.m_type != m_mTypeName_iqmeshNetworkBondNodeLocal ) {
        THROW_EXC( std::logic_error, "Unsupported message type: " << PAR( msgType.m_type ) );
      }

      // creating representation object
      ComIqmeshNetworkBondNodeLocal comBondNodeLocal( doc );

      // service input parameters
      uint8_t deviceAddr, bondingMask, bondingTestRetries;

      // parsing and checking service parameters
      try {
        m_repeat = parseAndCheckRepeat( comBondNodeLocal.getRepeat() );

        if ( !comBondNodeLocal.isSetDeviceAddr() ) {
          THROW_EXC( std::logic_error, "deviceAddr not set" );
        }
        
        deviceAddr = parseAndCheckDeviceAddr( comBondNodeLocal.getDeviceAddr() );
        bondingMask = parseAndCheckBondingMask( comBondNodeLocal.getBondingMask() );
        bondingTestRetries = parseAndCheckBondingTestRetries( comBondNodeLocal.getBondingTestRetries() );

        m_returnVerbose = comBondNodeLocal.getVerbose();
      }
      // parsing and checking service parameters failed 
      catch ( std::exception& ex ) {
        Document failResponse = createCheckParamsFailedResponse( comBondNodeLocal.getMsgId(), msgType, ex.what() );
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

        Document failResponse = getExclusiveAccessFailedResponse(comBondNodeLocal.getMsgId(), msgType, errorStr);
        m_iMessagingSplitterService->sendMessage(messagingId, std::move(failResponse));

        TRC_FUNCTION_LEAVE("");
        return;
      }

      BondResult bondResult = bondNode( deviceAddr, bondingMask, bondingTestRetries );

      // release exclusive access
      m_exclusiveAccess.reset();

      // creating response
      Document responseDoc = createResponse( comBondNodeLocal.getMsgId(), msgType, bondResult, comBondNodeLocal );

      // send response back
      m_iMessagingSplitterService->sendMessage( messagingId, std::move( responseDoc ) );

      TRC_FUNCTION_LEAVE( "" );
    }

    void activate( const shape::Properties *props )
    {
      TRC_FUNCTION_ENTER( "" );
      TRC_INFORMATION( std::endl <<
                       "************************************" << std::endl <<
                       "BondNodeLocalService instance activate" << std::endl <<
                       "************************************"
      );

      // for the sake of register function parameters 
      std::vector<std::string> supportedMsgTypes =
      {
        m_mTypeName_iqmeshNetworkBondNodeLocal
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
