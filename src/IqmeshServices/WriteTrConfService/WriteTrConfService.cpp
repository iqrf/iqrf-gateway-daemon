#define IWriteTrConfService_EXPORTS

#include "WriteTrConfService.h"
#include "IMessagingSplitterService.h"
#include "Trace.h"
#include "ComMngIqmeshWriteConfig.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "iqrf__WriteTrConfService.hxx"
#include <list>
#include <memory>
#include <math.h>
#include <bitset>

TRC_INIT_MODULE(iqrf::WriteTrConfService);

using namespace rapidjson;

namespace
{

  // RF channel band
  enum class RF_ChannelBand
  {
    UNSPECIFIED,
    BAND_433,
    BAND_868,
    BAND_916
  };

  // Baud rates
  static uint8_t BAUD_RATES_SIZE = 9;

  // service general fail code - may and probably will be changed later in the future
  static const int SERVICE_ERROR = 1000;
}; // namespace

namespace iqrf
{

  // Holds information about errors, which encounter during configuration write
  class WriteTrConfError
  {
  public:
    // Type of error
    enum class Type
    {
      NoError,
      CheckPerCoordAndOS,
      SetFrcParams,
      EnableFrc,
      DisableFrc,
      WriteTrConfByte,
      FrcAcknowledgedBroadcastBits,
      SetSecurity
    };

    WriteTrConfError() : m_type( Type::NoError ), m_message( "ko" ) {};
    WriteTrConfError( Type errorType ) : m_type( errorType ), m_message( "" ) {};
    WriteTrConfError( Type errorType, const std::string &message ) : m_type( errorType ), m_message( message ) {};

    Type getType() const { return m_type; };
    std::string getMessage() const { return m_message; };

