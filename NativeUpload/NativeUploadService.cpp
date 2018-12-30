#define INativeUploadService_EXPORTS

#include "NativeUploadService.h"
#include "HexFmtParser.h"
#include "IqrfFmtParser.h"
#include "TrconfFmtParser.h"
#include "Trace.h"
#include "ComIqmeshNetworkNativeUpload.h"
#include "ObjectFactory.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"

#include "iqrf__NativeUploadService.hxx"

#include <list>
#include <memory>
#include <math.h>
#include <bitset>

TRC_INIT_MODULE(iqrf::NativeUploadService);


using namespace rapidjson;

namespace {

  // maximum number of repeats
  static const uint8_t REPEAT_MAX = 3;

  // Programming communication direction
  static const uint8_t UPLOAD = 0x80;
  static const uint8_t DOWNLOAD = 0x00;

  // Programming targets
  static const uint8_t CFG_TARGET = 0x00;
  static const uint8_t RFPMG_TARGET = 0x01;
  static const uint8_t RFBAND_TARGET = 0x02;
  static const uint8_t ACCESS_PWD_TARGET = 0x03;
  static const uint8_t USER_KEY_TARGET = 0x04;
  static const uint8_t FLASH_TARGET = 0x05;
  static const uint8_t INTERNAL_EEPROM_TARGET = 0x06;
  static const uint8_t EXTERNAL_EEPROM_TARGET = 0x07;
  static const uint8_t SPECIAL_TARGET = 0x08;

  // Length, range and other constants
  static const size_t CFG_LEN = 32;
  static const uint8_t CFG_CHKSUM_INIT = 0x5f;
  static const size_t ACCESS_PWD_LEN = 16;
  static const size_t USER_KEY_LEN = 16;
  static const size_t FLASH_UP_MODULO = 16;
  static const size_t FLASH_DOWN_MODULO = 32;
  static const uint16_t FLASH_APP_LOW = 0x3a00;
  static const uint16_t FLASH_APP_HIGH = 0x3fff; // TODO: Fix upper limit
  static const uint16_t FLASH_EXT_LOW = 0x2c00;
  static const uint16_t FLASH_EXT_HIGH = 0x37bf; // TODO: Fix upper limits
  static const size_t FLASH_LEN = 32;
  static const uint16_t INT_EEPROM_UP_LOW = 0x0000;
  static const uint16_t INT_EEPROM_UP_HIGH = 0x00bf;
  static const size_t INT_EEPROM_UP_ADDR_LEN_MAX = 0x00c0;
  static const size_t INT_EEPROM_UP_LEN_MIN = 1;
  static const size_t INT_EEPROM_UP_LEN_MAX = 32;
  static const uint16_t INT_EEPROM_DOWN_LOW = 0x0000;
  static const uint16_t INT_EEPROM_DOWN_HIGH = 0x00a0;
  static const size_t INT_EEPROM_DOWN_LEN = 32;
  static const uint16_t EXT_EEPROM_LOW = 0x0000;
  static const uint16_t EXT_EEPROM_UP_HIGH = 0x3fe0;
  static const uint16_t EXT_EEPROM_DOWN_HIGH = 0x7fe0;
  static const size_t EXT_EEPROM_MODULO = 32;
  static const size_t EXT_EEPROM_LEN = 32;
  static const size_t SPECIAL_LEN = 18;


  // service general fail code - may and probably will be changed later in the future
  static const int SERVICE_ERROR = 1000;

  static const int SERVICE_ERROR_DATA_PREPARE = SERVICE_ERROR + 1;
  static const int SERVICE_ERROR_ENTER_PROG_STATE = SERVICE_ERROR + 2;
  static const int SERVICE_ERROR_TERMINATE_PROG_STATE = SERVICE_ERROR + 3;
};


namespace iqrf {

