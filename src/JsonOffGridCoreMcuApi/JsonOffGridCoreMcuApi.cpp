/**
 * Copyright 2020 MICRORISC,s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ApiMsg.h"
#include "ObjectFactory.h"
#include "JsonOffGridCoreMcuApi.h"
//#include "AutorunParams.h"
#include "Trace.h"
#include "ComBase.h"
#include <chrono>
#include <iostream>
#include <fstream>
#include <set>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <random>
#include <cstddef>
#include <tuple>
#include <cmath>
#include <list>
#include <vector>
#include <sstream>
#include <sys/stat.h>
 // for windows mkdir
#ifdef SHAPE_PLATFORM_WINDOWS
#include <direct.h>
#endif

#include "iqrf__JsonOffGridCoreMcuApi.hxx"

TRC_INIT_MODULE(iqrf::JsonOffGridCoreMcuApi);

using namespace rapidjson;

namespace iqrf
{
  class JsonOffGridCoreMcuApi::Imp {
  private:

    // Message type: iqrf apps dali
    const std::string mType_SetTimer = "iqrfGwMcu_SetTimer";
    const std::string mType_GetTimer = "iqrfGwMcu_GetTimer";
    const std::string mType_SetRTC = "iqrfGwMcu_SetRTC";
    const std::string mType_GetRTC = "iqrfGwMcu_GetRTC";
    const std::string mType_GetCharger = "iqrfGwMcu_GetCharger";
    const std::string mType_SetPower = "iqrfGwMcu_SetPower";
    const std::string mType_Getpower = "iqrfGwMcu_Getpower";

    /////////// message classes declarations
    class IqrfGwMcuMsg : public ApiMsg
    {
    public:
      IqrfGwMcuMsg()
        :ApiMsg()
      {}

      IqrfGwMcuMsg(const rapidjson::Document& doc)
        :ApiMsg(doc)
      {
        m_command = Pointer("/data/req/command").Get(doc)->GetString();
      }

      virtual ~IqrfGwMcuMsg()
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

        Pointer("/data/rsp/command").Set(doc, m_command);

        if (getVerbose()) {
          auto & a = doc.GetAllocator();
          Value vectVal(Type::kArrayType);
          
          for (auto raw : m_rawVect) {
            Value val(Type::kObjectType);

            Pointer("/request").Set(val, raw.request, a);
            Pointer("/requestTs").Set(val, encodeTimestamp(raw.requestTs), a);
            Pointer("/response").Set(val, raw.response, a);
            Pointer("/responseTs").Set(val, encodeTimestamp(raw.responseTs), a);

            vectVal.PushBack(val, a);
          }

          Pointer("/data/raw").Set(doc, vectVal);
        }
      }

      virtual void handleMsg(JsonOffGridCoreMcuApi::Imp* imp)
      {
        if (getVerbose()) {
          m_rawVect.clear();
        }
      }

      const std::string & getMessagingId() { return m_messagingId; }
      void setMessagingId(const std::string & messagingId) { m_messagingId = messagingId; }

    protected:
      Imp * m_imp = nullptr;
      std::string m_messagingId;
      std::string m_command;
      std::vector< IOffGridCoreMcu::Raw> m_rawVect;
    };

    //////////////////////////////////////////////
    class IqrfGwMcuMsgGetTimer : public IqrfGwMcuMsg
    {
    public:
      IqrfGwMcuMsgGetTimer() = delete;
      IqrfGwMcuMsgGetTimer(const rapidjson::Document& doc)
        :IqrfGwMcuMsg(doc)
      {
      }

      virtual ~IqrfGwMcuMsgGetTimer()
      {
      }

      void createResponsePayload(rapidjson::Document& doc) override
      {
        Pointer("/data/rsp/pwrOffTime").Set(doc, m_strPwrOffTime);
        Pointer("/data/rsp/wakeUpTime").Set(doc, m_strWakeUpTime);

        IqrfGwMcuMsg::createResponsePayload(doc);
      }

      void handleMsg(JsonOffGridCoreMcuApi::Imp* imp) override
      {
        TRC_FUNCTION_ENTER("");

        IqrfGwMcuMsg::handleMsg(imp);

        if (m_command == "timer") {
          m_strPwrOffTime = imp->m_iOffGridCoreMcu->getPwrOffTimeCmd();
          if (getVerbose()) {
            m_rawVect.push_back(imp->m_iOffGridCoreMcu->getLastRaw());
          }

          m_strWakeUpTime = imp->m_iOffGridCoreMcu->getWakeUpTimeCmd();
          if (getVerbose()) {
            m_rawVect.push_back(imp->m_iOffGridCoreMcu->getLastRaw());
          }
        }
        else {
          THROW_EXC_TRC_WAR(std::logic_error, "Unknown command: " << PAR(m_command));
        }


        TRC_FUNCTION_LEAVE("");
      }

    private:
      std::string m_strPwrOffTime;
      std::string m_strWakeUpTime;
    };

    //////////////////////////////////////////////
    class IqrfGwMcuMsgSetTimer : public IqrfGwMcuMsg
    {
    public:
      IqrfGwMcuMsgSetTimer() = delete;
      IqrfGwMcuMsgSetTimer(const rapidjson::Document& doc)
        :IqrfGwMcuMsg(doc)
      {
        using namespace rapidjson;
        m_time = Pointer("/data/req/time").Get(doc)->GetString();
      }

      virtual ~IqrfGwMcuMsgSetTimer()
      {
      }

      void createResponsePayload(rapidjson::Document& doc) override
      {
        Pointer("/data/rsp/time").Set(doc, m_time);
        IqrfGwMcuMsg::createResponsePayload(doc);
      }

      void handleMsg(JsonOffGridCoreMcuApi::Imp* imp) override
      {
        TRC_FUNCTION_ENTER("");

        IqrfGwMcuMsg::handleMsg(imp);

        if (m_command == "pwrOffTime") {
          imp->m_iOffGridCoreMcu->setPwrOffTimeCmd(m_time);
          if (getVerbose()) {
            m_rawVect.push_back(imp->m_iOffGridCoreMcu->getLastRaw());
          }
        }
        else if (m_command == "wakeUpTime") {
          imp->m_iOffGridCoreMcu->setWakeUpTimeCmd(m_time);
          if (getVerbose()) {
            m_rawVect.push_back(imp->m_iOffGridCoreMcu->getLastRaw());
          }
        }
        else {
          THROW_EXC_TRC_WAR(std::logic_error, "Unknown command: " << PAR(m_command));
        }

        TRC_FUNCTION_LEAVE("");
      }

    private:
      std::string m_time;
    };

    //////////////////////////////////////////////
    class IqrfGwMcuMsgGetRTC : public IqrfGwMcuMsg
    {
    public:
      IqrfGwMcuMsgGetRTC() = delete;
      IqrfGwMcuMsgGetRTC(const rapidjson::Document& doc)
        :IqrfGwMcuMsg(doc)
      {
      }

      virtual ~IqrfGwMcuMsgGetRTC()
      {
      }

      void createResponsePayload(rapidjson::Document& doc) override
      {
        Pointer("/data/rsp/dateTime").Set(doc, m_strDateTime);
        IqrfGwMcuMsg::createResponsePayload(doc);
      }

      void handleMsg(JsonOffGridCoreMcuApi::Imp* imp) override
      {
        TRC_FUNCTION_ENTER("");

        IqrfGwMcuMsg::handleMsg(imp);

        if (m_command == "rtc") {
          m_strDateTime = imp->m_iOffGridCoreMcu->getRtcDateCmd();
          if (getVerbose()) {
            m_rawVect.push_back(imp->m_iOffGridCoreMcu->getLastRaw());
          }

          m_strDateTime += 'T';

          m_strDateTime += imp->m_iOffGridCoreMcu->getRtcTimeCmd();
          if (getVerbose()) {
            m_rawVect.push_back(imp->m_iOffGridCoreMcu->getLastRaw());
          }
        }
        else {
          THROW_EXC_TRC_WAR(std::logic_error, "Unknown command: " << PAR(m_command));
        }

        TRC_FUNCTION_LEAVE("");
      }

    private:
      std::string m_strDateTime;
    };

    //////////////////////////////////////////////
    class IqrfGwMcuMsgSetRTC : public IqrfGwMcuMsg
    {
    public:
      IqrfGwMcuMsgSetRTC() = delete;
      IqrfGwMcuMsgSetRTC(const rapidjson::Document& doc)
        :IqrfGwMcuMsg(doc)
      {
        m_strDateTime = Pointer("/data/req/dateTime").Get(doc)->GetString();
      }

      virtual ~IqrfGwMcuMsgSetRTC()
      {
      }

      void createResponsePayload(rapidjson::Document& doc) override
      {
        Pointer("/data/rsp/dateTime").Set(doc, m_strDateTime);
        IqrfGwMcuMsg::createResponsePayload(doc);
      }

      void handleMsg(JsonOffGridCoreMcuApi::Imp* imp) override
      {
        TRC_FUNCTION_ENTER("");

        IqrfGwMcuMsg::handleMsg(imp);

        if (m_command == "rtc") {

          std::replace(m_strDateTime.begin(), m_strDateTime.end(), 'T', ' ');
          std::istringstream is(m_strDateTime);
          std::string strDate, strTime;
          try {
            is >> strDate >> strDate;
          }
          catch (std::exception &e) {
            THROW_EXC_TRC_WAR(std::logic_error, "Bad format: " << PAR(m_strDateTime));
          }

          imp->m_iOffGridCoreMcu->setRtcTimeCmd(strTime);
          if (getVerbose()) {
            m_rawVect.push_back(imp->m_iOffGridCoreMcu->getLastRaw());
          }

          imp->m_iOffGridCoreMcu->setRtcDateCmd(strTime);
          if (getVerbose()) {
            m_rawVect.push_back(imp->m_iOffGridCoreMcu->getLastRaw());
          }
        }
        else {
          THROW_EXC_TRC_WAR(std::logic_error, "Unknown command: " << PAR(m_command));
        }

        TRC_FUNCTION_LEAVE("");
      }

    private:
      std::string m_strDateTime;
    };

  public:
    //////////////////////////////
    // Services
    shape::IConfigurationService* m_iConfigurationService = nullptr;
    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    IOffGridCoreMcu* m_iOffGridCoreMcu = nullptr;
    shape::ILaunchService *m_iLaunchService = nullptr;

    ObjectFactory<IqrfGwMcuMsg, rapidjson::Document&> m_objectFactory;

    // Instance name
    std::string m_instanceName;

    // msgs to handle
    std::vector<std::string> m_filters =
    {
      "iqrfGwMcu_"
    };


  public:
    Imp()
    {
      m_objectFactory.registerClass<IqrfGwMcuMsgSetTimer>(mType_SetTimer);
      m_objectFactory.registerClass<IqrfGwMcuMsgGetTimer>(mType_GetTimer);
      m_objectFactory.registerClass<IqrfGwMcuMsgSetRTC>(mType_SetRTC);
      m_objectFactory.registerClass<IqrfGwMcuMsgGetRTC>(mType_GetRTC);
    }

    ~Imp() {}

    //--------------------------
    // Process incomming request
    //--------------------------
    void handleMsg(const std::string & messagingId, const IMessagingSplitterService::MsgType & msgType, rapidjson::Document reqDoc)
    {
      TRC_FUNCTION_ENTER(PAR(messagingId) << NAME_PAR(mType, msgType.m_type) <<
        NAME_PAR(major, msgType.m_major) << NAME_PAR(minor, msgType.m_minor) << NAME_PAR(micro, msgType.m_micro));

      std::unique_ptr<IqrfGwMcuMsg> msg = m_objectFactory.createObject(msgType.m_type, reqDoc);

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

    void activate(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl
        << "******************************" << std::endl
        << "JsonOffGridCoreMcuApi instance activate" << std::endl
        << "******************************");
      modify(props);

      //if (m_runDaliThreadAtStartup == true)
      //{
      //  if (m_daliThread.joinable())
      //    m_daliThread.join();

      //  m_daliThreadRun = true;
      //  m_daliThread = std::thread([&]() { daliThreadFunc(); });
      //}

      m_iMessagingSplitterService->registerFilteredMsgHandler(
        m_filters,
        [&](const std::string & messagingId, const IMessagingSplitterService::MsgType & msgType, rapidjson::Document doc)
        {
          handleMsg(messagingId, msgType, std::move(doc));
        });

      TRC_FUNCTION_LEAVE("")
    }

    void deactivate()
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl
        << "******************************" << std::endl
        << "JsonOffGridCoreMcuApi instance deactivate" << std::endl
        << "******************************");
      //m_daliThreadRun = false;
      //if (m_daliThread.joinable())
      //  m_daliThread.join();

      m_iMessagingSplitterService->unregisterFilteredMsgHandler(m_filters);

      TRC_FUNCTION_LEAVE("")
    }

    void modify(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");

      const Document& doc = props->getAsJson();
      m_instanceName = Pointer("/instance").Get(doc)->GetString();
      const Value* val = Pointer("/runDaliThreadAtStartup").Get(doc);
      //if (val && val->IsBool())
      //  m_runDaliThreadAtStartup = val->GetBool();

      //val = Pointer("/readDaliStatusPeriod").Get(doc);
      //if (val && val->IsInt())
      //  m_readDaliStatusPeriod = val->GetInt();

      //val = Pointer("/conductDaliFunctionalTestPeriod").Get(doc);
      //if (val && val->IsInt())
      //  m_conductDaliFunctionalTestPeriod = val->GetInt();

      //val = Pointer("/conductDaliFunctionalTestHour").Get(doc);
      //if (val && val->IsInt())
      //  m_conductDaliFunctionalTestHour = val->GetInt();

      //val = Pointer("/conductDaliDurationTestDay").Get(doc);
      //if (val && val->IsInt())
      //  m_conductDaliDurationTestDay = val->GetInt();

      //val = Pointer("/conductDaliDurationTestMonth").Get(doc);
      //if (val && val->IsInt())
      //  m_conductDaliDurationTestMonth = val->GetInt();

      //val = Pointer("/conductDaliDurationTestHour").Get(doc);
      //if (val && val->IsInt())
      //  m_conductDaliDurationTestHour = val->GetInt();

      //val = Pointer("/debugMode").Get(doc);
      //if (val && val->IsBool())
      //  m_debugMode = val->GetBool();

      TRC_FUNCTION_LEAVE("")
    }

    void attachInterface(shape::ILaunchService *iface)
    {
      m_iLaunchService = iface;
    }

    void detachInterface(shape::ILaunchService *iface)
    {
      if (m_iLaunchService == iface)
      {
        m_iLaunchService = nullptr;
      }
    }

    void attachInterface(IOffGridCoreMcu *iface)
    {
      m_iOffGridCoreMcu = iface;
    }

    void detachInterface(IOffGridCoreMcu *iface)
    {
      if (m_iOffGridCoreMcu == iface)
      {
        m_iOffGridCoreMcu = nullptr;
      }
    }

    void attachInterface(IMessagingSplitterService *iface)
    {
      m_iMessagingSplitterService = iface;
    }

    void detachInterface(IMessagingSplitterService *iface)
    {
      if (m_iMessagingSplitterService == iface)
      {
        m_iMessagingSplitterService = nullptr;
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

  };

  JsonOffGridCoreMcuApi::JsonOffGridCoreMcuApi()
  {
    m_imp = shape_new Imp();
  }

  JsonOffGridCoreMcuApi::~JsonOffGridCoreMcuApi()
  {
    delete m_imp;
  }

  void JsonOffGridCoreMcuApi::attachInterface(shape::ILaunchService *iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsonOffGridCoreMcuApi::detachInterface(shape::ILaunchService *iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsonOffGridCoreMcuApi::attachInterface(iqrf::IOffGridCoreMcu *iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsonOffGridCoreMcuApi::detachInterface(iqrf::IOffGridCoreMcu *iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsonOffGridCoreMcuApi::attachInterface(iqrf::IMessagingSplitterService *iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsonOffGridCoreMcuApi::detachInterface(iqrf::IMessagingSplitterService *iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsonOffGridCoreMcuApi::attachInterface(shape::IConfigurationService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsonOffGridCoreMcuApi::detachInterface(shape::IConfigurationService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsonOffGridCoreMcuApi::attachInterface(shape::ITraceService *iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void JsonOffGridCoreMcuApi::detachInterface(shape::ITraceService *iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

  void JsonOffGridCoreMcuApi::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void JsonOffGridCoreMcuApi::deactivate()
  {
    m_imp->deactivate();
  }

  void JsonOffGridCoreMcuApi::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
  }
} // namespace iqrf
