#define IOtaUploadService_EXPORTS

#include "OtaUploadService.h"
#include "DataPreparer.h"
#include "Trace.h"
#include "ComIqmeshNetworkOtaUpload.h"
//#include "ObjectFactory.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"

#include "iqrf__OtaUploadService.hxx"

#include <list>
#include <memory>
#include <math.h>
#include <bitset>
#include <sys/types.h>
#include <sys/stat.h>

TRC_INIT_MODULE(iqrf::OtaUploadService); 


using namespace rapidjson;

namespace {

  // maximum number of repeats
  static const uint8_t REPEAT_MAX = 3;

  // loading code action
  enum class LoadingAction {
    Upload,
    Verify,
    Load
  };

  // service general fail code - may and probably will be changed later in the future
  static const int SERVICE_ERROR = 1000;
  static const int SERVICE_ERROR_NOERROR = 0;
  static const int SERVICE_ERROR_INTERNAL = SERVICE_ERROR + 1;
  static const int SERVICE_ERROR_UNSUPPORTED_LOADING_CONTENT = SERVICE_ERROR + 2;
  static const int SERVICE_ERROR_DATA_PREPARE = SERVICE_ERROR + 3;
  static const int SERVICE_GET_BONDED_NODES = SERVICE_ERROR + 4;
  static const int SERVICE_ERROR_UPLOAD = SERVICE_ERROR + 5;
  static const int SERVICE_ERROR_VERIFY = SERVICE_ERROR + 6;
  static const int SERVICE_ERROR_LOAD = SERVICE_ERROR + 7;
};


namespace iqrf {

  // Holds information about errors, which encounter during upload
  class UploadError {
  public:

    enum class Type {
      NoError,
      UnsupportedLoadingContent,
      DataPrepare,
      GetBondedNodes,
      Upload,
      Verify,
      Load
    };

    UploadError() : m_type( Type::NoError ) {};
    UploadError( Type errorType ) : m_type( errorType ), m_message( "" ) {};
    UploadError( Type errorType, const std::string& message ) : m_type( errorType ), m_message( message ) {};

    Type getType() const { return m_type; };
    std::string getMessage() const { return m_message; };

    UploadError& operator=( const UploadError& error )
    {
      if ( this != &error )
      {
        this->m_type = error.m_type;
        this->m_message = error.m_message;
      }
      return *this;
    }

  private:
    Type m_type;
    std::string m_message;
  };


  class UploadResult {
  public:
    UploadResult() = delete;
    UploadResult( const uint16_t address, const uint16_t hwpId, LoadingAction action )
    {
      m_deviceAddress = address;
      m_hwpId = hwpId;
      m_loadingAction = action;
      m_nodesList.clear();
      m_verifyResultMap.clear();
      m_loadResultMap.clear();      
    }

  private:
    UploadError m_error;

    // Device address
    uint16_t m_deviceAddress;

    // Device hwpId
    uint16_t m_hwpId;

    // Loading action
    LoadingAction m_loadingAction;

    // Upload results
    bool m_uploadResult;

    std::list<uint16_t> m_nodesList;
    
    // Map of verify results
    std::map<uint16_t, bool> m_verifyResultMap;

    // Map of load result
    std::map<uint16_t, bool> m_loadResultMap;

    // List of transaction results
    std::list<std::unique_ptr<IDpaTransactionResult2>> m_transResults;

  public:
    UploadError getError() const { return m_error; };

    void setError( const UploadError& error ) {
      m_error = error;
    }

    uint16_t getDeviceAddress() const
    {
      return m_deviceAddress;
    }

    uint16_t getHwpId() const
    {
      return m_hwpId;
    }

    LoadingAction getLoadingAction() const
    {
      return m_loadingAction;
    }

    void putUploadResult(const bool result )
    {
      m_uploadResult = result;
    }

    bool getUploadResult() const
    {
      return m_uploadResult;
    }

    const std::list<uint16_t>& getNodesList() const
    {
      return m_nodesList;
    }

    const std::map<uint16_t, bool>& getVerifyResultsMap() const
    {
      return m_verifyResultMap;
    }

    const std::map<uint16_t, bool>& getLoadResultsMap() const
    {
      return m_loadResultMap;
    }

    void setNodesList( const std::list<uint16_t>& nodesList )
    {
      m_nodesList = nodesList;
    }

    bool getVerifyResult( const uint16_t address )
    {
      return m_verifyResultMap.find( address )->second;
    }

    void setVerifyResultsMap( const uint16_t address, const bool result )
    {
      m_verifyResultMap[address] = result;
    }

    void setLoadResultsMap( const uint16_t address, const bool result )
    {
      m_loadResultMap[address] = result;
    }

    // adds transaction result into the list of results
    void addTransactionResult( std::unique_ptr<IDpaTransactionResult2>& transResult ) 
    {
      m_transResults.push_back( std::move( transResult ) );
    }

    bool isNextTransactionResult()
    {
      return ( m_transResults.size() > 0 );
    }

    // consumes the first element in the transaction results list
    std::unique_ptr<IDpaTransactionResult2> consumeNextTransactionResult() 
    {
      std::list<std::unique_ptr<IDpaTransactionResult2>>::iterator iter = m_transResults.begin();
      std::unique_ptr<IDpaTransactionResult2> tranResult = std::move( *iter );
      m_transResults.pop_front();
      return std::move( tranResult );
    }
  };

  // implementation class
  class OtaUploadService::Imp {
  private:
    // parent object
    OtaUploadService & m_parent;

    // message type: IQMESH Network OTA Upload
    // for temporal reasons
    const std::string m_mTypeName_iqmeshNetworkOtaUpload = "iqmeshNetwork_OtaUpload";
    //IMessagingSplitterService::MsgType* m_msgType_mngIqmeshWriteConfig;

    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    IIqrfDpaService* m_iIqrfDpaService = nullptr;
    std::unique_ptr<IIqrfDpaService::ExclusiveAccess> m_exclusiveAccess;

    // number of repeats
    uint8_t m_repeat;

    // if is set Verbose mode
    bool m_returnVerbose = false;

    // absolute path with hex file to upload
    std::string m_uploadPath;

