#define IReadTrConfService_EXPORTS

#include "ReadTrConfService.h"
#include "Trace.h"
#include "ComIqmeshNetworkReadTrConf.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "iqrf__ReadTrConfService.hxx"
#include <list>
#include <memory>
#include <math.h>
#include <bitset>

TRC_INIT_MODULE(iqrf::ReadTrConfService);

using namespace rapidjson;

namespace {
  // maximum number of repeats
  static const uint8_t REPEAT_MAX = 3;

  // length of the configuration part
  static const uint8_t CONFIGURATION_LEN = 31;

  // baud rates
  static uint8_t BAUD_RATES_SIZE = 9;
  static uint32_t BaudRates[] = {
    1200,
    2400,
    4800,
    9600,
    19200,
    38400,
    57600,
    115200,
    230400
  };
};

namespace iqrf {
  // Holds information about errors, which encounter during read TR config service run
  class ReadTrConfigError
  {
  public:
    // Type of error
    enum class Type
    {
      NoError = 0,
      ServiceError = 1000,
      NotBonded = 1001,
      ReadHwp = 1002
    };
    ReadTrConfigError() : m_type( Type::NoError ), m_message( "ok" ) {};
    ReadTrConfigError( Type errorType ) : m_type( errorType ), m_message( "" ) {};
    ReadTrConfigError( Type errorType, const std::string& message ) : m_type( errorType ), m_message( message ) {};
    Type getType() const { return m_type; };
    std::string getMessage() const { return m_message; };

    ReadTrConfigError& operator=( const ReadTrConfigError& error )
    {
      if ( this == &error )
        return *this;

      this->m_type = error.m_type;
      this->m_message = error.m_message;

      return *this;
    }

  private:
    Type m_type;
    std::string m_message;
  };

  // Holds information about result of read Tr configuration
  class ReadTrConfigResult
  {
  private:
    ReadTrConfigError m_readTrConfigReadError;
    ReadTrConfigError m_bondedError;
    TPerOSReadCfg_Response m_hwpConfig;

    // Transaction results
    std::list<std::unique_ptr<IDpaTransactionResult2>> m_transResults;

  public:

    ReadTrConfigError getBondedError() const { return m_bondedError; };
    void setBondedError( const ReadTrConfigError& error )
    {
      m_bondedError = error;
    }

    ReadTrConfigError getReadTrConfigError() const { return m_readTrConfigReadError; };
    void setReadTrConfigError( const ReadTrConfigError& error )
    {
      m_readTrConfigReadError = error;
    }

    TPerOSReadCfg_Response getHwpConfig() const {
      return m_hwpConfig;
    }
    void setHwpConfig( TPerOSReadCfg_Response hwpConfig ) {
      m_hwpConfig = hwpConfig;
    }

    // Adds transaction result into the list of results
    void addTransactionResult( std::unique_ptr<IDpaTransactionResult2>& transResult ) {
      if ( transResult != nullptr )
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
      return std::move( tranResult );
    }
  };

  // Implementation class
  class ReadTrConfService::Imp {
  private:
    // Parent object
    ReadTrConfService& m_parent;

    // Message type: IQMESH Network Read TR Configuration
    const std::string m_mTypeName_iqmeshNetworkReadTrConf = "iqmeshNetwork_ReadTrConf";
    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    IIqrfDpaService* m_iIqrfDpaService = nullptr;
    std::unique_ptr<IIqrfDpaService::ExclusiveAccess> m_exclusiveAccess;

    // Number of repeats
    uint8_t m_repeat;

    // Verbose mode
    bool m_returnVerbose = false;

  public:
    Imp( ReadTrConfService& parent ) : m_parent( parent )
    {
    }

    ~Imp()
    {
    }

  private:

    // Check the specified node is bonded (in the future read from DB)
    bool isNodeBonded( ReadTrConfigResult& readTrConfigResult, const uint16_t deviceAddr )
    {
      TRC_FUNCTION_ENTER( "" );
      bool result = false;
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage bondedNodesRequest;
        DpaMessage::DpaPacket_t bondedNodesPacket;
        bondedNodesPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        bondedNodesPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
        bondedNodesPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_BONDED_DEVICES;
        bondedNodesPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        bondedNodesRequest.DataToBuffer( bondedNodesPacket.Buffer, sizeof( TDpaIFaceHeader ) );
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat( bondedNodesRequest, transResult, m_repeat );
        TRC_DEBUG( "Result from get bonded nodes transaction as string:" << PAR( transResult->getErrorString() ) );
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION( "Get bonded nodes successful!" );
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR( bondedNodesRequest.PeripheralType(), bondedNodesRequest.NodeAddress() )
          << PAR( (unsigned)bondedNodesRequest.PeripheralCommand() )
        );
        // Parse response pdata
        uns8* bondedNodesArr = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData;
        uint8_t byteIndex = deviceAddr / 8;
        uint8_t bitIndex = deviceAddr % 8;
        uint16_t compareByte = 1 << bitIndex;        
        if ( (result = ( bondedNodesArr[byteIndex] & compareByte ) == compareByte) == false )
        {
          ReadTrConfigError error( ReadTrConfigError::Type::NotBonded, "Node not bonded." );
          readTrConfigResult.setBondedError( error );
        }
      }
      catch ( std::exception& e )
      {
        ReadTrConfigError error( ReadTrConfigError::Type::NotBonded, e.what() );
        readTrConfigResult.setBondedError( error );
      }
      readTrConfigResult.addTransactionResult( transResult );
      TRC_FUNCTION_LEAVE( "" );
      return result;
    }

    // Reads configuration of one node
    void readTrConfig( ReadTrConfigResult& readTrConfigResult, const uint16_t deviceAddr, const uint16_t hwpId )
    {
      TRC_FUNCTION_ENTER( "" );
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare the DPA request
        DpaMessage readHwpRequest;
        DpaMessage::DpaPacket_t readHwpPacket;
        readHwpPacket.DpaRequestPacket_t.NADR = deviceAddr;
        readHwpPacket.DpaRequestPacket_t.PNUM = PNUM_OS;
        readHwpPacket.DpaRequestPacket_t.PCMD = CMD_OS_READ_CFG;
        readHwpPacket.DpaRequestPacket_t.HWPID = hwpId;
        readHwpRequest.DataToBuffer( readHwpPacket.Buffer, sizeof( TDpaIFaceHeader ) );
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat( readHwpRequest, transResult, m_repeat );
        TRC_INFORMATION( "Read HWP successful!" );
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR( readHwpRequest.PeripheralType(), readHwpRequest.NodeAddress() )
          << PAR( readHwpRequest.PeripheralCommand() )
        );
        // Parse response pdata
        DpaMessage dpaResponse = transResult->getResponse();
        TPerOSReadCfg_Response hwpConfig = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerOSReadCfg_Response;
        readTrConfigResult.setHwpConfig( hwpConfig );
      }
      catch ( std::exception& e )
      {
        ReadTrConfigError error( ReadTrConfigError::Type::ReadHwp, e.what() );
        readTrConfigResult.setReadTrConfigError( error );
      }
      readTrConfigResult.addTransactionResult( transResult );
      TRC_FUNCTION_LEAVE( "" );
    }

    // Handle the request
    void handleMsg( const std::string& messagingId, const IMessagingSplitterService::MsgType& msgType, rapidjson::Document doc )
    {
      TRC_FUNCTION_ENTER(
        PAR( messagingId ) <<
        NAME_PAR( mType, msgType.m_type ) <<
        NAME_PAR( major, msgType.m_major ) <<
        NAME_PAR( minor, msgType.m_minor ) <<
        NAME_PAR( micro, msgType.m_micro )
      );

      // Unsupported type of request
      if ( msgType.m_type != m_mTypeName_iqmeshNetworkReadTrConf )
        THROW_EXC( std::logic_error, "Unsupported message type: " << PAR( msgType.m_type ) );

      // Creating representation object
      ComIqmeshNetworkReadTrConf comReadTrConf( doc );

      // Create response and set cpommon parameters
      Document response;
      Pointer( "/mType" ).Set( response, msgType.m_type );
      Pointer( "/data/msgId" ).Set( response, comReadTrConf.getMsgId() );

      // Service input parameters
      uint16_t deviceAddr;
      uint16_t hwpId;
      // Parsing and checking service parameters
      try
      {
        m_repeat = comReadTrConf.getRepeat();
        if ( !comReadTrConf.isSetDeviceAddr() )
          THROW_EXC( std::logic_error, "deviceAddr not set" );
        deviceAddr = comReadTrConf.getDeviceAddr();
        if ( deviceAddr > 239 )
          THROW_EXC( std::out_of_range, "Device address outside of valid range. " << NAME_PAR_HEX( "Address", deviceAddr ) );
        if ( comReadTrConf.isSetHwpId() )
          hwpId = comReadTrConf.getHwpId();
        else
          hwpId = HWPID_DoNotCheck;
        m_returnVerbose = comReadTrConf.getVerbose();
      }
      catch ( std::exception& ex )
      {
        // Parsing and checking service parameters failed 
        Pointer( "/data/status" ).Set( response, (int32_t)ReadTrConfigError::Type::ServiceError );
        Pointer( "/data/statusStr" ).Set( response, ex.what() );
        // Send response
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
        Pointer( "/data/status" ).Set( response, (int32_t)ReadTrConfigError::Type::ServiceError );
        Pointer( "/data/statusStr" ).Set( response, errorStr );
        // Send response
        m_iMessagingSplitterService->sendMessage( messagingId, std::move( response ) );
        TRC_FUNCTION_LEAVE( "" );
        return;
      }

      // Result
      ReadTrConfigResult readTrConfigResult;

      // Check node is bonded
      if ( deviceAddr == COORDINATOR_ADDRESS || isNodeBonded( readTrConfigResult, deviceAddr ) )
      {
        // Read HWP configuration
        readTrConfig( readTrConfigResult, deviceAddr, hwpId );
      }

      // Release exclusive access
      m_exclusiveAccess.reset();

      // Only one node - for the present time
      Pointer( "/data/rsp/deviceAddr" ).Set( response, deviceAddr );

      // Node bonded ?
      ReadTrConfigError readTrConfigError = readTrConfigResult.getBondedError();
      if ( readTrConfigError.getType() == ReadTrConfigError::Type::NoError )
      {
        // Yes, readTrConfig transaction success ?
        readTrConfigError = readTrConfigResult.getReadTrConfigError();
        if ( readTrConfigError.getType() == ReadTrConfigError::Type::NoError )
        {
          // osRead object
          TPerOSReadCfg_Response hwpConfig = readTrConfigResult.getHwpConfig();

          // getting DPA version
          IIqrfDpaService::CoordinatorParameters coordParams = m_iIqrfDpaService->getCoordinatorParameters();
          uint16_t dpaVer = ( coordParams.dpaVerMajor << 8 ) + coordParams.dpaVerMinor;

          uns8* configuration = hwpConfig.Configuration;

          if ( dpaVer < 0x0303 ) {
            for ( int i = 0; i < CONFIGURATION_LEN; i++ ) {
              configuration[i] = configuration[i] ^ 0x34;
            }
          }

          Document::AllocatorType& allocator = response.GetAllocator();

          // predefined peripherals - bits
          rapidjson::Value embPerBitsJsonArray( kArrayType );
          for ( int i = 0; i < 4; i++ ) {
            embPerBitsJsonArray.PushBack( configuration[i], allocator );
          }
          Pointer( "/data/rsp/embPers/values" ).Set( response, embPerBitsJsonArray );

          // embedded peripherals bits - parsed
          // byte 0x01
          uint8_t byte01 = configuration[0x00];

          bool coordPresent = ( ( byte01 & 0b1 ) == 0b1 ) ? true : false;
          Pointer( "/data/rsp/embPers/coordinator" ).Set( response, coordPresent );

          bool nodePresent = ( ( byte01 & 0b10 ) == 0b10 ) ? true : false;
          Pointer( "/data/rsp/embPers/node" ).Set( response, nodePresent );

          bool osPresent = ( ( byte01 & 0b100 ) == 0b100 ) ? true : false;
          Pointer( "/data/rsp/embPers/os" ).Set( response, osPresent );

          bool eepromPresent = ( ( byte01 & 0b1000 ) == 0b1000 ) ? true : false;
          Pointer( "/data/rsp/embPers/eeprom" ).Set( response, eepromPresent );

          bool eeepromPresent = ( ( byte01 & 0b10000 ) == 0b10000 ) ? true : false;
          Pointer( "/data/rsp/embPers/eeeprom" ).Set( response, eeepromPresent );

          bool ramPresent = ( ( byte01 & 0b100000 ) == 0b100000 ) ? true : false;
          Pointer( "/data/rsp/embPers/ram" ).Set( response, ramPresent );

          bool ledrPresent = ( ( byte01 & 0b1000000 ) == 0b1000000 ) ? true : false;
          Pointer( "/data/rsp/embPers/ledr" ).Set( response, ledrPresent );

          bool ledgPresent = ( ( byte01 & 0b10000000 ) == 0b10000000 ) ? true : false;
          Pointer( "/data/rsp/embPers/ledg" ).Set( response, ledgPresent );

          // byte 0x02
          uint8_t byte02 = configuration[0x01];

          bool spiPresent = ( ( byte02 & 0b1 ) == 0b1 ) ? true : false;
          Pointer( "/data/rsp/embPers/spi" ).Set( response, spiPresent );

          bool ioPresent = ( ( byte02 & 0b10 ) == 0b10 ) ? true : false;
          Pointer( "/data/rsp/embPers/io" ).Set( response, ioPresent );

          bool thermometerPresent = ( ( byte02 & 0b100 ) == 0b100 ) ? true : false;
          Pointer( "/data/rsp/embPers/thermometer" ).Set( response, thermometerPresent );

          bool pwmPresent = ( ( byte02 & 0b1000 ) == 0b1000 ) ? true : false;
          Pointer( "/data/rsp/embPers/pwm" ).Set( response, pwmPresent );

          bool uartPresent = ( ( byte02 & 0b10000 ) == 0b10000 ) ? true : false;
          Pointer( "/data/rsp/embPers/uart" ).Set( response, uartPresent );
          
          if ( dpaVer < 0x0400 )
          {
            bool frcPresent = ( ( byte02 & 0b100000 ) == 0b100000 ) ? true : false;
            Pointer( "/data/rsp/embPers/frc" ).Set( response, frcPresent );
          }

          // byte 0x05
          uint8_t byte05 = configuration[0x04];

          bool customDpaHandler = ( ( byte05 & 0b00000001 ) == 0b00000001 ) ? true : false;
          Pointer( "/data/rsp/customDpaHandler" ).Set( response, customDpaHandler );

          // for DPA v4.00 downwards
          if ( dpaVer < 0x0400 ) {
            bool nodeDpaInterface = ( ( byte05 & 0b00000010 ) == 0b00000010 ) ? true : false;
            Pointer( "/data/rsp/nodeDpaInterface" ).Set( response, nodeDpaInterface );
          }

          // for DPA v4.10 upwards
          if ( dpaVer >= 0x0410 ) {
            bool dpaPeerToPeer = ( ( byte05 & 0b00000010 ) == 0b00000010 ) ? true : false;
            Pointer( "/data/rsp/dpaPeerToPeer" ).Set( response, dpaPeerToPeer );
          }

          bool dpaAutoexec = ( ( byte05 & 0b00000100 ) == 0b00000100 ) ? true : false;
          Pointer( "/data/rsp/dpaAutoexec" ).Set( response, dpaAutoexec );

          bool routingOff = ( ( byte05 & 0b00001000 ) == 0b00001000 ) ? true : false;
          Pointer( "/data/rsp/routingOff" ).Set( response, routingOff );

          bool ioSetup = ( ( byte05 & 0b00010000 ) == 0b00010000 ) ? true : false;
          Pointer( "/data/rsp/ioSetup" ).Set( response, ioSetup );

          bool peerToPeer = ( ( byte05 & 0b00100000 ) == 0b00100000 ) ? true : false;
          Pointer( "/data/rsp/peerToPeer" ).Set( response, peerToPeer );


          // for DPA v3.03 onwards
          if ( dpaVer >= 0x0303 ) {
            bool neverSleep = ( ( byte05 & 0b01000000 ) == 0b01000000 ) ? true : false;
            Pointer( "/data/rsp/neverSleep" ).Set( response, neverSleep );
          }

          // for DPA v4.00 onwards
          if ( dpaVer >= 0x0400 ) {
            bool stdAndLpNetwork = ( ( byte05 & 0b10000000 ) == 0b10000000 ) ? true : false;
            Pointer( "/data/rsp/stdAndLpNetwork" ).Set( response, stdAndLpNetwork );
          }

          // bytes fields
          Pointer( "/data/rsp/rfChannelA" ).Set( response, configuration[0x10] );
          Pointer( "/data/rsp/rfChannelB" ).Set( response, configuration[0x11] );

          // up to DPA < 4.00
          if ( dpaVer < 0x0400 ) {
            Pointer( "/data/rsp/rfSubChannelA" ).Set( response, configuration[0x05] );
            Pointer( "/data/rsp/rfSubChannelB" ).Set( response, configuration[0x06] );
          }

          Pointer( "/data/rsp/txPower" ).Set( response, configuration[0x07] );
          Pointer( "/data/rsp/rxFilter" ).Set( response, configuration[0x08] );
          Pointer( "/data/rsp/lpRxTimeout" ).Set( response, configuration[0x09] );
          Pointer( "/data/rsp/rfAltDsmChannel" ).Set( response, configuration[0x0B] );

          // BaudRate
          if ( configuration[0x0A] <= BAUD_RATES_SIZE )
            Pointer( "/data/rsp/uartBaudrate" ).Set( response, BaudRates[configuration[0x0A]] );
          else
          {
            TRC_WARNING( "Unknown baud rate constant: " << PAR( configuration[0x0A] ) );
            Pointer( "/data/rsp/uartBaudrate" ).Set( response, 0 );
          }

          // RFPGM byte
          uint8_t rfpgm = hwpConfig.RFPGM;

          bool rfPgmDualChannel = ( ( rfpgm & 0b00000011 ) == 0b00000011 ) ? true : false;
          Pointer( "/data/rsp/rfPgmDualChannel" ).Set( response, rfPgmDualChannel );

          bool rfPgmLpMode = ( ( rfpgm & 0b00000100 ) == 0b00000100 ) ? true : false;
          Pointer( "/data/rsp/rfPgmLpMode" ).Set( response, rfPgmLpMode );

          bool rfPgmIncorrectUpload = ( ( rfpgm & 0b00001000 ) == 0b00001000 ) ? true : false;
          Pointer( "/data/rsp/rfPgmIncorrectUpload" ).Set( response, rfPgmIncorrectUpload );

          bool enableAfterReset = ( ( rfpgm & 0b00010000 ) == 0b00010000 ) ? true : false;
          Pointer( "/data/rsp/rfPgmEnableAfterReset" ).Set( response, enableAfterReset );

          bool rfPgmTerminateAfter1Min = ( ( rfpgm & 0b01000000 ) == 0b01000000 ) ? true : false;
          Pointer( "/data/rsp/rfPgmTerminateAfter1Min" ).Set( response, rfPgmTerminateAfter1Min );

          bool rfPgmTerminateMcuPin = ( ( rfpgm & 0b10000000 ) == 0b10000000 ) ? true : false;
          Pointer( "/data/rsp/rfPgmTerminateMcuPin" ).Set( response, rfPgmTerminateMcuPin );

          // RF band - undocumented byte
          std::string rfBand = "";
          switch ( hwpConfig.Undocumented[0] & 0x03 )
          {
            case 0b00:
              rfBand = "868";
              break;
            case 0b01:
              rfBand = "916";
              break;
            case 0b10:
              rfBand = "433";
              break;
            default:
              TRC_WARNING( "Unknown baud rate constant: " << PAR( ( hwpConfig.Undocumented[0] & 0x03 ) ) );
          }
          Pointer( "/data/rsp/rfBand" ).Set( response, rfBand );
        }
      }
     
      // Set raw fields, if verbose mode is active
      if ( comReadTrConf.getVerbose() )
      {
        rapidjson::Value rawArray( kArrayType );
        Document::AllocatorType& allocator = response.GetAllocator();
        while ( readTrConfigResult.isNextTransactionResult() )
        {
          std::unique_ptr<IDpaTransactionResult2> transResult = readTrConfigResult.consumeNextTransactionResult();
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
          // Add object into array
          rawArray.PushBack( rawObject, allocator );
        }
        // Add array into response document
        Pointer( "/data/raw" ).Set( response, rawArray );
      }

      // Set response status
      Pointer( "/data/status" ).Set( response, (int32_t)readTrConfigError.getType() );
      Pointer( "/data/statusStr" ).Set( response, readTrConfigError.getMessage() );

      // Send response
      m_iMessagingSplitterService->sendMessage( messagingId, std::move( response ) );
      TRC_FUNCTION_LEAVE( "" );
    }

  public:
    void activate( const shape::Properties *props )
    {
      TRC_FUNCTION_ENTER( "" );
      TRC_INFORMATION( std::endl <<
                       "************************************" << std::endl <<
                       "ReadTrConfService instance activate" << std::endl <<
                       "************************************"
      );

      // for the sake of register function parameters 
      std::vector<std::string> supportedMsgTypes =
      {
        m_mTypeName_iqmeshNetworkReadTrConf
      };

      m_iMessagingSplitterService->registerFilteredMsgHandler(
        supportedMsgTypes,
        [&]( const std::string & messagingId, const IMessagingSplitterService::MsgType & msgType, rapidjson::Document doc )
      {
        handleMsg( messagingId, msgType, std::move( doc ) );
      } );

      TRC_FUNCTION_LEAVE( "" );
    }

    void deactivate()
    {
      TRC_FUNCTION_ENTER( "" );
      TRC_INFORMATION( std::endl <<
                       "**************************************" << std::endl <<
                       "ReadTrConfService instance deactivate" << std::endl <<
                       "**************************************"
      );

      // for the sake of unregister function parameters 
      std::vector<std::string> supportedMsgTypes =
      {
        m_mTypeName_iqmeshNetworkReadTrConf
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

  ReadTrConfService::ReadTrConfService()
  {
    m_imp = shape_new Imp( *this );
  }

  ReadTrConfService::~ReadTrConfService()
  {
    delete m_imp;
  }

  void ReadTrConfService::attachInterface( iqrf::IIqrfDpaService* iface )
  {
    m_imp->attachInterface( iface );
  }

  void ReadTrConfService::detachInterface( iqrf::IIqrfDpaService* iface )
  {
    m_imp->detachInterface( iface );
  }

  void ReadTrConfService::attachInterface( iqrf::IMessagingSplitterService* iface )
  {
    m_imp->attachInterface( iface );
  }

  void ReadTrConfService::detachInterface( iqrf::IMessagingSplitterService* iface )
  {
    m_imp->detachInterface( iface );
  }

  void ReadTrConfService::attachInterface( shape::ITraceService* iface )
  {
    shape::Tracer::get().addTracerService( iface );
  }

  void ReadTrConfService::detachInterface( shape::ITraceService* iface )
  {
    shape::Tracer::get().removeTracerService( iface );
  }

  void ReadTrConfService::activate( const shape::Properties *props )
  {
    m_imp->activate( props );
  }

  void ReadTrConfService::deactivate()
  {
    m_imp->deactivate();
  }

  void ReadTrConfService::modify( const shape::Properties *props )
  {
    m_imp->modify( props );
  }
}