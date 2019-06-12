#define IEnumerateService_EXPORTS

#include "EnumerateService.h"
#include "Trace.h"
#include "JsonUtils.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/pointer.h"

#include "FastEnumeration.h"
#include "RawDpaEmbedCoordinator.h"
#include "RawDpaEmbedOS.h"
#include "RawDpaEmbedExplore.h"
#include "JsDriverEmbedOS.h"
#include "JsDriverEmbedEEEPROM.h"
#include "JsDriverEmbedExploration.h"
#include "JsDriverEmbedCoordinator.h"
#include "JsDriverSensorEnumerate.h"
#include "JsDriverBinaryOutputEnumerate.h"

#include "iqrf__EnumerateService.hxx"

#include <list>
#include <memory>
#include <math.h>
#include <bitset>

TRC_INIT_MODULE(iqrf::EnumerateService);


using namespace rapidjson;

namespace iqrf {
  class NodeDataImpl : public IEnumerateService::INodeData
  {
  public:
    NodeDataImpl() = delete;

    NodeDataImpl(int nadr, int hwpid, embed::explore::RawDpaEnumeratePtr & e, embed::os::RawDpaReadPtr & r)
      :m_nadr(nadr)
      , m_hwpid(hwpid)
      , m_exploreEnumerate(std::move(e))
      ,m_osRead(std::move(r))
    {}

    const embed::explore::EnumeratePtr & getEmbedExploreEnumerate() const override
    {
      return m_exploreEnumerate;
    }

    const embed::os::ReadPtr & getEmbedOsRead() const override
    {
      return m_osRead;
    }

    int getNadr() const override
    {
      return m_nadr;
    }

    int getHwpid() const override
    {
      return m_hwpid;
    }

    //unsigned getMid() const override { return m_osRead->getMid(); }
    ////int getNadr() const { return m_osRead->gm_nadr; }
    ////int getHwpid() const { return m_hwpid; }
    //int getHwpidVer() const { return m_exploreEnumerate->getHwpidVer(); }
    //int getOsBuild() const { return m_osRead->getOsBuild(); }
    //int getOsVer() const { return m_osRead->getOsVersion(); }
    //int getDpaVer() const { return m_exploreEnumerate->getDpaVer(); }
    //bool getModeStd() const { return m_modeStd; }
    //bool getStdAndLpNet() const { return m_stdAndLpNet; }
    //const std::set<int> & getEmbedPer() { return m_embedPer; }
    //const std::set<int> & getUserPer() { return m_userPer; }
  private:
    int m_nadr;
    int m_hwpid;
    embed::explore::EnumeratePtr m_exploreEnumerate;
    embed::os::ReadPtr m_osRead;
  };

  // implementation class
  class EnumerateService::Imp {
  private:
    // parent object
    EnumerateService& m_parent;

    IIqrfDpaService* m_iIqrfDpaService = nullptr;
    iqrf::IJsCacheService* m_iJsCacheService = nullptr;
    iqrf::IJsDriverService* m_iJsDriverService = nullptr;
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

    //TODO to standalone service? Analogy to IJsDriverService
    DpaMessage createDpaRequest(RawDpaCommandSolver & rawDpaCommandSolver) const
    {
      TRC_FUNCTION_ENTER("");
      DpaMessage retval = rawDpaCommandSolver.encodeRequest();
      TRC_FUNCTION_LEAVE("");
      return retval;
    }

    //TODO to standalone service? Analogy to IJsDriverService
    void processDpaTransactionResult(RawDpaCommandSolver & rawDpaCommandSolver, std::unique_ptr<IDpaTransactionResult2> res) const
    {
      TRC_FUNCTION_ENTER("");

      rawDpaCommandSolver.processResult(std::move(res));
      rawDpaCommandSolver.parseResponse();
      TRC_FUNCTION_LEAVE("");
    }

    IEnumerateService::IFastEnumerationPtr getFastEnumeration() const
    {
      TRC_FUNCTION_ENTER("");

      std::unique_ptr<FastEnumeration> retval(shape_new FastEnumeration);
      
      auto exclusiveAccess = m_iIqrfDpaService->getExclusiveAccess();

      iqrf::embed::coordinator::RawDpaBondedDevices iqrfEmbedCoordinatorBondedDevices;
      iqrf::embed::coordinator::RawDpaDiscoveredDevices iqrfEmbedCoordinatorDiscoveredDevices;
      
      {
        std::unique_ptr<IDpaTransactionResult2> transResult;
        exclusiveAccess->executeDpaTransactionRepeat(createDpaRequest(iqrfEmbedCoordinatorBondedDevices), transResult, m_repeat);
        processDpaTransactionResult(iqrfEmbedCoordinatorBondedDevices, std::move(transResult));
        retval->setBonded(iqrfEmbedCoordinatorBondedDevices.getBondedDevices());
      }

      {
        std::unique_ptr<IDpaTransactionResult2> transResult;
        exclusiveAccess->executeDpaTransactionRepeat(createDpaRequest(iqrfEmbedCoordinatorDiscoveredDevices), transResult, m_repeat);
        processDpaTransactionResult(iqrfEmbedCoordinatorDiscoveredDevices, std::move(transResult));
        retval->setDiscovered(iqrfEmbedCoordinatorDiscoveredDevices.getDiscoveredDevices());
      }

      std::set<int> evaluated = retval->getDiscovered();
      evaluated.insert(0); //eval coordinator

      for (auto nadr : evaluated) {
        //TODO do it by FRC for DPA > 4.02
        try {
          auto nd = getNodeDataPriv((uint16_t)nadr, exclusiveAccess);
          retval->addItem(nd->getNadr(), nd->getEmbedOsRead()->getMid(), nd->getHwpid(), nd->getEmbedExploreEnumerate()->getHwpidVer());
        }
        catch (std::logic_error &e) {
          CATCH_EXC_TRC_WAR(std::logic_error, e, "Cannot fast enum: " << PAR(nadr));
        }
      }

      TRC_FUNCTION_LEAVE("");
      return retval;
    }

