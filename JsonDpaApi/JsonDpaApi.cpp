#define IMessagingSplitterService_EXPORTS

#include "ComRaws.h"
#include "ComSdevs.h"
#include "DpaHandler2.h"
#include "JsonDpaApi.h"
#include "ObjectFactory.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "ITemplateService.h"
#include "Trace.h"
#include <algorithm>
#include <fstream>

#include "iqrf__JsonDpaApi.hxx"

#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

TRC_INIT_MODULE(iqrf::JsonDpaApi);

using namespace rapidjson;

namespace iqrf {
  class JsonDpaApi::Imp
  {
  private:
    const std::string mType_comEperCoordRebond = "comEperCoordRebond";
    const std::string mType_comEperCoordRemoveBond = "comEperCoordRemoveBond";
    const std::string mType_comEperCoordRestore = "comEperCoordRestore";
    const std::string mType_comEperCoordSetDpaParams = "comEperCoordSetDpaParams";
    const std::string mType_comEperCoordSetHops = "comEperCoordSetHops";
    const std::string mType_comEperExploreEnum = "comEperExploreEnum";
    const std::string mType_comEperExploreMorePerInfo = "comEperExploreMorePerInfo";
    const std::string mType_comEperExplorePerInfo = "comEperExplorePerInfo";
    const std::string mType_comEperFrcExtraResult = "comEperFrcExtraResult";
    const std::string mType_comEperFrcSend = "comEperFrcSend";
    const std::string mType_comEperFrcSendSelective = "comEperFrcSendSelective";
    const std::string mType_comEperFrcSetParams = "comEperFrcSetParams";
    const std::string mType_comEperIoDir = "comEperIoDir";
    const std::string mType_comEperIoGet = "comEperIoGet";
    const std::string mType_comEperIoSet = "comEperIoSet";
    const std::string mType_comEperLedGet = "comEperLedGet";
    const std::string mType_comEperLedPulse = "comEperLedPulse";
    const std::string mType_comEperLedSet = "comEperLedSet";
    const std::string mType_comEperMemoryRead = "comEperMemoryRead";
    const std::string mType_comEperMemoryWrite = "comEperMemoryWrite";
    const std::string mType_comEperNodeBackup = "comEperNodeBackup";
    const std::string mType_comEperNodeClearRemotelyBondedMid = "comEperNodeClearRemotelyBondedMid";
    const std::string mType_comEperNodeEnableRemoteBond = "comEperNodeEnableRemoteBond";
    const std::string mType_comEperNodeRead = "comEperNodeRead";
    const std::string mType_comEperNodeReadRemotelyBondedMid = "comEperNodeReadRemotelyBondedMid";
    const std::string mType_comEperNodeRemoveBond = "comEperNodeRemoveBond";
    const std::string mType_comEperNodeRestore = "comEperNodeRestore";
    const std::string mType_comEperOsBatch = "comEperOsBatch";
    const std::string mType_comEperOsInitRR = "comEperOsInitRR";
    const std::string mType_comEperOsLoadCode = "comEperOsLoadCode";
    const std::string mType_comEperOsRead = "comEperOsRead";
    const std::string mType_comEperOsReadCfg = "comEperOsReadCfg";
    const std::string mType_comEperOsRunRfpgm = "comEperOsRunRfpgm";
    const std::string mType_comEperOsSelectiveBatch = "comEperOsSelectiveBatch";
    const std::string mType_comEperOsSetSecurity = "comEperOsSetSecurity";
    const std::string mType_comEperOsSleep = "comEperOsSleep";
    const std::string mType_comEperOsWriteCfg = "comEperOsWriteCfg";
    const std::string mType_comEperOsWriteCfgByte = "comEperOsWriteCfgByte";
    const std::string mType_comEperSpiWriteRead = "comEperSpiWriteRead";
    const std::string mType_comEperThermometerRead = "comEperThermometerRead";
    const std::string mType_comEperUartClearWriteRead = "comEperUartClearWriteRead";
    const std::string mType_comEperUartClose = "comEperUartClose";
    const std::string mType_comEperUartOpen = "comEperUartOpen";
    const std::string mType_comEperUartWriteRead = "comEperUartWriteRead";
    const std::string mType_comRaw = "comRaw";
    const std::string mType_comRawHdp = "comRawHdp";
    const std::string mType_comSdevBinaryOutputEnum = "comSdevBinaryOutputEnum";
    const std::string mType_comSdevBinaryOutputSetOutput = "comSdevBinaryOutputSetOutput";
    const std::string mType_comSdevLightDecrementPower = "comSdevLightDecrementPower";
    const std::string mType_comSdevLightEnum = "comSdevLightEnum";
    const std::string mType_comSdevLightIncrementPower = "comSdevLightIncrementPower";
    const std::string mType_comSdevLightSetPower = "comSdevLightSetPower";
    const std::string mType_comSdevSensorEnum = "comSdevSensorEnum";
    const std::string mType_comSdevSensorFrc = "comSdevSensorFrc";
    const std::string mType_comSdevSensorReadwt = "comSdevSensorReadwt";