    WriteTrConfError &operator=( const WriteTrConfError &error )
    {
      if ( this == &error )
      {
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

  // Configuration byte - according to DPA spec. - TPerOSWriteCfgByteTriplet
  struct TrConfigByte
  {
    uint8_t address;
    uint8_t value;
    uint8_t mask;

    TrConfigByte()
    {
      address = 0;
      value = 0;
      mask = 0;
    }

    TrConfigByte( const uint8_t _address, const uint8_t _value, const uint8_t _mask )
    {
      address = _address;
      value = _value;
      mask = _mask;
    }
  };

  // Holds information about configuration writing result
  class WriteTrConfResult
  {
  private:
    // Error type
    WriteTrConfError m_error;

    // Nodes list
    std::basic_string<uint8_t> m_RespondedNodes;

    // Transaction results
    std::list<std::unique_ptr<IDpaTransactionResult2>> m_transResults;

  public:
    // Error type
    WriteTrConfError getError() const { return m_error; };
    void setError( const WriteTrConfError &error )
    {
      m_error = error;
    }

    // Nodes list
    std::basic_string<uint8_t> getRespondedNodes() const { return m_RespondedNodes; };
    void setRespondedNodes( const std::basic_string<uint8_t> nodes )
    {
      m_RespondedNodes = nodes;
    }

    // Adds transaction result into the list of results
    void addTransactionResult( std::unique_ptr<IDpaTransactionResult2> &transResult )
    {
      m_transResults.push_back( std::move( transResult ) );
    }

    bool isNextTransactionResult()
    {
      return ( m_transResults.size() > 0 );
    }

    // Consumes the first element in the transaction results list
    std::unique_ptr<IDpaTransactionResult2> consumeNextTransactionResult()
    {
      std::list<std::unique_ptr<IDpaTransactionResult2>>::iterator iter = m_transResults.begin();
      std::unique_ptr<IDpaTransactionResult2> tranResult = std::move( *iter );
      m_transResults.pop_front();
      return std::move( tranResult );
    }
  };

  // implementation class
  class WriteTrConfService::Imp
  {
  private:
    // Parent object
    WriteTrConfService &m_parent;

    // Message type
    const std::string m_mTypeName_iqmeshNetwork_WriteTrConf = "iqmeshNetwork_WriteTrConf";
    IMessagingSplitterService *m_iMessagingSplitterService = nullptr;
    IIqrfDpaService *m_iIqrfDpaService = nullptr;
    std::unique_ptr<IIqrfDpaService::ExclusiveAccess> m_exclusiveAccess;
    const std::string* m_messagingId = nullptr;
    const IMessagingSplitterService::MsgType* m_msgType = nullptr;
    const ComMngIqmeshWriteConfig* m_comWriteConfig = nullptr;

    // Service input parameters
    TWriteTrConfInputParams m_writeTrConfParams;

    // if is set Verbose mode
    bool m_returnVerbose = false;

    // indication, if FRC has been temporarily enabled
    bool m_frcEnabled = false;

  public:
    Imp( WriteTrConfService &parent ) : m_parent( parent )
    {
      /*
        m_msgType_mngIqmeshWriteConfig
          = shape_new IMessagingSplitterService::MsgType(m_mTypeName_mngIqmeshWriteConfig, 1, 0, 0);
          */
    }

    ~Imp()
    {
    }

  private:

    // Check presence of Coordinator and OS peripherals on coordinator node
    void checkPresentCoordAndCoordOs( WriteTrConfResult& writeTrConfResult )
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
        // Data to buffer
        perEnumRequest.DataToBuffer( perEnumPacket.Buffer, sizeof( TDpaIFaceHeader ) );
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat( perEnumRequest, transResult, m_writeTrConfParams.repeat );
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
        if ( ( dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.EnumPeripheralsAnswer.EmbeddedPers[0] & ( 1 << PNUM_FRC ) ) != ( 1 << PNUM_FRC ) )
          THROW_EXC( std::logic_error, "Coordinator peripheral NOT found." );
        if ( ( dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.EnumPeripheralsAnswer.EmbeddedPers[0] & ( 1 << PNUM_OS ) ) != ( 1 << PNUM_OS ) )
          THROW_EXC( std::logic_error, "OS peripheral NOT found." );
        writeTrConfResult.addTransactionResult( transResult );
        TRC_FUNCTION_LEAVE( "" );
      }
      catch ( std::exception& e )
      {
        WriteTrConfError error( WriteTrConfError::Type::CheckPerCoordAndOS, e.what() );
        writeTrConfResult.setError( error );
        writeTrConfResult.addTransactionResult( transResult );
        THROW_EXC( std::logic_error, e.what() );
      }
    }

    // Set FRC response time
    uint8_t setFrcReponseTime( WriteTrConfResult& writeTrConfResult, uint8_t FRCresponseTime )
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
        m_exclusiveAccess->executeDpaTransactionRepeat( setFrcParamRequest, transResult, m_writeTrConfParams.repeat );
        TRC_DEBUG( "Result from Set Hops transaction as string:" << PAR( transResult->getErrorString() ) );
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION( "Set Hops successful!" );
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR( Peripheral type, setFrcParamRequest.PeripheralType() )
          << NAME_PAR( Node address, setFrcParamRequest.NodeAddress() )
          << NAME_PAR( Command, (int)setFrcParamRequest.PeripheralCommand() )
        );
        writeTrConfResult.addTransactionResult( transResult );
        TRC_FUNCTION_LEAVE( "" );
        return dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSetParams_RequestResponse.FRCresponseTime;
      }
      catch ( std::exception& e )
      {
        WriteTrConfError error( WriteTrConfError::Type::SetFrcParams, e.what() );
        writeTrConfResult.setError( error );
        writeTrConfResult.addTransactionResult( transResult );
        THROW_EXC( std::logic_error, e.what() );
      }
    }

    // FRC_AcknowledgedBroadcastBits
    TPerFrcSend_Response FrcAcknowledgedBroadcastBits( WriteTrConfResult& writeTrConfResult, const std::basic_string<uint8_t> userData )
    {
      TRC_FUNCTION_ENTER( "" );
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage frcAckBroadcastBitsRequest;
        DpaMessage::DpaPacket_t frcAckBroadcastBitsPacket;
        frcAckBroadcastBitsPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        frcAckBroadcastBitsPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
        frcAckBroadcastBitsPacket.DpaRequestPacket_t.PCMD = CMD_FRC_SEND;
        frcAckBroadcastBitsPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        // FRC Command
        frcAckBroadcastBitsPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.FrcCommand = FRC_AcknowledgedBroadcastBits;
        // Set FRC user data
        std::copy( userData.begin(), userData.end(), frcAckBroadcastBitsPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData );
        // Data to buffer
        frcAckBroadcastBitsRequest.DataToBuffer( frcAckBroadcastBitsPacket.Buffer, sizeof( TDpaIFaceHeader ) + sizeof( TPerFrcSend_Request ) );
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat( frcAckBroadcastBitsRequest, transResult, m_writeTrConfParams.repeat );
        TRC_DEBUG( "Result from FRC Acknowledged Broadcast Bits transaction as string:" << PAR( transResult->getErrorString() ) );
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION( "FRC Acknowledged Broadcast Bits successful!" );
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR( Peripheral type, frcAckBroadcastBitsRequest.PeripheralType() )
          << NAME_PAR( Node address, frcAckBroadcastBitsRequest.NodeAddress() )
          << NAME_PAR( Command, (int)frcAckBroadcastBitsRequest.PeripheralCommand() )
        );
        // Check status
        uint8_t status = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.Status;
        if ( status > 0xef )
        {
          TRC_WARNING( "FRC Prebonded Memory Read NOT ok." << NAME_PAR_HEX( "Status", (int)status ) );
          THROW_EXC( std::logic_error, "Bad FRC status: " << PAR( (int)status ) );
        }
        // Add FRC result
        TRC_INFORMATION( "FRC Prebonded Memory Read status ok." << NAME_PAR_HEX( "Status", (int)status ) );
        writeTrConfResult.addTransactionResult( transResult );
        TRC_FUNCTION_LEAVE( "" );
        return dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response;
      }
      catch ( std::exception& e )
      {
        WriteTrConfError error( WriteTrConfError::Type::FrcAcknowledgedBroadcastBits, e.what() );
        writeTrConfResult.setError( error );
        writeTrConfResult.addTransactionResult( transResult );
        THROW_EXC( std::logic_error, e.what() );
      }
    }

    // Write TR config by unicast request
    void writeTrConfUnicast( WriteTrConfResult &writeTrConfResult, const uint16_t deviceAddr, const uint16_t hwpId, const std::vector<TrConfigByte> &trConfigBytes )
    {
      TRC_FUNCTION_ENTER( "" );
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage writeCfgByteRequest;
        DpaMessage::DpaPacket_t writeCfgBytePacket;
        writeCfgBytePacket.DpaRequestPacket_t.NADR = deviceAddr;
        writeCfgBytePacket.DpaRequestPacket_t.PNUM = PNUM_OS;
        writeCfgBytePacket.DpaRequestPacket_t.PCMD = CMD_OS_WRITE_CFG_BYTE;
        writeCfgBytePacket.DpaRequestPacket_t.HWPID = hwpId;
        // Fill config bytes
        uint8_t index = 0x00;
        for ( const TrConfigByte trConfigByte : trConfigBytes )
        {
          writeCfgBytePacket.DpaRequestPacket_t.DpaMessage.PerOSWriteCfgByte_Request.Triplets[index].Address = trConfigByte.address;
          writeCfgBytePacket.DpaRequestPacket_t.DpaMessage.PerOSWriteCfgByte_Request.Triplets[index].Value = trConfigByte.value;
          writeCfgBytePacket.DpaRequestPacket_t.DpaMessage.PerOSWriteCfgByte_Request.Triplets[index++].Mask = trConfigByte.mask;
        }
        // Data to buffer
        writeCfgByteRequest.DataToBuffer( writeCfgBytePacket.Buffer, sizeof( TDpaIFaceHeader ) + index * sizeof( TPerOSWriteCfgByteTriplet ) );
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat( writeCfgByteRequest, transResult, m_writeTrConfParams.repeat );
        TRC_DEBUG( "Result from Write TR Configuration byte transaction as string:" << PAR( transResult->getErrorString() ) );
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION( "Write TR Configuration byte successful!" );
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR( Peripheral type, writeCfgByteRequest.PeripheralType() )
          << NAME_PAR( Node address, writeCfgByteRequest.NodeAddress() )
          << NAME_PAR( Command, (int)writeCfgByteRequest.PeripheralCommand() )
        );
        // Add transaction
        writeTrConfResult.addTransactionResult( transResult );
        TRC_FUNCTION_LEAVE( "" );
      }
      catch ( std::exception& e )
      {
        WriteTrConfError error( WriteTrConfError::Type::WriteTrConfByte, e.what() );
        writeTrConfResult.setError( error );
        writeTrConfResult.addTransactionResult( transResult );
        THROW_EXC( std::logic_error, e.what() );
      }
    }

    // Set security unicat
    void setSecurityUnicast( WriteTrConfResult &writeTrConfResult, const uint16_t deviceAddr, const uint16_t hwpId, const uint8_t type, const std::basic_string<uint8_t> key )
    {
      TRC_FUNCTION_ENTER( "" );
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage setSecurityRequest;
        DpaMessage::DpaPacket_t setSecurityPacket;
        setSecurityPacket.DpaRequestPacket_t.NADR = deviceAddr;
        setSecurityPacket.DpaRequestPacket_t.PNUM = PNUM_OS;
        setSecurityPacket.DpaRequestPacket_t.PCMD = CMD_OS_SET_SECURITY;
        setSecurityPacket.DpaRequestPacket_t.HWPID = hwpId;
        // Fill security type and key
        setSecurityPacket.DpaRequestPacket_t.DpaMessage.PerOSSetSecurity_Request.Type = type;
        std::copy( key.begin(), key.end(), setSecurityPacket.DpaRequestPacket_t.DpaMessage.PerOSSetSecurity_Request.Data );
        // Data to buffer
        setSecurityRequest.DataToBuffer( setSecurityPacket.Buffer, sizeof( TDpaIFaceHeader ) + sizeof( TPerOSSetSecurity_Request ) );
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat( setSecurityRequest, transResult, m_writeTrConfParams.repeat );
        TRC_DEBUG( "Result from Set security transaction as string:" << PAR( transResult->getErrorString() ) );
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION( "Set security successful!" );
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR( Peripheral type, setSecurityRequest.PeripheralType() )
          << NAME_PAR( Node address, setSecurityRequest.NodeAddress() )
          << NAME_PAR( Command, (int)setSecurityRequest.PeripheralCommand() )
        );
        // Add transaction
        writeTrConfResult.addTransactionResult( transResult );
        TRC_FUNCTION_LEAVE( "" );
      }
      catch ( std::exception& e )
      {
        WriteTrConfError error( WriteTrConfError::Type::WriteTrConfByte, e.what() );
        writeTrConfResult.setError( error );
        writeTrConfResult.addTransactionResult( transResult );
        THROW_EXC( std::logic_error, e.what() );
      }
    }

    // Convert bitmap returned by FrcAcknowledgedBroadcastBits to Node address array (containing Nodes that set bit0)
    std::basic_string<uint8_t> bitmapToNodes( const TPerFrcSend_Response frcResponse )
    {
      std::basic_string<uint8_t> nodesListBit0;
      nodesListBit0.clear();
      for ( uint8_t i = 1; i < MAX_ADDRESS; i++ )
        if ( frcResponse.FrcData[i / 8] & ( 1 << ( i % 8 ) ) )
          nodesListBit0.push_back( i );
      return nodesListBit0;
    }

    // Send WriteTrConfResult
    void sendResult( WriteTrConfResult &writeTrConfResult )
    {
      Document writeResult;

      // Set common parameters
      Pointer( "/mType" ).Set( writeResult, m_msgType->m_type );
      Pointer( "/data/msgId" ).Set( writeResult, m_comWriteConfig->getMsgId() );

      // Add writeTrConf result
      Pointer( "/data/rsp/deviceAddr" ).Set( writeResult, m_writeTrConfParams.deviceAddress );
      if ( writeTrConfResult.getError().getType() == WriteTrConfError::Type::NoError )
        Pointer( "/data/rsp/writeSuccess" ).Set( writeResult, true );
      else
        Pointer( "/data/rsp/writeSuccess" ).Set( writeResult, false );
      Pointer( "/data/rsp/restartNeeded" ).Set( writeResult, m_writeTrConfParams.restartNeeded );

      // Set raw fields, if verbose mode is active
      if ( m_comWriteConfig->getVerbose() == true )
      {
        rapidjson::Value rawArray( kArrayType );
        Document::AllocatorType& allocator = writeResult.GetAllocator();

        while ( writeTrConfResult.isNextTransactionResult() )
        {
          std::unique_ptr<IDpaTransactionResult2> transResult = writeTrConfResult.consumeNextTransactionResult();
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
        Pointer( "/data/raw" ).Set( writeResult, rawArray );
      }

      // Set status
      int status = writeTrConfResult.getError().getType() == WriteTrConfError::Type::NoError ? 0 : SERVICE_ERROR + (int)writeTrConfResult.getError().getType();
      Pointer( "/data/status" ).Set( writeResult, status );
      Pointer( "/data/statusStr" ).Set( writeResult, writeTrConfResult.getError().getMessage() );

      // Send message      
      m_iMessagingSplitterService->sendMessage( *m_messagingId, std::move( writeResult ) );
    }

    // Write TR configuration
    void writeTrConf( void )
    {
      TRC_FUNCTION_ENTER( "" );

      // WriteTrConfResult
      WriteTrConfResult writeTrConfResult;

      try
      {
        // Get configuration bytes from input parameters
        std::vector<TrConfigByte> trConfigBytes;
        trConfigBytes.clear();

        // Embedded peripherals (address 0x01 - 0x04)
        for ( uint8_t i = 0; i < PNUM_USER / 8; i++ )
        {
          // embPers specified in request ?
          if ( m_writeTrConfParams.embPers.maskBytes[i] != 0x00 )
          {
            // Yes, add to trConfigBytes
            TrConfigByte embPers( CFGIND_DPA_PERIPHERALS + i, m_writeTrConfParams.embPers.valueBytes[i], m_writeTrConfParams.embPers.maskBytes[i] );
            trConfigBytes.push_back( embPers );
          }
        }

        // DPA configuration bits (address 0x05)
        if ( m_writeTrConfParams.dpaConfigBits.mask != 0x00 )
        {
          TrConfigByte dpaConfigBits( CFGIND_DPA_FLAGS, m_writeTrConfParams.dpaConfigBits.value, m_writeTrConfParams.dpaConfigBits.mask );
          trConfigBytes.push_back( dpaConfigBits );
        }

        // Subordinate network channels for DPA 3.03 and DPA 3.04
        IIqrfDpaService::CoordinatorParameters coordParams = m_iIqrfDpaService->getCoordinatorParameters();
        if ( ( coordParams.dpaVerWord == 0x0303 ) || ( coordParams.dpaVerWord == 0x0304 ) )
        {
          // Main RF channel A of the optional subordinate network
          if ( m_writeTrConfParams.rfSettings.rfSubChannelA != -1 )
          {
            TrConfigByte rfSubChannelA( CFGIND_CHANNEL_2ND_A, (uint8_t)m_writeTrConfParams.rfSettings.rfSubChannelA, 0xff );
            trConfigBytes.push_back( rfSubChannelA );
          }

          // Main RF channel B of the optional subordinate network
          if ( m_writeTrConfParams.rfSettings.rfSubChannelB != -1 )
          {
            TrConfigByte rfSubChannelB( CFGIND_CHANNEL_2ND_B, (uint8_t)m_writeTrConfParams.rfSettings.rfSubChannelB, 0xff );
            trConfigBytes.push_back( rfSubChannelB );
          }
        }

        // RF output power (address 0x08)
        if ( m_writeTrConfParams.rfSettings.txPower != -1 )
        {
          TrConfigByte rfOutputPower( CFGIND_TXPOWER, (uint8_t)m_writeTrConfParams.rfSettings.txPower, 0x07 );
          trConfigBytes.push_back( rfOutputPower );
        }

        // RF signal filter (address 0x09)
        if ( m_writeTrConfParams.rfSettings.rxFilter != -1 )
        {
          TrConfigByte rxFilter( CFGIND_RXFILTER, (uint8_t)m_writeTrConfParams.rfSettings.rxFilter, 0xff );
          trConfigBytes.push_back( rxFilter );
        }

        // Timeout for receiving RF packets at LP-RX mode at LP [N] (address 0x0a)
        if ( m_writeTrConfParams.rfSettings.lpRxTimeout != -1 )
        {
          TrConfigByte lpRxTimeout( CFGIND_DPA_LP_TOUTRF, (uint8_t)m_writeTrConfParams.rfSettings.lpRxTimeout, 0xff );
          trConfigBytes.push_back( lpRxTimeout );
        }

        // Baud rate of the UART interface or the UART peripheral (address 0x0b)
        if ( m_writeTrConfParams.uartBaudRate != -1 )
        {
          TrConfigByte uartBaudRate( CFGIND_DPA_UART_IFACE_SPEED, (uint8_t)m_writeTrConfParams.uartBaudRate, 0xff );
          trConfigBytes.push_back( uartBaudRate );
        }

        // A nonzero value specifies an alternative DPA service mode channel (address 0x0c)
        if ( m_writeTrConfParams.rfSettings.rfAltDsmChannel != -1 )
        {
          TrConfigByte rfAltDsmChannel( CFGIND_ALTERNATE_DSM_CHANNEL, (uint8_t)m_writeTrConfParams.rfSettings.rfAltDsmChannel, 0xff );
          trConfigBytes.push_back( rfAltDsmChannel );
        }

        // Main RF channel A of the main network (address 0x11)
        if ( m_writeTrConfParams.rfSettings.rfChannelA != -1 )
        {
          TrConfigByte rfChannelA( CFGIND_CHANNEL_A, (uint8_t)m_writeTrConfParams.rfSettings.rfChannelA, 0xff );
          trConfigBytes.push_back( rfChannelA );
        }

        // Main RF channel B of the main network (address 0x12)
        if ( m_writeTrConfParams.rfSettings.rfChannelB != -1 )
        {
          TrConfigByte rfChannelB( CFGIND_CHANNEL_B, (uint8_t)m_writeTrConfParams.rfSettings.rfChannelB, 0xff );
          trConfigBytes.push_back( rfChannelB );
        }

        // RFPGM (address 0x20)
        if ( m_writeTrConfParams.RFPGM.mask != 0x00 )
        {
          TrConfigByte RFPGM( 0x20, m_writeTrConfParams.RFPGM.value, m_writeTrConfParams.RFPGM.mask );
          trConfigBytes.push_back( RFPGM );
        }

        // Check, if Coordinator and OS peripherals are present at coordinator's
        checkPresentCoordAndCoordOs( writeTrConfResult );

        // Set FRC param to 0, store previous value
        uint8_t frcResponseTime = 0;
        frcResponseTime = setFrcReponseTime( writeTrConfResult, frcResponseTime );

        // Unicats address ?
        if ( m_writeTrConfParams.deviceAddress != BROADCAST_ADDRESS )
        {
          // Any TR configuration byte to set ?
          if ( trConfigBytes.size() != 0 )
            writeTrConfUnicast( writeTrConfResult, m_writeTrConfParams.deviceAddress, m_writeTrConfParams.hwpId, trConfigBytes );

          // Set Access pasword
          if ( m_writeTrConfParams.security.accessPassword.length() != 0 )
            setSecurityUnicast( writeTrConfResult, m_writeTrConfParams.deviceAddress, m_writeTrConfParams.hwpId, 0x00, m_writeTrConfParams.security.accessPassword );

          // Set User key
          if ( m_writeTrConfParams.security.userKey.length() != 0 )
            setSecurityUnicast( writeTrConfResult, m_writeTrConfParams.deviceAddress, m_writeTrConfParams.hwpId, 0x01, m_writeTrConfParams.security.userKey );
        }
        else
        {
          // Broadcast address, any TR configuration byte ?
          if ( trConfigBytes.size() != 0 )
          {
            uint8_t confBytesIndex = 0;
            std::basic_string<uint8_t> frcUserData;
            do
            {
              // Fill OS Write Configuration byte request
              frcUserData.clear();
              frcUserData.push_back( 0x05 );
              frcUserData.push_back( PNUM_OS );
              frcUserData.push_back( CMD_OS_WRITE_CFG_BYTE );
              frcUserData.push_back( m_writeTrConfParams.hwpId & 0xff );
              frcUserData.push_back( m_writeTrConfParams.hwpId > 0x08 );
              do
              {
                // Fill current TPerOSWriteCfgByteTriplet
                frcUserData.push_back( trConfigBytes[confBytesIndex].address );
                frcUserData.push_back( trConfigBytes[confBytesIndex].value );
                frcUserData.push_back( trConfigBytes[confBytesIndex].mask );
                // Add TPerOSWriteCfgByteTriplet length
                frcUserData[0x00] += sizeof( TPerOSWriteCfgByteTriplet );
                // Erase consumed TrConfigByte
                trConfigBytes.erase( trConfigBytes.begin() );
              } while ( ( ++confBytesIndex < 8 ) && ( trConfigBytes.size() != 0 ) );
              // Send FrcAcknowledgedBroadcastBits
              FrcAcknowledgedBroadcastBits( writeTrConfResult, frcUserData );
            } while ( trConfigBytes.size() != 0 );
          }

          // Set Access pasword
          if ( m_writeTrConfParams.security.accessPassword.length() != 0 )
          {
            // Fill OS Set security request
            std::basic_string<uint8_t> frcUserData;
            frcUserData.clear();
            frcUserData.push_back( 0x06 + m_writeTrConfParams.security.accessPassword.length() );
            frcUserData.push_back( PNUM_OS );
            frcUserData.push_back( CMD_OS_SET_SECURITY );
            frcUserData.push_back( m_writeTrConfParams.hwpId & 0xff );
            frcUserData.push_back( m_writeTrConfParams.hwpId > 0x08 );
            // Type 0x00 - Sets an access password
            frcUserData.push_back( 0x00 );
            std::copy( m_writeTrConfParams.security.accessPassword.begin(), m_writeTrConfParams.security.accessPassword.end(), frcUserData.begin() + frcUserData.size() );
            // Send FrcAcknowledgedBroadcastBits
            FrcAcknowledgedBroadcastBits( writeTrConfResult, frcUserData );
          }

          // Set User key
          if ( m_writeTrConfParams.security.userKey.length() != 0 )
          {
            // Fill OS Set security request
            std::basic_string<uint8_t> frcUserData;
            frcUserData.clear();
            frcUserData.push_back( 0x06 + m_writeTrConfParams.security.accessPassword.length() );
            frcUserData.push_back( PNUM_OS );
            frcUserData.push_back( CMD_OS_SET_SECURITY );
            frcUserData.push_back( m_writeTrConfParams.hwpId & 0xff );
            frcUserData.push_back( m_writeTrConfParams.hwpId > 0x08 );
            // Type 0x01 - Sets a user key
            frcUserData.push_back( 0x01 );
            std::copy( m_writeTrConfParams.security.userKey.begin(), m_writeTrConfParams.security.userKey.end(), frcUserData.begin() + frcUserData.size() );
            // Send FrcAcknowledgedBroadcastBits
            FrcAcknowledgedBroadcastBits( writeTrConfResult, frcUserData );
          }
        }

        // Set initial FRC param
        if ( frcResponseTime != 0 )
          frcResponseTime = setFrcReponseTime( writeTrConfResult, frcResponseTime );

        // Evaluate restartNeeded flag
        // SPI and UART peripherals need restart
        if ( m_writeTrConfParams.embPers.mask & ( ( 1 << PNUM_SPI ) | ( 1 << PNUM_UART ) ) != 0 )
          m_writeTrConfParams.restartNeeded = true;

        // dpaConfigBits need restart
        if ( m_writeTrConfParams.dpaConfigBits.mask != 0 )
          m_writeTrConfParams.restartNeeded = true;

        // lpRxTimeout and uartBaudRate need restart
        if ( ( m_writeTrConfParams.rfSettings.lpRxTimeout != -1 ) || ( m_writeTrConfParams.uartBaudRate != -1 ) )
          m_writeTrConfParams.restartNeeded = true;

        // RF signal filter needs restart for DPA < 4.12
        if ( coordParams.dpaVerWord < 0x0412 )
        {
          if ( m_writeTrConfParams.rfSettings.rxFilter != -1 )
            m_writeTrConfParams.restartNeeded = true;
        }

        // Main RF channel A and RF output power needs restart for DPA < 3.02
        if ( ( coordParams.dpaVerWord < 0x0300 ) || ( coordParams.dpaVerWord < 0x0301 ) )
        {
          if ( ( m_writeTrConfParams.rfSettings.rfChannelA != -1 ) || ( m_writeTrConfParams.rfSettings.txPower != -1 ) )
            m_writeTrConfParams.restartNeeded = true;
        }

        // Send result
        sendResult( writeTrConfResult );
        TRC_FUNCTION_LEAVE( "" );
      }
      catch ( std::exception& ex )
      {
        TRC_WARNING( "Error during algorithm run: " << ex.what() );
        // Send result
        sendResult( writeTrConfResult );
      }
    }

    // Handle message
    void handleMsg( const std::string &messagingId, const IMessagingSplitterService::MsgType &msgType, rapidjson::Document doc )
    {
      TRC_FUNCTION_ENTER( PAR( messagingId ) << NAME_PAR( mType, msgType.m_type ) << NAME_PAR( major, msgType.m_major ) << NAME_PAR( minor, msgType.m_minor ) << NAME_PAR( micro, msgType.m_micro ) );

      // Unsupported type of request
      if ( msgType.m_type != m_mTypeName_iqmeshNetwork_WriteTrConf )
        THROW_EXC( std::out_of_range, "Unsupported message type: " << PAR( msgType.m_type ) );

      // Creating representation object
      ComMngIqmeshWriteConfig comWriteConfig( doc );

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
        Pointer( "/data/msgId" ).Set( response, comWriteConfig.getMsgId() );
        // Set result
        Pointer( "/data/status" ).Set( response, SERVICE_ERROR );
        Pointer( "/data/statusStr" ).Set( response, errorStr );
        m_iMessagingSplitterService->sendMessage( messagingId, std::move( response ) );

        TRC_FUNCTION_LEAVE( "" );
        return;
      }

      // Parsing and checking service parameters
      try
      {
        m_writeTrConfParams = comWriteConfig.getWriteTrConfParams();
      }
      catch ( std::exception& e )
      {
        const char* errorStr = e.what();
        TRC_WARNING( "Error while parsing service input parameters: " << PAR( errorStr ) );
        // Create error response
        Document response;
        Pointer( "/mType" ).Set( response, msgType.m_type );
        Pointer( "/data/msgId" ).Set( response, comWriteConfig.getMsgId() );
        // Set result
        Pointer( "/data/status" ).Set( response, SERVICE_ERROR );
        Pointer( "/data/statusStr" ).Set( response, errorStr );
        m_iMessagingSplitterService->sendMessage( messagingId, std::move( response ) );

        TRC_FUNCTION_LEAVE( "" );
        return;
      }

      // Write TR configuration
      m_msgType = &msgType;
      m_messagingId = &messagingId;
      m_comWriteConfig = &comWriteConfig;
      writeTrConf();
      // Release exclusive access
      m_exclusiveAccess.reset();

      TRC_FUNCTION_LEAVE( "" );
    }

  public:
    void activate( const shape::Properties *props )
    {
      TRC_FUNCTION_ENTER( "" );
      TRC_INFORMATION( std::endl
                       << "************************************" << std::endl
                       << "WriteTrConfService instance activate" << std::endl
                       << "************************************" );

      // for the sake of register function parameters
      std::vector<std::string> supportedMsgTypes =
      {
          m_mTypeName_iqmeshNetwork_WriteTrConf };

      m_iMessagingSplitterService->registerFilteredMsgHandler(
        supportedMsgTypes,
        [&]( const std::string &messagingId, const IMessagingSplitterService::MsgType &msgType, rapidjson::Document doc ) {
        handleMsg( messagingId, msgType, std::move( doc ) );
      } );

      TRC_FUNCTION_LEAVE( "" );
    }

    void deactivate()
    {
      TRC_FUNCTION_ENTER( "" );
      TRC_INFORMATION( std::endl
                       << "**************************************" << std::endl
                       << "WriteTrConfService instance deactivate" << std::endl
                       << "**************************************" );

      // for the sake of unregister function parameters
      std::vector<std::string> supportedMsgTypes =
      {
          m_mTypeName_iqmeshNetwork_WriteTrConf };

      m_iMessagingSplitterService->unregisterFilteredMsgHandler( supportedMsgTypes );

      TRC_FUNCTION_LEAVE( "" );
    }

    void modify( const shape::Properties *props )
    {
    }

    void attachInterface( IIqrfDpaService *iface )
    {
      m_iIqrfDpaService = iface;
    }

    void detachInterface( IIqrfDpaService *iface )
    {
      if ( m_iIqrfDpaService == iface )
      {
        m_iIqrfDpaService = nullptr;
      }
    }

    void attachInterface( IMessagingSplitterService *iface )
    {
      m_iMessagingSplitterService = iface;
    }

    void detachInterface( IMessagingSplitterService *iface )
    {
      if ( m_iMessagingSplitterService == iface )
      {
        m_iMessagingSplitterService = nullptr;
      }
    }
  };

  WriteTrConfService::WriteTrConfService()
  {
    m_imp = shape_new Imp( *this );
  }

  WriteTrConfService::~WriteTrConfService()
  {
    delete m_imp;
  }

  void WriteTrConfService::attachInterface( iqrf::IIqrfDpaService *iface )
  {
    m_imp->attachInterface( iface );
  }

  void WriteTrConfService::detachInterface( iqrf::IIqrfDpaService *iface )
  {
    m_imp->detachInterface( iface );
  }

  void WriteTrConfService::attachInterface( iqrf::IMessagingSplitterService *iface )
  {
    m_imp->attachInterface( iface );
  }

  void WriteTrConfService::detachInterface( iqrf::IMessagingSplitterService *iface )
  {
    m_imp->detachInterface( iface );
  }

  void WriteTrConfService::attachInterface( shape::ITraceService *iface )
  {
    shape::Tracer::get().addTracerService( iface );
  }

  void WriteTrConfService::detachInterface( shape::ITraceService *iface )
  {
    shape::Tracer::get().removeTracerService( iface );
  }

  void WriteTrConfService::activate( const shape::Properties *props )
  {
    m_imp->activate( props );
  }

  void WriteTrConfService::deactivate()
  {
    m_imp->deactivate();
  }

  void WriteTrConfService::modify( const shape::Properties *props )
  {
    m_imp->modify( props );
  }
}