  public:
    Imp( OtaUploadService& parent ) : m_parent( parent )
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

    // returns file suffix
    std::string getFileSuffix( const std::string& fileName ) {
      size_t dotPos = fileName.find_last_of( '.' );
      if ( ( dotPos == std::string::npos ) || ( dotPos == ( fileName.length() - 1 ) ) ) {
        THROW_EXC( std::logic_error, "Bad file name format - no suffix" );
      }

      return fileName.substr( dotPos + 1 );
    }

    // encounters type of loading content
    // TODO
    IOtaUploadService::LoadingContentType parseLoadingContentType( const std::string& fileName )
    {
      std::string fileSuffix = getFileSuffix( fileName );

      if ( fileSuffix == "hex" ) {
        return IOtaUploadService::LoadingContentType::Hex;
      }

      if ( fileSuffix == "iqrf" ) {
        return IOtaUploadService::LoadingContentType::Iqrf_plugin;
      }

      THROW_EXC( std::logic_error, "Not implemented." );
    }

    void processUploadError( UploadResult& uploadResult, UploadError::Type errType, const std::string& errMsg )
    {
      uploadResult.putUploadResult( false );
      uploadResult.setError( UploadError( errType, errMsg ) );
    }

    // Parses bit array of bonded nodes
    std::list<uint16_t> getNodesFromBitmap( const unsigned char* pData )
    {
      std::list<uint16_t> nodes;
      nodes.clear();

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
            nodes.push_back( byteId * 8 + bitId );
          }
          bitComp <<= 1;
        }
      }

      return nodes;
    }

    // Send DPA request
    DpaMessage sendDPAcommand( const DpaMessage::DpaPacket_t &requestPacket, const uint8_t length, UploadResult &uploadResult )
    {
      TRC_FUNCTION_ENTER( "" );

      // Prepare the DPA request
      DpaMessage request;
      request.DataToBuffer( requestPacket.Buffer, length );

      // Send the request
      IDpaTransactionResult2::ErrorCode errorCode;
      std::shared_ptr<IDpaTransaction2> transaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;
      DpaMessage dpaResponse;

      for ( int rep = 0; rep <= m_repeat; rep++ )
      {
        try
        {
          transaction = m_exclusiveAccess->executeDpaTransaction( request );          
          transResult = transaction->get();
          dpaResponse = transResult->getResponse();
          errorCode = ( IDpaTransactionResult2::ErrorCode )transResult->getErrorCode();
          if ( errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK )
          {
            uploadResult.addTransactionResult( transResult );
            TRC_DEBUG( "DPA transaction OK." );
            TRC_FUNCTION_LEAVE( "" );
            return dpaResponse;
          }
        }
        catch ( std::exception& e )
        {
          TRC_DEBUG( "DPA transaction error : " << e.what() );
          if ( rep < m_repeat )
            continue;
        }
      }

      // Evaluate error
      uploadResult.addTransactionResult( transResult );
      std::string errorStr;
      if ( errorCode < 0 )
        errorStr = "Transaction error.";
      else
        errorStr = "Dpa error.";
      TRC_DEBUG( errorStr << NAME_PAR_HEX( " Error code", errorCode ) );
      TRC_FUNCTION_LEAVE( "" );
      // Throw exception
      THROW_EXC( std::logic_error, errorStr );
    }

    // Set RFC params
    IDpaTransaction2::FrcResponseTime setFRCparams( IDpaTransaction2::FrcResponseTime responseTime, UploadResult& uploadResult )
    {
      TRC_FUNCTION_ENTER( "" );

      // Send the DPA request
      DpaMessage::DpaPacket_t setFrcParamsPacket;
      setFrcParamsPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      setFrcParamsPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
      setFrcParamsPacket.DpaRequestPacket_t.PCMD = CMD_FRC_SET_PARAMS;
      setFrcParamsPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
      setFrcParamsPacket.DpaRequestPacket_t.DpaMessage.PerFrcSetParams_RequestResponse.FRCresponseTime = responseTime;
      DpaMessage dpaResponse = sendDPAcommand( setFrcParamsPacket, sizeof( TDpaIFaceHeader ) + sizeof( TPerFrcSetParams_RequestResponse ), uploadResult );

      TRC_INFORMATION( "Set FRC params successful!" );
      // Return bonded nodes
      TRC_FUNCTION_LEAVE( "" );
      return ( IDpaTransaction2::FrcResponseTime ) dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSetParams_RequestResponse.FRCresponseTime;
    }

    // Returns list of bonded nodes
    std::list<uint16_t> getBondedNodes( UploadResult &uploadResult )
    {
      TRC_FUNCTION_ENTER( "" );

      // Send DPA request
      DpaMessage::DpaPacket_t getBondedNodesPacket;
      getBondedNodesPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      getBondedNodesPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
      getBondedNodesPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_BONDED_DEVICES;
      getBondedNodesPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
      DpaMessage dpaResponse = sendDPAcommand( getBondedNodesPacket, sizeof( TDpaIFaceHeader ), uploadResult );

      TRC_INFORMATION( "Get bonded nodes successful!" );
      // Return bonded nodes
      TRC_FUNCTION_LEAVE( "" );
      return getNodesFromBitmap( dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData );
    }

    // Sets PData of specified EEEPROM extended write packet
    void setExtendedWritePacketData( DpaMessage::DpaPacket_t& packet, uint16_t address, const std::basic_string<uint8_t>& data )
    {
      uint8_t* pData = packet.DpaRequestPacket_t.DpaMessage.Request.PData;
      pData[0] = address & 0xFF;
      pData[1] = ( address >> 8 ) & 0xFF;

      for ( int i = 0; i < data.size(); i++ )
        pData[i + 2] = data[i];
    }

    // size of the embedded write packet INCLUDING target address, which to write data to
    const uint8_t EMB_WRITE_PACKET_HEADER_SIZE = sizeof( TDpaIFaceHeader ) - 1 + 2;

    // sets specified EEEPROM extended write packet
    void setExtWritePacket(
      DpaMessage::DpaPacket_t& packet,
      const uint16_t address,
      const std::basic_string<uint8_t>& data,
      const uint16_t nodeAddr
    )
    {
      setExtendedWritePacketData( packet, address, data );
      packet.DpaRequestPacket_t.NADR = nodeAddr;
    }

    // adds embedded write packet data into batch data
    void addEmbeddedWritePacket(
      uint8_t* batchPacketPData,
      const uint16_t address,
      const uint16_t hwpId,
      const std::basic_string<uint8_t>& data,
      const uint8_t offset
    )
    {
      // length
      batchPacketPData[offset] = EMB_WRITE_PACKET_HEADER_SIZE + data.size();
      batchPacketPData[offset + 1] = PNUM_EEEPROM;
      batchPacketPData[offset + 2] = CMD_EEEPROM_XWRITE;
      batchPacketPData[offset + 3] = hwpId & 0xFF;
      batchPacketPData[offset + 4] = ( hwpId >> 8 ) & 0xFF;
      batchPacketPData[offset + 5] = address & 0xFF;
      batchPacketPData[offset + 6] = ( address >> 8 ) & 0xFF;
      for ( int i = 0; i < data.size(); i++ )
        batchPacketPData[offset + EMB_WRITE_PACKET_HEADER_SIZE + i] = data[i];
    }

    void writeDataToExtEEPROM(
      UploadResult& uploadResult,
      const uint16_t startMemAddress,
      const std::vector<std::basic_string<uint8_t>>& data
    )
    {
      TRC_FUNCTION_ENTER( "" );
      TRC_INFORMATION( "Creating DPA request to execute batch command." );

      uint16_t address = uploadResult.getDeviceAddress();
      uint16_t hwpId;

      // Use requested hwpId for broadcast packet
      if ( address == BROADCAST_ADDRESS )
        hwpId = uploadResult.getHwpId();
      else
        hwpId = HWPID_DoNotCheck;

      // eeeprom extended write packet
      DpaMessage::DpaPacket_t extendedWritePacket;
      extendedWritePacket.DpaRequestPacket_t.NADR = address;
      extendedWritePacket.DpaRequestPacket_t.PNUM = PNUM_EEEPROM;
      extendedWritePacket.DpaRequestPacket_t.PCMD = CMD_EEEPROM_XWRITE;
      extendedWritePacket.DpaRequestPacket_t.HWPID = hwpId;
      // PData will be specified inside loop in advance

      // batch packet
      DpaMessage::DpaPacket_t batchPacket;
      batchPacket.DpaRequestPacket_t.NADR = address;
      batchPacket.DpaRequestPacket_t.PNUM = PNUM_OS;
      batchPacket.DpaRequestPacket_t.PCMD = CMD_OS_BATCH;
      batchPacket.DpaRequestPacket_t.HWPID = hwpId;
      // PData will be specified inside loop in advance 

      uint16_t actualAddress = startMemAddress;
      size_t index = 0;

      while ( index < data.size() )
      {
        if ( ( index + 1 ) < data.size() )
        {
          if ( ( data[index].size() == 16 ) && ( data[index + 1].size() == 16 ) ) 
          {
            // delete previous batch request data
            uint8_t* batchRequestData = batchPacket.DpaRequestPacket_t.DpaMessage.Request.PData;
            memset( batchRequestData, 0, DPA_MAX_DATA_LENGTH );

            // add 1st embedded packet into PData of the BATCH
            addEmbeddedWritePacket( batchRequestData, actualAddress, hwpId, data[index], 0 );
            actualAddress += data[index].size();

            // Size of the first packet
            uint8_t firstPacketSize = EMB_WRITE_PACKET_HEADER_SIZE + data[index].size();

            // Add 2nd embedded packet into PData of the BATCH
            addEmbeddedWritePacket( batchRequestData, actualAddress, hwpId, data[index + 1], firstPacketSize );

            // Issue batch request
            uint8_t batchPacketLen = sizeof( TDpaIFaceHeader ) + 2 * EMB_WRITE_PACKET_HEADER_SIZE + data[index].size() + data[index + 1].size() + 1;
            DpaMessage dpaResponse = sendDPAcommand( batchPacket, batchPacketLen, uploadResult );

            TRC_INFORMATION( "Batch extended write done!" );
            actualAddress += data[index + 1].size();
            index += 2;
          }
          else
          {
            setExtWritePacket( extendedWritePacket, actualAddress, data[index], address );

            uint8_t extendedWritePacketLen = sizeof( TDpaIFaceHeader ) + 2 + data[index].size();
            DpaMessage dpaResponse = sendDPAcommand( extendedWritePacket, extendedWritePacketLen, uploadResult );

            TRC_INFORMATION( "Extended write done!" );
            actualAddress += data[index].size();
            index++;
          }
        }
        else
        {
          setExtWritePacket( extendedWritePacket, actualAddress, data[index], address );

          uint8_t extendedWriteRequestLen = sizeof( TDpaIFaceHeader ) + 2 + data[index].size();
          DpaMessage dpaResponse = sendDPAcommand( extendedWritePacket, extendedWriteRequestLen, uploadResult );

          TRC_INFORMATION( "Extended write done!" );
          actualAddress += data[index].size();
          index++;
        }
      }

      // write into node result
      uploadResult.putUploadResult( true );
    }

    // Get FRC extra result
    DpaMessage getFrcExtraResult( UploadResult& uploadResult )
    {
      //DpaMessage frcRequest;
      DpaMessage::DpaPacket_t frcExtraResultPacket;
      frcExtraResultPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      frcExtraResultPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
      frcExtraResultPacket.DpaRequestPacket_t.PCMD = CMD_FRC_EXTRARESULT;
      frcExtraResultPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
      // Send request
      DpaMessage dpaResponse = sendDPAcommand( frcExtraResultPacket, sizeof( TDpaIFaceHeader ), uploadResult );
      return( dpaResponse );
    }

    // Load code into into device using unicast request
    void loadCodeUnicast(
      const uint16_t startAddress,
      const LoadingAction loadingAction,
      const IOtaUploadService::LoadingContentType loadingContentType,
      const uint16_t length,
      const uint16_t checksum,
      UploadResult& uploadResult
    )
    {
      // Prepare unicast OS LoadCode request
      uint16_t deviceAddress = uploadResult.getDeviceAddress();
      DpaMessage::DpaPacket_t loadCodePacket;
      loadCodePacket.DpaRequestPacket_t.NADR = deviceAddress;
      loadCodePacket.DpaRequestPacket_t.PNUM = PNUM_OS;
      loadCodePacket.DpaRequestPacket_t.PCMD = CMD_OS_LOAD_CODE;
      loadCodePacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
      loadCodePacket.DpaRequestPacket_t.DpaMessage.PerOSLoadCode_Request.Flags = 0x00;
      if ( loadingAction == LoadingAction::Load )
        loadCodePacket.DpaRequestPacket_t.DpaMessage.PerOSLoadCode_Request.Flags |= 0x01;
      if ( loadingContentType == IOtaUploadService::LoadingContentType::Iqrf_plugin )
        loadCodePacket.DpaRequestPacket_t.DpaMessage.PerOSLoadCode_Request.Flags |= 0x02;
      loadCodePacket.DpaRequestPacket_t.DpaMessage.PerOSLoadCode_Request.Address = startAddress;
      loadCodePacket.DpaRequestPacket_t.DpaMessage.PerOSLoadCode_Request.CheckSum = checksum;
      loadCodePacket.DpaRequestPacket_t.DpaMessage.PerOSLoadCode_Request.Length = length;
      DpaMessage loadCodeResult = sendDPAcommand( loadCodePacket, sizeof( TDpaIFaceHeader ) + sizeof( TPerOSLoadCode_Request ), uploadResult );
      uint8_t result = loadCodeResult.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData[0];
      if ( loadingAction == LoadingAction::Load )
        uploadResult.setLoadResultsMap( deviceAddress, result == 0x01 );
      else
        uploadResult.setVerifyResultsMap( deviceAddress, result == 0x01 );
      TRC_INFORMATION( "Load code done!" );
      TRC_FUNCTION_LEAVE( "" );
    }

    // Verify code using selective FRC (Memory read plus 1)
    void verifyCode(
      const uint16_t startAddress,
      const LoadingAction loadingAction,
      const IOtaUploadService::LoadingContentType loadingContentType,
      const uint16_t length,
      const uint16_t checksum,
      UploadResult& uploadResult
    )
    {
      DpaMessage::DpaPacket_t frcPacket;
      uint16_t hwpId = uploadResult.getHwpId();
      std::list<uint16_t> nodesList;
      nodesList.clear();
      uint8_t frcStatus;

      
      if ( hwpId == HWPID_DoNotCheck )
      {       
        // Verify code at all bonded nodes
        nodesList = getBondedNodes( uploadResult );
      }
      else
      {
        // Verify code at nodes with selected hwpId only
        frcPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        frcPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
        frcPacket.DpaRequestPacket_t.PCMD = CMD_FRC_SEND;
        frcPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        frcPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.FrcCommand = FRC_AcknowledgedBroadcastBits;
        frcPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[0] = 0x05;
        frcPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[1] = PNUM_OS;
        frcPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[2] = CMD_OS_READ;
        frcPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[3] = hwpId & 0xff;
        frcPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[4] = hwpId >> 0x08;
        DpaMessage readOSInfo = sendDPAcommand( frcPacket, sizeof( TDpaIFaceHeader ) + 6, uploadResult );
        // Check FRC status
        frcStatus = readOSInfo.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.Status;
        if ( ( frcStatus >= 0x00 ) && ( frcStatus <= 0xEF ) )
        {
          TRC_INFORMATION( "FRC Read OS Info successful." );
          TRC_FUNCTION_LEAVE( "" );
          nodesList = getNodesFromBitmap( readOSInfo.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.FrcData );
        }
        else
        {
          TRC_DEBUG( "FRC Read OS Info failed." << NAME_PAR( "Status", frcStatus ) );
          TRC_FUNCTION_LEAVE( "" );
          // Throw exception
          std::string errorStr = "FRC Read OS Info failed: " + std::to_string( frcStatus );
          THROW_EXC( std::logic_error, errorStr );
        }
      }
      uploadResult.setNodesList( nodesList );
      
      // Set FRC params according to file type and length
      IDpaTransaction2::FrcResponseTime frcResponsTimeOTA;
      if ( loadingContentType == LoadingContentType::Hex )
      {
        if ( length > 0x2700 )
          frcResponsTimeOTA = IDpaTransaction2::FrcResponseTime::k5160Ms;
        else
          if ( length > 0x0F00 )
            frcResponsTimeOTA = IDpaTransaction2::FrcResponseTime::k2600Ms;
          else
            frcResponsTimeOTA = IDpaTransaction2::FrcResponseTime::k1320Ms;
      }
      else
      {
        if ( length > 0x3100 )
          frcResponsTimeOTA = IDpaTransaction2::FrcResponseTime::k20620Ms;
        else
          if ( length > 0x1500 )
            frcResponsTimeOTA = IDpaTransaction2::FrcResponseTime::k10280Ms;
          else
            if ( length > 0x0B00 )
              frcResponsTimeOTA = IDpaTransaction2::FrcResponseTime::k5160Ms;
            else
              frcResponsTimeOTA = IDpaTransaction2::FrcResponseTime::k2600Ms;
      }

      // Set calculated FRC response time
      m_iIqrfDpaService->setFrcResponseTime( frcResponsTimeOTA );
      setFRCparams( frcResponsTimeOTA, uploadResult );

      // Verify the code at selected nodes
      while ( nodesList.size() > 0 )
      {        
        frcPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        frcPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
        frcPacket.DpaRequestPacket_t.PCMD = CMD_FRC_SEND_SELECTIVE;
        frcPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        // Initialize command to FRC_MemoryReadPlus1
        frcPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.FrcCommand = FRC_MemoryReadPlus1;
        // Initialize SelectedNodes
        memset( frcPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.SelectedNodes, 0, 30 * sizeof( uint8_t ) );
        std::list<uint16_t> selectedNodes;
        selectedNodes.clear();
        do
        {
          uint16_t addr = nodesList.front();
          nodesList.pop_front();
          selectedNodes.push_back( addr );
          uint8_t byteIndex = addr / 8;
          uint8_t bitIndex = addr % 8;
          frcPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.SelectedNodes[byteIndex] |= ( 0x01 << bitIndex );                    
        } while ( nodesList.size() != 0 && selectedNodes.size() < 63 );        
        // Initialize user data to zero
        memset( frcPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData, 0, 25 * sizeof( uint8_t ) );
        // Read from RAM address 0x04a0
        frcPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0] = 0xa0;
        frcPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[1] = 0x04;
        // Embedded OS Load Code command
        frcPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[2] = PNUM_OS;
        frcPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[3] = CMD_OS_LOAD_CODE;
        frcPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[4] = sizeof( TPerOSLoadCode_Request );
        TPerOSLoadCode_Request* perOsLoadCodeRequest = (TPerOSLoadCode_Request*)&frcPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[5];
        perOsLoadCodeRequest->Flags = 0x00;
        if ( loadingAction == LoadingAction::Load )
          perOsLoadCodeRequest->Flags |= 0x01;
        if ( loadingContentType == IOtaUploadService::LoadingContentType::Iqrf_plugin )
          perOsLoadCodeRequest->Flags |= 0x02;
        perOsLoadCodeRequest->Address = startAddress;
        perOsLoadCodeRequest->CheckSum = checksum;
        perOsLoadCodeRequest->Length = length;
        // Send FRC
        DpaMessage frcResult = sendDPAcommand( frcPacket, sizeof( TDpaIFaceHeader ) + 1 + 30 + 12, uploadResult );
        // Check FRC status
        frcStatus = frcResult.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.Status;
        if ( ( frcStatus >= 0x00 ) && ( frcStatus <= 0xEF ) )
        {
          // Process FRC data
          std::basic_string<uint8_t> frcData;
          frcData.append( &frcResult.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData[2], 54 );
          // Get extra result
          if ( selectedNodes.size() > 54 )
          {
            DpaMessage frcExtraResult = getFrcExtraResult( uploadResult );
            frcData.append( frcResult.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData, 9 );
          }
          // Set Verify result
          uint8_t frcPDataIndex = 0;
          for ( uint16_t addr : selectedNodes )
            uploadResult.setVerifyResultsMap( addr, frcData[frcPDataIndex++] == 0x02 );
        }
        else
        {          
          TRC_DEBUG( "Selective FRC Verify code failed." << NAME_PAR( "Status", frcStatus ) );
          TRC_FUNCTION_LEAVE( "" );
          // Throw exception
          std::string errorStr = "Selective FRC Verify code failed: " + std::to_string( frcStatus );
          THROW_EXC( std::logic_error, errorStr );
        }
      }

      TRC_INFORMATION( "FRC Verify code successful." );
      TRC_FUNCTION_LEAVE( "" );
    }

    // Load code into Nodes using acknowledged broadcast FRC
    DpaMessage loadCodeBroadcast(
      const uint16_t startAddress,
      const IOtaUploadService::LoadingContentType loadingContentType,
      const uint16_t length,
      const uint16_t checksum,
      UploadResult& uploadResult
    )
    {
      //DpaMessage frcRequest;
      DpaMessage::DpaPacket_t frcPacket;
      frcPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      frcPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
      frcPacket.DpaRequestPacket_t.PCMD = CMD_FRC_SEND;
      frcPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
      // Initialize command to FRC_AcknowledgedBroadcastBits
      frcPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.FrcCommand = FRC_AcknowledgedBroadcastBits;
      // Initialize user data to zero
      memset( frcPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData, 0, 30 * sizeof( uint8_t ) );
      // Embedded OS Load Code command
      frcPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[0] = sizeof( TDpaIFaceHeader ) - 1 * sizeof( uint8_t ) + sizeof( TPerOSLoadCode_Request );
      frcPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[1] = PNUM_OS;
      frcPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[2] = CMD_OS_LOAD_CODE; 
      uint16_t hwpId = uploadResult.getHwpId();
      frcPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[3] = hwpId & 0xff;
      frcPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[4] = hwpId >> 0x08;
      TPerOSLoadCode_Request* perOsLoadCodeRequest = (TPerOSLoadCode_Request*)&frcPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[5];
      perOsLoadCodeRequest->Flags = 0x01;
      if ( loadingContentType == IOtaUploadService::LoadingContentType::Iqrf_plugin )
        perOsLoadCodeRequest->Flags |= 0x02;
      perOsLoadCodeRequest->Address = startAddress;
      perOsLoadCodeRequest->CheckSum = checksum;
      perOsLoadCodeRequest->Length = length;
      // Send FRC
      DpaMessage frcResult = sendDPAcommand( frcPacket, sizeof( TDpaIFaceHeader ) + 1 + frcPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[0], uploadResult );
      // Check FRC status
      uint8_t status = frcResult.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.Status;
      if ( ( status >= 0x00 ) && ( status <= 0xEF ) )
      {
        // Get extra result
        DpaMessage frcExtraResult = getFrcExtraResult( uploadResult );
        // Set Load Result
        std::list<uint16_t> nodesList = uploadResult.getNodesList();
        for ( uint16_t addr : nodesList )
        {
          uint8_t byteIndex = addr / 8 + 1;
          uint8_t bitIndex = addr % 8;
          uint8_t bitMask = 1 << bitIndex;
          bool loadResult = uploadResult.getVerifyResult( addr );
          if ( loadResult )
            loadResult = frcResult.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData[byteIndex] & bitMask;
          uploadResult.setLoadResultsMap( addr, loadResult );
        }
        
        TRC_INFORMATION( "FRC Load code successful." );
        TRC_FUNCTION_LEAVE( "" );
        return frcResult;
      }

      TRC_DEBUG( "FRC Load code failed." << NAME_PAR( "Status", status ) );
      TRC_FUNCTION_LEAVE( "" );
      // Throw exception
      std::string errorStr = "FRC Load code failed: " + std::to_string( status );
      THROW_EXC( std::logic_error, errorStr );
    }

    UploadResult upload(
      const uint16_t deviceAddress,
      const uint16_t hwpId,
      const std::string& fileName,
      const uint16_t startMemAddr,
      const LoadingAction loadingAction
    )
    {
      TRC_FUNCTION_ENTER( "" );

      // Create result
      UploadResult uploadResult( deviceAddress, hwpId, loadingAction );

      IOtaUploadService::LoadingContentType loadingContentType;
      try {
        loadingContentType = parseLoadingContentType( fileName );
      }
      catch ( std::exception& ex ) {
        UploadError error( UploadError::Type::UnsupportedLoadingContent, ex.what() );
        uploadResult.setError( error );
        return uploadResult;
      }

      // prepare data to write and load
      std::unique_ptr<PreparedData> preparedData;
      try
      {
        preparedData = DataPreparer::prepareData( loadingContentType, fileName, deviceAddress == BROADCAST_ADDRESS );
      }
      catch ( std::exception& ex )
      {
        UploadError error( UploadError::Type::DataPrepare, ex.what() );
        uploadResult.setError( error );
        return uploadResult;
      }

      // Upload - write prepared data into external eeprom memory
      if ( loadingAction == LoadingAction::Upload )
      {
        try
        {
          // Initial version supports only one [N] (or [C]) or all Nodes
          writeDataToExtEEPROM( uploadResult, startMemAddr, preparedData->getData() );
        }
        catch ( std::exception& ex )
        {
          UploadError error( UploadError::Type::Upload, ex.what() );
          uploadResult.setError( error );
        }
      }
        // Verify (or load) action - check the external eeprom content
      if ( ( loadingAction == LoadingAction::Verify ) || ( loadingAction == LoadingAction::Load ) )
      {
        try
        {
          // Vefiry the external eeprom content
          if ( deviceAddress != BROADCAST_ADDRESS )
          {
            // Unicast address
            loadCodeUnicast( startMemAddr, LoadingAction::Verify, loadingContentType, preparedData->getLength(), preparedData->getChecksum(), uploadResult );
          }
          else
          {
            // Save actual FRC params
            IDpaTransaction2::FrcResponseTime frcResponseTime = m_iIqrfDpaService->getFrcResponseTime();
            // Verify the external eeprom memory content
            verifyCode( startMemAddr, LoadingAction::Verify, loadingContentType, preparedData->getLength(), preparedData->getChecksum(), uploadResult );                        
            // Finally set FRC param back to initial value
            m_iIqrfDpaService->setFrcResponseTime( frcResponseTime );
            setFRCparams( frcResponseTime, uploadResult );
          }
        }
        catch ( std::exception& ex )
        {
          UploadError error( UploadError::Type::Verify, ex.what() );
          uploadResult.setError( error );          
        }
      }


        // Load action - load external eeprom content to flash
      if ( loadingAction == LoadingAction::Load )
      {
        try
        {
          // Load the external eeprom content to flash
          if ( deviceAddress != BROADCAST_ADDRESS )
            loadCodeUnicast( startMemAddr, LoadingAction::Load, loadingContentType, preparedData->getLength(), preparedData->getChecksum(), uploadResult );
          else
            loadCodeBroadcast( startMemAddr, loadingContentType, preparedData->getLength(), preparedData->getChecksum(), uploadResult );
        }
        catch ( std::exception& ex )
        {
          UploadError error( UploadError::Type::Load, ex.what() );
          uploadResult.setError( error );
        }
      }

      TRC_FUNCTION_LEAVE( "" );
      return uploadResult;
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
      Pointer("/mType").Set(response, msgType.m_type);
      Pointer("/data/msgId").Set(response, msgId);

      // set result
      Pointer("/data/status").Set(response, SERVICE_ERROR);
      Pointer("/data/statusStr").Set(response, errorMsg);

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
    void setVerboseData(rapidjson::Document& response, UploadResult& uploadResult)
    {
      rapidjson::Value rawArray(kArrayType);
      Document::AllocatorType& allocator = response.GetAllocator();

      while (uploadResult.isNextTransactionResult()) {
        std::unique_ptr<IDpaTransactionResult2> transResult = uploadResult.consumeNextTransactionResult();
        rapidjson::Value rawObject(kObjectType);

        rawObject.AddMember(
          "request",
          encodeBinary(transResult->getRequest().DpaPacket().Buffer, transResult->getRequest().GetLength()),
          allocator
        );

        rawObject.AddMember(
          "requestTs",
          encodeTimestamp(transResult->getRequestTs()),
          allocator
        );

        rawObject.AddMember(
          "confirmation",
          encodeBinary(transResult->getConfirmation().DpaPacket().Buffer, transResult->getConfirmation().GetLength()),
          allocator
        );

        rawObject.AddMember(
          "confirmationTs",
          encodeTimestamp(transResult->getConfirmationTs()),
          allocator
        );

        rawObject.AddMember(
          "response",
          encodeBinary(transResult->getResponse().DpaPacket().Buffer, transResult->getResponse().GetLength()),
          allocator
        );

        rawObject.AddMember(
          "responseTs",
          encodeTimestamp(transResult->getResponseTs()),
          allocator
        );

        // add object into array
        rawArray.PushBack(rawObject, allocator);
      }

      // add array into response document
      Pointer("/data/raw").Set(response, rawArray);
    }

    // sets status inside specified response accoding to specified error
    void setResponseStatus(Document& response, const UploadError& error)
    {
      switch (error.getType()) {
      case UploadError::Type::NoError:
        Pointer("/data/status").Set(response, SERVICE_ERROR_NOERROR);
        break;
      case UploadError::Type::DataPrepare:
        Pointer("/data/status").Set(response, SERVICE_ERROR_DATA_PREPARE);
        break;
      case UploadError::Type::UnsupportedLoadingContent:
        Pointer("/data/status").Set(response, SERVICE_ERROR_UNSUPPORTED_LOADING_CONTENT);
        break;
      case UploadError::Type::Upload:
        Pointer("/data/status").Set(response, SERVICE_ERROR_UPLOAD);
        break;
      case UploadError::Type::Verify:
        Pointer( "/data/status" ).Set( response, SERVICE_ERROR_VERIFY );
        break;
      case UploadError::Type::Load:
        Pointer("/data/status").Set(response, SERVICE_ERROR_LOAD);
        break;
      default:
        // some other unsupported error
        Pointer("/data/status").Set(response, SERVICE_ERROR);
        break;
      }

      if (error.getType() == UploadError::Type::NoError) {
        Pointer("/data/statusStr").Set(response, "ok");
      }
      else {
        Pointer("/data/statusStr").Set(response, error.getMessage());
      }
    }

    // creates response on the basis of read TR config result
    Document createResponse(
      const std::string& msgId,
      const IMessagingSplitterService::MsgType& msgType,
      UploadResult& uploadResult,
      const ComIqmeshNetworkOtaUpload& comOtaUpload
    )
    {
      Document response;

      // Set common parameters
      Pointer( "/mType" ).Set( response, msgType.m_type );
      Pointer( "/data/msgId" ).Set( response, msgId );

      // Device address
      Pointer( "/data/rsp/deviceAddr" ).Set( response, uploadResult.getDeviceAddress() );

      // hwpId
      Pointer( "/data/rsp/hwpId" ).Set( response, uploadResult.getHwpId() );

      // Load action
      LoadingAction action = uploadResult.getLoadingAction();
      std::string actionStr = "???";
      switch ( action )
      {
        case LoadingAction::Upload:
          actionStr = "Upload";
          break;

        case LoadingAction::Verify:
          actionStr = "Verify";
          break;

        case LoadingAction::Load:
          actionStr = "Load";
          break;
      }
      Pointer( "/data/rsp/loadingAction" ).Set( response, actionStr );

      // Checking of error
      UploadError error = uploadResult.getError();
      if ( error.getType() == UploadError::Type::NoError )
      {
        // OK
        if ( action == LoadingAction::Upload )
          Pointer( "/data/rsp/uploadResult" ).Set( response, uploadResult.getUploadResult() );

        if ( action == LoadingAction::Verify || action == LoadingAction::Load )
        {
          // Array of objects
          Document::AllocatorType& allocator = response.GetAllocator();
          rapidjson::Value verifyResult( kArrayType );
          std::map<uint16_t, bool> verifyResultMap = uploadResult.getVerifyResultsMap();
          for ( std::map<uint16_t, bool>::iterator i = verifyResultMap.begin(); i != verifyResultMap.end(); i++ )
          {
            rapidjson::Value verifyResultItem( kObjectType );
            verifyResultItem.AddMember( "address", i->first, allocator );
            verifyResultItem.AddMember( "result", i->second, allocator );
            verifyResult.PushBack( verifyResultItem, allocator );
          }
          Pointer( "/data/rsp/verifyResult" ).Set( response, verifyResult );
        }

        if ( action == LoadingAction::Load )
        {
          Document::AllocatorType& allocator = response.GetAllocator();
          rapidjson::Value loadResult( kArrayType );
          std::map<uint16_t, bool> loadResultMap = uploadResult.getLoadResultsMap();
          for ( std::map<uint16_t, bool>::iterator i = loadResultMap.begin(); i != loadResultMap.end(); i++ )
          {
            rapidjson::Value loadResultItem( kObjectType );
            loadResultItem.AddMember( "address", i->first, allocator );
            loadResultItem.AddMember( "result", i->second, allocator );
            loadResult.PushBack( loadResultItem, allocator );
          }
          Pointer( "/data/rsp/loadResult" ).Set( response, loadResult );
        }
      }
      else
      {
        // Error
        if(uploadResult.getLoadingAction() == LoadingAction::Upload )
          Pointer( "/data/rsp/uploadResult" ).Set( response, false );
      }

      // set raw fields, if verbose mode is active
      if ( comOtaUpload.getVerbose() )
        setVerboseData( response, uploadResult );

      setResponseStatus( response, uploadResult.getError() );

      return response;
    }
    
    uint8_t parseAndCheckRepeat(const int repeat) {
      if (repeat < 0) {
        TRC_WARNING("repeat cannot be less than 0. It will be set to 0.");
        return 0;
      }

      if (repeat > 0xFF) {
        TRC_WARNING("repeat exceeds maximum. It will be trimmed to maximum of: " << PAR(REPEAT_MAX));
        return REPEAT_MAX;
      }

      return repeat;
    }

    uint16_t parseAndCheckHwpId( const int& hwpId ) {
      if ( ( hwpId < 0 ) || ( hwpId > 0xffff ) ) {
        THROW_EXC(
          std::out_of_range, "hwpId outside of valid range. " << NAME_PAR_HEX( "hwpId", hwpId )
        );
      }
      return hwpId;
    }

    uint16_t parseAndCheckDeviceAddr(const int& deviceAddr) {
      if ((deviceAddr < 0) || (deviceAddr > 0xff)) {
        THROW_EXC(
          std::out_of_range, "Device address outside of valid range. " << NAME_PAR_HEX("Address", deviceAddr)
        );
      }
      return deviceAddr;
    }

    std::string checkFileName(const std::string& fileName) {
      if (fileName.empty()) {
        THROW_EXC(std::logic_error, "Empty file name.")
      }
      return fileName;
    }

    // creates and returns full file name
    std::string getFullFileName( const std::string& uploadPath, const std::string& fileName )
    {
      char fileSeparator;

#if defined(WIN32) || defined(_WIN32)
      fileSeparator = '\\';
#else
      fileSeparator = '/';
#endif

      std::string fullFileName = uploadPath;

      if ( uploadPath[uploadPath.size() - 1] != fileSeparator ) {
        fullFileName += fileSeparator;
      }

      fullFileName += fileName;

      return fullFileName;
    }

    uint16_t parseAndCheckStartMemAddr(int startMemAddr) {
      if (startMemAddr < 0) {
        THROW_EXC(std::logic_error, "Start address must be nonnegative.");
      }
      return startMemAddr;
    }

    LoadingAction parseAndCheckLoadingAction(const std::string& loadingActionStr) {
      if (loadingActionStr == "Upload") {
        return LoadingAction::Upload;
      }

      if (loadingActionStr == "Verify") {
        return LoadingAction::Verify;
      }

      if ( loadingActionStr == "Load" ) {
        return LoadingAction::Load;
      }

      THROW_EXC(std::out_of_range, "Unsupported loading action: " << PAR(loadingActionStr));
    }

    void handleMsg(
      const std::string& messagingId,
      const IMessagingSplitterService::MsgType& msgType,
      rapidjson::Document doc
    )
    {
      TRC_FUNCTION_ENTER(
        PAR(messagingId) <<
        NAME_PAR(mType, msgType.m_type) <<
        NAME_PAR(major, msgType.m_major) <<
        NAME_PAR(minor, msgType.m_minor) <<
        NAME_PAR(micro, msgType.m_micro)
      );

      // unsupported type of request
      if (msgType.m_type != m_mTypeName_iqmeshNetworkOtaUpload) {
        THROW_EXC(std::logic_error, "Unsupported message type: " << PAR(msgType.m_type));
      }

      // creating representation object
      ComIqmeshNetworkOtaUpload comOtaUpload(doc);

      // if upload path (service's configuration parameter) is empty, return error message
      if (m_uploadPath.empty()) {
        Document failResponse = createCheckParamsFailedResponse(comOtaUpload.getMsgId(), msgType, "Empty upload path");
        m_iMessagingSplitterService->sendMessage(messagingId, std::move(failResponse));

        TRC_FUNCTION_LEAVE("");
        return;
      }

      /*
      struct stat statbuf;
      if ( stat( m_uploadPath.c_str(), &statbuf ) != -1 )
      {
        if ( S_ISDIR( statbuf.st_mode ) )
        {

        }
      }
      */

      // service input parameters
      uint16_t deviceAddr;
      uint16_t hwpId;
      std::string fileName;
      uint16_t startMemAddr;
      LoadingAction loadingAction;

      // parsing and checking service parameters
      try {
        m_repeat = parseAndCheckRepeat(comOtaUpload.getRepeat());
       
        if (!comOtaUpload.isSetDeviceAddr()) {
          THROW_EXC(std::logic_error, "deviceAddr not set");
        }
        deviceAddr = parseAndCheckDeviceAddr(comOtaUpload.getDeviceAddr());

        if ( comOtaUpload.isSetHwpId() )
          hwpId = parseAndCheckHwpId( comOtaUpload.getHwpId() );
        else
          hwpId = 0xffff;

        if (!comOtaUpload.isSetFileName()) {
          THROW_EXC(std::logic_error, "fileName not set");
        }
        fileName = checkFileName(comOtaUpload.getFileName());

        if (!comOtaUpload.isSetStartMemAddr()) {
          THROW_EXC(std::logic_error, "startMemAddr not set");
        }
        startMemAddr = parseAndCheckStartMemAddr(comOtaUpload.getStartMemAddr());

        if (!comOtaUpload.isSetLoadingAction()) {
          THROW_EXC(std::logic_error, "loadingAction not set");
        }
        loadingAction = parseAndCheckLoadingAction(comOtaUpload.getLoadingAction());        

        m_returnVerbose = comOtaUpload.getVerbose();
      }
      // parsing and checking service parameters failed 
      catch (std::exception& ex) {
        Document failResponse = createCheckParamsFailedResponse(comOtaUpload.getMsgId(), msgType, ex.what());
        m_iMessagingSplitterService->sendMessage(messagingId, std::move(failResponse));

        TRC_FUNCTION_LEAVE("");
        return;
      }
      
      // construct full file name
      std::string fullFileName = getFullFileName(m_uploadPath, fileName);

      // try to establish exclusive access
      try {
        m_exclusiveAccess = m_iIqrfDpaService->getExclusiveAccess();        
      }
      catch (std::exception &e) {
        const char* errorStr = e.what();
        TRC_WARNING("Error while establishing exclusive DPA access: " << PAR(errorStr));

        Document failResponse = getExclusiveAccessFailedResponse(comOtaUpload.getMsgId(), msgType, errorStr);
        m_iMessagingSplitterService->sendMessage(messagingId, std::move(failResponse));

        TRC_FUNCTION_LEAVE("");
        return;
      }

      // call service with checked params
      UploadResult uploadResult = upload( deviceAddr, hwpId, fullFileName, startMemAddr, loadingAction);

      // release exclusive access
      m_exclusiveAccess.reset();

      // create and send response
      Document responseDoc = createResponse(comOtaUpload.getMsgId(), msgType, uploadResult, comOtaUpload);
      m_iMessagingSplitterService->sendMessage(messagingId, std::move(responseDoc));

      TRC_FUNCTION_LEAVE("");
    }
    

  public:
    void activate(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "************************************" << std::endl <<
        "OtaUploadService instance activate" << std::endl <<
        "************************************"
      );

      props->getMemberAsString("uploadPath", m_uploadPath);
      TRC_INFORMATION(PAR(m_uploadPath));

      if (m_uploadPath.empty()) {       
        TRC_ERROR("Upload path is empty.");
      }

      // for the sake of register function parameters 
      std::vector<std::string> supportedMsgTypes = 
      {
        m_mTypeName_iqmeshNetworkOtaUpload
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
        "OtaUploadService instance deactivate" << std::endl <<
        "**************************************"
      );

      // for the sake of unregister function parameters 
      std::vector<std::string> supportedMsgTypes =
      {
        m_mTypeName_iqmeshNetworkOtaUpload
      };

      m_iMessagingSplitterService->unregisterFilteredMsgHandler(supportedMsgTypes);

      TRC_FUNCTION_LEAVE("");
    }

    void modify(const shape::Properties *props)
    {
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

  OtaUploadService::OtaUploadService()
  {
    m_imp = shape_new Imp(*this);
  }

  OtaUploadService::~OtaUploadService()
  {
    delete m_imp;
  }

  void OtaUploadService::attachInterface(iqrf::IIqrfDpaService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void OtaUploadService::detachInterface(iqrf::IIqrfDpaService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void OtaUploadService::attachInterface(iqrf::IMessagingSplitterService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void OtaUploadService::detachInterface(iqrf::IMessagingSplitterService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void OtaUploadService::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void OtaUploadService::detachInterface(shape::ITraceService* iface)
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
}
