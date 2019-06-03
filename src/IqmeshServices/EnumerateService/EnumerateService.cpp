#define IEnumerateService_EXPORTS

#include "EnumerateService.h"
#include "Trace.h"
#include "JsonUtils.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/pointer.h"

#include "JsDriverEmbedOS.h"
#include "JsDriverEmbedEEEPROM.h"
#include "JsDriverEmbedDpaExploration.h"
#include "JsDriverEmbedCoordinator.h"
#include "JsDriverSensorEnumerate.h"

#include "iqrf__EnumerateService.hxx"

#include <list>
#include <memory>
#include <math.h>
#include <bitset>

TRC_INIT_MODULE(iqrf::EnumerateService);


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

  // service general fail code - may and probably will be changed later in the future
  static const int SERVICE_ERROR = 1000;

  static const int SERVICE_ERROR_NOERROR = 0;

  static const int SERVICE_ERROR_INTERNAL = SERVICE_ERROR + 1;
  static const int SERVICE_ERROR_NODE_NOT_BONDED = SERVICE_ERROR + 2;
  static const int SERVICE_ERROR_INFO_MISSING = SERVICE_ERROR + 3;
};


namespace iqrf {

  // Holds information about errors, which encounter during device enumerate service run
  class DeviceEnumerateError {
  public:
    // Type of error
    enum class Type {
      NoError,
      NotBonded,
      InfoMissing,
      OsRead,
      PerEnum,
      ReadHwp,
      MorePersInfo
    };

    DeviceEnumerateError() : m_type(Type::NoError), m_message("") {};
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
    DeviceEnumerateError m_discoveredError;
    DeviceEnumerateError m_vrnError;
    DeviceEnumerateError m_zoneError;
    DeviceEnumerateError m_parentError;
    DeviceEnumerateError m_osReadError;
    DeviceEnumerateError m_perEnumError;
    DeviceEnumerateError m_readHwpConfigError;
    DeviceEnumerateError m_morePersInfoError;

    uint8_t m_deviceAddr;
    bool m_discovered;
    uint8_t m_vrn;
    uint8_t m_zone;
    uint8_t m_parent;
    uint16_t m_enumeratedNodeHwpIdVer;
    //std::vector<uns8> m_osRead;
    //uint16_t m_osBuild;
    //OsReadData m_osReadData;
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


    DeviceEnumerateError getDiscoveredError() const { return m_discoveredError; };
    void setDiscoveredError(const DeviceEnumerateError& error) {
      m_discoveredError = error;
    }

    DeviceEnumerateError getVrnError() const { return m_vrnError; };
    void setVrnError(const DeviceEnumerateError& error) {
      m_vrnError = error;
    }

    DeviceEnumerateError getZoneError() const { return m_zoneError; };
    void setZoneError(const DeviceEnumerateError& error) {
      m_zoneError = error;
    }

    DeviceEnumerateError getParentError() const { return m_parentError; };
    void setParentError(const DeviceEnumerateError& error) {
      m_parentError = error;
    }

    DeviceEnumerateError getOsReadError() const { return m_osReadError; };
    void setOsReadError(const DeviceEnumerateError& error) {
      m_osReadError = error;
    }

    DeviceEnumerateError getPerEnumError() const { return m_perEnumError; };
    void setPerEnumError(const DeviceEnumerateError& error) {
      m_perEnumError = error;
    }

    DeviceEnumerateError getReadHwpConfigError() const { return m_readHwpConfigError; };
    void setReadHwpConfigError(const DeviceEnumerateError& error) {
      m_readHwpConfigError = error;
    }

    DeviceEnumerateError getMorePersInfoError() const { return m_morePersInfoError; };
    void setMorePersInfoError(const DeviceEnumerateError& error) {
      m_morePersInfoError = error;
    }


    uint8_t getDeviceAddr() const {
      return m_deviceAddr;
    }

