#define IRemoveBondService_EXPORTS

#include "RemoveBondService.h"
#include "Trace.h"
#include "ComIqmeshNetworkRemoveBond.h"
#include "iqrf__RemoveBondService.hxx"
#include <list>
#include <cmath>
#include <thread> 

TRC_INIT_MODULE(iqrf::RemoveBondService);

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

  // values of result error codes
  // service general fail code - may and probably will be changed later in the future
  static const int SERVICE_ERROR = 1000;

  static const int SERVICE_ERROR_INTERNAL = SERVICE_ERROR + 1;
  static const int SERVICE_ERROR_REMOVE_BOND_AT_NODE = SERVICE_ERROR + 2;
  static const int SERVICE_ERROR_REMOVE_BOND_AT_COORDINATOR = SERVICE_ERROR + 3;
  static const int SERVICE_ERROR_CLEAR_ALL_BONDS = SERVICE_ERROR + 4;
  static const int SERVICE_ERROR_BATCH_REMOVE_AND_RESTART = SERVICE_ERROR + 5;
};


namespace iqrf {

  // Holds information about errors, which encounter during local bond service run
  class RemoveBondError {
  public:
    // Type of error
    enum class Type {
      NoError,
      RemoveBondAtNodeSideError,
      RemoveBondAtCoordinatorSideError,
      ClearAllBonds,
      BatchRemoveBondAndRestartNode
    };

    RemoveBondError() : m_type( Type::NoError ), m_message( "" ) {};
    RemoveBondError( Type errorType ) : m_type( errorType ), m_message( "" ) {};
    RemoveBondError( Type errorType, const std::string& message ) : m_type( errorType ), m_message( message ) {};

    Type getType() const { return m_type; };
    std::string getMessage() const { return m_message; };

