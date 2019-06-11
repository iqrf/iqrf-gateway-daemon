#define IEnumerateService_EXPORTS

#include "EnumerateService.h"
#include "Trace.h"
#include "JsonUtils.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/pointer.h"

#include "FastEnumeration.h"
#include "RawDpaEmbedCoordinator.h"
#include "JsDriverEmbedOS.h"
#include "JsDriverEmbedEEEPROM.h"
#include "JsDriverEmbedDpaExploration.h"
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
          retval->addItem(nadr, nd.getMid(), nd.getHwpid(), nd.getHwpidVer());
        }
        catch (std::logic_error &e) {
          CATCH_EXC_TRC_WAR(std::logic_error, e, "Cannot fast enum: " << PAR(nadr));
        }
      }

      TRC_FUNCTION_LEAVE("");
      return retval;
    }

    IEnumerateService::CoordinatorData getCoordinatorData() const
    {
      TRC_FUNCTION_ENTER("");

      IEnumerateService::CoordinatorData coordinatorData;
      coordinatorData.m_valid = false;

      auto exclusiveAccess = m_iIqrfDpaService->getExclusiveAccess();

      {
        iqrf::embed::coordinator::JsDriverBondedDevices iqrfEmbedCoordinatorBondedDevices;
        std::unique_ptr<IDpaTransactionResult2> transResult;
        exclusiveAccess->executeDpaTransactionRepeat(m_iJsDriverService->createDpaRequest(iqrfEmbedCoordinatorBondedDevices), transResult, m_repeat);
        m_iJsDriverService->processDpaTransactionResult(iqrfEmbedCoordinatorBondedDevices, std::move(transResult));

        coordinatorData.m_bonded =  iqrfEmbedCoordinatorBondedDevices.getBondedDevices();
      }

      {
        iqrf::embed::coordinator::JsDriverDiscoveredDevices iqrfEmbedCoordinatorDiscoveredDevices;
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

    IEnumerateService::NodeData getNodeDataPriv(uint16_t nadr, std::unique_ptr<iqrf::IIqrfDpaService::ExclusiveAccess> & exclusiveAccess) const
    {
      TRC_FUNCTION_ENTER(nadr);

      IEnumerateService::NodeData nodeData;

      {
        iqrf::embed::os::RawDpaRead iqrfEmbedOsRead(nadr);
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

      nodeData.setValid(true);

      TRC_FUNCTION_LEAVE("");
      return nodeData;
    }

    IEnumerateService::NodeData getNodeData(uint16_t nadr) const
    {
      TRC_FUNCTION_ENTER(nadr);

      IEnumerateService::NodeData nodeData;

      auto exclusiveAccess = m_iIqrfDpaService->getExclusiveAccess();

      getNodeDataPriv(nadr, exclusiveAccess);

      //{
      //  iqrf::sensor::Enumerate iqrfSensorEnumerate(nadr);
      //  std::unique_ptr<IDpaTransactionResult2> transResult;
      //  exclusiveAccess->executeDpaTransactionRepeat(m_iJsDriverService->createDpaRequest(iqrfSensorEnumerate), transResult, m_repeat);
      //  m_iJsDriverService->processDpaTransactionResult(iqrfSensorEnumerate, std::move(transResult));
      //}

      //TODO other params

      nodeData.setValid(true);

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

    IEnumerateService::IPeripheralInformationDataPtr getPeripheralInformationData(uint16_t nadr, int per) const
    {
      std::unique_ptr<iqrf::embed::explore::PeripheralInformation> retval(shape_new iqrf::embed::explore::PeripheralInformation(nadr, per));
      std::unique_ptr<IDpaTransactionResult2> transResult;
      m_iIqrfDpaService->executeDpaTransactionRepeat(m_iJsDriverService->createDpaRequest(*retval), transResult, m_repeat);
      m_iJsDriverService->processDpaTransactionResult(*retval, std::move(transResult));
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

  IEnumerateService::CoordinatorData EnumerateService::getCoordinatorData() const
  {
    return m_imp->getCoordinatorData();
  }

  IEnumerateService::NodeData EnumerateService::getNodeData(uint16_t nadr) const
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

  IEnumerateService::IPeripheralInformationDataPtr EnumerateService::getPeripheralInformationData(uint16_t nadr, int per) const
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
