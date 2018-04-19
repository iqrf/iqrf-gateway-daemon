#define IMessagingSplitterService_EXPORTS

#include "ApiMsg.h"
#include "JsonMngApi.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "Trace.h"
#include <algorithm>
#include <fstream>

#include "iqrf__JsonMngApi.hxx"

#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

TRC_INIT_MODULE(iqrf::JsonMngApi);

using namespace rapidjson;

namespace iqrf {
  class MngModeMsg : public ApiMsg
  {
  public:
    MngModeMsg() = delete;
    MngModeMsg(rapidjson::Document& doc)
      :ApiMsg(doc)
    {
      m_mode = ModeStringConvertor::str2enum(rapidjson::Pointer("/data/req/operMode").Get(doc)->GetString());
    }

    virtual ~MngModeMsg()
    {
    }

    IUdpConnectorService::Mode getMode() const
    {
      return m_mode;
    }

  private:
    IUdpConnectorService::Mode m_mode;
  };

  class JsonMngApi::Imp
  {
  private:

    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    IUdpConnectorService* m_iUdpConnectorService = nullptr;

    std::vector<std::string> m_filters =
    {
      "mngDaemon"
    };

  public:
    Imp()
    {
    }

    ~Imp()
    {
    }

    void handleMsg(const std::string & messagingId, const IMessagingSplitterService::MsgType & msgType, rapidjson::Document doc)
    {
      TRC_FUNCTION_ENTER(PAR(messagingId) << NAME_PAR(mType, msgType.m_type) <<
        NAME_PAR(major, msgType.m_major) << NAME_PAR(minor, msgType.m_minor) << NAME_PAR(micro, msgType.m_micro));

      Document respDoc;
      if (msgType.m_type == "mngDaemon_Mode") {
        MngModeMsg msg(doc);

        if (m_iUdpConnectorService) {
          m_iUdpConnectorService->setMode(msg.getMode());
        }

        rapidjson::Pointer("/rsp/operMode").Set(respDoc, ModeStringConvertor::enum2str(msg.getMode()));

        if (msg.getVerbose()) {
          rapidjson::Pointer("/data/insId").Set(respDoc, "iqrfgd2-1"); // TODO replace by daemon instance id
          rapidjson::Pointer("/data/statusStr").Set(respDoc, "ok");
        }

        rapidjson::Pointer("/data/status").Set(respDoc, 0);
      }

      //TODO validate response in debug
      m_iMessagingSplitterService->sendMessage(messagingId, std::move(respDoc));

      TRC_FUNCTION_LEAVE("");
    }

    void activate(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "JsonMngApi instance activate" << std::endl <<
        "******************************"
      );

      m_iMessagingSplitterService->registerFilteredMsgHandler(m_filters,
        [&](const std::string & messagingId, const IMessagingSplitterService::MsgType & msgType, rapidjson::Document doc)
      {
        handleMsg(messagingId, msgType, std::move(doc));
      });

      TRC_FUNCTION_LEAVE("")
    }

    void deactivate()
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "JsonMngApi instance deactivate" << std::endl <<
        "******************************"
      );

      m_iMessagingSplitterService->unregisterFilteredMsgHandler(m_filters);

      TRC_FUNCTION_LEAVE("")
    }

    void modify(const shape::Properties *props)
    {
    }

    void attachInterface(IUdpConnectorService* iface)
    {
      m_iUdpConnectorService = iface;
    }

    void detachInterface(IUdpConnectorService* iface)
    {
      if (m_iUdpConnectorService == iface) {
        m_iUdpConnectorService = nullptr;
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
  JsonMngApi::JsonMngApi()
  {
    m_imp = shape_new Imp();
  }

  JsonMngApi::~JsonMngApi()
  {
    delete m_imp;
  }

  void JsonMngApi::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void JsonMngApi::deactivate()
  {
    m_imp->deactivate();
  }

  void JsonMngApi::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
  }

  void JsonMngApi::attachInterface(IUdpConnectorService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsonMngApi::detachInterface(IUdpConnectorService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsonMngApi::attachInterface(IMessagingSplitterService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsonMngApi::detachInterface(IMessagingSplitterService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsonMngApi::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void JsonMngApi::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

}
