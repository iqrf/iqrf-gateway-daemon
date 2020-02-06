#include "ApiMsg.h"
#include "JsonIqrfInfoApi.h"
#include "ObjectFactory.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "JsonUtils.h"
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
    const std::string mType_Enumeration = "infoDaemon_Enumeration";
    const std::string mType_MidMetaDataAnnotate = "infoDaemon_MidMetaDataAnnotate";
    const std::string mType_GetMidMetaData = "infoDaemon_GetMidMetaData";
    const std::string mType_SetMidMetaData = "infoDaemon_SetMidMetaData";
    const std::string mType_GetNodeMetaData = "infoDaemon_GetNodeMetaData";
    const std::string mType_SetNodeMetaData = "infoDaemon_SetNodeMetaData";
    const std::string mType_OrphanedMids = "infoDaemon_OrphanedMids";

    /////////// message classes declarations
    class InfoDaemonMsg : public ApiMsg
    {
    public:
      InfoDaemonMsg()
        :ApiMsg()
      {}

      InfoDaemonMsg(const rapidjson::Document& doc)
        :ApiMsg(doc)
      {}

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
        m_imp = imp;
      }

      virtual void handleMsg(JsonIqrfInfoApi::Imp* imp) = 0;

      const std::string & getMessagingId() { return m_messagingId; }
      void setMessagingId(const std::string & messagingId) { m_messagingId = messagingId; }

    protected:
      IMetaDataApi* m_iMetaDataApi = nullptr;
      Imp * m_imp = nullptr;
      std::string m_messagingId;
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

          // old metadata
          if (m_iMetaDataApi) {
            if (m_iMetaDataApi->iSmetaDataToMessages()) {
              Pointer("/metaData").Set(devVal, m_iMetaDataApi->getMetaData(nadr), a);
            }
          }

          // db metadata
          if (m_imp) {
            if (m_imp->getMidMetaDataAnnotate()) {
              Pointer("/midMetaData").Set(devVal, m_imp->getNodeMetaData(nadr), a);
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

          // old metadata
          if (m_iMetaDataApi) {
            if (m_iMetaDataApi->iSmetaDataToMessages()) {
              Pointer("/metaData").Set(devVal, m_iMetaDataApi->getMetaData(nadr), a);
            }
          }

          // db metadata
          if (m_imp) {
            if (m_imp->getMidMetaDataAnnotate()) {
              Pointer("/midMetaData").Set(devVal, m_imp->getNodeMetaData(nadr), a);
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

          // old metadata
          if (m_iMetaDataApi) {
            if (m_iMetaDataApi->iSmetaDataToMessages()) {
              Pointer("/metaData").Set(devVal, m_iMetaDataApi->getMetaData(nadr), a);
            }
          }

          // db metadata
          if (m_imp) {
            if (m_imp->getMidMetaDataAnnotate()) {
              Pointer("/midMetaData").Set(devVal, m_imp->getNodeMetaData(nadr), a);
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

          // old metadata
          if (m_iMetaDataApi) {
            if (m_iMetaDataApi->iSmetaDataToMessages()) {
              Pointer("/metaData").Set(devVal, m_iMetaDataApi->getMetaData(nadr), a);
            }
          }

          // db metadata
          if (m_imp) {
            if (m_imp->getMidMetaDataAnnotate()) {
              Pointer("/midMetaData").Set(devVal, m_imp->getNodeMetaData(nadr), a);
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

          // old metadata
          if (m_iMetaDataApi) {
            if (m_iMetaDataApi->iSmetaDataToMessages()) {
              Pointer("/metaData").Set(devVal, m_iMetaDataApi->getMetaData(nadr), a);
            }
          }

          // db metadata
          if (m_imp) {
            if (m_imp->getMidMetaDataAnnotate()) {
              Pointer("/midMetaData").Set(devVal, m_imp->getNodeMetaData(nadr), a);
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
    class InfoDaemonMsgEnumeration : public InfoDaemonMsg
    {
    public:
      enum class Cmd {
        Unknown,
        Start,
        Stop,
        GetPeriod,
        SetPeriod,
        Now
      };

      class CmdConvertTable
      {
      public:
        static const std::vector<std::pair<Cmd, std::string>>& table()
        {
          static std::vector <std::pair<Cmd, std::string>> table = {
            { Cmd::Unknown, "unknown" },
            { Cmd::Start, "start" },
            { Cmd::Stop, "stop" },
            { Cmd::GetPeriod, "getPeriod" },
            { Cmd::SetPeriod, "setPeriod" },
            { Cmd::Now, "now" }
          };
          return table;
        }
        static Cmd defaultEnum()
        {
          return Cmd::Unknown;
        }
        static const std::string& defaultStr()
        {
          static std::string u("unknown");
          return u;
        }
      };
      typedef shape::EnumStringConvertor<Cmd, CmdConvertTable> CmdStringConvertor;

      InfoDaemonMsgEnumeration()
        :InfoDaemonMsg()
      {}

      InfoDaemonMsgEnumeration(const rapidjson::Document& doc)
        :InfoDaemonMsg(doc)
      {
        using namespace rapidjson;
        std::string cmd = Pointer("/data/req/command").Get(doc)->GetString();
        m_command = CmdStringConvertor::str2enum(cmd);
        if (m_command == Cmd::Unknown) {
          THROW_EXC_TRC_WAR(std::logic_error, "Unknown command: " << cmd);
        }

        const Value *val = Pointer("/data/req/period").Get(doc);
        if (val && val->IsInt()) {
          m_period = val->GetInt();
        }
      }

      virtual ~InfoDaemonMsgEnumeration()
      {
      }

      void createResponsePayload(rapidjson::Document& doc) override
      {
        InfoDaemonMsg::createResponsePayload(doc);
        Document::AllocatorType & a = doc.GetAllocator();
        Pointer("/data/rsp/command").Set(doc, CmdStringConvertor::enum2str(m_command), a);
        if (m_command == Cmd::GetPeriod || m_command == Cmd::SetPeriod) {
          Pointer("/data/rsp/period").Set(doc, m_period, a);
        }
        if (m_command == Cmd::Now) {
          Pointer("/data/rsp/enumPhase").Set(doc, (int)m_estate.m_phase, a);
          Pointer("/data/rsp/step").Set(doc, m_estate.m_step, a);
          Pointer("/data/rsp/steps").Set(doc, m_estate.m_steps, a);
        }
      }

      void handleMsg(JsonIqrfInfoApi::Imp* imp) override
      {
        TRC_FUNCTION_ENTER("");
        switch (m_command) {
        case Cmd::Start:
          imp->startEnumeration();
          break;
        case Cmd::Stop:
          imp->stopEnumeration();
          break;
        case Cmd::GetPeriod:
          m_period = imp->getPeriodEnumeration();
          break;
        case Cmd::SetPeriod:
          imp->setPeriodEnumeration(m_period);
          break;
        case Cmd::Now:
          imp->enumerate(*this);
          break;
        default:
          ;
        }
        TRC_FUNCTION_LEAVE("");
      }

      Cmd getCommand() const { return m_command; }
      int getPeriod() const { return m_period; }
      void setEnumerationState(IIqrfInfo::EnumerationState estate) { m_estate = estate; }

    private:
      Cmd m_command = Cmd::Start;
      int m_period = 0;
      IIqrfInfo::EnumerationState m_estate;
    };

    //////////////////////////////////////////////
    class InfoDaemonMsgMidMetaDataAnnotate : public InfoDaemonMsg
    {
    public:
      InfoDaemonMsgMidMetaDataAnnotate() = delete;
      InfoDaemonMsgMidMetaDataAnnotate(const rapidjson::Document& doc)
        :InfoDaemonMsg(doc)
      {
        using namespace rapidjson;
        m_annotate = Pointer("/data/req/annotate").Get(doc)->GetBool();
      }

      virtual ~InfoDaemonMsgMidMetaDataAnnotate()
      {
      }

      void createResponsePayload(rapidjson::Document& doc) override
      {
        using namespace rapidjson;
        Document::AllocatorType & a = doc.GetAllocator();
        Pointer("/data/rsp/annotate").Set(doc, m_annotate, a);

        InfoDaemonMsg::createResponsePayload(doc);
      }

      void handleMsg(JsonIqrfInfoApi::Imp* imp) override
      {
        TRC_FUNCTION_ENTER("");
        imp->setMidMetaDataAnnotate(m_annotate);
        TRC_FUNCTION_LEAVE("");
      }

    private:
      bool m_annotate;
    };

    //////////////////////////////////////////////
    class InfoDaemonMsgGetMidMetaData : public InfoDaemonMsg
    {
    public:
      InfoDaemonMsgGetMidMetaData() = delete;
      InfoDaemonMsgGetMidMetaData(const rapidjson::Document& doc)
        :InfoDaemonMsg(doc)
      {
        using namespace rapidjson;
        const Value * midVal = Pointer("/data/req/mid").Get(doc);
        if (!midVal->IsUint()) {
          int64_t bad_mid = midVal->GetInt64();
          THROW_EXC_TRC_WAR(std::logic_error, "Passed value is not valid: " << PAR(bad_mid));
        }
        m_mid = midVal->GetUint();
      }

      virtual ~InfoDaemonMsgGetMidMetaData()
      {
      }

      void createResponsePayload(rapidjson::Document& doc) override
      {
        using namespace rapidjson;
        Document::AllocatorType & a = doc.GetAllocator();
        Pointer("/data/rsp/mid").Set(doc, m_mid, a);
        Pointer("/data/rsp/metaData").Set(doc, m_metaData, a);

        InfoDaemonMsg::createResponsePayload(doc);
      }

      void handleMsg(JsonIqrfInfoApi::Imp* imp) override
      {
        TRC_FUNCTION_ENTER("");
        m_metaData.CopyFrom(imp->getMidMetaData(m_mid), m_metaData.GetAllocator());
        TRC_FUNCTION_LEAVE("");
      }

    private:
      uint32_t m_mid;
      rapidjson::Document m_metaData;
    };

    //////////////////////////////////////////////
    class InfoDaemonMsgSetMidMetaData : public InfoDaemonMsg
    {
    public:
      InfoDaemonMsgSetMidMetaData() = delete;
      InfoDaemonMsgSetMidMetaData(const rapidjson::Document& doc)
        :InfoDaemonMsg(doc)
      {
        using namespace rapidjson;
        const Value * midVal = Pointer("/data/req/mid").Get(doc);
        if (!midVal->IsUint()) {
          int64_t bad_mid = midVal->GetInt64();
          THROW_EXC_TRC_WAR(std::logic_error, "Passed value is not valid: " << PAR(bad_mid));
        }
        m_mid = midVal->GetUint();

        const Value *val = Pointer("/data/req/metaData").Get(doc);
        m_metaDataDoc.CopyFrom(*val, m_metaDataDoc.GetAllocator());
      }

      virtual ~InfoDaemonMsgSetMidMetaData()
      {
      }

      void createResponsePayload(rapidjson::Document& doc) override
      {
        using namespace rapidjson;
        Document::AllocatorType & a = doc.GetAllocator();
        Pointer("/data/rsp/mid").Set(doc, m_mid, a);
        Pointer("/data/rsp/metaData").Set(doc, m_metaDataDoc, a);

        InfoDaemonMsg::createResponsePayload(doc);
      }

      void handleMsg(JsonIqrfInfoApi::Imp* imp) override
      {
        TRC_FUNCTION_ENTER("");
        imp->setMidMetaData(m_mid, m_metaDataDoc);
        TRC_FUNCTION_LEAVE("");
      }

    private:
      uint32_t m_mid;
      rapidjson::Document m_metaDataDoc;
    };

    //////////////////////////////////////////////
    class InfoDaemonMsgGetNodeMetaData : public InfoDaemonMsg
    {
    public:
      InfoDaemonMsgGetNodeMetaData() = delete;
      InfoDaemonMsgGetNodeMetaData(const rapidjson::Document& doc)
        :InfoDaemonMsg(doc)
      {
        using namespace rapidjson;
        m_nadr = Pointer("/data/req/nAdr").Get(doc)->GetInt();
      }

      virtual ~InfoDaemonMsgGetNodeMetaData()
      {
      }

      void createResponsePayload(rapidjson::Document& doc) override
      {
        using namespace rapidjson;
        Document::AllocatorType & a = doc.GetAllocator();
        Pointer("/data/rsp/nAdr").Set(doc, m_nadr, a);
        Pointer("/data/rsp/metaData").Set(doc, m_metaData, a);

        InfoDaemonMsg::createResponsePayload(doc);
      }

      void handleMsg(JsonIqrfInfoApi::Imp* imp) override
      {
        TRC_FUNCTION_ENTER("");
        m_metaData.CopyFrom(imp->getNodeMetaData(m_nadr), m_metaData.GetAllocator());
        TRC_FUNCTION_LEAVE("");
      }

    private:
      int m_nadr;
      rapidjson::Document m_metaData;
    };

    //////////////////////////////////////////////
    class InfoDaemonMsgSetNodeMetaData : public InfoDaemonMsg
    {
    public:
      InfoDaemonMsgSetNodeMetaData() = delete;
      InfoDaemonMsgSetNodeMetaData(const rapidjson::Document& doc)
        :InfoDaemonMsg(doc)
      {
        using namespace rapidjson;
        m_nadr = Pointer("/data/req/nAdr").Get(doc)->GetInt();
        const Value *val = Pointer("/data/req/metaData").Get(doc);
        m_metaDataDoc.CopyFrom(*val, m_metaDataDoc.GetAllocator());
      }

      virtual ~InfoDaemonMsgSetNodeMetaData()
      {
      }

      void createResponsePayload(rapidjson::Document& doc) override
      {
        using namespace rapidjson;
        Document::AllocatorType & a = doc.GetAllocator();
        Pointer("/data/rsp/nAdr").Set(doc, m_nadr, a);
        Pointer("/data/rsp/metaData").Set(doc, m_metaDataDoc, a);

        InfoDaemonMsg::createResponsePayload(doc);
      }

      void handleMsg(JsonIqrfInfoApi::Imp* imp) override
      {
        TRC_FUNCTION_ENTER("");
        imp->setNodeMetaData(m_nadr, m_metaDataDoc);
        TRC_FUNCTION_LEAVE("");
      }

    private:
      int m_nadr;
      rapidjson::Document m_metaDataDoc;
    };

    //////////////////////////////////////////////
    class InfoDaemonMsgOrphanedMids : public InfoDaemonMsg
    {
    public:
      enum class Cmd {
        Unknown,
        Get,
        Remove,
      };

      class CmdConvertTable
      {
      public:
        static const std::vector<std::pair<Cmd, std::string>>& table()
        {
          static std::vector <std::pair<Cmd, std::string>> table = {
            { Cmd::Unknown, "unknown" },
            { Cmd::Get, "get" },
            { Cmd::Remove, "remove" },
          };
          return table;
        }
        static Cmd defaultEnum()
        {
          return Cmd::Unknown;
        }
        static const std::string& defaultStr()
        {
          static std::string u("unknown");
          return u;
        }
      };
      typedef shape::EnumStringConvertor<Cmd, CmdConvertTable> CmdStringConvertor;

      InfoDaemonMsgOrphanedMids() = delete;
      InfoDaemonMsgOrphanedMids(const rapidjson::Document& doc)
        :InfoDaemonMsg(doc)
      {
        using namespace rapidjson;

        std::string cmd = Pointer("/data/req/command").Get(doc)->GetString();
        m_command = CmdStringConvertor::str2enum(cmd);
        if (m_command == Cmd::Unknown) {
          THROW_EXC_TRC_WAR(std::logic_error, "Unknown command: " << cmd);
        }

        const Value * midsVal = Pointer("/data/req/mids").Get(doc);
        if (midsVal && midsVal->IsArray()) {
          for (auto itr = midsVal->Begin(); itr != midsVal->End(); ++itr) {
            if (!itr->IsUint()) {
              int64_t bad_mid = itr->GetInt64();
              THROW_EXC_TRC_WAR(std::logic_error, "Passed value is not valid: " << PAR(bad_mid));
            }
            m_mids.push_back(itr->GetUint());
          }
        }
      }

      virtual ~InfoDaemonMsgOrphanedMids()
      {
      }

      void createResponsePayload(rapidjson::Document& doc) override
      {
        using namespace rapidjson;
        Document::AllocatorType & a = doc.GetAllocator();

        Pointer("/data/rsp/command").Set(doc, CmdStringConvertor::enum2str(m_command), a);

        Value midsVal;
        midsVal.SetArray();
        for (auto mid : m_mids) {
          midsVal.PushBack(mid, a);
        }
        Pointer("/data/rsp/mids").Set(doc, midsVal, a);

        InfoDaemonMsg::createResponsePayload(doc);
      }

      void handleMsg(JsonIqrfInfoApi::Imp* imp) override
      {
        TRC_FUNCTION_ENTER("");
        switch (m_command) {
        case Cmd::Get:
          m_mids = imp->getUnbondMids();
          break;
        case Cmd::Remove:
          imp->removeUnbondMids(m_mids);
          break;
        default:
          ;
        }
        TRC_FUNCTION_LEAVE("");
      }

    private:
      std::vector<uint32_t> m_mids;
      Cmd m_command;
    };

  ///////////////////// Imp members
  private:

    IMetaDataApi* m_iMetaDataApi = nullptr;
    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    IIqrfInfo* m_iIqrfInfo = nullptr;
    ObjectFactory<InfoDaemonMsg, rapidjson::Document&> m_objectFactory;

    std::vector<std::string> m_filters =
    {
      "infoDaemon_",
    };

    // async enumeration reporting
    std::unique_ptr<InfoDaemonMsgEnumeration> m_infoDaemonMsgEnumeration;
    std::mutex m_infoDaemonMsgEnumerationMtx;

  public:
    Imp()
    {
      m_objectFactory.registerClass<InfoDaemonMsgGetSensors>(mType_GetSensors);
      m_objectFactory.registerClass<InfoDaemonMsgGetBinaryOutputs>(mType_GetBinaryOutputs);
      m_objectFactory.registerClass<InfoDaemonMsgGetDalis>(mType_GetDalis);
      m_objectFactory.registerClass<InfoDaemonMsgGetLights>(mType_GetLights);
      m_objectFactory.registerClass <InfoDaemonMsgGetNodes>(mType_GetNodes);
      m_objectFactory.registerClass<InfoDaemonMsgEnumeration>(mType_Enumeration);
      m_objectFactory.registerClass<InfoDaemonMsgMidMetaDataAnnotate>(mType_MidMetaDataAnnotate);
      m_objectFactory.registerClass<InfoDaemonMsgGetMidMetaData>(mType_GetMidMetaData);
      m_objectFactory.registerClass<InfoDaemonMsgSetMidMetaData>(mType_SetMidMetaData);
      m_objectFactory.registerClass<InfoDaemonMsgGetNodeMetaData>(mType_GetNodeMetaData);
      m_objectFactory.registerClass<InfoDaemonMsgSetNodeMetaData>(mType_SetNodeMetaData);
      m_objectFactory.registerClass<InfoDaemonMsgOrphanedMids>(mType_OrphanedMids);
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
        msg->setMessagingId(messagingId);
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

    void stopEnumeration()
    {
      m_iIqrfInfo->stopEnumeration();
    }

    int getPeriodEnumeration()
    {
      return m_iIqrfInfo->getPeriodEnumerate();
    }

    void setPeriodEnumeration(int period)
    {
      m_iIqrfInfo->setPeriodEnumerate(period); 
    }
    
    void enumerate(InfoDaemonMsgEnumeration & msg)
    {
      std::unique_lock<std::mutex> lck(m_infoDaemonMsgEnumerationMtx);
      if (m_infoDaemonMsgEnumeration) {
        THROW_EXC_TRC_WAR(std::logic_error, "Enumeration transaction is already running");
      }
      m_infoDaemonMsgEnumeration = std::unique_ptr<InfoDaemonMsgEnumeration>(shape_new InfoDaemonMsgEnumeration(msg));
      m_iIqrfInfo->enumerate();
    }

    //async handler of enumeration events
    void handleEnumEvent(IIqrfInfo::EnumerationState estate)
    {
      std::unique_lock<std::mutex> lck(m_infoDaemonMsgEnumerationMtx);
      if (m_infoDaemonMsgEnumeration) {
        rapidjson::Document rspDoc;
        m_infoDaemonMsgEnumeration->setEnumerationState(estate);
        m_infoDaemonMsgEnumeration->setStatus("ok", 0);
        m_infoDaemonMsgEnumeration->createResponse(rspDoc);
        m_iMessagingSplitterService->sendMessage(m_infoDaemonMsgEnumeration->getMessagingId(), std::move(rspDoc));
        if (estate.m_phase == IIqrfInfo::EnumerationState::Phase::finish) {
          //final state destroy handling
          m_infoDaemonMsgEnumeration.reset();
        }
      }
    }

    void registerCb(InfoDaemonMsgEnumeration & msg)
    {
      //TODO remove
    }

    void unRegisterCb(InfoDaemonMsgEnumeration & msg)
    {
      //TODO remove
    }

    std::vector<uint32_t> getUnbondMids() const
    {
      return m_iIqrfInfo->getUnbondMids();
    }

    void removeUnbondMids(std::vector<uint32_t> unbondVec)
    {
      m_iIqrfInfo->removeUnbondMids(unbondVec);
    }

    bool getMidMetaDataAnnotate() const
    {
      return m_iIqrfInfo->getMidMetaDataToMessages();
    }

    void setMidMetaDataAnnotate(bool val) const
    {
      return m_iIqrfInfo->setMidMetaDataToMessages(val);
    }

    rapidjson::Document getMidMetaData(uint32_t mid) const
    {
      return m_iIqrfInfo->getMidMetaData(mid);
    }

    void setMidMetaData(uint32_t mid, const rapidjson::Value & metaData)
    {
      m_iIqrfInfo->setMidMetaData(mid, metaData);
    }

    rapidjson::Document getNodeMetaData(int nadr) const
    {
      return m_iIqrfInfo->getNodeMetaData(nadr);
    }

    void setNodeMetaData(int nadr, const rapidjson::Value & metaData)
    {
      m_iIqrfInfo->setNodeMetaData(nadr, metaData);
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

      m_iIqrfInfo->registerEnumerateHandler("kkk",
        [&](IIqrfInfo::EnumerationState estate)
      {
        handleEnumEvent(estate);
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
      m_iIqrfInfo->unregisterEnumerateHandler("kkk");

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