    iqrf::IJsCacheService* m_iJsCacheService = nullptr;
    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    IIqrfDpaService* m_iIqrfDpaService = nullptr;
    
    //Scheme support
    std::vector<IMessagingSplitterService::MsgType> m_supported =
    {
      { mType_comRaw, 1,0,0},
      { mType_comRawHdp, 1,0,0},
      { mType_comSdevBinaryOutputEnum, 1,0,0 },
      { mType_comSdevBinaryOutputSetOutput, 1,0,0 },
      { mType_comSdevLightDecrementPower, 1,0,0 },
      { mType_comSdevLightEnum, 1,0,0 },
      { mType_comSdevLightIncrementPower, 1,0,0 },
      { mType_comSdevLightSetPower, 1,0,0 },
      { mType_comSdevSensorEnum, 1,0,0 },
      { mType_comSdevSensorFrc, 1,0,0 },
      { mType_comSdevSensorReadwt, 1,0,0 }

    };

    ObjectFactory<ComBase, rapidjson::Document&> m_objectFactory;

  public:
    Imp()
    {
      m_objectFactory.registerClass<ComRaw>(mType_comRaw);
      m_objectFactory.registerClass<ComRawHdp>(mType_comRawHdp);
      m_objectFactory.registerClass<ComSdevBinaryOutputEnum>(mType_comSdevBinaryOutputEnum);
      m_objectFactory.registerClass<ComSdevBinaryOutputSetOutput>(mType_comSdevBinaryOutputSetOutput);
      m_objectFactory.registerClass<ComSdevLightDecrementPower>(mType_comSdevLightDecrementPower);
      m_objectFactory.registerClass<ComSdevLightEnum>(mType_comSdevLightEnum);
      m_objectFactory.registerClass<ComSdevLightIncrementPower>(mType_comSdevLightIncrementPower);
      m_objectFactory.registerClass<ComSdevLightSetPower>(mType_comSdevLightSetPower);
      m_objectFactory.registerClass<ComSdevSensorEnum>(mType_comSdevSensorEnum);
      m_objectFactory.registerClass<ComSdevSensorFrc>(mType_comSdevSensorFrc);
      m_objectFactory.registerClass<ComSdevSensorReadwt>(mType_comSdevSensorReadwt);
    }

    ~Imp()
    {
    }

    void handleMsg(const std::string & messagingId, const IMessagingSplitterService::MsgType & msgType, rapidjson::Document doc)
    {
      TRC_FUNCTION_ENTER(PAR(messagingId) << NAME_PAR(mType, msgType.m_type) <<
        NAME_PAR(major, msgType.m_major) << NAME_PAR(minor, msgType.m_minor) << NAME_PAR(micro, msgType.m_micro));

      std::unique_ptr<ComBase> com = m_objectFactory.createObject(msgType.m_type, doc);

      auto trn = m_iIqrfDpaService->executeDpaTransaction(com->getDpaRequest());
      auto res = trn->get();

      Document respDoc;
      com->createResponse(respDoc, *res);

      //update message type - type is the same for request/response
      Pointer("/mType").Set(respDoc, msgType.m_type);

      //TODO validate response in debug
      m_iMessagingSplitterService->sendMessage(messagingId, std::move(respDoc));

      TRC_FUNCTION_LEAVE("");
    }

    void activate(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "JsonDpaApi instance activate" << std::endl <<
        "******************************"
      );

      for (auto & sup : m_supported) {
        sup.m_handlerFunc =
          [&](const std::string & messagingId, const IMessagingSplitterService::MsgType & msgType, rapidjson::Document doc)
        {
          handleMsg(messagingId, msgType, std::move(doc));
        };
      }
      m_iMessagingSplitterService->registerFilteredMsgHandler(m_supported);

      TRC_FUNCTION_LEAVE("")
    }

    void deactivate()
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "JsonDpaApi instance deactivate" << std::endl <<
        "******************************"
      );

      m_iMessagingSplitterService->unregisterFilteredMsgHandler(m_supported);

      TRC_FUNCTION_LEAVE("")
    }

    void modify(const shape::Properties *props)
    {
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

  /////////////////////////
  JsonDpaApi::JsonDpaApi()
  {
    m_imp = shape_new Imp();
  }

  JsonDpaApi::~JsonDpaApi()
  {
    delete m_imp;
  }

  void JsonDpaApi::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void JsonDpaApi::deactivate()
  {
    m_imp->deactivate();
  }

  void JsonDpaApi::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
  }

  void JsonDpaApi::attachInterface(iqrf::IJsCacheService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsonDpaApi::detachInterface(iqrf::IJsCacheService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsonDpaApi::attachInterface(IIqrfDpaService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsonDpaApi::detachInterface(IIqrfDpaService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsonDpaApi::attachInterface(IMessagingSplitterService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsonDpaApi::detachInterface(IMessagingSplitterService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsonDpaApi::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void JsonDpaApi::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

}