    void setDeviceAddr(uint8_t deviceAddr) {
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
  class EnumerateService::Imp {
  private:
    // parent object
    EnumerateService& m_parent;

    iqrf::IJsDriverService* m_iJsDriverService = nullptr;
    IIqrfDpaService* m_iIqrfDpaService = nullptr;
    std::unique_ptr<IIqrfDpaService::ExclusiveAccess> m_exclusiveAccess;

    // number of repeats
    int m_repeat = 3;

    // if is set Verbose mode
    bool m_returnVerbose = false;

    bool m_morePeripheralsInfo = false;


  public:
    Imp(EnumerateService& parent) : m_parent(parent)
    {
    }

    ~Imp()
    {
    }

    //TODO use for detect network topology
#if 0
    uint8_t readDiscoveryByte(uint16_t address)
    {
      TRC_FUNCTION_ENTER(PAR(address));

      iqrf::embed::eeeprom::Read iqrfEmbedEeepromRead(0, 0x20, 1);

      try {
        auto transResult = dpaRepeat(m_exclusiveAccess, m_iJsDriverService->createDpaRequest(iqrfEmbedEeepromRead), m_repeat);
        m_iJsDriverService->processDpaTransactionResult(iqrfEmbedEeepromRead, std::move(transResult));
      }
      catch (std::exception & e)
      {
        TRC_WARNING("Cannot read EeeProm")
      }

      uint8_t retval = iqrfEmbedEeepromRead.getPdata()[0];
      TRC_FUNCTION_LEAVE(PAR((int)retval));
      return retval;
    }

    // read discovery data
    void discoveryData(DeviceEnumerateResult& deviceEnumerateResult)
    {
      // get discovered indicator
      try {
        uint16_t address = 0x20 + deviceEnumerateResult.getDeviceAddr() / 8;
        uint8_t discoveredDevicesByte = readDiscoveryByte(address);
        uint8_t bitIndex = deviceEnumerateResult.getDeviceAddr() % 8;
        uint8_t compareByte = uint8_t(pow(2, bitIndex));

        deviceEnumerateResult.setDiscovered(
          (discoveredDevicesByte & compareByte) == compareByte
        );
      }
      catch (std::exception& ex) {
        DeviceEnumerateError error(DeviceEnumerateError::Type::InfoMissing, ex.what());
        deviceEnumerateResult.setDiscoveredError(error);
      }

      // VRN
      try {
        uint16_t address = 0x5000 + deviceEnumerateResult.getDeviceAddr();

        uint8_t vrnByte = readDiscoveryByte(address);
        deviceEnumerateResult.setVrn(vrnByte);
      }
      catch (std::exception& ex) {
        DeviceEnumerateError error(DeviceEnumerateError::Type::InfoMissing, ex.what());
        deviceEnumerateResult.setVrnError(error);
      }

      // zone
      try {
        uint16_t address = 0x5200 + deviceEnumerateResult.getDeviceAddr();

        uint8_t zoneByte = readDiscoveryByte(address);
        deviceEnumerateResult.setZone(zoneByte);
      }
      catch (std::exception& ex) {
        DeviceEnumerateError error(DeviceEnumerateError::Type::InfoMissing, ex.what());
        deviceEnumerateResult.setZoneError(error);
      }

      // parent
      try {
        uint16_t address = 0x5300 + deviceEnumerateResult.getDeviceAddr();

        uint8_t parentByte = readDiscoveryByte(address);
        deviceEnumerateResult.setParent(parentByte);
      }
      catch (std::exception& ex) {
        DeviceEnumerateError error(DeviceEnumerateError::Type::InfoMissing, ex.what());
        deviceEnumerateResult.setParentError(error);
      }
    }
#endif

    IEnumerateService::CoordinatorData getCoordinatorData() const
    {
      TRC_FUNCTION_ENTER("");

      IEnumerateService::CoordinatorData coordinatorData;
      coordinatorData.m_valid = false;

      auto exclusiveAccess = m_iIqrfDpaService->getExclusiveAccess();

      {
        iqrf::embed::coordinator::BondedDevices iqrfEmbedCoordinatorBondedDevices;
        std::unique_ptr<IDpaTransactionResult2> transResult;
        exclusiveAccess->executeDpaTransactionRepeat(m_iJsDriverService->createDpaRequest(iqrfEmbedCoordinatorBondedDevices), transResult, m_repeat);
        m_iJsDriverService->processDpaTransactionResult(iqrfEmbedCoordinatorBondedDevices, std::move(transResult));

        coordinatorData.m_bonded =  iqrfEmbedCoordinatorBondedDevices.getBondedDevices();
      }

      {
        iqrf::embed::coordinator::DiscoveredDevices iqrfEmbedCoordinatorDiscoveredDevices;
        std::unique_ptr<IDpaTransactionResult2> transResult;
        exclusiveAccess->executeDpaTransactionRepeat(m_iJsDriverService->createDpaRequest(iqrfEmbedCoordinatorDiscoveredDevices), transResult, m_repeat);
        m_iJsDriverService->processDpaTransactionResult(iqrfEmbedCoordinatorDiscoveredDevices, std::move(transResult));

        coordinatorData.m_discovered = iqrfEmbedCoordinatorDiscoveredDevices.getDiscoveredDevices();
      }

      //TODO other params

      coordinatorData.m_valid = true;
       
      TRC_FUNCTION_LEAVE("");
      return coordinatorData;
    }

    IEnumerateService::NodeData getNodeData(uint16_t nadr) const
    {
      TRC_FUNCTION_ENTER(nadr);

      IEnumerateService::NodeData nodeData;

      auto exclusiveAccess = m_iIqrfDpaService->getExclusiveAccess();

      {
        iqrf::embed::os::Read iqrfEmbedOsRead(nadr);
        std::unique_ptr<IDpaTransactionResult2> transResult;
        exclusiveAccess->executeDpaTransactionRepeat(m_iJsDriverService->createDpaRequest(iqrfEmbedOsRead), transResult, m_repeat);
        m_iJsDriverService->processDpaTransactionResult(iqrfEmbedOsRead, std::move(transResult));

        nodeData.setNadr((int)nadr);
        nodeData.setHwpid(iqrfEmbedOsRead.getHwpid());
        nodeData.setOsBuild(iqrfEmbedOsRead.getOsBuild());
        nodeData.setOsVer(iqrfEmbedOsRead.getOsVersion());
        nodeData.setMid(iqrfEmbedOsRead.getMid());
      }
      
      {
        iqrf::embed::explore::Enumerate iqrfEmbedExploreEnumerate(nadr);
        std::unique_ptr<IDpaTransactionResult2> transResult;
        exclusiveAccess->executeDpaTransactionRepeat(m_iJsDriverService->createDpaRequest(iqrfEmbedExploreEnumerate), transResult, m_repeat);
        m_iJsDriverService->processDpaTransactionResult(iqrfEmbedExploreEnumerate, std::move(transResult));
      
        nodeData.setHwpidVer(iqrfEmbedExploreEnumerate.getHwpidVer());
        nodeData.setDpaVer(iqrfEmbedExploreEnumerate.getDpaVer());
        nodeData.setModeStd(iqrfEmbedExploreEnumerate.isModeStd());
        nodeData.setStdAndLpNet(iqrfEmbedExploreEnumerate.isStdAndLpSupport());
        nodeData.setEmbedPer(iqrfEmbedExploreEnumerate.getEmbedPer());
        nodeData.setUserPer(iqrfEmbedExploreEnumerate.getUserPer());
      }

      {
        iqrf::sensor::Enumerate iqrfSensorEnumerate(nadr);
        std::unique_ptr<IDpaTransactionResult2> transResult;
        exclusiveAccess->executeDpaTransactionRepeat(m_iJsDriverService->createDpaRequest(iqrfSensorEnumerate), transResult, m_repeat);
        m_iJsDriverService->processDpaTransactionResult(iqrfSensorEnumerate, std::move(transResult));
      }

      //TODO other params

      nodeData.setValid(true);

      TRC_FUNCTION_LEAVE("");
      return nodeData;
    }

  public:
    void activate(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************************" << std::endl <<
        "EnumerateService instance activate" << std::endl <<
        "******************************************"
      );

      TRC_FUNCTION_LEAVE("");
    }

    void deactivate()
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "**************************************" << std::endl <<
        "EnumerateService instance deactivate" << std::endl <<
        "**************************************"
      );

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

    void attachInterface(IJsDriverService* iface)
    {
      m_iJsDriverService = iface;
    }

    void detachInterface(IJsDriverService* iface)
    {
      if (m_iJsDriverService == iface) {
        m_iJsDriverService = nullptr;
      }
    }

  };

  EnumerateService::EnumerateService()
  {
    m_imp = shape_new Imp(*this);
  }

  EnumerateService::~EnumerateService()
  {
    delete m_imp;
  }

  IEnumerateService::CoordinatorData EnumerateService::getCoordinatorData() const
  {
    return m_imp->getCoordinatorData();
  }

  IEnumerateService::NodeData EnumerateService::getNodeData(uint16_t nadr) const
  {
    return m_imp->getNodeData(nadr);
  }

  void EnumerateService::attachInterface(iqrf::IIqrfDpaService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void EnumerateService::detachInterface(iqrf::IIqrfDpaService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void EnumerateService::attachInterface(iqrf::IJsDriverService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void EnumerateService::detachInterface(iqrf::IJsDriverService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void EnumerateService::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void EnumerateService::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }


  void EnumerateService::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void EnumerateService::deactivate()
  {
    m_imp->deactivate();
  }

  void EnumerateService::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
  }

}