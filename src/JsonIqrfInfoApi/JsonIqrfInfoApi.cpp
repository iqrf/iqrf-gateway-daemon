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
    /////////// msg types as string
    const std::string mType_GetSensors = "infoDaemon_GetSensors";
    const std::string mType_GetBinaryOutputs = "infoDaemon_GetBinaryOutputs";
    const std::string mType_GetDalis = "infoDaemon_GetDalis";
    const std::string mType_GetLights = "infoDaemon_GetLights";
    const std::string mType_GetNodes = "infoDaemon_GetNodes";
    const std::string mType_StartEnumeration = "infoDaemon_StartEnumeration";

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

      void createResponsePayload(rapidjson::Document& doc) override
      {
        Value *notEmpty = Pointer("/data/rsp").Get(doc);
        if (!(notEmpty)) {
          // set empty rsp if not exists to satisfy validator
          Value empty;
          empty.SetObject();
          Pointer("/data/rsp").Set(doc, empty);
        }
      }

      void setMetaDataApi(JsonIqrfInfoApi::Imp* imp) // store for later use in response
      {
        m_iMetaDataApi = imp->getMetadataApi();
      }

      virtual void handleMsg(JsonIqrfInfoApi::Imp* imp) = 0;

    protected:
      IMetaDataApi* m_iMetaDataApi = nullptr;
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

          int nadr = enm.first;
          Pointer("/nAdr").Set(devVal, nadr, a);
          Pointer("/sensors").Set(devVal, sensorsVal, a);

          if (m_iMetaDataApi) {
            if (m_iMetaDataApi->iSmetaDataToMessages()) {
              Pointer("/metaData").Set(devVal, m_iMetaDataApi->getMetaData(nadr), a);
            }
          }

          devicesVal.PushBack(devVal, a);
        }

        Pointer("/data/rsp/sensorDevices").Set(doc, devicesVal, a);

        InfoDaemonMsg::createResponsePayload(doc);
      }

      void handleMsg(JsonIqrfInfoApi::Imp* imp) override
      {
        TRC_FUNCTION_ENTER("");

        setMetaDataApi(imp); //can be used in response
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

          int nadr = enm.first;
          Pointer("/nAdr").Set(devVal, nadr, a);
          Pointer("/binOuts").Set(devVal, enm.second->getBinaryOutputsNum(), a);

          if (m_iMetaDataApi) {
            if (m_iMetaDataApi->iSmetaDataToMessages()) {
              Pointer("/metaData").Set(devVal, m_iMetaDataApi->getMetaData(nadr), a);
            }
          }

          devicesVal.PushBack(devVal, a);
        }

        Pointer("/data/rsp/binOutDevices").Set(doc, devicesVal, a);

        InfoDaemonMsg::createResponsePayload(doc);
      }

      void handleMsg(JsonIqrfInfoApi::Imp* imp) override
      {
        TRC_FUNCTION_ENTER("");

        setMetaDataApi(imp); //can be used in response
        m_enmMap = imp->getBinaryOutputs();

        TRC_FUNCTION_LEAVE("");
      }

    private:
      std::map<int, binaryoutput::EnumeratePtr> m_enmMap;
    };

    //////////////////////////////////////////////
    class InfoDaemonMsgGetDalis : public InfoDaemonMsg
    {
    public:
      InfoDaemonMsgGetDalis() = delete;
      InfoDaemonMsgGetDalis(const rapidjson::Document& doc)
        :InfoDaemonMsg(doc)
      {
      }

      virtual ~InfoDaemonMsgGetDalis()
      {
      }

      void createResponsePayload(rapidjson::Document& doc) override
      {
        Document::AllocatorType & a = doc.GetAllocator();

        Value devicesVal;
        devicesVal.SetArray();

        for (auto & enm : m_enmMap) {
          Value devVal;

          int nadr = enm.first;
          Pointer("/nAdr").Set(devVal, nadr, a);

          if (m_iMetaDataApi) {
            if (m_iMetaDataApi->iSmetaDataToMessages()) {
              Pointer("/metaData").Set(devVal, m_iMetaDataApi->getMetaData(nadr), a);
            }
          }

          devicesVal.PushBack(devVal, a);
        }

        Pointer("/data/rsp/daliDevices").Set(doc, devicesVal, a);

        InfoDaemonMsg::createResponsePayload(doc);
      }

      void handleMsg(JsonIqrfInfoApi::Imp* imp) override
      {
        TRC_FUNCTION_ENTER("");

        setMetaDataApi(imp); //can be used in response
        m_enmMap = imp->getDalis();

        TRC_FUNCTION_LEAVE("");
      }

    private:
      std::map<int, dali::EnumeratePtr> m_enmMap;
    };
    
    //////////////////////////////////////////////
    class InfoDaemonMsgGetLights : public InfoDaemonMsg
    {
    public:
      InfoDaemonMsgGetLights() = delete;
      InfoDaemonMsgGetLights(const rapidjson::Document& doc)
        :InfoDaemonMsg(doc)
      {
      }

      virtual ~InfoDaemonMsgGetLights()
      {
      }

      void createResponsePayload(rapidjson::Document& doc) override
      {
        Document::AllocatorType & a = doc.GetAllocator();

        Value devicesVal;
        devicesVal.SetArray();

        for (auto & enm : m_enmMap) {
          Value devVal;

          int nadr = enm.first;
          Pointer("/nAdr").Set(devVal, nadr, a);
          Pointer("/lights").Set(devVal, enm.second->getLightsNum(), a);

          if (m_iMetaDataApi) {
            if (m_iMetaDataApi->iSmetaDataToMessages()) {
              Pointer("/metaData").Set(devVal, m_iMetaDataApi->getMetaData(nadr), a);
            }
          }

          devicesVal.PushBack(devVal, a);
        }

        Pointer("/data/rsp/lightDevices").Set(doc, devicesVal, a);

        InfoDaemonMsg::createResponsePayload(doc);
      }

      void handleMsg(JsonIqrfInfoApi::Imp* imp) override
      {
        TRC_FUNCTION_ENTER("");

        setMetaDataApi(imp); //can be used in response
        m_enmMap = imp->getLights();

        TRC_FUNCTION_LEAVE("");
      }

    private:
      std::map<int, light::EnumeratePtr> m_enmMap;
    };

    //////////////////////////////////////////////
    class InfoDaemonMsgGetNodes : public InfoDaemonMsg
    {
    public:
      InfoDaemonMsgGetNodes() = delete;
      InfoDaemonMsgGetNodes(const rapidjson::Document& doc)
        :InfoDaemonMsg(doc)
      {
      }

      virtual ~InfoDaemonMsgGetNodes()
      {
      }

      void createResponsePayload(rapidjson::Document& doc) override
      {
        Document::AllocatorType & a = doc.GetAllocator();

        Value devicesVal;
        devicesVal.SetArray();

        for (auto & enm : m_enmMap) {
          Value devVal;

          int nadr = enm.first;
          Pointer("/nAdr").Set(devVal, nadr, a);
          Pointer("/mid").Set(devVal, enm.second->getMid(), a);
          Pointer("/disc").Set(devVal, enm.second->getDisc(), a);
          Pointer("/hwpid").Set(devVal, enm.second->getHwpid(), a);
          Pointer("/hwpidVer").Set(devVal, enm.second->getHwpidVer(), a);
          Pointer("/osBuild").Set(devVal, enm.second->getOsBuild(), a);
          Pointer("/dpaVer").Set(devVal, enm.second->getDpaVer(), a);

          if (m_iMetaDataApi) {
            if (m_iMetaDataApi->iSmetaDataToMessages()) {
              Pointer("/metaData").Set(devVal, m_iMetaDataApi->getMetaData(nadr), a);
            }
          }

          devicesVal.PushBack(devVal, a);
        }

        Pointer("/data/rsp/nodes").Set(doc, devicesVal, a);

        InfoDaemonMsg::createResponsePayload(doc);
      }

      void handleMsg(JsonIqrfInfoApi::Imp* imp) override
      {
        TRC_FUNCTION_ENTER("");
        
        setMetaDataApi(imp); //can be used in response
        m_enmMap = imp->getNodes();

        TRC_FUNCTION_LEAVE("");
      }

    private:
      std::map<int, embed::node::BriefInfoPtr> m_enmMap;
    };

    //////////////////////////////////////////////
    class InfoDaemonMsgStartEnumeration : public InfoDaemonMsg
    {
    public:
      InfoDaemonMsgStartEnumeration() = delete;
      InfoDaemonMsgStartEnumeration(const rapidjson::Document& doc)
        :InfoDaemonMsg(doc)
      {
      }

      virtual ~InfoDaemonMsgStartEnumeration()
      {
      }

      void createResponsePayload(rapidjson::Document& doc) override
      {
        InfoDaemonMsg::createResponsePayload(doc);
      }

      void handleMsg(JsonIqrfInfoApi::Imp* imp) override
      {
        TRC_FUNCTION_ENTER("");
        imp->startEnumeration();
        TRC_FUNCTION_LEAVE("");
      }

    private:
      std::map<int, binaryoutput::EnumeratePtr> m_enmMap;
    };

  ///////////////////// Imp members
  private:

    IMetaDataApi* m_iMetaDataApi = nullptr;
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
      m_objectFactory.registerClass<InfoDaemonMsgGetDalis>(mType_GetDalis);
      m_objectFactory.registerClass<InfoDaemonMsgGetLights>(mType_GetLights);
      m_objectFactory.registerClass <InfoDaemonMsgGetNodes>(mType_GetNodes);
      m_objectFactory.registerClass<InfoDaemonMsgStartEnumeration>(mType_StartEnumeration);
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

      std::unique_ptr<InfoDaemonMsg> msg = m_objectFactory.createObject(msgType.m_type, reqDoc);

      try {
        Document respDoc;
        msg->handleMsg(this);
        msg->setStatus("ok", 0);
        msg->createResponse(respDoc);
        m_iMessagingSplitterService->sendMessage(messagingId, std::move(respDoc));
      }
      catch (std::exception & e) {
        msg->setStatus(e.what(), -1);
        Document respErrDoc;
        msg->createResponse(respErrDoc);
        m_iMessagingSplitterService->sendMessage(messagingId, std::move(respErrDoc));
      }

      TRC_FUNCTION_LEAVE("");
    }

    IMetaDataApi* getMetadataApi()
    {
      return m_iMetaDataApi;
    }

    std::map<int, sensor::EnumeratePtr> getSensors() const
    {
      return m_iIqrfInfo->getSensors();
    }

    std::map<int, binaryoutput::EnumeratePtr> getBinaryOutputs() const
    {
      return m_iIqrfInfo->getBinaryOutputs();
    }

    std::map<int, dali::EnumeratePtr> getDalis() const
    {
      return m_iIqrfInfo->getDalis();
    }

    std::map<int, light::EnumeratePtr> getLights() const
    {
      return m_iIqrfInfo->getLights();
    }

    std::map<int, embed::node::BriefInfoPtr> getNodes() const
    {
      return m_iIqrfInfo->getNodes();
    }

    void startEnumeration()
    {
      m_iIqrfInfo->startEnumeration();
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

    void attachInterface(IMetaDataApi* iface)
    {
      m_iMetaDataApi = iface;
    }

    void detachInterface(IMetaDataApi* iface)
    {
      if (m_iMetaDataApi == iface) {
        m_iMetaDataApi = nullptr;
      }
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

  void JsonIqrfInfoApi::attachInterface(iqrf::IMetaDataApi* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsonIqrfInfoApi::detachInterface(iqrf::IMetaDataApi* iface)
  {
    m_imp->detachInterface(iface);
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
