#define IMessagingSplitterService_EXPORTS

#include "JsonDpaApiRaw.h"
#include "JsonApiMessageNames.h"
#include "ComRaws.h"
#include "DpaHandler2.h"
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

#include "iqrf__JsonDpaApiRaw.hxx"

#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

TRC_INIT_MODULE(iqrf::JsonDpaApiRaw);

using namespace rapidjson;

namespace iqrf {
  class JsonDpaApiRaw::Imp
  {
  private:

    iqrf::IJsCacheService* m_iJsCacheService = nullptr;
    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    IIqrfDpaService* m_iIqrfDpaService = nullptr;

    //Scheme support
    std::vector<IMessagingSplitterService::MsgType> m_supported =
    {
      { mType_iqrfRaw, 1,0,0},
      { mType_iqrfRawHdp, 1,0,0},
    };

    ObjectFactory<ComBase, rapidjson::Document&> m_objectFactory;

  public:
    Imp()
    {
      m_objectFactory.registerClass<ComRaw>(mType_iqrfRaw);
      m_objectFactory.registerClass<ComRawHdp>(mType_iqrfRawHdp);
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
        "JsonDpaApiRaw instance activate" << std::endl <<
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
        "JsonDpaApiRaw instance deactivate" << std::endl <<
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
  JsonDpaApiRaw::JsonDpaApiRaw()
  {
    m_imp = shape_new Imp();
  }

  JsonDpaApiRaw::~JsonDpaApiRaw()
  {
    delete m_imp;
  }

  void JsonDpaApiRaw::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void JsonDpaApiRaw::deactivate()
  {
    m_imp->deactivate();
  }

  void JsonDpaApiRaw::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
  }

  void JsonDpaApiRaw::attachInterface(iqrf::IJsCacheService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsonDpaApiRaw::detachInterface(iqrf::IJsCacheService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsonDpaApiRaw::attachInterface(IIqrfDpaService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsonDpaApiRaw::detachInterface(IIqrfDpaService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsonDpaApiRaw::attachInterface(IMessagingSplitterService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsonDpaApiRaw::detachInterface(IMessagingSplitterService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsonDpaApiRaw::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void JsonDpaApiRaw::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

}
