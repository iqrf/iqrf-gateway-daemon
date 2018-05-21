#include "ApiMsg.h"
#include "JsonCfgApi.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "Trace.h"
#include <algorithm>
#include <fstream>
#include <iostream>

#include "iqrf__JsonCfgApi.hxx"

#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

TRC_INIT_MODULE(iqrf::JsonCfgApi);

using namespace rapidjson;

namespace iqrf {
  class CfgMsg : public ApiMsg
  {
  public:
    CfgMsg() = delete;
    CfgMsg(rapidjson::Document& doc)
      :ApiMsg(doc)
    {
      {
        rapidjson::Value* v = rapidjson::Pointer("/data/req/componentName").Get(doc);
        if (v && v->IsString()) {
          m_componentName = v->GetString();
        }
      }
      {
        rapidjson::Value* v = rapidjson::Pointer("/data/req/componentInstance").Get(doc);
        if (v && v->IsString()) {
          m_componentInstance = v->GetString();
        }
      }
      {
        rapidjson::Value* v = rapidjson::Pointer("/data/req/configuration").Get(doc);
        if (v && v->IsObject()) {
          m_cfgDoc.CopyFrom(*v, m_cfgDoc.GetAllocator());
        }
      }
    }

    virtual ~CfgMsg()
    {
    }

    const std::string& getComponentName() const
    {
      return m_componentName;
    }

    const std::string& getComponentInstance() const
    {
      return m_componentInstance;
    }

    const rapidjson::Document& getConfiguration() const
    {
      return m_cfgDoc;
    }

  private:
    rapidjson::Document m_cfgDoc;
    std::string m_componentName;
    std::string m_componentInstance;

  };

  class JsonCfgApi::Imp
  {
  private:

    shape::ILaunchService* m_iLaunchService = nullptr;
    shape::IConfigurationService* m_iConfigurationService = nullptr;
    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;

    std::vector<std::string> m_filters =
    {
      "cfgDaemon"
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

      using namespace rapidjson;

      Document respDoc;
      if (msgType.m_type == "cfgDaemon_Component") {
        CfgMsg msg(doc);
        
        try {
          const Document& cfgDoc = msg.getConfiguration();
          {
            StringBuffer buffer;
            PrettyWriter<StringBuffer> writer(buffer);
            cfgDoc.Accept(writer);
            std::string cfgStr = buffer.GetString();
            TRC_DEBUG(std::endl << cfgStr);
          }

          shape::IConfiguration* cfgPtr =
            m_iConfigurationService->getConfiguration(msg.getComponentName(), msg.getComponentInstance());

          if (cfgPtr) {
            Document& propDoc = cfgPtr->getProperties()->getAsJson();
            {
              StringBuffer buffer;
              PrettyWriter<StringBuffer> writer(buffer);
              propDoc.Accept(writer);
              std::string cfgStr = buffer.GetString();
              TRC_DEBUG(std::endl << cfgStr);
            }

            propDoc.CopyFrom(cfgDoc, propDoc.GetAllocator());
            Pointer("/component").Set(propDoc, msg.getComponentName());
            Pointer("/instance").Set(propDoc, msg.getComponentInstance());

            cfgPtr->update();

            // prepare ok response
            Pointer("/data/rsp/componentName").Set(respDoc, msg.getComponentName());
            Pointer("/data/rsp/componentInstance").Set(respDoc, msg.getComponentInstance());

            if (msg.getVerbose()) {
              Pointer("/data/insId").Set(respDoc, "iqrfgd2-1"); // TODO replace by daemon instance id
              Pointer("/data/statusStr").Set(respDoc, "ok");
            }

            Pointer("/data/status").Set(respDoc, 0);
          }
          else {
            THROW_EXC_TRC_WAR(std::logic_error, "Cannot find configuration");
          }
        }
        catch (std::exception &e) {
          Pointer("/data/rsp/componentName").Set(respDoc, msg.getComponentName());
          Pointer("/data/rsp/componentInstance").Set(respDoc, msg.getComponentInstance());

          if (msg.getVerbose()) {
            Pointer("/data/insId").Set(respDoc, "iqrfgd2-1"); // TODO replace by daemon instance id
            Pointer("/data/statusStr").Set(respDoc, "err");
            Pointer("/data/errorStr").Set(respDoc, e.what());
          }

          Pointer("/data/status").Set(respDoc, -1);
        }
      }

      m_iMessagingSplitterService->sendMessage(messagingId, std::move(respDoc));

      TRC_FUNCTION_LEAVE("");
    }

    void activate(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "JsonCfgApi instance activate" << std::endl <<
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
        "JsonCfgApi instance deactivate" << std::endl <<
        "******************************"
      );

      m_iMessagingSplitterService->unregisterFilteredMsgHandler(m_filters);

      TRC_FUNCTION_LEAVE("")
    }

    void modify(const shape::Properties *props)
    {
    }

    void attachInterface(shape::ILaunchService* iface)
    {
      m_iLaunchService = iface;
    }

    void detachInterface(shape::ILaunchService* iface)
    {
      if (m_iLaunchService == iface) {
        m_iLaunchService = nullptr;
      }
    }

    void attachInterface(shape::IConfigurationService* iface)
    {
      m_iConfigurationService = iface;
    }

    void detachInterface(shape::IConfigurationService* iface)
    {
      if (m_iConfigurationService == iface) {
        m_iConfigurationService = nullptr;
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
  JsonCfgApi::JsonCfgApi()
  {
    m_imp = shape_new Imp();
  }

  JsonCfgApi::~JsonCfgApi()
  {
    delete m_imp;
  }

  void JsonCfgApi::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void JsonCfgApi::deactivate()
  {
    m_imp->deactivate();
  }

  void JsonCfgApi::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
  }

  void JsonCfgApi::attachInterface(shape::ILaunchService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsonCfgApi::detachInterface(shape::ILaunchService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsonCfgApi::attachInterface(shape::IConfigurationService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsonCfgApi::detachInterface(shape::IConfigurationService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsonCfgApi::attachInterface(IMessagingSplitterService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsonCfgApi::detachInterface(IMessagingSplitterService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsonCfgApi::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void JsonCfgApi::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

}
