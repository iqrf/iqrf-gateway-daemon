#define IReadTrConfService_EXPORTS

#include "ReadTrConfService.h"
#include "Trace.h"
#include "ComIqmeshNetworkReadTrConf.h"
//#include "ObjectFactory.h"
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


  // service general fail code - may and probably will be changed later in the future
  static const int SERVICE_ERROR = 1000;

  static const int SERVICE_ERROR_NOERROR = 0;
  
  static const int SERVICE_ERROR_INTERNAL = SERVICE_ERROR + 1;
  static const int SERVICE_ERROR_READ_HWP = SERVICE_ERROR + 2;
};


namespace iqrf {

  // Holds information about errors, which encounter during read TR config service run
  class ReadTrConfigError {
  public:
    // Type of error
    enum class Type {
      NoError,
      ReadHwp
    };

    ReadTrConfigError() : m_type(Type::NoError), m_message("") {};
    ReadTrConfigError(Type errorType) : m_type(errorType), m_message("") {};
    ReadTrConfigError(Type errorType, const std::string& message) : m_type(errorType), m_message(message) {};

    Type getType() const { return m_type; };
    std::string getMessage() const { return m_message; };

    ReadTrConfigError& operator=(const ReadTrConfigError& error) {
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


  // holds information about result of read Tr configuration
  class ReadTrConfigResult {
  private:
    ReadTrConfigError m_error;
    std::list<uint16_t> m_deviceAddrs;
    TPerOSReadCfg_Response m_hwpConfig;

    // transaction results
    std::list<std::unique_ptr<IDpaTransactionResult2>> m_transResults;

  public:
    ReadTrConfigError getError() const { return m_error; };

    void setError(const ReadTrConfigError& error) {
      m_error = error;
    }

    std::list<uint16_t> getDeviceAddrs() {
      return m_deviceAddrs;
    }

    void setDeviceAddrs(const std::list<uint16_t>& deviceAddrs) {
      m_deviceAddrs = deviceAddrs;
    }

    TPerOSReadCfg_Response getHwpConfig() const {
      return m_hwpConfig;
    }

    void setHwpConfig(TPerOSReadCfg_Response hwpConfig) {
      m_hwpConfig = hwpConfig;
    }

    // adds transaction result into the list of results
    void addTransactionResult(std::unique_ptr<IDpaTransactionResult2>& transResult) {
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
  class ReadTrConfService::Imp {
  private:
    // parent object
    ReadTrConfService& m_parent;

    // message type: IQMESH Network Read TR Configuration
    // for temporal reasons
    const std::string m_mTypeName_iqmeshNetworkReadTrConf = "iqmeshNetwork_ReadTrConf";
    //IMessagingSplitterService::MsgType* m_msgType_mngIqmeshWriteConfig;

    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    IIqrfDpaService* m_iIqrfDpaService = nullptr;

    // number of repeats
    uint8_t m_repeat;

    // if is set Verbose mode
    bool m_returnVerbose = false;


  public:
    Imp(ReadTrConfService& parent) : m_parent(parent)
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
    
    

    // reads configuration of one node
    void _readTrConfigOneNode(
      ReadTrConfigResult& readTrConfigResult,
      const uint16_t deviceAddr,
      const uint16_t hwpId
    ) 
    {
      TRC_FUNCTION_ENTER("");
      
      DpaMessage readHwpRequest;
      DpaMessage::DpaPacket_t readHwpPacket;
      readHwpPacket.DpaRequestPacket_t.NADR = deviceAddr;
      readHwpPacket.DpaRequestPacket_t.PNUM = PNUM_OS;
      readHwpPacket.DpaRequestPacket_t.PCMD = CMD_OS_READ_CFG;
      readHwpPacket.DpaRequestPacket_t.HWPID = hwpId;
      readHwpRequest.DataToBuffer(readHwpPacket.Buffer, sizeof(TDpaIFaceHeader));

      // issue the DPA request
      std::shared_ptr<IDpaTransaction2> readHwpTransaction;
      std::unique_ptr<IDpaTransactionResult2> transResult;

      for (int rep = 0; rep <= m_repeat; rep++) {
        try {
          readHwpTransaction = m_iIqrfDpaService->executeDpaTransaction(readHwpRequest);
          transResult = readHwpTransaction->get();
        }
        catch (std::exception& e) {
          TRC_WARNING("DPA transaction error : " << e.what());

          if (rep < m_repeat) {
            continue;
          }

          ReadTrConfigError error(ReadTrConfigError::Type::ReadHwp, e.what());
          readTrConfigResult.setError(error);

          TRC_FUNCTION_LEAVE("");
          return;
        }

        TRC_DEBUG("Result from read HWP config transaction as string:" << PAR(transResult->getErrorString()));

        IDpaTransactionResult2::ErrorCode errorCode = (IDpaTransactionResult2::ErrorCode)transResult->getErrorCode();

        // because of the move-semantics
        DpaMessage dpaResponse = transResult->getResponse();
        readTrConfigResult.addTransactionResult(transResult);

        if (errorCode == IDpaTransactionResult2::ErrorCode::TRN_OK) {
          TRC_INFORMATION("Read HWP successful!");
          TRC_DEBUG(
            "DPA transaction: "
            << NAME_PAR(readHwpRequest.PeripheralType(), readHwpRequest.NodeAddress())
            << PAR(readHwpRequest.PeripheralCommand())
          );

          // parsing response pdata
          TPerOSReadCfg_Response hwpConfig = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerOSReadCfg_Response;
          readTrConfigResult.setHwpConfig(hwpConfig);

          TRC_FUNCTION_LEAVE("");
          return;
        }

        // transaction error
        if (errorCode < 0) {
          TRC_WARNING("Transaction error. " << NAME_PAR_HEX("Error code", errorCode));

          if (rep < m_repeat) {
            continue;
          }

          ReadTrConfigError error(ReadTrConfigError::Type::ReadHwp, "Transaction error.");
          readTrConfigResult.setError(error);

          TRC_FUNCTION_LEAVE("");
          return;
        }

        // DPA error
        TRC_WARNING("DPA error. " << NAME_PAR_HEX("Error code", errorCode));

        if (rep < m_repeat) {
          continue;
        }

        ReadTrConfigError error(ReadTrConfigError::Type::ReadHwp, "Dpa error.");
        readTrConfigResult.setError(error);

        TRC_FUNCTION_LEAVE("");
      } 
    }

    // reads configuration from specified target nodes
    void _readTrConfigNodes(
      ReadTrConfigResult& readTrConfigResult,
      const std::list<uint16_t>& targetNodes,
      const uint16_t hwpId
    ) 
    {
      // currently not implemented - return INTERNAL ERROR in the response
      THROW_EXC(std::logic_error, "Reading not implemented for multiple nodes.");
    }

    void _readTrConfig(
      ReadTrConfigResult& readTrConfigResult,
      const std::list<uint16_t>& targetNodes,
      const uint16_t hwpId
    ) 
    {
      if (targetNodes.size() == 1) {
        _readTrConfigOneNode(readTrConfigResult, targetNodes.front(), hwpId);
      }
      else {
        _readTrConfigNodes(readTrConfigResult, targetNodes, hwpId);
      }
    }

    ReadTrConfigResult readTrConfig(const std::list<uint16_t>& deviceAddrs, uint16_t hwpId)
    {
      TRC_FUNCTION_ENTER("");

      // result
      ReadTrConfigResult readTrConfigResult;
      
      // set adresses of target nodes
      readTrConfigResult.setDeviceAddrs(deviceAddrs);

      // read HWP configuration
      _readTrConfig(readTrConfigResult, deviceAddrs, hwpId);

      TRC_FUNCTION_LEAVE("");
      return readTrConfigResult;
    }


    // parses RF band
    std::string parseRfBand(const uint8_t rfBand) {
      switch (rfBand) {
        case 0b00:
          return "868";
        case 0b01:
          return "916";
        case 0b10:
          return "433";
        default:
          THROW_EXC(std::out_of_range, "Unsupported coordinator RF band: " << PAR(rfBand));
      }
    }

    uint32_t parseBaudRate(uint8_t baudRateId) {
      if ((baudRateId < 0) || (baudRateId >= BAUD_RATES_SIZE)) {
        THROW_EXC(std::out_of_range,"Baud rate ID out of range: " << PAR(baudRateId) );
      }
      return BaudRates[baudRateId];
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

    // sets response VERBOSE data
    void setVerboseData(rapidjson::Document& response, ReadTrConfigResult& readTrConfigResult)
    {
      rapidjson::Value rawArray(kArrayType);
      Document::AllocatorType& allocator = response.GetAllocator();

      while (readTrConfigResult.isNextTransactionResult()) {
        std::unique_ptr<IDpaTransactionResult2> transResult = readTrConfigResult.consumeNextTransactionResult();
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
    void setResponseStatus(Document& response, const ReadTrConfigError& error)
    {
      switch (error.getType()) {
        case ReadTrConfigError::Type::NoError:
          Pointer("/data/status").Set(response, SERVICE_ERROR_NOERROR);
          break;
        case ReadTrConfigError::Type::ReadHwp:
          Pointer("/data/status").Set(response, SERVICE_ERROR_READ_HWP);
          break;
        default:
          // some other unsupported error
          Pointer("/data/status").Set(response, SERVICE_ERROR);
          break;
      }

      if (error.getType() == ReadTrConfigError::Type::NoError) {
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
      ReadTrConfigResult& readTrConfigResult,
      const ComIqmeshNetworkReadTrConf& comReadTrConf
    )
    {
      Document response;

      // set common parameters
      Pointer("/mType").Set(response, msgType.m_type);
      Pointer("/data/msgId").Set(response, msgId);

      // only one node - for the present time
      uint16_t firstAddr = readTrConfigResult.getDeviceAddrs().front();
      Pointer("/data/rsp/deviceAddr").Set(response, firstAddr);

      // checking of error
      ReadTrConfigError error = readTrConfigResult.getError();

      if (error.getType() != ReadTrConfigError::Type::NoError) {
        // set raw fields, if verbose mode is active
        if (comReadTrConf.getVerbose()) {
          setVerboseData(response, readTrConfigResult);
        }

        setResponseStatus(response, error);
        return response;
      }

      // osRead object
      TPerOSReadCfg_Response hwpConfig = readTrConfigResult.getHwpConfig();

      // getting DPA version
      IIqrfDpaService::CoordinatorParameters coordParams = m_iIqrfDpaService->getCoordinatorParameters();
      uint16_t dpaVer = (coordParams.dpaVerMajor << 8) + coordParams.dpaVerMinor;

      uns8* configuration = hwpConfig.Configuration;

      if (dpaVer < 0x0303) {
        for (int i = 0; i < CONFIGURATION_LEN; i++) {
          configuration[i] = configuration[i] ^ 0x34;
        }
      }

      Document::AllocatorType& allocator = response.GetAllocator();

      // predefined peripherals - bits
      rapidjson::Value embPerBitsJsonArray(kArrayType);
      for (int i = 0; i < 4; i++) {
        embPerBitsJsonArray.PushBack(configuration[i], allocator);
      }
      Pointer("/data/rsp/embPers/values").Set(response, embPerBitsJsonArray);

      // embedded peripherals bits - parsed
      // byte 0x01
      uint8_t byte01 = configuration[0x00];

      bool coordPresent = ((byte01 & 0b1) == 0b1) ? true : false;
      Pointer("/data/rsp/embPers/coordinator").Set(response, coordPresent);

      bool nodePresent = ((byte01 & 0b10) == 0b10) ? true : false;
      Pointer("/data/rsp/embPers/node").Set(response, nodePresent);

      bool osPresent = ((byte01 & 0b100) == 0b100) ? true : false;
      Pointer("/data/rsp/embPers/os").Set(response, osPresent);

      bool eepromPresent = ((byte01 & 0b1000) == 0b1000) ? true : false;
      Pointer("/data/rsp/embPers/eeprom").Set(response, eepromPresent);

      bool eeepromPresent = ((byte01 & 0b10000) == 0b10000) ? true : false;
      Pointer("/data/rsp/embPers/eeeprom").Set(response, eeepromPresent);

      bool ramPresent = ((byte01 & 0b100000) == 0b100000) ? true : false;
      Pointer("/data/rsp/embPers/ram").Set(response, ramPresent);

      bool ledrPresent = ((byte01 & 0b1000000) == 0b1000000) ? true : false;
      Pointer("/data/rsp/embPers/ledr").Set(response, ledrPresent);

      bool ledgPresent = ((byte01 & 0b10000000) == 0b10000000) ? true : false;
      Pointer("/data/rsp/embPers/ledg").Set(response, ledgPresent);


      // byte 0x02
      uint8_t byte02 = configuration[0x01];

      bool spiPresent = ((byte02 & 0b1) == 0b1) ? true : false;
      Pointer("/data/rsp/embPers/spi").Set(response, spiPresent);

      bool ioPresent = ((byte02 & 0b10) == 0b10) ? true : false;
      Pointer("/data/rsp/embPers/io").Set(response, ioPresent);

      bool thermometerPresent = ((byte02 & 0b100) == 0b100) ? true : false;
      Pointer("/data/rsp/embPers/thermometer").Set(response, thermometerPresent);

      bool pwmPresent = ((byte02 & 0b1000) == 0b1000) ? true : false;
      Pointer("/data/rsp/embPers/pwm").Set(response, pwmPresent);

      bool uartPresent = ((byte02 & 0b10000) == 0b10000) ? true : false;
      Pointer("/data/rsp/embPers/uart").Set(response, uartPresent);

      bool frcPresent = ((byte02 & 0b100000) == 0b100000) ? true : false;
      Pointer("/data/rsp/embPers/frc").Set(response, frcPresent);
      


      // byte 0x05
      uint8_t byte05 = configuration[0x04];

      bool customDpaHandler = ((byte05 & 0b00000001) == 0b00000001) ? true : false;
      Pointer("/data/rsp/customDpaHandler").Set(response, customDpaHandler);


      // for DPA v4.00 downwards
      if (dpaVer < 0x0400) {
        bool nodeDpaInterface = ((byte05 & 0b00000010) == 0b00000010) ? true : false;
        Pointer("/data/rsp/nodeDpaInterface").Set(response, nodeDpaInterface);
      }

      bool dpaAutoexec = ((byte05 & 0b00000100) == 0b00000100) ? true : false;
      Pointer("/data/rsp/dpaAutoexec").Set(response, dpaAutoexec);

      bool routingOff = ((byte05 & 0b00001000) == 0b00001000) ? true : false;
      Pointer("/data/rsp/routingOff").Set(response, routingOff);

      bool ioSetup = ((byte05 & 0b00010000) == 0b00010000) ? true : false;
      Pointer("/data/rsp/ioSetup").Set(response, ioSetup);

      bool peerToPeer = ((byte05 & 0b00100000) == 0b00100000) ? true : false;
      Pointer("/data/rsp/peerToPeer").Set(response, peerToPeer);


      // for DPA v3.03 onwards
      if (dpaVer >= 0x0303) {
        bool neverSleep = ((byte05 & 0b01000000) == 0b01000000) ? true : false;
        Pointer("/data/rsp/neverSleep").Set(response, neverSleep);
      }

      // for DPA v4.00 onwards
      if (dpaVer >= 0x0400) {
        bool stdAndLpNetwork = ((byte05 & 0b10000000) == 0b10000000) ? true : false;
        Pointer("/data/rsp/stdAndLpNetwork").Set(response, stdAndLpNetwork);
      }

      // bytes fields
      Pointer("/data/rsp/rfChannelA").Set(response, configuration[0x10]);      
      Pointer("/data/rsp/rfChannelB").Set(response, configuration[0x11]);
      Pointer("/data/rsp/rfSubChannelA").Set(response, configuration[0x05]);
      Pointer("/data/rsp/rfSubChannelB").Set(response, configuration[0x06]);
      Pointer("/data/rsp/txPower").Set(response, configuration[0x07]);
      Pointer("/data/rsp/rxFilter").Set(response, configuration[0x08]);
      Pointer("/data/rsp/lpRxTimeout").Set(response, configuration[0x09]);
      Pointer("/data/rsp/rfPgmAltChannel").Set(response, configuration[0x0B]);

      try {
        uint32_t baudRate = parseBaudRate(configuration[0x0A]);
        Pointer("/data/rsp/uartBaudrate").Set(response, baudRate);
      }
      catch (std::exception& ex) {
        TRC_WARNING("Unknown baud rate constant: " << PAR(configuration[0x0A]));
        Pointer("/data/rsp/uartBaudrate").Set(response, 0);
      }

      // RFPGM byte
      uint8_t rfpgm = hwpConfig.RFPGM;

      bool rfPgmDualChannel = ((rfpgm & 0b00000011) == 0b00000011) ? true : false;
      Pointer("/data/rsp/rfPgmDualChannel").Set(response, rfPgmDualChannel);

      bool rfPgmLpMode = ((rfpgm & 0b00000100) == 0b00000100) ? true : false;
      Pointer("/data/rsp/rfPgmLpMode").Set(response, rfPgmLpMode);

      bool rfPgmIncorrectUpload = ((rfpgm & 0b00001000) == 0b00001000) ? true : false;
      Pointer("/data/rsp/rfPgmIncorrectUpload").Set(response, rfPgmIncorrectUpload);

      bool enableAfterReset = ((rfpgm & 0b00010000) == 0b00010000) ? true : false;
      Pointer("/data/rsp/rfPgmEnableAfterReset").Set(response, enableAfterReset);

      bool rfPgmTerminateAfter1Min = ((rfpgm & 0b01000000) == 0b01000000) ? true : false;
      Pointer("/data/rsp/rfPgmTerminateAfter1Min").Set(response, rfPgmTerminateAfter1Min);

      bool rfPgmTerminateMcuPin = ((rfpgm & 0b10000000) == 0b10000000) ? true : false;
      Pointer("/data/rsp/rfPgmTerminateMcuPin").Set(response, rfPgmTerminateMcuPin);


      // RF band - undocumented byte
      std::string rfBand;
      try {
        rfBand = parseRfBand(hwpConfig.Undocumented[0] & 0x03);
      }
      catch (std::exception& ex) {
        rfBand = "";
      }
      Pointer("/data/rsp/rfBand").Set(response, rfBand);


      // set raw fields, if verbose mode is active
      if (comReadTrConf.getVerbose()) {
        setVerboseData(response, readTrConfigResult);
      }

      // status - ok
      ReadTrConfigError noError(ReadTrConfigError::Type::NoError);
      setResponseStatus(response, noError);

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

    uint16_t parseAndCheckDeviceAddr(const int deviceAddr) {
      if ((deviceAddr < 0) || (deviceAddr > 0xEF)) {
        THROW_EXC(
          std::out_of_range, "Device address outside of valid range. " << NAME_PAR_HEX("Address", deviceAddr)
        );
      }
      return deviceAddr;
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
      if (msgType.m_type != m_mTypeName_iqmeshNetworkReadTrConf) {
        THROW_EXC(std::logic_error, "Unsupported message type: " << PAR(msgType.m_type));
      }

      // creating representation object
      ComIqmeshNetworkReadTrConf comReadTrConf(doc);

      // service input parameters
      uint16_t deviceAddr;
      uint16_t hwpId;

      // parsing and checking service parameters
      try {
        m_repeat = parseAndCheckRepeat(comReadTrConf.getRepeat());
       
        if (!comReadTrConf.isSetDeviceAddr()) {
          THROW_EXC(std::logic_error, "deviceAddr not set");
        }
        deviceAddr = parseAndCheckDeviceAddr(comReadTrConf.getDeviceAddr());

        if (comReadTrConf.isSetHwpId()) {
          hwpId = comReadTrConf.getHwpId();
        }
        else {
          hwpId = HWPID_DoNotCheck;
        }
        
        m_returnVerbose = comReadTrConf.getVerbose();
      }
      // parsing and checking service parameters failed 
      catch (std::exception& ex) {
        Document failResponse = createCheckParamsFailedResponse(comReadTrConf.getMsgId(), msgType, ex.what());
        m_iMessagingSplitterService->sendMessage(messagingId, std::move(failResponse));

        TRC_FUNCTION_LEAVE("");
        return;
      }
      
      std::list<uint16_t> deviceAddrs =
      {
        deviceAddr
      };

      // call service with checked params
      ReadTrConfigResult readTrConfigResult = readTrConfig(deviceAddrs, hwpId);

      // create and send response
      Document responseDoc = createResponse(comReadTrConf.getMsgId(), msgType, readTrConfigResult, comReadTrConf);
      m_iMessagingSplitterService->sendMessage(messagingId, std::move(responseDoc));

      TRC_FUNCTION_LEAVE("");
    }
    

  public:
    void activate(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
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
        "ReadTrConfService instance deactivate" << std::endl <<
        "**************************************"
      );

      // for the sake of unregister function parameters 
      std::vector<std::string> supportedMsgTypes =
      {
        m_mTypeName_iqmeshNetworkReadTrConf
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



  ReadTrConfService::ReadTrConfService()
  {
    m_imp = shape_new Imp(*this);
  }

  ReadTrConfService::~ReadTrConfService()
  {
    delete m_imp;
  }


  void ReadTrConfService::attachInterface(iqrf::IIqrfDpaService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void ReadTrConfService::detachInterface(iqrf::IIqrfDpaService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void ReadTrConfService::attachInterface(iqrf::IMessagingSplitterService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void ReadTrConfService::detachInterface(iqrf::IMessagingSplitterService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void ReadTrConfService::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void ReadTrConfService::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }


  void ReadTrConfService::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void ReadTrConfService::deactivate()
  {
    m_imp->deactivate();
  }

  void ReadTrConfService::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
  }

}