    IEnumerateService::INodeDataPtr getNodeDataPriv(uint16_t nadr, std::unique_ptr<iqrf::IIqrfDpaService::ExclusiveAccess> & exclusiveAccess) const
    {
      TRC_FUNCTION_ENTER(nadr);

      IEnumerateService::INodeDataPtr nodeData;

      std::unique_ptr<embed::explore::RawDpaEnumerate> exploreEnumeratePtr(shape_new embed::explore::RawDpaEnumerate(nadr));
      std::unique_ptr <embed::os::RawDpaRead> osReadPtr(shape_new embed::os::RawDpaRead(nadr));

      {
        std::unique_ptr<IDpaTransactionResult2> transResult;
        exclusiveAccess->executeDpaTransactionRepeat(createDpaRequest(*osReadPtr), transResult, m_repeat);
        processDpaTransactionResult(*osReadPtr, std::move(transResult));
      }

      {
        std::unique_ptr<IDpaTransactionResult2> transResult;
        exclusiveAccess->executeDpaTransactionRepeat(createDpaRequest(*exploreEnumeratePtr), transResult, m_repeat);
        processDpaTransactionResult(*exploreEnumeratePtr, std::move(transResult));
      }

      nodeData.reset(shape_new NodeDataImpl(nadr, osReadPtr->getHwpid(), exploreEnumeratePtr, osReadPtr));

      TRC_FUNCTION_LEAVE("");
      return nodeData;
    }

    IEnumerateService::INodeDataPtr getNodeData(uint16_t nadr) const
    {
      TRC_FUNCTION_ENTER(nadr);

      IEnumerateService::INodeDataPtr nodeData;

      auto exclusiveAccess = m_iIqrfDpaService->getExclusiveAccess();

      nodeData = getNodeDataPriv(nadr, exclusiveAccess);

      //{
      //  iqrf::sensor::Enumerate iqrfSensorEnumerate(nadr);
      //  std::unique_ptr<IDpaTransactionResult2> transResult;
      //  exclusiveAccess->executeDpaTransactionRepeat(m_iJsDriverService->createDpaRequest(iqrfSensorEnumerate), transResult, m_repeat);
      //  m_iJsDriverService->processDpaTransactionResult(iqrfSensorEnumerate, std::move(transResult));
      //}

      //TODO other params

      TRC_FUNCTION_LEAVE("");
      return nodeData;
    }

    IEnumerateService::IStandardSensorDataPtr getStandardSensorData(uint16_t nadr) const
    {
      std::unique_ptr<iqrf::sensor::Enumerate> retval(shape_new iqrf::sensor::Enumerate(nadr));
      std::unique_ptr<IDpaTransactionResult2> transResult;
      m_iIqrfDpaService->executeDpaTransactionRepeat(m_iJsDriverService->createDpaRequest(*retval), transResult, m_repeat);
      m_iJsDriverService->processDpaTransactionResult(*retval, std::move(transResult));
      return retval;
    }

    IEnumerateService::IStandardBinaryOutputDataPtr getStandardBinaryOutputData(uint16_t nadr) const
    {
      std::unique_ptr<iqrf::binaryoutput::Enumerate> retval(shape_new iqrf::binaryoutput::Enumerate(nadr));
      std::unique_ptr<IDpaTransactionResult2> transResult;
      m_iIqrfDpaService->executeDpaTransactionRepeat(m_iJsDriverService->createDpaRequest(*retval), transResult, m_repeat);
      m_iJsDriverService->processDpaTransactionResult(*retval, std::move(transResult));
      return retval;
    }

    embed::explore::PeripheralInformationPtr getPeripheralInformationData(uint16_t nadr, int per) const
    {
      std::unique_ptr<iqrf::embed::explore::RawDpaPeripheralInformation> retval(shape_new iqrf::embed::explore::RawDpaPeripheralInformation(nadr, per));
      std::unique_ptr<IDpaTransactionResult2> transResult;
      m_iIqrfDpaService->executeDpaTransactionRepeat(createDpaRequest(*retval), transResult, m_repeat);
      processDpaTransactionResult(*retval, std::move(transResult));
      return retval;
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

  IEnumerateService::IFastEnumerationPtr EnumerateService::getFastEnumeration() const
  {
    return m_imp->getFastEnumeration();
  }

  //IEnumerateService::CoordinatorData EnumerateService::getCoordinatorData() const
  //{
  //  return m_imp->getCoordinatorData();
  //}

  IEnumerateService::INodeDataPtr EnumerateService::getNodeData(uint16_t nadr) const
  {
    return m_imp->getNodeData(nadr);
  }

  IEnumerateService::IStandardSensorDataPtr EnumerateService::getStandardSensorData(uint16_t nadr) const
  {
    return m_imp->getStandardSensorData(nadr);
  }

  IEnumerateService::IStandardBinaryOutputDataPtr EnumerateService::getStandardBinaryOutputData(uint16_t nadr) const
  {
    return m_imp->getStandardBinaryOutputData(nadr);
  }

  embed::explore::PeripheralInformationPtr EnumerateService::getPeripheralInformationData(uint16_t nadr, int per) const
  {
    return m_imp->getPeripheralInformationData(nadr, per);
  }

  void EnumerateService::attachInterface(iqrf::IIqrfDpaService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void EnumerateService::detachInterface(iqrf::IIqrfDpaService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void EnumerateService::attachInterface(iqrf::IJsCacheService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void EnumerateService::detachInterface(iqrf::IJsCacheService* iface)
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
