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
#include "Trace.h"
#include "HexStringCoversion.h"
#include <chrono>
#include <vector>
#include <sstream>
#include <algorithm>

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
    const std::string mType_GetPower = "iqrfGwMcu_GetPower";
    const std::string mType_LoraSend = "iqrfGwMcu_LoraSend";
    const std::string mType_LoraReceive = "iqrfGwMcu_LoraReceive";

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
          std::string strDateTime(m_strDateTime);
          std::replace(strDateTime.begin(), strDateTime.end(), 'T', ' ');
          std::istringstream is(strDateTime);
          std::string strDate, strTime;
          try {
            is >> strDate >> strTime;
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

    //////////////////////////////////////////////
    class IqrfGwMcuMsgGetCharger : public IqrfGwMcuMsg
    {
    public:
      IqrfGwMcuMsgGetCharger() = delete;
      IqrfGwMcuMsgGetCharger(const rapidjson::Document& doc)
        :IqrfGwMcuMsg(doc)
      {
      }

      virtual ~IqrfGwMcuMsgGetCharger()
      {
      }

      void createResponsePayload(rapidjson::Document& doc) override
      {
        Pointer("/data/rsp/voltage").Set(doc, m_voltage);
        Pointer("/data/rsp/current").Set(doc, m_current);
        Pointer("/data/rsp/power").Set(doc, m_power);
        Pointer("/data/rsp/temperature").Set(doc, m_temperature);
        Pointer("/data/rsp/repCap").Set(doc, m_repCap);
        Pointer("/data/rsp/repSOC").Set(doc, m_repSOC);
        Pointer("/data/rsp/tte").Set(doc, m_tte);
        Pointer("/data/rsp/ttf").Set(doc, m_ttf);
        Pointer("/data/rsp/mcuVersion").Set(doc, m_mcuVersion);

        IqrfGwMcuMsg::createResponsePayload(doc);
      }

      void handleMsg(JsonOffGridCoreMcuApi::Imp* imp) override
      {
        TRC_FUNCTION_ENTER("");

        IqrfGwMcuMsg::handleMsg(imp);

        if (m_command == "charger") {
          m_voltage = imp->m_iOffGridCoreMcu->getVoltageCmd();
          if (getVerbose()) {
            m_rawVect.push_back(imp->m_iOffGridCoreMcu->getLastRaw());
          }

          m_current = imp->m_iOffGridCoreMcu->getCurrentCmd();
          if (getVerbose()) {
            m_rawVect.push_back(imp->m_iOffGridCoreMcu->getLastRaw());
          }

          m_power = imp->m_iOffGridCoreMcu->getPowerCmd();
          if (getVerbose()) {
            m_rawVect.push_back(imp->m_iOffGridCoreMcu->getLastRaw());
          }

          m_temperature = imp->m_iOffGridCoreMcu->getTemperatureCmd();
          if (getVerbose()) {
            m_rawVect.push_back(imp->m_iOffGridCoreMcu->getLastRaw());
          }

          m_repCap = imp->m_iOffGridCoreMcu->getRepCapCmd();
          if (getVerbose()) {
            m_rawVect.push_back(imp->m_iOffGridCoreMcu->getLastRaw());
          }

          m_repSOC = imp->m_iOffGridCoreMcu->getRepSocCmd();
          if (getVerbose()) {
            m_rawVect.push_back(imp->m_iOffGridCoreMcu->getLastRaw());
          }

          m_tte = imp->m_iOffGridCoreMcu->getTteCmd();
          if (getVerbose()) {
            m_rawVect.push_back(imp->m_iOffGridCoreMcu->getLastRaw());
          }

          m_ttf = imp->m_iOffGridCoreMcu->getTtfCmd();
          if (getVerbose()) {
            m_rawVect.push_back(imp->m_iOffGridCoreMcu->getLastRaw());
          }

          m_mcuVersion = imp->m_iOffGridCoreMcu->getMcuVersionCmd();
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
      float m_voltage;
      float m_current;
      float m_power;
      float m_temperature;
      float m_repCap;
      float m_repSOC;
      float m_tte;
      float m_ttf;
      std::string m_mcuVersion;
    };

    //////////////////////////////////////////////
    // base for Get/Set
    class IqrfGwMcuMsgPower : public IqrfGwMcuMsg
    {
    public:
      IqrfGwMcuMsgPower() = delete;
      IqrfGwMcuMsgPower(const rapidjson::Document& doc)
        :IqrfGwMcuMsg(doc)
      {
      }

      virtual ~IqrfGwMcuMsgPower()
      {
      }

      void createResponsePayload(rapidjson::Document& doc) override
      {
        if (m_lte >= 0) {
          Pointer("/data/rsp/lte").Set(doc, (bool)(m_lte > 0 ? true : false));
        }
        if (m_lora >= 0) {
          Pointer("/data/rsp/lora").Set(doc, (bool)(m_lora > 0 ? true : false));
        }

        IqrfGwMcuMsg::createResponsePayload(doc);
      }

      void handleMsg(JsonOffGridCoreMcuApi::Imp* imp) override
      {
        TRC_FUNCTION_ENTER("");

        IqrfGwMcuMsg::handleMsg(imp);
        
        if (m_command != "power") {
          THROW_EXC_TRC_WAR(std::logic_error, "Unknown command: " << PAR(m_command));
        }
      }

    protected:
      // 3-state: invalid = -1, 0 = false, 1 = true
      int m_lte = -1;
      int m_lora = -1;
    };

    //////////////////////////////////////////////
    class IqrfGwMcuMsgGetPower : public IqrfGwMcuMsgPower
    {
    public:
      IqrfGwMcuMsgGetPower() = delete;
      IqrfGwMcuMsgGetPower(const rapidjson::Document& doc)
        :IqrfGwMcuMsgPower(doc)
      {
      }

      virtual ~IqrfGwMcuMsgGetPower()
      {
      }

      void handleMsg(JsonOffGridCoreMcuApi::Imp* imp) override
      {
        TRC_FUNCTION_ENTER("");

        IqrfGwMcuMsgPower::handleMsg(imp);

        m_lte = imp->m_iOffGridCoreMcu->getLteStateCmd();
        if (getVerbose()) {
          m_rawVect.push_back(imp->m_iOffGridCoreMcu->getLastRaw());
        }
        
        m_lora = imp->m_iOffGridCoreMcu->getLoraStateCmd();
        if (getVerbose()) {
          m_rawVect.push_back(imp->m_iOffGridCoreMcu->getLastRaw());
        }

        TRC_FUNCTION_LEAVE("");
      }

    };

    //////////////////////////////////////////////
    class IqrfGwMcuMsgSetPower : public IqrfGwMcuMsgPower
    {
    public:
      IqrfGwMcuMsgSetPower() = delete;
      IqrfGwMcuMsgSetPower(const rapidjson::Document& doc)
        :IqrfGwMcuMsgPower(doc)
      {
        {
          const rapidjson::Value *val = rapidjson::Pointer("/data/req/lte").Get(doc);
          if (val && val->IsBool()) {
            m_lte = val->GetBool() ? 1 : 0;
          }
        }
        {
          const rapidjson::Value *val = rapidjson::Pointer("/data/req/lora").Get(doc);
          if (val && val->IsBool()) {
            m_lora = val->GetBool() ? 1 : 0;
          }
        }
      }

      virtual ~IqrfGwMcuMsgSetPower()
      {
      }

      void handleMsg(JsonOffGridCoreMcuApi::Imp* imp) override
      {
        TRC_FUNCTION_ENTER("");

        IqrfGwMcuMsgPower::handleMsg(imp);

        if (m_lte == 1) {
          imp->m_iOffGridCoreMcu->setLteOnCmd();
          if (getVerbose()) {
            m_rawVect.push_back(imp->m_iOffGridCoreMcu->getLastRaw());
          }
        }
        else if (m_lte == 0) {
          imp->m_iOffGridCoreMcu->setLteOffCmd();
          if (getVerbose()) {
            m_rawVect.push_back(imp->m_iOffGridCoreMcu->getLastRaw());
          }
        }

        if (m_lora == 1) {
          imp->m_iOffGridCoreMcu->setLoraOnCmd();
          if (getVerbose()) {
            m_rawVect.push_back(imp->m_iOffGridCoreMcu->getLastRaw());
          }
        }
        else if (m_lora == 0) {
          imp->m_iOffGridCoreMcu->setLoraOffCmd();
          if (getVerbose()) {
            m_rawVect.push_back(imp->m_iOffGridCoreMcu->getLastRaw());
          }
        }

        TRC_FUNCTION_LEAVE("");
      }

    };

    //////////////////////////////////////////////
    class IqrfGwMcuMsgLoraSend : public IqrfGwMcuMsg
    {
    public:
      IqrfGwMcuMsgLoraSend() = delete;
      IqrfGwMcuMsgLoraSend(const rapidjson::Document& doc)
        :IqrfGwMcuMsg(doc)
      {
        const rapidjson::Value* val = rapidjson::Pointer("/data/req/data").Get(doc);
        if (val && val->IsString()) {
          m_data = val->GetString();
        }
      }

      virtual ~IqrfGwMcuMsgLoraSend()
      {
      }

      void createResponsePayload(rapidjson::Document& doc) override
      {
        Pointer("/data/rsp/data").Set(doc, m_data);
        IqrfGwMcuMsg::createResponsePayload(doc);
      }

      void handleMsg(JsonOffGridCoreMcuApi::Imp* imp) override
      {
        TRC_FUNCTION_ENTER("");

        IqrfGwMcuMsg::handleMsg(imp);

        if (m_command == "lora") {
          m_data = imp->m_iOffGridCoreMcu->sendLoraAtCmd(m_data);
          if (getVerbose()) {
            m_rawVect.push_back(imp->m_iOffGridCoreMcu->getLastRaw());
          }
        }

        TRC_FUNCTION_LEAVE("");
      }

    protected:
      std::string m_data;
    };

    //////////////////////////////////////////////
    class IqrfGwMcuMsgLoraReceive : public IqrfGwMcuMsg
    {
    public:
      IqrfGwMcuMsgLoraReceive() = delete;
      IqrfGwMcuMsgLoraReceive(const rapidjson::Document& doc)
        :IqrfGwMcuMsg(doc)
      {
      }

      virtual ~IqrfGwMcuMsgLoraReceive()
      {
      }

      void createResponsePayload(rapidjson::Document& doc) override
      {
        Pointer("/data/rsp/data").Set(doc, m_data);
        IqrfGwMcuMsg::createResponsePayload(doc);
      }

      void handleMsg(JsonOffGridCoreMcuApi::Imp* imp) override
      {
        TRC_FUNCTION_ENTER("");

        IqrfGwMcuMsg::handleMsg(imp);

        if (m_command == "lora") {
          m_data = imp->m_iOffGridCoreMcu->recieveLoraAtCmd();
          if (getVerbose()) {
            m_rawVect.push_back(imp->m_iOffGridCoreMcu->getLastRaw());
          }
        }

        TRC_FUNCTION_LEAVE("");
      }

    protected:
      std::string m_data;
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
      m_objectFactory.registerClass<IqrfGwMcuMsgGetCharger>(mType_GetCharger);
      m_objectFactory.registerClass<IqrfGwMcuMsgSetPower>(mType_SetPower);
      m_objectFactory.registerClass<IqrfGwMcuMsgGetPower>(mType_GetPower);
      m_objectFactory.registerClass<IqrfGwMcuMsgLoraSend>(mType_LoraSend);
      m_objectFactory.registerClass<IqrfGwMcuMsgLoraReceive>(mType_LoraReceive);
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

      m_iMessagingSplitterService->unregisterFilteredMsgHandler(m_filters);

      TRC_FUNCTION_LEAVE("")
    }

    void modify(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");

      const Document& doc = props->getAsJson();
      m_instanceName = Pointer("/instance").Get(doc)->GetString();

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
