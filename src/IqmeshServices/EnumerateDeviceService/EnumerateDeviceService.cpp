#define IEnumerateDeviceService_EXPORTS

#include "EnumerateDeviceService.h"
#include "Trace.h"
#include "ComIqmeshNetworkEnumerateDevice.h"
#include "ObjectFactory.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "HexStringCoversion.h"
#include "iqrf__EnumerateDeviceService.hxx"
#include <list>
#include <memory>
#include <math.h>
#include <bitset>

TRC_INIT_MODULE(iqrf::EnumerateDeviceService);

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

  // holds parsed data from OS read response
  class OsReadObject {
  public:
    class TrMcuType {
    public:
      uint8_t value;
      std::string trType;
      bool fccCertified;
      std::string mcuType;
    };

    class Flags {
    public:
      uint8_t value;
      bool insufficientOsBuild;
      std::string interface;
      bool dpaHandlerDetected;
      bool dpaHandlerNotDetectedButEnabled;
      bool noInterfaceSupported;
    };

    class SlotLimits {
    public:
      uint8_t value;
      std::string shortestTimeslot;
      std::string longestTimeslot;
    };

    std::string mid;
    std::string osVersion;
    TrMcuType trMcuType;
    std::string osBuild;
    std::string rssi;
    std::string supplyVoltage;
    Flags flags;
    SlotLimits slotLimits;

  };

  // maximum number of repeats
  static const uint8_t REPEAT_MAX = 3;

  // size of discovered devices bitmap
  static const uint8_t DISCOVERED_DEVICES_BITMAP_SIZE = 30;

  // length of MID [in bytes]
  static const uint8_t MID_LEN = 4;

  // length of EmbeddedPers field [in bytes]
  static const uint8_t EMBEDDED_PERS_LEN = 4;

  // length of UserPer field [in bytes]
  static const uint8_t USER_PER_LEN = 12;

  // length of configuration field [in bytes]
  static const uint8_t CONFIGURATION_LEN = 31;

  // number of peripherals in DPA response: get info for more peripherals
  static const uint8_t PERIPHERALS_NUM = 14;


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
  // Holds information about errors, which encounter during device enumerate service run
  class DeviceEnumerateError {
  public:
    // Type of error
    enum class Type {
      NoError = 0,
      ServiceError = 1000,
      NotBonded = 1001,
      ReadDiscoveryByte = 1002,
      OsRead = 1003,
      PerEnum = 1004,
      ReadHwp = 1005,
      MorePersInfo = 1006
    };

    DeviceEnumerateError() : m_type(Type::NoError), m_message("ok") {};
    DeviceEnumerateError(Type errorType) : m_type(errorType), m_message("") {};
    DeviceEnumerateError(Type errorType, const std::string& message) : m_type(errorType), m_message(message) {};

    Type getType() const { return m_type; };
    std::string getMessage() const { return m_message; };

    DeviceEnumerateError& operator=(const DeviceEnumerateError& error) {
      if (this == &error) {
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

  // holds information about result of device enumeration
  class DeviceEnumerateResult {
  private:
    // results of partial DPA requests
    DeviceEnumerateError m_bondedError;
    DeviceEnumerateError m_discoveryByteReadError;
    DeviceEnumerateError m_osReadError;
    DeviceEnumerateError m_perEnumError;
    DeviceEnumerateError m_hwpConfigReadError;
    DeviceEnumerateError m_morePersInfoError;
    uint16_t m_deviceAddr;
    bool m_discovered;
    uint8_t m_vrn;
    uint8_t m_zone;
    uint8_t m_parent;
    uint16_t m_enumeratedNodeHwpIdVer;
    std::vector<uns8> m_osRead;
    uint16_t m_osBuild;
    TEnumPeripheralsAnswer m_perEnum;
    TPerOSReadCfg_Response m_hwpConfig;
    std::vector<TPeripheralInfoAnswer> m_morePersInfo;
    uint16_t m_nodeHwpId;
    std::string m_manufacturer = "";
    std::string m_product = "";
    std::list<std::string> m_standards = { "" };
    // transaction results
    std::list<std::unique_ptr<IDpaTransactionResult2>> m_transResults;

  public:
  
    DeviceEnumerateError getBondedError() const { return m_bondedError; };
    void setBondedError(const DeviceEnumerateError& error) {
      m_bondedError = error;
    }

    DeviceEnumerateError getDiscoveryByteReadError() const { return m_discoveryByteReadError; };
    void setDiscoveryByteReadError(const DeviceEnumerateError& error) {
      m_discoveryByteReadError = error;
    }

    DeviceEnumerateError getOsReadError() const { return m_osReadError; };
    void setOsReadError(const DeviceEnumerateError& error) {
      m_osReadError = error;
    }

    DeviceEnumerateError getPerEnumError() const { return m_perEnumError; };
    void setPerEnumError(const DeviceEnumerateError& error) {
      m_perEnumError = error;
    }

    DeviceEnumerateError getHwpConfigReadError() const { return m_hwpConfigReadError; };
    void setHwpConfigReadError(const DeviceEnumerateError& error) {
      m_hwpConfigReadError = error;
    }

    DeviceEnumerateError getMorePersInfoError() const { return m_morePersInfoError; };
    void setMorePersInfoError(const DeviceEnumerateError& error) {
      m_morePersInfoError = error;
    }


    uint16_t getDeviceAddr() const {
      return m_deviceAddr;
    }

    void setDeviceAddr(const uint16_t deviceAddr) {
      m_deviceAddr = deviceAddr;
    }

    // sets HwpId of enumerated node
    void setEnumeratedNodeHwpId(const uint16_t hwpId) {
      m_nodeHwpId = hwpId;
    }

    // returns HwpId of enumerated node
    uint16_t getEnumeratedNodeHwpId() const {
      return m_nodeHwpId;
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

    // sets HwpId version of enumerated node
    void setEnumeratedNodeHwpIdVer(const uint16_t hwpIdVer) {
      m_enumeratedNodeHwpIdVer = hwpIdVer;
    }

    // returns HwpId version of enumerated node
    uint16_t getEnumeratedNodeHwpIdVer() const {
      return m_enumeratedNodeHwpIdVer;
    }

    bool isDiscovered() const {
      return m_discovered;
    }

    void setDiscovered(bool discovered) {
      m_discovered = discovered;
    }

    uint8_t getVrn() const {
      return m_vrn;
    }

    void setVrn(uint8_t vrn) {
      m_vrn = vrn;
    }

    uint8_t getZone() const {
      return m_zone;
    }

    void setZone(uint8_t zone) {
      m_zone = zone;
    }

    uint8_t getParent() const {
      return m_parent;
    }

    void setParent(uint8_t parent) {
      m_parent = parent;
    }

    const std::vector<uns8> getOsRead() {
      return m_osRead;
    }

    void setOsRead(const uns8* osRead) {
      m_osRead.insert(m_osRead.begin(), osRead, osRead + DPA_MAX_DATA_LENGTH);
    }

    uint16_t getOsBuild() const {
      return m_osBuild;
    }

    void setOsBuild(uint16_t osBuild) {
      m_osBuild = osBuild;
    }

    TEnumPeripheralsAnswer getPerEnum() const {
      return m_perEnum;
    }

    void setPerEnum(TEnumPeripheralsAnswer perEnum) {
      m_perEnum = perEnum;
    }

    TPerOSReadCfg_Response getHwpConfig() const {
      return m_hwpConfig;
    }

    void setHwpConfig(TPerOSReadCfg_Response hwpConfig) {
      m_hwpConfig = hwpConfig;
    }

    std::vector<TPeripheralInfoAnswer> getMorePersInfo() const {
      return m_morePersInfo;
    }

    void setMorePersInfo(const std::vector<TPeripheralInfoAnswer>& morePersInfo) {
      m_morePersInfo = morePersInfo;
    }

    std::list<std::string> getStandards() {
      return m_standards;
    }

    void setStandards(std::list<std::string> standards) {
      m_standards = standards;
    }

    // adds transaction result into the list of results
    void addTransactionResult(std::unique_ptr<IDpaTransactionResult2>& transResult)
    {
      if( transResult != nullptr )
        m_transResults.push_back(std::move(transResult));
    }

    bool isNextTransactionResult() {
      return (m_transResults.size() > 0);
    }

    // consumes the first element in the transaction results list
    std::unique_ptr<IDpaTransactionResult2> consumeNextTransactionResult() {
      std::list<std::unique_ptr<IDpaTransactionResult2>>::iterator iter = m_transResults.begin();
      std::unique_ptr<IDpaTransactionResult2> tranResult = std::move(*iter);
      m_transResults.pop_front();
      return std::move(tranResult);
    }
  };

  // implementation class
  class EnumerateDeviceService::Imp {
  private:
    // parent object
    EnumerateDeviceService& m_parent;

    // message type: IQMESH Network Enumerate Device
    // for temporal reasons
    const std::string m_mTypeName_iqmeshNetworkEnumerateDevice = "iqmeshNetwork_EnumerateDevice";

    iqrf::IJsCacheService* m_iJsCacheService = nullptr;
    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    IIqrfDpaService* m_iIqrfDpaService = nullptr;
    std::unique_ptr<IIqrfDpaService::ExclusiveAccess> m_exclusiveAccess;

    // number of repeats
    uint8_t m_repeat;
    // if is set Verbose mode
    bool m_returnVerbose = false;
    bool m_morePeripheralsInfo = false;
    
  public:
    Imp( EnumerateDeviceService& parent ) : m_parent( parent )
    {
    }

    ~Imp()
    {
    }
    
  private:

    // Returns 1 byte of discovery data read from the coordinator private external EEPROM storage
    uint8_t readDiscoveryByte( DeviceEnumerateResult& deviceEnumerateResult, uint16_t address )
    {
      TRC_FUNCTION_ENTER( "" );
      DpaMessage eeepromReadRequest;
      DpaMessage::DpaPacket_t eeepromReadPacket;
      eeepromReadPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
      eeepromReadPacket.DpaRequestPacket_t.PNUM = PNUM_EEEPROM;
      eeepromReadPacket.DpaRequestPacket_t.PCMD = CMD_EEEPROM_XREAD;
      eeepromReadPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;

      // read data from specified address
      uns8* pData = eeepromReadPacket.DpaRequestPacket_t.DpaMessage.Request.PData;
      pData[0] = address & 0xFF;
      pData[1] = ( address >> 8 ) & 0xFF;
      // length of data to read[in bytes]
      pData[2] = 1;
      eeepromReadRequest.DataToBuffer( eeepromReadPacket.Buffer, sizeof( TDpaIFaceHeader ) + 3 );

      // Execute the DPA request
      std::unique_ptr<IDpaTransactionResult2> transResult;
      m_exclusiveAccess->executeDpaTransactionRepeat( eeepromReadRequest, transResult, m_repeat );
      TRC_DEBUG( "Result from EEEPROM X read transaction as string:" << PAR( transResult->getErrorString() ) );
      // because of the move-semantics
      DpaMessage dpaResponse = transResult->getResponse();
      deviceEnumerateResult.addTransactionResult( transResult );
      TRC_INFORMATION( "EEEPROM X read successful!" );
      TRC_DEBUG(
        "DPA transaction: "
        << NAME_PAR( eeepromReadRequest.PeripheralType(), eeepromReadRequest.NodeAddress() )
        << PAR( eeepromReadRequest.PeripheralCommand() )
      );
      TRC_FUNCTION_LEAVE( "" );
      return dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData[0];
    }

    // Read discovery data
    void discoveryData(DeviceEnumerateResult& deviceEnumerateResult) 
    {
      // get discovered indicator
      try {
        uint16_t address = 0x20 + deviceEnumerateResult.getDeviceAddr() / 8;

        uint8_t discoveredDevicesByte = readDiscoveryByte(deviceEnumerateResult, address);
        
        uint8_t bitIndex = deviceEnumerateResult.getDeviceAddr() % 8;
        uint8_t compareByte = uint8_t(pow(2, bitIndex));

        deviceEnumerateResult.setDiscovered(
          (discoveredDevicesByte & compareByte) == compareByte
        );
      }
      catch (std::exception& ex) {
        DeviceEnumerateError error( DeviceEnumerateError::Type::ReadDiscoveryByte, ex.what() );
        deviceEnumerateResult.setDiscoveryByteReadError( error );
      }

      // VRN
      try {
        uint16_t address = 0x5000 + deviceEnumerateResult.getDeviceAddr();

        uint8_t vrnByte = readDiscoveryByte(deviceEnumerateResult, address);
        deviceEnumerateResult.setVrn(vrnByte);
      }
      catch (std::exception& ex) {
        DeviceEnumerateError error( DeviceEnumerateError::Type::ReadDiscoveryByte, ex.what() );
        deviceEnumerateResult.setDiscoveryByteReadError( error );
      }

      // zone
      try {
        uint16_t address = 0x5200 + deviceEnumerateResult.getDeviceAddr();

        uint8_t zoneByte = readDiscoveryByte(deviceEnumerateResult, address);
        deviceEnumerateResult.setZone(zoneByte);
      }
      catch (std::exception& ex) {
        DeviceEnumerateError error( DeviceEnumerateError::Type::ReadDiscoveryByte, ex.what() );
        deviceEnumerateResult.setDiscoveryByteReadError( error );
      }

      // parent
      try {
        uint16_t address = 0x5300 + deviceEnumerateResult.getDeviceAddr();

        uint8_t parentByte = readDiscoveryByte(deviceEnumerateResult, address);
        deviceEnumerateResult.setParent(parentByte);
      }
      catch (std::exception& ex) {
        DeviceEnumerateError error( DeviceEnumerateError::Type::ReadDiscoveryByte, ex.what() );
        deviceEnumerateResult.setDiscoveryByteReadError( error );
      }
    }
    
    // Reads OS info about smart connected node
    void osRead( DeviceEnumerateResult& deviceEnumerateResult )
    {
      TRC_FUNCTION_ENTER( "" );
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage osReadRequest;
        DpaMessage::DpaPacket_t osReadPacket;
        osReadPacket.DpaRequestPacket_t.NADR = deviceEnumerateResult.getDeviceAddr();
        osReadPacket.DpaRequestPacket_t.PNUM = PNUM_OS;
        osReadPacket.DpaRequestPacket_t.PCMD = CMD_OS_READ;
        osReadPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        osReadRequest.DataToBuffer( osReadPacket.Buffer, sizeof( TDpaIFaceHeader ) );
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat( osReadRequest, transResult, m_repeat );
        TRC_DEBUG( "Result from OS read transaction as string:" << PAR( transResult->getErrorString() ) );
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION( "OS read successful!" );
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR( osReadRequest.PeripheralType(), osReadRequest.NodeAddress() )
          << PAR( (unsigned)osReadRequest.PeripheralCommand() )
        );
        // Parse response pdata
        uns8* osData = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData;
        deviceEnumerateResult.setOsRead( osData );
        TPerOSRead_Response resp = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerOSRead_Response;
        deviceEnumerateResult.setOsBuild( resp.OsBuild );
        deviceEnumerateResult.setEnumeratedNodeHwpId( dpaResponse.DpaPacket().DpaResponsePacket_t.HWPID );
      }
      catch ( std::exception& e )
      {
        DeviceEnumerateError error( DeviceEnumerateError::Type::OsRead, e.what() );
        deviceEnumerateResult.setOsReadError( error );
      }
      deviceEnumerateResult.addTransactionResult( transResult );
      TRC_FUNCTION_LEAVE( "" );
    }
    
    // Peripheral enumeration
    void peripheralEnumeration( DeviceEnumerateResult& deviceEnumerateResult )
    {
      TRC_FUNCTION_ENTER( "" );
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage perEnumRequest;
        DpaMessage::DpaPacket_t perEnumPacket;
        perEnumPacket.DpaRequestPacket_t.NADR = deviceEnumerateResult.getDeviceAddr();
        perEnumPacket.DpaRequestPacket_t.PNUM = 0xFF;
        perEnumPacket.DpaRequestPacket_t.PCMD = 0x3F;
        perEnumPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        perEnumRequest.DataToBuffer( perEnumPacket.Buffer, sizeof( TDpaIFaceHeader ) );
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat( perEnumRequest, transResult, m_repeat );
        TRC_DEBUG( "Result from peripheral enumeration transaction as string:" << PAR( transResult->getErrorString() ) );
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION( "Peripheral enumeration successful!" );
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR( perEnumRequest.PeripheralType(), perEnumRequest.NodeAddress() )
          << PAR( perEnumRequest.PeripheralCommand() )
        );
        // Parse response pdata
        TEnumPeripheralsAnswer perEnum = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.EnumPeripheralsAnswer;
        deviceEnumerateResult.setPerEnum( perEnum );
        uns8* respData = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData;
        uint8_t minorHwpIdVer = respData[9];
        uint8_t majorHwpIdVer = respData[10];
        deviceEnumerateResult.setEnumeratedNodeHwpIdVer( minorHwpIdVer + ( majorHwpIdVer << 8 ) );

        TPeripheralInfoAnswer* persInfoArr = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PeripheralInfoAnswers;
        std::vector<TPeripheralInfoAnswer> persInfoList;
        for ( int i = 0; i < PERIPHERALS_NUM; i++ )
          persInfoList.push_back( persInfoArr[i] );
        deviceEnumerateResult.setMorePersInfo( persInfoList );
      }
      catch ( std::exception& e )
      {
        DeviceEnumerateError error( DeviceEnumerateError::Type::PerEnum, e.what() );
        deviceEnumerateResult.setPerEnumError( error );
      }
      deviceEnumerateResult.addTransactionResult( transResult );
      TRC_FUNCTION_LEAVE( "" );
    }

    // Read HWP configuration
    void readHwpConfiguration( DeviceEnumerateResult& deviceEnumerateResult )
    {
      TRC_FUNCTION_ENTER( "" );
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage readHwpRequest;
        DpaMessage::DpaPacket_t readHwpPacket;
        readHwpPacket.DpaRequestPacket_t.NADR = deviceEnumerateResult.getDeviceAddr();
        readHwpPacket.DpaRequestPacket_t.PNUM = PNUM_OS;
        readHwpPacket.DpaRequestPacket_t.PCMD = CMD_OS_READ_CFG;
        readHwpPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        readHwpRequest.DataToBuffer( readHwpPacket.Buffer, sizeof( TDpaIFaceHeader ) );
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat( readHwpRequest, transResult, m_repeat );
        TRC_DEBUG( "Result from read HWP config transaction as string:" << PAR( transResult->getErrorString() ) );
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION( "Read HWP configuration successful!" );
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR( readHwpRequest.PeripheralType(), readHwpRequest.NodeAddress() )
          << PAR( (unsigned)readHwpRequest.PeripheralCommand() )
        );
        // Parse response pdata
        TPerOSReadCfg_Response readHwpConfig = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerOSReadCfg_Response;
        deviceEnumerateResult.setHwpConfig( readHwpConfig );
      }
      catch ( std::exception& e )
      {
        DeviceEnumerateError error( DeviceEnumerateError::Type::ReadHwp, e.what() );
        deviceEnumerateResult.setHwpConfigReadError( error );
      }
      deviceEnumerateResult.addTransactionResult( transResult );
      TRC_FUNCTION_LEAVE( "" );
    }

    // Check the specified node is bonded (in the future read from DB)
    bool isNodeBonded( DeviceEnumerateResult& deviceEnumerateResult, const uint16_t deviceAddr )
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
        if ( ( result = ( bondedNodesArr[byteIndex] & compareByte ) == compareByte ) == false )
        {
          DeviceEnumerateError deviceEnumerateError( DeviceEnumerateError::Type::NotBonded, "Node not bonded." );
          deviceEnumerateResult.setBondedError( deviceEnumerateError );
        }
      }
      catch ( std::exception& e )
      {
        DeviceEnumerateError deviceEnumerateError( DeviceEnumerateError::Type::NotBonded, e.what() );
        deviceEnumerateResult.setBondedError( deviceEnumerateError );
      }
      deviceEnumerateResult.addTransactionResult( transResult );
      TRC_FUNCTION_LEAVE( "" );
      return result;
    }     

    // Sets data for discovery response
    void setDiscoveryDataResponse( DeviceEnumerateResult& deviceEnumerateResult, rapidjson::Document& response )
    {
      // if no data was successfully read, return empty message
      Pointer( "/data/rsp/discovery/discovered" ).Set( response, deviceEnumerateResult.isDiscovered() );
      Pointer( "/data/rsp/discovery/vrn" ).Set( response, deviceEnumerateResult.getVrn() );
      Pointer( "/data/rsp/discovery/zone" ).Set( response, deviceEnumerateResult.getZone() );
      Pointer( "/data/rsp/discovery/parent" ).Set( response, deviceEnumerateResult.getParent() );
    }

    // Parses OS read info
    // TEMPORAL SOLUTION !!!
    OsReadObject parseOsReadResponse(const std::vector<uns8>& osReadInfo)
    {
      TRC_FUNCTION_ENTER(PAR(osReadInfo.size()))
      OsReadObject osReadObject;

      std::ostringstream moduleId;
      moduleId.fill('0');
      moduleId << std::hex << std::uppercase <<
        std::setw(2) << (int)osReadInfo[3] <<
        std::setw(2) << (int)osReadInfo[2] <<
        std::setw(2) << (int)osReadInfo[1] <<
        std::setw(2) << (int)osReadInfo[0];

      osReadObject.mid = moduleId.str();

      // OS version - string
      std::ostringstream osVer;
      uns8 osVersion = osReadInfo[4];

      osVer << std::hex << (int)(osVersion >> 4) << '.';
      osVer.fill('0');
      osVer << std::setw(2) << (int)(osVersion & 0xf) << 'D';

      osReadObject.osVersion = osVer.str();

      // trMcuType
      osReadObject.trMcuType.value = osReadInfo[5];
      uns8 mcuType = osReadInfo[5];

      std::string trTypeStr = "(DC)TR-";
      switch (mcuType >> 4) {
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

      osReadObject.trMcuType.trType = trTypeStr;
      osReadObject.trMcuType.fccCertified = ((mcuType & 0x08) == 0x08) ? true : false;
      osReadObject.trMcuType.mcuType = ((mcuType & 0x07) == 0x04) ? "PIC16LF1938" : "UNKNOWN";
    
      // OS build - string
      uint16_t osBuild = (osReadInfo[7] << 8) + osReadInfo[6];
      osReadObject.osBuild = encodeHexaNum_CapitalLetters(osBuild);

      // RSSI [dBm]
      int8_t rssi = osReadInfo[8] - 130;
      std::string rssiStr = std::to_string(rssi) + " dBm";
      osReadObject.rssi = rssiStr;

      // Supply voltage [V]
      float supplyVoltage = 261.12f / (float)(127 - osReadInfo[9]);
      char supplyVoltageStr[8];
      std::sprintf(supplyVoltageStr, "%1.2f V", supplyVoltage);
      osReadObject.supplyVoltage = supplyVoltageStr;

      // Flags
      uns8 flags = osReadInfo[10];
      osReadObject.flags.value = flags;
      osReadObject.flags.insufficientOsBuild = ((flags & 0x01) == 0x01) ? true : false;
      osReadObject.flags.interface = ((flags & 0x02) == 0x02) ? "UART" : "SPI";
      osReadObject.flags.dpaHandlerDetected = ((flags & 0x04) == 0x04) ? true : false;
      osReadObject.flags.dpaHandlerNotDetectedButEnabled = ((flags & 0x08) == 0x08) ? true : false;
      osReadObject.flags.noInterfaceSupported = ((flags & 0x10) == 0x10) ? true : false;

      // Slot limits
      uns8 slotLimits = osReadInfo[11];
      osReadObject.slotLimits.value = slotLimits;
      uint8_t shortestTimeSlot = ((slotLimits & 0x0f) + 3) * 10;
      uint8_t longestTimeSlot = (((slotLimits >> 0x04) & 0x0f) + 3) * 10;

      osReadObject.slotLimits.shortestTimeslot = std::to_string(shortestTimeSlot) + " ms";
      osReadObject.slotLimits.longestTimeslot = std::to_string(longestTimeSlot) + " ms";

      TRC_FUNCTION_LEAVE("")
      return osReadObject;
    }

    // Sets OS read part of the response
    void setOsReadResponse( const OsReadObject& osReadObject, Document& response )
    {
      rapidjson::Pointer( "/data/rsp/osRead/mid" ).Set( response, osReadObject.mid );
      rapidjson::Pointer( "/data/rsp/osRead/osVersion" ).Set( response, osReadObject.osVersion );

      rapidjson::Pointer( "/data/rsp/osRead/trMcuType/value" ).Set( response, osReadObject.trMcuType.value );
      rapidjson::Pointer( "/data/rsp/osRead/trMcuType/trType" ).Set( response, osReadObject.trMcuType.trType );
      rapidjson::Pointer( "/data/rsp/osRead/trMcuType/fccCertified" ).Set( response, osReadObject.trMcuType.fccCertified );
      rapidjson::Pointer( "/data/rsp/osRead/trMcuType/mcuType" ).Set( response, osReadObject.trMcuType.mcuType );

      rapidjson::Pointer( "/data/rsp/osRead/osBuild" ).Set( response, osReadObject.osBuild );

      // RSSI [dBm]
      rapidjson::Pointer( "/data/rsp/osRead/rssi" ).Set( response, osReadObject.rssi );

      // Supply voltage [V]
      rapidjson::Pointer( "/data/rsp/osRead/supplyVoltage" ).Set( response, osReadObject.supplyVoltage );

      // Flags
      rapidjson::Pointer( "/data/rsp/osRead/flags/value" ).Set( response, osReadObject.flags.value );
      rapidjson::Pointer( "/data/rsp/osRead/flags/insufficientOsBuild" ).Set( response, osReadObject.flags.insufficientOsBuild );
      rapidjson::Pointer( "/data/rsp/osRead/flags/interfaceType" ).Set( response, osReadObject.flags.interface );
      rapidjson::Pointer( "/data/rsp/osRead/flags/dpaHandlerDetected" ).Set( response, osReadObject.flags.dpaHandlerDetected );
      rapidjson::Pointer( "/data/rsp/osRead/flags/dpaHandlerNotDetectedButEnabled" ).Set( response, osReadObject.flags.dpaHandlerNotDetectedButEnabled );
      rapidjson::Pointer( "/data/rsp/osRead/flags/noInterfaceSupported" ).Set( response, osReadObject.flags.noInterfaceSupported );

      // Slot limits
      rapidjson::Pointer( "/data/rsp/osRead/slotLimits/value" ).Set( response, osReadObject.slotLimits.value );
      rapidjson::Pointer( "/data/rsp/osRead/slotLimits/shortestTimeslot" ).Set( response, osReadObject.slotLimits.shortestTimeslot );
      rapidjson::Pointer( "/data/rsp/osRead/slotLimits/longestTimeslot" ).Set( response, osReadObject.slotLimits.longestTimeslot );
    }

    // Sets peripheral enumeration part of the response
    void setPeripheralEnumerationResponse( const DeviceEnumerateResult& deviceEnumerateResult, rapidjson::Document& response ) 
    {
      // per enum object
      TEnumPeripheralsAnswer perEnum = deviceEnumerateResult.getPerEnum();
      
      // dpa version - string
      std::string dpaVerStr = std::to_string((perEnum.DpaVersion >> 8) & 0xFF)
        + "." + encodeHexaNum((uint8_t)(perEnum.DpaVersion & 0xFF));
      Pointer("/data/rsp/peripheralEnumeration/dpaVer").Set(response, dpaVerStr);
      
      // perNr
      Pointer("/data/rsp/peripheralEnumeration/perNr").Set(response, perEnum.UserPerNr);

      Document::AllocatorType& allocator = response.GetAllocator();

      // embPers
      rapidjson::Value embPersJsonArray(kArrayType);
      for (int i = 0; i < EMBEDDED_PERS_LEN; i++) {
        embPersJsonArray.PushBack(perEnum.EmbeddedPers[i], allocator);
      }
      Pointer("/data/rsp/peripheralEnumeration/embPers").Set(response, embPersJsonArray);

      // hwpId
      Pointer("/data/rsp/peripheralEnumeration/hwpId").Set(response, perEnum.HWPID);

      // hwpIdVer
      Pointer("/data/rsp/peripheralEnumeration/hwpIdVer").Set(response, perEnum.HWPIDver);


      // flags - int value
      Pointer("/data/rsp/peripheralEnumeration/flags/value").Set(response, perEnum.Flags);

      // flags - parsed
      bool stdModeSupported = ((perEnum.Flags & 0b1) == 0b1) ? true : false;
      if (stdModeSupported) {
        Pointer("/data/rsp/peripheralEnumeration/flags/rfModeStd").Set(response, true);
        Pointer("/data/rsp/peripheralEnumeration/flags/rfModeLp").Set(response, false);
      }
      else {
        Pointer("/data/rsp/peripheralEnumeration/flags/rfModeStd").Set(response, false);
        Pointer("/data/rsp/peripheralEnumeration/flags/rfModeLp").Set(response, true);
      }

      // Getting DPA version
      IIqrfDpaService::CoordinatorParameters coordParams = m_iIqrfDpaService->getCoordinatorParameters();
      uint16_t dpaVer = (coordParams.dpaVerMajor << 8) + coordParams.dpaVerMinor;

      TPerOSReadCfg_Response hwpConfig = deviceEnumerateResult.getHwpConfig();
      uns8* configuration = hwpConfig.Configuration;

      // STD+LP network is running, otherwise STD network.
      if (dpaVer >= 0x0400) {
        bool stdAndLpModeNetwork = ((perEnum.Flags & 0b100) == 0b100) ? true : false;
        if (stdAndLpModeNetwork) {
          Pointer("/data/rsp/peripheralEnumeration/flags/stdAndLpNetwork").Set(response, true);
        }
        else {
          Pointer("/data/rsp/peripheralEnumeration/flags/stdAndLpNetwork").Set(response, false);
        }
      }

      // UserPers
      rapidjson::Value userPerJsonArray(kArrayType);
      for (int i = 0; i < USER_PER_LEN; i++) {
        userPerJsonArray.PushBack(perEnum.UserPer[i], allocator);
      }
      Pointer("/data/rsp/peripheralEnumeration/userPers").Set(response, userPerJsonArray);
    }

    // Sets Read HWP Configuration part of the response
    void setReadHwpConfigurationResponse( const DeviceEnumerateResult& deviceEnumerateResult, rapidjson::Document& response )
    {
      // Getting DPA version
      IIqrfDpaService::CoordinatorParameters coordParams = m_iIqrfDpaService->getCoordinatorParameters();
      uint16_t dpaVer = ( coordParams.dpaVerMajor << 8 ) + coordParams.dpaVerMinor;

      TPerOSReadCfg_Response hwpConfig = deviceEnumerateResult.getHwpConfig();
      uns8* configuration = hwpConfig.Configuration;

      if ( dpaVer < 0x0303 )
        for ( int i = 0; i < CONFIGURATION_LEN; i++ )
          configuration[i] = configuration[i] ^ 0x34;

      Document::AllocatorType& allocator = response.GetAllocator();

      // Embedded periherals
      rapidjson::Value embPerBitsJsonArray( kArrayType );
      for ( int i = 0; i < 4; i++ )
        embPerBitsJsonArray.PushBack( configuration[i], allocator );
      Pointer( "/data/rsp/trConfiguration/embPers/values" ).Set( response, embPerBitsJsonArray );

      // embedded peripherals bits - parsed
      // byte 0x01
      uint8_t byte01 = configuration[0x00];

      bool coordPresent = ( ( byte01 & 0b1 ) == 0b1 ) ? true : false;
      Pointer( "/data/rsp/trConfiguration/embPers/coordinator" ).Set( response, coordPresent );

      bool nodePresent = ( ( byte01 & 0b10 ) == 0b10 ) ? true : false;
      Pointer( "/data/rsp/trConfiguration/embPers/node" ).Set( response, nodePresent );

      bool osPresent = ( ( byte01 & 0b100 ) == 0b100 ) ? true : false;
      Pointer( "/data/rsp/trConfiguration/embPers/os" ).Set( response, osPresent );

      bool eepromPresent = ( ( byte01 & 0b1000 ) == 0b1000 ) ? true : false;
      Pointer( "/data/rsp/trConfiguration/embPers/eeprom" ).Set( response, eepromPresent );

      bool eeepromPresent = ( ( byte01 & 0b10000 ) == 0b10000 ) ? true : false;
      Pointer( "/data/rsp/trConfiguration/embPers/eeeprom" ).Set( response, eeepromPresent );

      bool ramPresent = ( ( byte01 & 0b100000 ) == 0b100000 ) ? true : false;
      Pointer( "/data/rsp/trConfiguration/embPers/ram" ).Set( response, ramPresent );

      bool ledrPresent = ( ( byte01 & 0b1000000 ) == 0b1000000 ) ? true : false;
      Pointer( "/data/rsp/trConfiguration/embPers/ledr" ).Set( response, ledrPresent );

      bool ledgPresent = ( ( byte01 & 0b10000000 ) == 0b10000000 ) ? true : false;
      Pointer( "/data/rsp/trConfiguration/embPers/ledg" ).Set( response, ledgPresent );


      // byte 0x02
      uint8_t byte02 = configuration[0x01];

      bool spiPresent = ( ( byte02 & 0b1 ) == 0b1 ) ? true : false;
      Pointer( "/data/rsp/trConfiguration/embPers/spi" ).Set( response, spiPresent );

      bool ioPresent = ( ( byte02 & 0b10 ) == 0b10 ) ? true : false;
      Pointer( "/data/rsp/trConfiguration/embPers/io" ).Set( response, ioPresent );

      bool thermometerPresent = ( ( byte02 & 0b100 ) == 0b100 ) ? true : false;
      Pointer( "/data/rsp/trConfiguration/embPers/thermometer" ).Set( response, thermometerPresent );

      bool pwmPresent = ( ( byte02 & 0b1000 ) == 0b1000 ) ? true : false;
      Pointer( "/data/rsp/trConfiguration/embPers/pwm" ).Set( response, pwmPresent );

      bool uartPresent = ( ( byte02 & 0b10000 ) == 0b10000 ) ? true : false;
      Pointer( "/data/rsp/trConfiguration/embPers/uart" ).Set( response, uartPresent );

      bool frcPresent = ( ( byte02 & 0b100000 ) == 0b100000 ) ? true : false;
      Pointer( "/data/rsp/trConfiguration/embPers/frc" ).Set( response, frcPresent );

      // byte 0x05
      uint8_t byte05 = configuration[0x04];

      bool customDpaHandler = ( ( byte05 & 0b1 ) == 0b1 ) ? true : false;
      Pointer( "/data/rsp/trConfiguration/customDpaHandler" ).Set( response, customDpaHandler );

      // for DPA v4.00 downwards
      if ( dpaVer < 0x0400 ) {
        bool nodeDpaInterface = ( ( byte05 & 0b00000010 ) == 0b00000010 ) ? true : false;
        Pointer( "/data/rsp/trConfiguration/nodeDpaInterface" ).Set( response, nodeDpaInterface );
      }

      bool dpaAutoexec = ( ( byte05 & 0b100 ) == 0b100 ) ? true : false;
      Pointer( "/data/rsp/trConfiguration/dpaAutoexec" ).Set( response, dpaAutoexec );

      bool routingOff = ( ( byte05 & 0b1000 ) == 0b1000 ) ? true : false;
      Pointer( "/data/rsp/trConfiguration/routingOff" ).Set( response, routingOff );

      bool ioSetup = ( ( byte05 & 0b10000 ) == 0b10000 ) ? true : false;
      Pointer( "/data/rsp/trConfiguration/ioSetup" ).Set( response, ioSetup );

      bool peerToPeer = ( ( byte05 & 0b100000 ) == 0b100000 ) ? true : false;
      Pointer( "/data/rsp/trConfiguration/peerToPeer" ).Set( response, peerToPeer );

      // for DPA v3.03 onwards
      if ( dpaVer >= 0x0303 ) {
        bool neverSleep = ( ( byte05 & 0b01000000 ) == 0b01000000 ) ? true : false;
        Pointer( "/data/rsp/trConfiguration/neverSleep" ).Set( response, neverSleep );
      }

      // for DPA v4.00 onwards
      if ( dpaVer >= 0x0400 ) {
        bool stdAndLpNetwork = ( ( byte05 & 0b10000000 ) == 0b10000000 ) ? true : false;
        Pointer( "/data/rsp/trConfiguration/stdAndLpNetwork" ).Set( response, stdAndLpNetwork );
      }

      // bytes fields
      Pointer( "/data/rsp/trConfiguration/rfChannelA" ).Set( response, configuration[0x10] );
      Pointer( "/data/rsp/trConfiguration/rfChannelB" ).Set( response, configuration[0x11] );

      if ( dpaVer < 0x0400 ) {
        Pointer( "/data/rsp/trConfiguration/rfSubChannelA" ).Set( response, configuration[0x05] );
        Pointer( "/data/rsp/trConfiguration/rfSubChannelB" ).Set( response, configuration[0x06] );
      }

      Pointer( "/data/rsp/trConfiguration/txPower" ).Set( response, configuration[0x07] );
      Pointer( "/data/rsp/trConfiguration/rxFilter" ).Set( response, configuration[0x08] );
      Pointer( "/data/rsp/trConfiguration/lpRxTimeout" ).Set( response, configuration[0x09] );
      Pointer( "/data/rsp/trConfiguration/rfAltDsmChannel" ).Set( response, configuration[0x0B] );

      // BaudRate
      if ( configuration[0x0A] <= BAUD_RATES_SIZE )
        Pointer( "/data/rsp/trConfiguration/uartBaudrate" ).Set( response, BaudRates[configuration[0x0A]] );
      else
      {
        TRC_WARNING( "Unknown baud rate constant: " << PAR( configuration[0x0A] ) );
        Pointer( "/data/rsp/trConfiguration/uartBaudrate" ).Set( response, 0 );
      }

      // RFPGM byte
      uint8_t rfpgm = hwpConfig.RFPGM;

      bool rfPgmDualChannel = ( ( rfpgm & 0b00000011 ) == 0b00000011 ) ? true : false;
      Pointer( "/data/rsp/trConfiguration/rfPgmDualChannel" ).Set( response, rfPgmDualChannel );

      bool rfPgmLpMode = ( ( rfpgm & 0b00000100 ) == 0b00000100 ) ? true : false;
      Pointer( "/data/rsp/trConfiguration/rfPgmLpMode" ).Set( response, rfPgmLpMode );

      bool rfPgmIncorrectUpload = ( ( rfpgm & 0b00001000 ) == 0b00001000 ) ? true : false;
      Pointer( "/data/rsp/trConfiguration/rfPgmIncorrectUpload" ).Set( response, rfPgmIncorrectUpload );

      bool enableAfterReset = ( ( rfpgm & 0b00010000 ) == 0b00010000 ) ? true : false;
      Pointer( "/data/rsp/trConfiguration/rfPgmEnableAfterReset" ).Set( response, enableAfterReset );

      bool rfPgmTerminateAfter1Min = ( ( rfpgm & 0b01000000 ) == 0b01000000 ) ? true : false;
      Pointer( "/data/rsp/trConfiguration/rfPgmTerminateAfter1Min" ).Set( response, rfPgmTerminateAfter1Min );

      bool rfPgmTerminateMcuPin = ( ( rfpgm & 0b10000000 ) == 0b10000000 ) ? true : false;
      Pointer( "/data/rsp/trConfiguration/rfPgmTerminateMcuPin" ).Set( response, rfPgmTerminateMcuPin );

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
      Pointer( "/data/rsp/trConfiguration/rfBand" ).Set( response, rfBand );
    }

    // Sets Info for more peripherals part of the response
    void setInfoForMorePeripheralsResponse( const DeviceEnumerateResult& deviceEnumerateResult, rapidjson::Document& response )
    {
      // per enum object
      std::vector<TPeripheralInfoAnswer> moreInfoPers = deviceEnumerateResult.getMorePersInfo();

      // array of objects
      Document::AllocatorType& allocator = response.GetAllocator();
      rapidjson::Value perInfoJsonArray( kArrayType );

      for ( int i = 0; i < PERIPHERALS_NUM; i++ ) {
        rapidjson::Value perInfoObj( kObjectType );
        perInfoObj.AddMember(
          "perTe",
          moreInfoPers[i].PerTE,
          allocator
        );
        perInfoObj.AddMember(
          "perT",
          moreInfoPers[i].PerT,
          allocator
        );
        perInfoObj.AddMember(
          "par1",
          moreInfoPers[i].Par1,
          allocator
        );
        perInfoObj.AddMember(
          "par2",
          moreInfoPers[i].Par2,
          allocator
        );
        perInfoJsonArray.PushBack( perInfoObj, allocator );
      }
      Pointer( "/data/rsp/morePeripheralsInfo" ).Set( response, perInfoJsonArray );
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
      if ( msgType.m_type != m_mTypeName_iqmeshNetworkEnumerateDevice )
        THROW_EXC( std::logic_error, "Unsupported message type: " << PAR( msgType.m_type ) );

      // creating representation object
      ComIqmeshNetworkEnumerateDevice comEnumerateDevice( doc );

      // Create response and set cpommon parameters
      Document response;
      Pointer( "/mType" ).Set( response, msgType.m_type );
      Pointer( "/data/msgId" ).Set( response, comEnumerateDevice.getMsgId() );

      // Service input parameters
      uint16_t deviceAddr;

      // Parsing and checking service parameters
      try
      {
        m_repeat = comEnumerateDevice.getRepeat();
        if ( !comEnumerateDevice.isSetDeviceAddr() )
          THROW_EXC( std::logic_error, "Device address not set." );
        deviceAddr = comEnumerateDevice.getDeviceAddr();
        if ( deviceAddr > 239 )
          THROW_EXC( std::out_of_range, "Device address outside of valid range. " << NAME_PAR_HEX( "Address", deviceAddr ) );
        m_morePeripheralsInfo = comEnumerateDevice.getMorePeripheralsInfo();
        m_returnVerbose = comEnumerateDevice.getVerbose();
      }
      catch ( std::exception& ex )
      {
        // Parsing and checking service parameters failed 
        Pointer( "/data/status" ).Set( response, (int32_t)DeviceEnumerateError::Type::ServiceError );
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
        Pointer( "/data/status" ).Set( response, (int32_t)DeviceEnumerateError::Type::ServiceError );
        Pointer( "/data/statusStr" ).Set( response, errorStr );
        // Send response
        m_iMessagingSplitterService->sendMessage( messagingId, std::move( response ) );
        TRC_FUNCTION_LEAVE( "" );
        return;
      }

      // Result
      DeviceEnumerateResult deviceEnumerateResult;
      deviceEnumerateResult.setDeviceAddr( deviceAddr );

      // DeviceEnumerateError
      DeviceEnumerateError deviceEnumerateError;

      // Check the node is bonded
      if ( deviceAddr == COORDINATOR_ADDRESS || isNodeBonded( deviceEnumerateResult, deviceAddr ) )
      {
        // Discovery data
        discoveryData( deviceEnumerateResult );

        // OS read info
        osRead( deviceEnumerateResult );

        // Obtains hwpId, which in turn is needed to get manufacturer and product
        const IJsCacheService::Manufacturer* manufacturer = m_iJsCacheService->getManufacturer( deviceEnumerateResult.getEnumeratedNodeHwpId() );
        if ( manufacturer != nullptr )
          deviceEnumerateResult.setManufacturer( manufacturer->m_name );
        const IJsCacheService::Product* product = m_iJsCacheService->getProduct( deviceEnumerateResult.getEnumeratedNodeHwpId() );
        if ( product != nullptr )
          deviceEnumerateResult.setProduct( product->m_name );

        // Peripheral enumeration
        peripheralEnumeration( deviceEnumerateResult );

        // Fill standards
        std::string osBuildStr;
        {
          std::ostringstream os;
          os.fill( '0' );
          os << std::hex << std::uppercase << std::setw( 4 ) << (int)deviceEnumerateResult.getOsBuild();
          osBuildStr = os.str();
        }
        const IJsCacheService::Package* package = m_iJsCacheService->getPackage(
          deviceEnumerateResult.getEnumeratedNodeHwpId(),
          deviceEnumerateResult.getEnumeratedNodeHwpIdVer(),
          osBuildStr,
          m_iIqrfDpaService->getCoordinatorParameters().dpaVerWordAsStr
        );
        if ( package != nullptr )
        {
          std::list<std::string> standards;
          for ( const IJsCacheService::StdDriver* driver : package->m_stdDriverVect ) {
            standards.push_back( driver->getName() );
          }
          deviceEnumerateResult.setStandards( standards );
        }
        else
          TRC_INFORMATION( "Package not found" );

        // Read Hwp configuration
        readHwpConfiguration( deviceEnumerateResult );

        // Only one node - for the present time
        Pointer( "/data/rsp/deviceAddr" ).Set( response, deviceEnumerateResult.getDeviceAddr() );

        // Manufacturer and product - if OS Read was correct - to get HWP ID which is needed
        if ( deviceEnumerateResult.getOsReadError().getType() == DeviceEnumerateError::Type::NoError )
        {
          Pointer( "/data/rsp/manufacturer" ).Set( response, deviceEnumerateResult.getManufacturer() );
          Pointer( "/data/rsp/product" ).Set( response, deviceEnumerateResult.getProduct() );
          if ( deviceEnumerateResult.getPerEnumError().getType() == DeviceEnumerateError::Type::NoError )
          {
            // Standards - array of strings
            rapidjson::Value standardsJsonArray( kArrayType );
            Document::AllocatorType &allocator = response.GetAllocator();
            for ( std::string standard : deviceEnumerateResult.getStandards() )
            {
              rapidjson::Value standardJsonString;
              standardJsonString.SetString( standard.c_str(), (SizeType)standard.length(), allocator );
              standardsJsonArray.PushBack( standardJsonString, allocator );
            }
            Pointer( "/data/rsp/standards" ).Set( response, standardsJsonArray );
          }
        }

        // Discovery bytes read
        deviceEnumerateError = deviceEnumerateResult.getDiscoveryByteReadError();
        if ( deviceEnumerateError.getType() == DeviceEnumerateError::Type::NoError )
          setDiscoveryDataResponse( deviceEnumerateResult, response );

        // OS Read data
        deviceEnumerateError = deviceEnumerateResult.getOsReadError();
        if ( deviceEnumerateError.getType() == DeviceEnumerateError::Type::NoError )
        {
          const std::vector<uns8> osInfo = deviceEnumerateResult.getOsRead();
          OsReadObject osReadObject = parseOsReadResponse( osInfo );
          setOsReadResponse( osReadObject, response );
        }

        // Peripheral enumeration
        deviceEnumerateError = deviceEnumerateResult.getPerEnumError();
        if ( deviceEnumerateError.getType() == DeviceEnumerateError::Type::NoError )
          setPeripheralEnumerationResponse( deviceEnumerateResult, response );

        // HWP configuration
        deviceEnumerateError = deviceEnumerateResult.getHwpConfigReadError();
        if ( deviceEnumerateError.getType() == DeviceEnumerateError::Type::NoError )
          setReadHwpConfigurationResponse( deviceEnumerateResult, response );

        // Result of more peripherals info according to request
        if ( m_morePeripheralsInfo && deviceEnumerateResult.getOsReadError().getType() == DeviceEnumerateError::Type::NoError ) 
          setInfoForMorePeripheralsResponse( deviceEnumerateResult, response );
      }
      else
        deviceEnumerateError = deviceEnumerateResult.getBondedError();

      // Set raw fields, if verbose mode is active
      if ( comEnumerateDevice.getVerbose() )
      {
        rapidjson::Value rawArray( kArrayType );
        Document::AllocatorType& allocator = response.GetAllocator();
        while ( deviceEnumerateResult.isNextTransactionResult() )
        {
          std::unique_ptr<IDpaTransactionResult2> transResult = deviceEnumerateResult.consumeNextTransactionResult();
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

      // Release exclusive access
      m_exclusiveAccess.reset();

      // Set response status
      Pointer( "/data/status" ).Set( response, (int32_t)deviceEnumerateError.getType() );
      Pointer( "/data/statusStr" ).Set( response, deviceEnumerateError.getMessage() );

      // Send response
      m_iMessagingSplitterService->sendMessage( messagingId, std::move( response ) );
      TRC_FUNCTION_LEAVE( "" );
    }

  public:
    void activate(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************************" << std::endl <<
        "EnumerateDeviceService instance activate" << std::endl <<
        "******************************************"
      );

      // for the sake of register function parameters 
      std::vector<std::string> supportedMsgTypes =
      {
        m_mTypeName_iqmeshNetworkEnumerateDevice
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
        "EnumerateDeviceService instance deactivate" << std::endl <<
        "**************************************"
      );

      // for the sake of unregister function parameters 
      std::vector<std::string> supportedMsgTypes =
      {
        m_mTypeName_iqmeshNetworkEnumerateDevice
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

  EnumerateDeviceService::EnumerateDeviceService()
  {
    m_imp = shape_new Imp(*this);
  }

  EnumerateDeviceService::~EnumerateDeviceService()
  {
    delete m_imp;
  }

  void EnumerateDeviceService::attachInterface(iqrf::IIqrfDpaService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void EnumerateDeviceService::detachInterface(iqrf::IIqrfDpaService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void EnumerateDeviceService::attachInterface(iqrf::IJsCacheService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void EnumerateDeviceService::detachInterface(iqrf::IJsCacheService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void EnumerateDeviceService::attachInterface(iqrf::IMessagingSplitterService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void EnumerateDeviceService::detachInterface(iqrf::IMessagingSplitterService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void EnumerateDeviceService::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void EnumerateDeviceService::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }


  void EnumerateDeviceService::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void EnumerateDeviceService::deactivate()
  {
    m_imp->deactivate();
  }

  void EnumerateDeviceService::modify( const shape::Properties *props )
  {
    m_imp->modify( props );
  }
}