  // Holds information about errors, which encounter during service run
  class NativeUploadError {
  public:
    // Type of error
    enum class Type {
      NoError,
      DataPrepare,
      EnterProgState,
      TerminateProgState
    };

    NativeUploadError() : m_type(Type::NoError), m_message("") {};
    NativeUploadError(Type errorType) : m_type(errorType), m_message("") {};
    NativeUploadError(Type errorType, const std::string& message) : m_type(errorType), m_message(message) {};

    Type getType() const { return m_type; };
    std::string getMessage() const { return m_message; };

    NativeUploadError& operator=(const NativeUploadError& error) {
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


  // holds information about result of native upload
  class NativeUploadResult {
  private:
    // result of upload
    IIqrfChannelService::Accessor::UploadErrorCode m_errorCode = IIqrfChannelService::Accessor::UploadErrorCode::UPLOAD_NO_ERROR;

    NativeUploadError m_error;

    // transaction results
    std::list<std::unique_ptr<IDpaTransactionResult2>> m_transResults;


  public:
    IIqrfChannelService::Accessor::UploadErrorCode getErrorCode() {
      return m_errorCode;
    }

    void setErrorCode(IIqrfChannelService::Accessor::UploadErrorCode errCode) {
      m_errorCode = errCode;
    }

    NativeUploadError getError() const { return m_error; };

    void setError(const NativeUploadError& error) {
      m_error = error;
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
  class NativeUploadService::Imp {
  private:
    // parent object
    NativeUploadService& m_parent;

    // message type: Native Upload Service
    // for temporal reasons
    const std::string m_mTypeName_mngDaemonUpload = "mngDaemon_Upload";

    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    IIqrfDpaService* m_iIqrfDpaService = nullptr;
    IIqrfChannelService* m_iIqrfChannelService = nullptr;

    // number of repeats
    uint8_t m_repeat;

    // if is set Verbose mode
    bool m_returnVerbose = false;

    // absolute path with hex file to upload
    std::string m_uploadPath;

    // upload receive function
    IIqrfChannelService::ReceiveFromFunc recvFunction;

    int uploadRecvFunction(const std::basic_string<unsigned char>& msg) {
      return 0;
    }
    
  public:
    Imp(NativeUploadService& parent) : m_parent(parent)
    {
      /*
      m_msgType_mngIqmeshWriteConfig
        = new IMessagingSplitterService::MsgType(m_mTypeName_mngIqmeshWriteConfig, 1, 0, 0);
        */
      //recvFunction = uploadRecvFunction;
      recvFunction = [](const std::basic_string<unsigned char>& msg) { return 0; };
    }

    ~Imp()
    {
    }


  private:
    
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
    void setVerboseData(rapidjson::Document& response, NativeUploadResult& nativeUploadResult)
    {
      rapidjson::Value rawArray(kArrayType);
      Document::AllocatorType& allocator = response.GetAllocator();

      while (nativeUploadResult.isNextTransactionResult()) {
        std::unique_ptr<IDpaTransactionResult2> transResult = nativeUploadResult.consumeNextTransactionResult();
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

    // sets error status for specified response
    void setResponseStatus(
      rapidjson::Document& response, int32_t errorCode, const std::string& msg
    ) 
    {
      Pointer("/status").Set(response, errorCode);
      Pointer("/statusStr").Set(response, msg);
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

    std::string parseAndCheckFileName(const std::string& fileName) {
      if (fileName.empty()) {
        THROW_EXC(std::out_of_range, "File name empty");
      }
      return fileName;
    }

    // type of upload
    enum class TargetType {
      Hex,
      Iqrf,
      Config
    };

    TargetType parseAndCheckTarget(const std::string& targetStr) {
      if (targetStr == "hex") {
        return TargetType::Hex;
      }

      if (targetStr == "iqrf") {
        return TargetType::Iqrf;
      }

      if (targetStr == "trcnfg") {
        return TargetType::Config;
      }

      throw std::logic_error(
        "Unsupported target."
      );
    }

    // returns type of source code file
    // based solely on the file suffix - no internal file analysis implemented
    TargetType getSourceCodeFileType(const std::string& fileName)
    {
      size_t suffixPos = fileName.find_last_of('.');
      if (suffixPos == std::string::npos) {
        throw std::logic_error(
          "Bad format of source code file name - no suffix found."
        );
      }

      std::string suffix = fileName.substr(suffixPos + 1);

      if (suffix == "hex") {
        return TargetType::Hex;
      }

      if (suffix == "iqrf") {
        return TargetType::Iqrf;
      }

      if (suffix == "trcnfg") {
        return TargetType::Config;
      }

      throw std::logic_error(
        "Unknown source code file suffix."
      );
    }


    void insertAddressAndData(
      std::basic_string<uint8_t> &msg, 
      const uint16_t addr,
      const std::basic_string<uint8_t>& data
    ) 
    {
      msg += addr & 0xff;
      msg += (addr >> 8) & 0xff;
      msg += data;
    }

    IIqrfChannelService::Accessor::UploadErrorCode 
      uploadFlash(const uint16_t addr, const std::basic_string<uint8_t>& data)
    {
      std::basic_string<uint8_t> msg;

      if (addr % FLASH_UP_MODULO != 0) {
        THROW_EXC(std::out_of_range, "Address in flash memory should be modulo 16!");
      }

      if (!(((addr >= FLASH_APP_LOW) && (addr <= FLASH_APP_HIGH)) || ((addr >= FLASH_EXT_LOW) && (addr <= FLASH_EXT_HIGH)))) {
        THROW_EXC(std::out_of_range, "Address in flash memory is outside application or extended flash memory!");
      }

      if (data.length() != FLASH_LEN) {
        THROW_EXC(std::out_of_range, "Data to be programmed into the flash memory must be 32B long!");
      }

      insertAddressAndData(msg, addr, data);

      return m_iIqrfChannelService
        ->getAccess(recvFunction, IIqrfChannelService::AccesType::Normal)
        ->upload(IIqrfChannelService::Accessor::UploadTarget::UPLOAD_TARGET_FLASH, data, addr);
    }

    IIqrfChannelService::Accessor::UploadErrorCode
      uploadInternalEeprom(uint16_t addr, const std::basic_string<uint8_t>& data)
    {
      std::basic_string<uint8_t> msg;

      if (!((addr >= INT_EEPROM_UP_LOW) && (addr <= INT_EEPROM_UP_HIGH))) {
        THROW_EXC(std::out_of_range, "Address in internal eeprom memory is outside of addressable range!");
      }

      if (addr + data.length() >= INT_EEPROM_UP_ADDR_LEN_MAX) {
        THROW_EXC(std::out_of_range, "End of write is out of the addressable range of the internal eeprom!");
      }

      if ((data.length() < INT_EEPROM_UP_LEN_MIN) || (data.length() > INT_EEPROM_UP_LEN_MAX)) {
        THROW_EXC(std::out_of_range, "Data to be programmed into the internal eeprom memory must be 1-32B long!");
      }

      insertAddressAndData(msg, addr, data);

      return m_iIqrfChannelService
        ->getAccess(recvFunction, IIqrfChannelService::AccesType::Normal)
        ->upload(IIqrfChannelService::Accessor::UploadTarget::UPLOAD_TARGET_INTERNAL_EEPROM, data, addr);
    }

    IIqrfChannelService::Accessor::UploadErrorCode
      uploadExternalEeprom(const uint16_t addr, const std::basic_string<uint8_t>& data)
    {
      std::basic_string<uint8_t> msg;

      if (!((addr >= EXT_EEPROM_LOW) && (addr <= EXT_EEPROM_UP_HIGH))) {
        THROW_EXC(std::out_of_range, "Address in external eeprom memory is outside of addressable range!");
      }

      if (addr % EXT_EEPROM_MODULO != 0) {
        THROW_EXC(std::out_of_range, "Address in external eeprom memory should be modulo 32!");
      }

      if (data.length() != EXT_EEPROM_LEN) {
        THROW_EXC(std::out_of_range, "Data to be programmed into the external eeprom memory must be 32B long!");
      }

      insertAddressAndData(msg, addr, data);

      return m_iIqrfChannelService
        ->getAccess(recvFunction, IIqrfChannelService::AccesType::Normal)
        ->upload(IIqrfChannelService::Accessor::UploadTarget::UPLOAD_TARGET_EXTERNAL_EEPROM, data, addr);
    }
    

    void uploadFromHex(NativeUploadResult& uploadResult, const std::string& fileName) 
    {
      HexFmtParser parser(fileName);
      parser.parse();

      // enter into programming state
      if (!m_iIqrfChannelService->getAccess(recvFunction, IIqrfChannelService::AccesType::Normal)->enterProgrammingState()) {
        NativeUploadError error(NativeUploadError::Type::EnterProgState, "Could not enter into programming state.");
        uploadResult.setError(error);
        return;
      }

      IIqrfChannelService::Accessor::UploadErrorCode errCode = IIqrfChannelService::Accessor::UploadErrorCode::UPLOAD_NO_ERROR;

      // parse hex file
      HexFmtParser::iterator itr;

      for (itr = parser.begin(); itr != parser.end(); itr++) {
        switch (itr->memory) {
        case TrMemory::FLASH:
          errCode = uploadFlash((*itr).addr, (*itr).data);
          break;
        case TrMemory::INTERNAL_EEPROM:
          errCode = uploadInternalEeprom((*itr).addr, (*itr).data);
          break;
        case TrMemory::EXTERNAL_EEPROM:
          errCode = uploadExternalEeprom((*itr).addr, (*itr).data);
          break;
        }
      }

      uploadResult.setErrorCode(errCode);

      // terminate programming state
      if (!m_iIqrfChannelService->getAccess(recvFunction, IIqrfChannelService::AccesType::Normal)->terminateProgrammingState()) {
        NativeUploadError error(NativeUploadError::Type::TerminateProgState, "Could not terminate programming state.");
        uploadResult.setError(error);
        return;
      }
    }

    IIqrfChannelService::Accessor::UploadErrorCode
      uploadSpecial(const std::basic_string<uint8_t>& data)
    {
      if (data.length() != SPECIAL_LEN) {
        THROW_EXC(std::out_of_range, "Data to be programmed by the special upload must be 18B long!");
      }

      // will not be used in special type of uploading
      uint16_t addrNotUsed = 0;

      return m_iIqrfChannelService
        ->getAccess(recvFunction, IIqrfChannelService::AccesType::Normal)
        ->upload(IIqrfChannelService::Accessor::UploadTarget::UPLOAD_TARGET_SPECIAL, data, addrNotUsed);
    }

    void uploadFromIqrf(NativeUploadResult& uploadResult, const std::string& fileName)
    {
      IqrfFmtParser::iterator itr;

      // enter into programming state
      if (!m_iIqrfChannelService->getAccess(recvFunction, IIqrfChannelService::AccesType::Normal)->enterProgrammingState()) {
        NativeUploadError error(NativeUploadError::Type::EnterProgState, "Could not enter into programming state.");
        uploadResult.setError(error);
        return;
      }

      // will be solved later
      /*
      TrModuleInfo info = getTrModuleInfo(static_cast<ModuleInfo*>(ifc->getTRModuleInfo()));
      if (!parser.check(info)) {
        THROW_EXC(
          std::out_of_range, "IQRF file " << PAR(fileName) << " can not be upload to TR! TR is not in supported types specified in the IQRF file. This message is caused by incopatible type of TR, OS version or OS build."
        );
      }
      */
      IqrfFmtParser parser(fileName);
      parser.parse();  

      IIqrfChannelService::Accessor::UploadErrorCode errCode = IIqrfChannelService::Accessor::UploadErrorCode::UPLOAD_NO_ERROR;

      for (itr = parser.begin(); itr != parser.end(); itr++) {
        errCode = uploadSpecial(*itr);

        // if some error occured, break uploading
        if (errCode != IIqrfChannelService::Accessor::UploadErrorCode::UPLOAD_NO_ERROR) {
          uploadResult.setErrorCode(errCode);
          break;
        }
      }
      
      // terminate programming state
      if (!m_iIqrfChannelService->getAccess(recvFunction, IIqrfChannelService::AccesType::Normal)->terminateProgrammingState()) {
        NativeUploadError error(NativeUploadError::Type::TerminateProgState, "Could not terminate programming state.");
        uploadResult.setError(error);
      }
    }

    uint8_t computeConfigChecksum(const std::basic_string<uint8_t>& data) {
      uint8_t chksum = CFG_CHKSUM_INIT;

      for (int i = 1; i < data.length(); i++) {
        chksum ^= data[i];
      }

      return chksum;
    }

    IIqrfChannelService::Accessor::UploadErrorCode 
      uploadCfg(const std::basic_string<uint8_t>& data)
    {
      if (data.length() != CFG_LEN) {
        THROW_EXC(std::out_of_range, "Invalid length of the TR HWP configuration data!");
      }

      if (computeConfigChecksum(data) != data[0]) {
        THROW_EXC(std::out_of_range, "Invalid TR HWP configuration checksum!");
      }

      // will not be used in special type of uploading
      uint16_t addrNotUsed = 0;

      return m_iIqrfChannelService
        ->getAccess(recvFunction, IIqrfChannelService::AccesType::Normal)
        ->upload(IIqrfChannelService::Accessor::UploadTarget::UPLOAD_TARGET_CFG, data, addrNotUsed);
    }

    IIqrfChannelService::Accessor::UploadErrorCode
      uploadRFPMG(uint8_t rfpmg)
    {
      std::basic_string<uint8_t> data;

      data += rfpmg;

      // will not be used in special type of uploading
      uint16_t addrNotUsed = 0;

      return m_iIqrfChannelService
        ->getAccess(recvFunction, IIqrfChannelService::AccesType::Normal)
        ->upload(IIqrfChannelService::Accessor::UploadTarget::UPLOAD_TARGET_RFPMG, data, addrNotUsed);
    }

    void uploadFromConfig(NativeUploadResult& uploadResult, const std::string& fileName)
    {
      uint8_t rfpmg;
      TrconfFmtParser parser(fileName);
      parser.parse();

      rfpmg = parser.getRFPMG();

      // will be solved later
      //parser.checkChannels(downloadRFBAND());

      // enter into programming state
      if (!m_iIqrfChannelService->getAccess(recvFunction, IIqrfChannelService::AccesType::Normal)->enterProgrammingState()) {
        NativeUploadError error(NativeUploadError::Type::EnterProgState, "Could not enter into programming state.");
        uploadResult.setError(error);
        return;
      }

      IIqrfChannelService::Accessor::UploadErrorCode errCode = uploadCfg(parser.getData());
      if (errCode == IIqrfChannelService::Accessor::UploadErrorCode::UPLOAD_NO_ERROR) {
        errCode = uploadRFPMG(rfpmg);
      }
      
      uploadResult.setErrorCode(errCode);

      // terminate programming state
      if (!m_iIqrfChannelService->getAccess(recvFunction, IIqrfChannelService::AccesType::Normal)->terminateProgrammingState()) {
        NativeUploadError error(NativeUploadError::Type::TerminateProgState, "Could not terminate programming state.");
        uploadResult.setError(error);
      }
    }


    // do native upload
    NativeUploadResult doNativeUpload(
      const std::string& fileName, 
      TargetType target, 
      bool isSetTarget
    )
    {
      TRC_FUNCTION_ENTER("");

      // result
      NativeUploadResult uploadResult;

      // determine type of source code file
      TargetType fileType;
      if (isSetTarget) {
        fileType = target;
      }
      else {
        try {
          fileType = getSourceCodeFileType(fileName);
        }
        catch (std::exception& ex) {
          NativeUploadError error(NativeUploadError::Type::DataPrepare, ex.what());
          uploadResult.setError(error);
          return uploadResult;
        }
      }

      switch (fileType) {
        case TargetType::Hex:
          uploadFromHex(uploadResult, fileName);
          break;
        case TargetType::Iqrf:
          uploadFromIqrf(uploadResult, fileName);
          break;
        case TargetType::Config:
          uploadFromConfig(uploadResult, fileName);
          break;
        default:
          NativeUploadError error(NativeUploadError::Type::DataPrepare, "Unsupported type source code file.");
          uploadResult.setError(error);
          return uploadResult;
      }

      TRC_FUNCTION_LEAVE("");
      return uploadResult;
    }


    // creates response on the basis of read TR config result
    Document createResponse(
      const std::string& msgId,
      const IMessagingSplitterService::MsgType& msgType,
      NativeUploadResult& uploadResult,
      const ComIqmeshNetworkNativeUpload& comNativeUpload
    )
    {
      Document response;

      // set common parameters
      Pointer("/mType").Set(response, msgType.m_type);
      Pointer("/data/msgId").Set(response, msgId);

      /*
      "Result of upload. 
      0 - upload successfull, 
      1 - general error, 
      2 - incorrect target memory, 
      3 - incorrect data length, 
      4 - incorrect memory address, 
      5 - target memory is write only, 
      6 - communication failure, 
      7 - operation is not supported by TR module, 
      8 - SPI bus is busy or TR module is not in programming mode"
      */

      // checking of error
      NativeUploadError error = uploadResult.getError();
      
      Pointer("/data/statusStr").Set(response, error.getMessage());

      switch (error.getType()) {
        case NativeUploadError::Type::NoError:
          Pointer("/data/status").Set(response, 0);
          break;
        case NativeUploadError::Type::DataPrepare:
          Pointer("/data/status").Set(response, SERVICE_ERROR_DATA_PREPARE);
          break;
        case NativeUploadError::Type::EnterProgState:
          Pointer("/data/status").Set(response, SERVICE_ERROR_ENTER_PROG_STATE);
          break;
        case NativeUploadError::Type::TerminateProgState:
          Pointer("/data/status").Set(response, SERVICE_ERROR_TERMINATE_PROG_STATE);
          break;
        default:
          Pointer("/data/status").Set(response, SERVICE_ERROR);
      }

      // set raw fields, if verbose mode is active
      if (comNativeUpload.getVerbose()) {
        setVerboseData(response, uploadResult);
      }
      
      // status - ok
      Pointer("/data/status").Set(response, 0);
      Pointer("/data/statusStr").Set(response, "ok");

      return response;
    }

    // creates and returns full file name
    std::string getFullFileName(const std::string& uploadPath, const std::string& fileName)
    {
      char fileSeparator;

      #if defined(WIN32) || defined(_WIN32)
        fileSeparator = '\\';
      #else
        fileSeparator = '/';
      #endif

      std::string fullFileName = uploadPath;

      if (uploadPath[uploadPath.size() - 1] != fileSeparator) {
        fullFileName += fileSeparator;
      }

      fullFileName += fileName;

      return fullFileName;
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
      if (msgType.m_type != m_mTypeName_mngDaemonUpload) {
        THROW_EXC(std::logic_error, "Unsupported message type: " << PAR(msgType.m_type));
      }

      // creating representation object
      ComIqmeshNetworkNativeUpload comNativeUpload(doc);

      // if upload path (service's configuration parameter) is empty, return error message
      if (m_uploadPath.empty()) {
        Document failResponse = createCheckParamsFailedResponse(comNativeUpload.getMsgId(), msgType, "Empty upload path");
        m_iMessagingSplitterService->sendMessage(messagingId, std::move(failResponse));

        TRC_FUNCTION_LEAVE("");
        return;
      }

      // service input parameters
      std::string fileName;

      // target of upload
      TargetType target;
      bool isSetTarget = false;

      // parsing and checking service parameters
      try {
        m_repeat = parseAndCheckRepeat(comNativeUpload.getRepeat());
       
        if (!comNativeUpload.isSetFileName()) {
          THROW_EXC(std::logic_error, "fileName not set");
        }
        fileName = parseAndCheckFileName(comNativeUpload.getFileName());

        if (comNativeUpload.isSetTarget()) {
          target = parseAndCheckTarget(comNativeUpload.getTarget());
          isSetTarget = true;
        }

        m_returnVerbose = comNativeUpload.getVerbose();
      }
      // parsing and checking service parameters failed 
      catch (std::exception& ex) {
        Document failResponse = createCheckParamsFailedResponse(comNativeUpload.getMsgId(), msgType, ex.what());
        m_iMessagingSplitterService->sendMessage(messagingId, std::move(failResponse));

        TRC_FUNCTION_LEAVE("");
        return;
      }
      
      // construct full file name
      std::string fullFileName = getFullFileName(m_uploadPath, fileName);

      // call service and get result
      NativeUploadResult nativeUploadResult = doNativeUpload(fullFileName, target, isSetTarget);

      // create and send response
      Document responseDoc = createResponse(comNativeUpload.getMsgId(), msgType, nativeUploadResult, comNativeUpload);
      m_iMessagingSplitterService->sendMessage(messagingId, std::move(responseDoc));

      TRC_FUNCTION_LEAVE("");
    }
    

  public:
    void activate(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************************" << std::endl <<
        "NativeUploadService instance activate" << std::endl <<
        "******************************************"
      );

      props->getMemberAsString("uploadPath", m_uploadPath);
      TRC_INFORMATION(PAR(m_uploadPath));

      if (m_uploadPath.empty()) {
        TRC_ERROR("Upload path is empty.");
      }

      // for the sake of register function parameters 
      std::vector<std::string> supportedMsgTypes =
      {
        m_mTypeName_mngDaemonUpload
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
        "NativeUploadService instance deactivate" << std::endl <<
        "**************************************"
      );

      // for the sake of unregister function parameters 
      std::vector<std::string> supportedMsgTypes =
      {
        m_mTypeName_mngDaemonUpload
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

    void attachInterface(IIqrfChannelService* iface)
    {
      m_iIqrfChannelService = iface;
    }

    void detachInterface(IIqrfChannelService* iface)
    {
      if (m_iIqrfChannelService == iface) {
        m_iIqrfChannelService = nullptr;
      }
    }

  };



  NativeUploadService::NativeUploadService()
  {
    m_imp = shape_new Imp(*this);
  }

  NativeUploadService::~NativeUploadService()
  {
    delete m_imp;
  }


  void NativeUploadService::attachInterface(iqrf::IIqrfDpaService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void NativeUploadService::detachInterface(iqrf::IIqrfDpaService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void NativeUploadService::attachInterface(iqrf::IMessagingSplitterService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void NativeUploadService::detachInterface(iqrf::IMessagingSplitterService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void NativeUploadService::attachInterface(iqrf::IIqrfChannelService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void NativeUploadService::detachInterface(iqrf::IIqrfChannelService* iface)
  {
    m_imp->detachInterface(iface);
  }


  void NativeUploadService::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void NativeUploadService::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }


  void NativeUploadService::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void NativeUploadService::deactivate()
  {
    m_imp->deactivate();
  }

  void NativeUploadService::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
  }

}