    RemoveBondError& operator=( const RemoveBondError& error ) {
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
  class RemoveBondResult {
  private:
    RemoveBondError m_error;
    uint8_t m_nodesNr;

    // transaction results
    std::list<std::unique_ptr<IDpaTransactionResult2>> m_transResults;

  public:
    RemoveBondError getError() const { return m_error; };

    void setError( const RemoveBondError& error ) {
      m_error = error;
    }

    uint8_t getNodesNr() {
      return m_nodesNr;
    }

    void setNodesNr(uint8_t nodesNr) {
      m_nodesNr = nodesNr;
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
  class RemoveBondService::Imp {
  private:
    // parent object
    RemoveBondService & m_parent;

    // message type: iqmesh network remove bond
    // for temporal reasons
    const std::string m_mTypeName_iqmeshNetworkRemoveBond = "iqmeshNetwork_RemoveBond";

    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    IIqrfDpaService* m_iIqrfDpaService = nullptr;
    std::unique_ptr<IIqrfDpaService::ExclusiveAccess> m_exclusiveAccess;

    // number of repeats
    uint8_t m_repeat = 1;

    // if is set Verbose mode
    bool m_returnVerbose = false;


  public:
    Imp( RemoveBondService& parent ) : m_parent( parent )
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

    // sets remove bond and restart data
    void setRemoveBondAndRestartData(uint8_t* packetData, const uint16_t hwpId)
    {
      // initialize packet data to zero
      memset(packetData, 0, DPA_MAX_DATA_LENGTH * sizeof(uint8_t));
      
      // remove bond request
      packetData[0] = 0x05;
      packetData[1] = PNUM_NODE;
      packetData[2] = CMD_NODE_REMOVE_BOND;
      packetData[3] = hwpId & 0xFF;
      packetData[4] = (hwpId >> 8) & 0xFF;

      // reset request
      packetData[5] = 0x05;
      packetData[6] = PNUM_OS;
      packetData[7] = CMD_OS_RESTART;
      packetData[8] = hwpId & 0xFF;
      packetData[9] = (hwpId >> 8) & 0xFF;

      // end of batch command
      packetData[10] = 0x00;
    }

    // tries remove bond and restart node at node's side
    void batchRemoveBondAndRestartNode(
      RemoveBondResult& removeBondResult, const uint8_t nodeAddr, const uint16_t hwpId
    )
    {
      TRC_FUNCTION_ENTER("");

      DpaMessage batchRemoveBondAndRestartRequest;
      DpaMessage::DpaPacket_t batchRemoveBondAndRestartPacket;
      batchRemoveBondAndRestartPacket.DpaRequestPacket_t.NADR = nodeAddr;
      batchRemoveBondAndRestartPacket.DpaRequestPacket_t.PNUM = PNUM_OS;
      batchRemoveBondAndRestartPacket.DpaRequestPacket_t.PCMD = CMD_OS_BATCH;
      batchRemoveBondAndRestartPacket.DpaRequestPacket_t.HWPID = hwpId;

      uint8_t* batchData = batchRemoveBondAndRestartPacket.DpaRequestPacket_t.DpaMessage.Response.PData;
      
      // setting 2 embedded requests into packet data: remove bond and restart
      setRemoveBondAndRestartData(batchData, hwpId);

      batchRemoveBondAndRestartRequest.DataToBuffer(
        batchRemoveBondAndRestartPacket.Buffer, sizeof(TDpaIFaceHeader) + 2 * 5 + 1
      );

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> batchRemoveBondAndRestartTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      for (int rep = 0; rep <= m_repeat; rep++)
      {
        try {
          batchRemoveBondAndRestartTransaction = m_exclusiveAccess->executeDpaTransaction(batchRemoveBondAndRestartRequest);
          transResult = batchRemoveBondAndRestartTransaction->get();
        }
        catch (std::exception& e) {
          TRC_WARNING("DPA transaction error : " << e.what());

          if (rep < m_repeat) {
            continue;
          }

          RemoveBondError error(RemoveBondError::Type::BatchRemoveBondAndRestartNode, e.what());
          removeBondResult.setError(error);

          TRC_FUNCTION_LEAVE("");
          return;
        }

        TRC_DEBUG("Result from Batch remove bond and restart node transaction as string:" << PAR(transResult->getErrorString()));

        IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

        // because of the move-semantics
        DpaMessage dpaResponse = transResult->getResponse();
        removeBondResult.addTransactionResult(transResult);

        if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
          TRC_INFORMATION("Batch remove bond and restart node successful!");
          TRC_DEBUG(
            "DPA transaction: "
            << NAME_PAR(batchRemoveBondAndRestartRequest.PeripheralType(), batchRemoveBondAndRestartRequest.NodeAddress())
            << PAR(batchRemoveBondAndRestartRequest.PeripheralCommand())
          );

          TRC_FUNCTION_LEAVE("");
          return;
        }

        // transaction error
        if (errorCode < 0) {
          TRC_WARNING("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

          if (rep < m_repeat) {
            continue;
          }

          RemoveBondError error(RemoveBondError::Type::BatchRemoveBondAndRestartNode, "Transaction error.");
          removeBondResult.setError(error);
        }
        else {
          // DPA error
          TRC_WARNING("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

          if (rep < m_repeat) {
            continue;
          }

          RemoveBondError error(RemoveBondError::Type::BatchRemoveBondAndRestartNode, "Dpa error.");
          removeBondResult.setError(error);
        }
      }

      TRC_FUNCTION_LEAVE("");
      return;
    }


    // tries to remove bond and returns result
    void _removeBond( 
      RemoveBondResult& removeBondResult, const uint8_t nodeAddr, const uint16_t hwpId
    )
    {
      TRC_FUNCTION_ENTER( "" );

      DpaMessage removeBondRequest;
      DpaMessage::DpaPacket_t removeBondPacket;
      removeBondPacket.DpaRequestPacket_t.NADR = nodeAddr;
      removeBondPacket.DpaRequestPacket_t.PNUM = PNUM_NODE;
      removeBondPacket.DpaRequestPacket_t.PCMD = CMD_NODE_REMOVE_BOND;
      removeBondPacket.DpaRequestPacket_t.HWPID = hwpId;

      removeBondRequest.DataToBuffer( removeBondPacket.Buffer, sizeof( TDpaIFaceHeader ) );

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> removeBondTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      for (int rep = 0; rep <= m_repeat; rep++)
      {
        try {
          removeBondTransaction = m_exclusiveAccess->executeDpaTransaction(removeBondRequest);
          transResult = removeBondTransaction->get();
        }
        catch (std::exception& e) {
          TRC_WARNING("DPA transaction error : " << e.what());

          if (rep < m_repeat) {
            continue;
          }

          RemoveBondError error(RemoveBondError::Type::RemoveBondAtNodeSideError, e.what());
          removeBondResult.setError(error);

          TRC_FUNCTION_LEAVE("");
          return;
        }

        TRC_DEBUG("Result from Remove Bond At Node Side transaction as string:" << PAR(transResult->getErrorString()));

        IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

        // because of the move-semantics
        DpaMessage dpaResponse = transResult->getResponse();
        removeBondResult.addTransactionResult(transResult);

        if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
          TRC_INFORMATION("Remove bond at node side successful!");
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

          if (rep < m_repeat) {
            continue;
          }

          RemoveBondError error(RemoveBondError::Type::RemoveBondAtNodeSideError, "Transaction error.");
          removeBondResult.setError(error);
        }
        else {
          // DPA error
          TRC_WARNING("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

          if (rep < m_repeat) {
            continue;
          }

          RemoveBondError error(RemoveBondError::Type::RemoveBondAtNodeSideError, "Dpa error.");
          removeBondResult.setError(error);
        }
      }

      TRC_FUNCTION_LEAVE( "" );
      return;
    }

    // removes specified address from coordinator's list of bonded addresses
    void removeBondedNode( 
      RemoveBondResult& removeBondResult, const uint8_t nodeAddr, const uint16_t hwpId
    )
    {
      TRC_FUNCTION_ENTER( "" );

      DpaMessage removeBondRequest;
      DpaMessage::DpaPacket_t removeBondPacket;
      removeBondPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      removeBondPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
      removeBondPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_REMOVE_BOND;
      removeBondPacket.DpaRequestPacket_t.HWPID = hwpId;

      uns8* pData = removeBondPacket.DpaRequestPacket_t.DpaMessage.Request.PData;
      pData[0] = uns8(nodeAddr & 0xFF);

      removeBondRequest.DataToBuffer( removeBondPacket.Buffer, sizeof( TDpaIFaceHeader ) + 1 );

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> removeBondTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      for ( int rep = 0; rep <= m_repeat; rep++ ) {
        try {
          removeBondTransaction = m_exclusiveAccess->executeDpaTransaction(removeBondRequest);
          transResult = removeBondTransaction->get();
        }
        catch ( std::exception& e ) {
          TRC_WARNING("DPA transaction error : " << e.what());

          if (rep < m_repeat) {
            continue;
          }

          RemoveBondError error(RemoveBondError::Type::RemoveBondAtCoordinatorSideError, e.what());
          removeBondResult.setError(error);

          TRC_FUNCTION_LEAVE("");
          return;
        }

        TRC_DEBUG( "Result from remove bond transaction as string:" << PAR( transResult->getErrorString() ) );

        IDpaTransactionResult2::ErrorCode errorCode = ( IDpaTransactionResult2::ErrorCode )transResult->getErrorCode();

        // because of the move-semantics
        DpaMessage dpaResponse = transResult->getResponse();
        removeBondResult.addTransactionResult( transResult );

        if ( errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK ) {
          TRC_INFORMATION( "Remove node bond done!" );
          TRC_DEBUG(
            "DPA transaction: "
            << NAME_PAR( removeBondRequest.PeripheralType(), removeBondRequest.NodeAddress() )
            << PAR( removeBondRequest.PeripheralCommand() )
          );

          // getting bond data
          uint8_t* respData = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData;

          removeBondResult.setNodesNr(respData[0]);

          TRC_FUNCTION_LEAVE( "" );
          return;
        }

        // transaction error
        if ( errorCode < 0 ) {
          TRC_WARNING( "Transaction error. " << NAME_PAR_HEX( "Error code", errorCode ) );

          if ( rep < m_repeat ) {
            continue;
          }

          RemoveBondError error(RemoveBondError::Type::RemoveBondAtCoordinatorSideError, "Transaction error.");
          removeBondResult.setError(error);
        } // DPA error
        else {
          TRC_WARNING( "DPA error. " << NAME_PAR_HEX( "Error code", errorCode ) );

          if ( rep < m_repeat ) {
            continue;
          }

          RemoveBondError error(RemoveBondError::Type::RemoveBondAtCoordinatorSideError, "Dpa error.");
          removeBondResult.setError(error);
        }
      }

      TRC_FUNCTION_LEAVE( "" );
      return;
    }


    void clearAllBonds(RemoveBondResult& removeBondResult, const uint16_t hwpId)
    {
      TRC_FUNCTION_ENTER("");
      
      DpaMessage clearAllBondsRequest;
      DpaMessage::DpaPacket_t clearAllBondsPacket;
      clearAllBondsPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      clearAllBondsPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
      clearAllBondsPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_CLEAR_ALL_BONDS;
      clearAllBondsPacket.DpaRequestPacket_t.HWPID = hwpId;

      clearAllBondsRequest.DataToBuffer(clearAllBondsPacket.Buffer, sizeof(TDpaIFaceHeader));

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> clearAllBondsTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      for (int rep = 0; rep <= m_repeat; rep++) {
        try {
          clearAllBondsTransaction = m_exclusiveAccess->executeDpaTransaction(clearAllBondsRequest);
          transResult = clearAllBondsTransaction->get();
        }
        catch (std::exception& e) {
          TRC_WARNING("DPA transaction error : " << e.what());

          if (rep < m_repeat) {
            continue;
          }

          RemoveBondError error(RemoveBondError::Type::ClearAllBonds, e.what());
          removeBondResult.setError(error);

          TRC_FUNCTION_LEAVE("");
          return;
        }

        TRC_DEBUG("Result from Clear All Bonds transaction as string:" << PAR(transResult->getErrorString()));

        IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

        // because of the move-semantics
        DpaMessage dpaResponse = transResult->getResponse();
        removeBondResult.addTransactionResult(transResult);

        if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
          TRC_INFORMATION("Clear All Bonds done!");
          TRC_DEBUG(
            "DPA transaction: "
            << NAME_PAR(clearAllBondsRequest.PeripheralType(), clearAllBondsRequest.NodeAddress())
            << PAR(clearAllBondsRequest.PeripheralCommand())
          );

          TRC_FUNCTION_LEAVE("");
          return;
        }

        // transaction error
        if (errorCode < 0) {
          TRC_WARNING("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

          if (rep < m_repeat) {
            continue;
          }

          RemoveBondError error(RemoveBondError::Type::ClearAllBonds, "Transaction error.");
          removeBondResult.setError(error);
        } // DPA error
        else {
          TRC_WARNING("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

          if (rep < m_repeat) {
            continue;
          }

          RemoveBondError error(RemoveBondError::Type::ClearAllBonds, "Dpa error.");
          removeBondResult.setError(error);
        }
      }
      
      TRC_FUNCTION_LEAVE("");
    }
    
    void removeAllBonds(
      RemoveBondResult& removeBondResult, 
      const uint16_t hwpId,
      const uint16_t dpaVer
    )
    {
      TRC_FUNCTION_ENTER("");
      
      if (dpaVer < 0x04) {
        batchRemoveBondAndRestartNode(removeBondResult, 0xFF, hwpId);
      }
      else {
        _removeBond(removeBondResult, 0xFF, hwpId);
      }

      // removing failed
      if (removeBondResult.getError().getType() != RemoveBondError::Type::NoError) {
        TRC_FUNCTION_LEAVE("");
        return;
      }

      clearAllBonds(removeBondResult, hwpId);

      TRC_FUNCTION_LEAVE("");
      return;
    }

    // removes bond of requested node
    RemoveBondResult removeBond( const uint8_t nodeAddr, const uint16_t hwpId)
    {
      TRC_FUNCTION_ENTER( "" );

      RemoveBondResult removeBondResult;

      // getting DPA version
      IIqrfDpaService::CoordinatorParameters coordParams = m_iIqrfDpaService->getCoordinatorParameters();
      uint16_t dpaVer = (coordParams.dpaVerMajor << 8) + coordParams.dpaVerMinor;

      // in the case of broadcast, remove all bonds
      if (nodeAddr == BROADCAST_ADDRESS) {
        removeAllBonds(removeBondResult, hwpId, dpaVer);

        TRC_FUNCTION_LEAVE("");
        return removeBondResult;
      }

      // remove node at the node's side
      if (dpaVer < 0x04) {
        batchRemoveBondAndRestartNode(removeBondResult, nodeAddr, hwpId);
      }
      else {
        _removeBond(removeBondResult, nodeAddr, hwpId);
      }

      // removing failed
      if ( removeBondResult.getError().getType() != RemoveBondError::Type::NoError ) {
        TRC_FUNCTION_LEAVE("");
        return removeBondResult;
      }

      // remove node at the coordinator's side
      removeBondedNode(removeBondResult, nodeAddr, hwpId);

      TRC_FUNCTION_LEAVE("");
      return removeBondResult;
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
    void setVerboseData( rapidjson::Document& response, RemoveBondResult& bondResult )
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
    void setOsReadSection(
      Document& response,
      const std::vector<uns8>& readInfo
    )
    {
      // MID - hex string without separator
      std::ostringstream moduleId;
      moduleId.fill('0');
      moduleId << std::hex << std::uppercase <<
        std::setw(2) << (int)readInfo[3] <<
        std::setw(2) << (int)readInfo[2] <<
        std::setw(2) << (int)readInfo[1] <<
        std::setw(2) << (int)readInfo[0];
      rapidjson::Pointer("/data/rsp/osRead/mid").Set(response, moduleId.str());

      // OS version - string
      std::ostringstream osVer;
      osVer << std::hex << (int)(readInfo[4] >> 4) << '.';
      osVer.fill('0');
      osVer << std::setw(2) << (int)(readInfo[4] & 0xf) << 'D';
      rapidjson::Pointer("/data/rsp/osRead/osVersion").Set(response, osVer.str());

      // trMcuType
      uns8 trMcuType = readInfo[5];

      rapidjson::Pointer("/data/rsp/osRead/trMcuType/value").Set(response, trMcuType);
      std::string trTypeStr = "(DC)TR-";
      switch (trMcuType >> 4) {
      case 2: trTypeStr += "72Dx";
        break;
      case 4: trTypeStr += "78Dx";
        break;
      case 11: trTypeStr += "76Dx";
        break;
      case 12: trTypeStr += "77Dx";
        break;
      case 13: trTypeStr += "75Dx";
        break;
      default: trTypeStr += "???";
        break;
      }
      rapidjson::Pointer("/data/rsp/osRead/trMcuType/trType").Set(response, trTypeStr);
      bool fccCertified = ((trMcuType & 0x08) == 0x08) ? true : false;
      rapidjson::Pointer("/data/rsp/osRead/trMcuType/fccCertified").Set(response, fccCertified);
      std::string mcuTypeStr = ((trMcuType & 0x07) == 0x04) ? "PIC16LF1938" : "UNKNOWN";
      rapidjson::Pointer("/data/rsp/osRead/trMcuType/mcuType").Set(response, mcuTypeStr);

      // OS build - string
      uint16_t osBuild = (readInfo[7] << 8) + readInfo[6];
      rapidjson::Pointer("/data/rsp/osRead/osBuild").Set(response, encodeHexaNum_CapitalLetters(osBuild));

      // RSSI [dBm]
      int8_t rssi = readInfo[8] - 130;
      std::string rssiStr = std::to_string(rssi) + " dBm";
      rapidjson::Pointer("/data/rsp/osRead/rssi").Set(response, rssiStr);

      // Supply voltage [V]
      float supplyVoltage = 261.12f / (float)(127 - readInfo[9]);
      char supplyVoltageStr[8];
      std::sprintf(supplyVoltageStr, "%1.2f V", supplyVoltage);
      rapidjson::Pointer("/data/rsp/osRead/supplyVoltage").Set(response, supplyVoltageStr);

      // Flags
      uns8 flags = readInfo[10];

      rapidjson::Pointer("/data/rsp/osRead/flags/value").Set(response, flags);
      bool insufficientOsBuild = ((flags & 0x01) == 0x01) ? true : false;
      rapidjson::Pointer("/data/rsp/osRead/flags/insufficientOsBuild").Set(response, insufficientOsBuild);
      std::string iface = ((flags & 0x02) == 0x02) ? "UART" : "SPI";
      rapidjson::Pointer("/data/rsp/osRead/flags/interfaceType").Set(response, iface);
      bool dpaHandlerDetected = ((flags & 0x04) == 0x04) ? true : false;
      rapidjson::Pointer("/data/rsp/osRead/flags/dpaHandlerDetected").Set(response, dpaHandlerDetected);
      bool dpaHandlerNotDetectedButEnabled = ((flags & 0x08) == 0x08) ? true : false;
      rapidjson::Pointer("/data/rsp/osRead/flags/dpaHandlerNotDetectedButEnabled").Set(response, dpaHandlerNotDetectedButEnabled);
      bool noInterfaceSupported = ((flags & 0x10) == 0x10) ? true : false;
      rapidjson::Pointer("/data/rsp/osRead/flags/noInterfaceSupported").Set(response, noInterfaceSupported);

      // SlotLimits
      uns8 slotLimits = readInfo[11];

      rapidjson::Pointer("/data/rsp/osRead/slotLimits/value").Set(response, slotLimits);
      uint8_t shortestTimeSlot = ((slotLimits & 0x0f) + 3) * 10;
      uint8_t longestTimeSlot = (((slotLimits >> 0x04) & 0x0f) + 3) * 10;
      rapidjson::Pointer("/data/rsp/osRead/slotLimits/shortestTimeslot").Set(response, std::to_string(shortestTimeSlot) + " ms");
      rapidjson::Pointer("/data/rsp/osRead/slotLimits/longestTimeslot").Set(response, std::to_string(longestTimeSlot) + " ms");

      // ibk - only if DPA version is >= 3.03
      // getting DPA version
      IIqrfDpaService::CoordinatorParameters coordParams = m_iIqrfDpaService->getCoordinatorParameters();
      uint16_t dpaVer = (coordParams.dpaVerMajor << 8) + coordParams.dpaVerMinor;

      if (dpaVer >= 0x0303) {
        Document::AllocatorType& allocator = response.GetAllocator();
        rapidjson::Value ibkBitsJsonArray(kArrayType);
        for (int i = 0; i < 16; i++) {
          ibkBitsJsonArray.PushBack(readInfo[12 + i], allocator);
        }
        Pointer("/data/rsp/osRead/ibk").Set(response, ibkBitsJsonArray);
      }
    }

    // creates response on the basis of bond result
    Document createResponse(
      const std::string& msgId,
      const IMessagingSplitterService::MsgType& msgType,
      RemoveBondResult& removeBondResult,
      const ComIqmeshNetworkRemoveBond& comRemoveBond
    )
    {
      Document response;

      // set common parameters
      Pointer( "/mType" ).Set( response, msgType.m_type );
      Pointer( "/data/msgId" ).Set( response, msgId );

      // checking of error
      RemoveBondError error = removeBondResult.getError();

      if ( error.getType() != RemoveBondError::Type::NoError ) 
      {
        int status = SERVICE_ERROR;

        switch ( error.getType() ) {
          case RemoveBondError::Type::RemoveBondAtNodeSideError:
            status = SERVICE_ERROR_REMOVE_BOND_AT_NODE;
            break;

          case RemoveBondError::Type::RemoveBondAtCoordinatorSideError:
            status = SERVICE_ERROR_REMOVE_BOND_AT_COORDINATOR;
            break;

          case RemoveBondError::Type::ClearAllBonds:
            status = SERVICE_ERROR_CLEAR_ALL_BONDS;
            break;

          case RemoveBondError::Type::BatchRemoveBondAndRestartNode:
            status = SERVICE_ERROR_BATCH_REMOVE_AND_RESTART;
            break;

          default:
            status = SERVICE_ERROR;
            break;
        }

        // set raw fields, if verbose mode is active
        if ( comRemoveBond.getVerbose() ) {
          setVerboseData( response, removeBondResult );
        }

        Pointer("/data/status").Set(response, status);
        Pointer("/data/statusStr").Set(response, error.getMessage());

        return response;
      }

      // no errors

      // rsp object
      rapidjson::Pointer( "/data/rsp/nodesNr" ).Set( response, removeBondResult.getNodesNr() );
      
      // set raw fields, if verbose mode is active
      if ( comRemoveBond.getVerbose() ) {
        setVerboseData( response, removeBondResult );
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
      if (devAddr == BROADCAST_ADDRESS) {
        return devAddr;
      }

      if ( ( devAddr < 0 ) || ( devAddr > 0xEF ) ) {
        THROW_EXC(
          std::out_of_range, "Device address outside of valid range. " << NAME_PAR_HEX( "deviceAddr", devAddr )
        );
      }

      return devAddr;
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
      if ( msgType.m_type != m_mTypeName_iqmeshNetworkRemoveBond ) {
        THROW_EXC( std::logic_error, "Unsupported message type: " << PAR( msgType.m_type ) );
      }

      // creating representation object
      ComIqmeshNetworkRemoveBond comRemoveBond( doc );

      // service input parameters
      uint8_t deviceAddr;
      uint16_t hwpId;

      // parsing and checking service parameters
      try {
        m_repeat = parseAndCheckRepeat( comRemoveBond.getRepeat() );

        if ( !comRemoveBond.isSetDeviceAddr() ) {
          THROW_EXC( std::logic_error, "deviceAddr not set" );
        }
        
        deviceAddr = parseAndCheckDeviceAddr( comRemoveBond.getDeviceAddr() );

        if (comRemoveBond.isSetHwpId()) {
          hwpId = comRemoveBond.getHwpId();
        }
        else {
          hwpId = HWPID_DoNotCheck;
        }

        m_returnVerbose = comRemoveBond.getVerbose();
      }
      // parsing and checking service parameters failed 
      catch ( std::exception& ex ) {
        Document failResponse = createCheckParamsFailedResponse( comRemoveBond.getMsgId(), msgType, ex.what() );
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

        Document failResponse = getExclusiveAccessFailedResponse(comRemoveBond.getMsgId(), msgType, errorStr);
        m_iMessagingSplitterService->sendMessage(messagingId, std::move(failResponse));

        TRC_FUNCTION_LEAVE("");
        return;
      }

      RemoveBondResult removeBondResult = removeBond( deviceAddr, hwpId );

      // release exclusive access
      m_exclusiveAccess.reset();

      // creating response
      Document responseDoc = createResponse( comRemoveBond.getMsgId(), msgType, removeBondResult, comRemoveBond );

      // send response back
      m_iMessagingSplitterService->sendMessage( messagingId, std::move( responseDoc ) );

      TRC_FUNCTION_LEAVE( "" );
    }

    void activate( const shape::Properties *props )
    {
      TRC_FUNCTION_ENTER( "" );
      TRC_INFORMATION( std::endl <<
                       "************************************" << std::endl <<
                       "RemoveBondService instance activate" << std::endl <<
                       "************************************"
      );

      // for the sake of register function parameters 
      std::vector<std::string> supportedMsgTypes =
      {
        m_mTypeName_iqmeshNetworkRemoveBond
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
                       "RemoveBondService instance deactivate" << std::endl <<
                       "************************************"
      );

      std::vector<std::string> supportedMsgTypes =
      {
        m_mTypeName_iqmeshNetworkRemoveBond
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


  RemoveBondService::RemoveBondService()
  {
    m_imp = shape_new Imp( *this );
  }

  RemoveBondService::~RemoveBondService()
  {
    delete m_imp;
  }


  void RemoveBondService::attachInterface( iqrf::IIqrfDpaService* iface )
  {
    m_imp->attachInterface( iface );
  }

  void RemoveBondService::detachInterface( iqrf::IIqrfDpaService* iface )
  {
    m_imp->detachInterface( iface );
  }

  void RemoveBondService::attachInterface( iqrf::IMessagingSplitterService* iface )
  {
    m_imp->attachInterface( iface );
  }

  void RemoveBondService::detachInterface( iqrf::IMessagingSplitterService* iface )
  {
    m_imp->detachInterface( iface );
  }

  void RemoveBondService::attachInterface( shape::ITraceService* iface )
  {
    shape::Tracer::get().addTracerService( iface );
  }

  void RemoveBondService::detachInterface( shape::ITraceService* iface )
  {
    shape::Tracer::get().removeTracerService( iface );
  }


  void RemoveBondService::activate( const shape::Properties *props )
  {
    m_imp->activate( props );
  }

  void RemoveBondService::deactivate()
  {
    m_imp->deactivate();
  }

  void RemoveBondService::modify( const shape::Properties *props )
  {
    m_imp->modify( props );
  }

}
