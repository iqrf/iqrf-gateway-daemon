#include "ApiMsg.h"
#include "JsonIqrfInfoApi.h"
#include "ObjectFactory.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "Trace.h"
#include <algorithm>
#include <fstream>

#include "iqrf__JsonIqrfInfoApi.hxx"

#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

TRC_INIT_MODULE(iqrf::JsonIqrfInfoApi);

using namespace rapidjson;

namespace iqrf {

  class JsonIqrfInfoApi::Imp
  {
  public:
    ////////////// status conversion
    enum class MsgStatus {
      st_ok,
      st_noDb,
      st_badParams
    };

    class MsgStatusConvertTable
    {
    public:
      static const std::vector<std::pair<MsgStatus, std::string>>& table()
      {
        static std::vector <std::pair<MsgStatus, std::string>> table = {
          { MsgStatus::st_ok, "ok" },
          { MsgStatus::st_noDb, "no DB" },
          { MsgStatus::st_badParams, "bad parameters" }
        };
        return table;
      }
      static MsgStatus defaultEnum()
      {
        return MsgStatus::st_badParams;
      }
      static const std::string& defaultStr()
      {
        static std::string u("unknown");
        return u;
      }
    };

    typedef shape::EnumStringConvertor<MsgStatus, MsgStatusConvertTable> MsgStatusConvertor;

    /////////// msg types as string
    const std::string mType_GetSensors = "infoDaemon_GetSensors";
    const std::string mType_GetBinaryOutputs = "infoDaemon_GetBinaryOutputs";

    /////////// message classes declarations
    class InfoDaemonMsg : public ApiMsg
    {
    public:
      InfoDaemonMsg() = delete;
      InfoDaemonMsg(const rapidjson::Document& doc)
        :ApiMsg(doc)
      {
      }

      virtual ~InfoDaemonMsg()
      {
      }

      MsgStatus getErr()
      {
        return m_st;
      }

      void setErr(MsgStatus st)
      {
        m_st = st;
        m_success = false;
      }

      bool isSuccess()
      {
        return m_success;
      }

      void createResponsePayload(rapidjson::Document& doc) override
      {
        if (m_success) {
          setStatus("ok", 0);
        }
        else {
          if (getVerbose()) {
            Pointer("/data/errorStr").Set(doc, MsgStatusConvertor::enum2str(m_st));
          }
          setStatus("err", -1);
        }
      }

      virtual void handleMsg(JsonIqrfInfoApi::Imp* imp) = 0;

    private:
      MsgStatus m_st = MsgStatus::st_ok;
      bool m_success = true;
    };

    //////////////////////////////////////////////
    class InfoDaemonMsgGetSensors : public InfoDaemonMsg
    {
    public:
      InfoDaemonMsgGetSensors() = delete;
      InfoDaemonMsgGetSensors(const rapidjson::Document& doc)
        :InfoDaemonMsg(doc)
      {
      }

      virtual ~InfoDaemonMsgGetSensors()
      {
      }

      void createResponsePayload(rapidjson::Document& doc) override
      {
        Document::AllocatorType & a = doc.GetAllocator();

        Value devicesVal;
        devicesVal.SetArray();

        for (auto & enm : m_enmMap) {
          Value devVal;
          Value sensorsVal;
          sensorsVal.SetArray();

          for (auto & s : enm.second->getSensors()) {
            // particular sensor
            Value senVal;
            Pointer("/idx").Set(senVal, s->getIdx(), a);
            Pointer("/id").Set(senVal, s->getSid(), a);
            Pointer("/type").Set(senVal, s->getType(), a);
            Pointer("/name").Set(senVal, s->getName(), a);
            Pointer("/shortName").Set(senVal, s->getShortName(), a);
            Pointer("/unit").Set(senVal, s->getUnit(), a);
            Pointer("/decimalPlaces").Set(senVal, s->getDecimalPlaces(), a);
            Value frcVal;
            frcVal.SetArray();
            for (auto frc : s->getFrcs()) {
              frcVal.PushBack(frc, a);
            }
            Pointer("/frcs").Set(senVal, frcVal, a);

            // add particular sensor to sensors[]
            sensorsVal.PushBack(senVal, a);
          }

          Pointer("/nadr").Set(devVal, enm.first, a);
          Pointer("/sensors").Set(devVal, sensorsVal, a);

          devicesVal.PushBack(devVal, a);
        }

        Pointer("/data/rsp/sensorDevices").Set(doc, devicesVal, a);

        InfoDaemonMsg::createResponsePayload(doc);
      }

      void handleMsg(JsonIqrfInfoApi::Imp* imp) override
      {
        TRC_FUNCTION_ENTER("");

        m_enmMap = imp->getSensors();

        TRC_FUNCTION_LEAVE("");
      }

    private:
      std::map<int, sensor::EnumeratePtr> m_enmMap;
    };

    //////////////////////////////////////////////
    class InfoDaemonMsgGetBinaryOutputs : public InfoDaemonMsg
    {
    public:
      InfoDaemonMsgGetBinaryOutputs() = delete;
      InfoDaemonMsgGetBinaryOutputs(const rapidjson::Document& doc)
        :InfoDaemonMsg(doc)
      {
      }

      virtual ~InfoDaemonMsgGetBinaryOutputs()
      {
      }

      void createResponsePayload(rapidjson::Document& doc) override
      {
        Document::AllocatorType & a = doc.GetAllocator();

        Value devicesVal;
        devicesVal.SetArray();

        for (auto & enm : m_enmMap) {
          Value devVal;

          Pointer("/nadr").Set(devVal, enm.first, a);
          Pointer("/binOuts").Set(devVal, enm.second->getBinaryOutputsNum(), a);

          devicesVal.PushBack(devVal, a);
        }

        Pointer("/data/rsp/binOutsDevices").Set(doc, devicesVal, a);

        InfoDaemonMsg::createResponsePayload(doc);
      }

      void handleMsg(JsonIqrfInfoApi::Imp* imp) override
      {
        TRC_FUNCTION_ENTER("");

        m_enmMap = imp->getBinaryOutputs();

        TRC_FUNCTION_LEAVE("");
      }

    private:
      std::map<int, binaryoutput::EnumeratePtr> m_enmMap;
    };

  ///////////////////// Imp members
  private:

    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    IIqrfInfo* m_iIqrfInfo = nullptr;
    ObjectFactory<InfoDaemonMsg, rapidjson::Document&> m_objectFactory;

    // TODO from cfg
    std::vector<std::string> m_filters =
    {
      "infoDaemon_",
    };

  public:
    Imp()
    {
      m_objectFactory.registerClass<InfoDaemonMsgGetSensors>(mType_GetSensors);
      m_objectFactory.registerClass<InfoDaemonMsgGetBinaryOutputs>(mType_GetBinaryOutputs);
    }

    ~Imp()
    {
    }

    //for debug only
    static std::string JsonToStr(const rapidjson::Value* val)
    {
      rapidjson::Document doc;
      doc.CopyFrom(*val, doc.GetAllocator());
      rapidjson::StringBuffer buffer;
      rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
      doc.Accept(writer);
      return buffer.GetString();
    }

    void handleMsg(const std::string & messagingId, const IMessagingSplitterService::MsgType & msgType, rapidjson::Document reqDoc)
    {
      TRC_FUNCTION_ENTER(PAR(messagingId) << NAME_PAR(mType, msgType.m_type) <<
        NAME_PAR(major, msgType.m_major) << NAME_PAR(minor, msgType.m_minor) << NAME_PAR(micro, msgType.m_micro));

      Document respDoc;
      std::unique_ptr<InfoDaemonMsg> msg = m_objectFactory.createObject(msgType.m_type, reqDoc);

      msg->handleMsg(this);
      msg->createResponse(respDoc);

      std::string t = JsonToStr(&respDoc);

      m_iMessagingSplitterService->sendMessage(messagingId, std::move(respDoc));

      TRC_FUNCTION_LEAVE("");
    }

    std::map<int, sensor::EnumeratePtr> getSensors() const
    {
      return m_iIqrfInfo->getSensors();
    }

    std::map<int, binaryoutput::EnumeratePtr> getBinaryOutputs() const
    {
      return m_iIqrfInfo->getBinaryOutputs();
    }

    void activate(const shape::Properties *props)
    {
      (void)props; //silence -Wunused-parameter
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "JsonIqrfInfoApi instance activate" << std::endl <<
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
        "JsonIqrfInfoApi instance deactivate" << std::endl <<
        "******************************"
      );

      m_iMessagingSplitterService->unregisterFilteredMsgHandler(m_filters);

      TRC_FUNCTION_LEAVE("")
    }

    void modify(const shape::Properties *props)
    {
      (void)props; //silence -Wunused-parameter
    }

    void attachInterface(IIqrfInfo* iface)
    {
      m_iIqrfInfo = iface;
    }

    void detachInterface(IIqrfInfo* iface)
    {
      if (m_iIqrfInfo == iface) {
        m_iIqrfInfo = nullptr;
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
  JsonIqrfInfoApi::JsonIqrfInfoApi()
  {
    m_imp = shape_new Imp();
  }

  JsonIqrfInfoApi::~JsonIqrfInfoApi()
  {
    delete m_imp;
  }

  void JsonIqrfInfoApi::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void JsonIqrfInfoApi::deactivate()
  {
    m_imp->deactivate();
  }

  void JsonIqrfInfoApi::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
  }

  void JsonIqrfInfoApi::attachInterface(IIqrfInfo* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsonIqrfInfoApi::detachInterface(IIqrfInfo* iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsonIqrfInfoApi::attachInterface(IMessagingSplitterService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsonIqrfInfoApi::detachInterface(IMessagingSplitterService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsonIqrfInfoApi::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void JsonIqrfInfoApi::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

}
