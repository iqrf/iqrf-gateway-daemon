/**
 * IQRF Gateway Daemon
 * Copyright (C) 2015-2025 IQRF Tech s.r.o., 2019-2025 MICRORISC s.r.o.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define IMessagingService_EXPORTS

#include "SchedulerMessaging.h"
#include "rapidjson/pointer.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "Trace.h"
#include <vector>

#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

#include "iqrf__SchedulerMessaging.hxx"

TRC_INIT_MODULE(iqrf::SchedulerMessaging)

const unsigned IQRF_MQ_BUFFER_SIZE = 64 * 1024;

namespace iqrf {

  ////////////////////////////////////
  // class SchedulerMessaging::Imp
  ////////////////////////////////////
  class SchedulerMessaging::Imp
  {
  private:
    MessagingInstance m_messagingInstance = MessagingInstance(MessagingType::SCHEDULER);

    //IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    ISchedulerService* m_iSchedulerService = nullptr;
    IMessagingService::MessageHandlerFunc m_messageHandlerFunc;

    void handleTaskObject(const rapidjson::Value & task)
    {
      using namespace rapidjson;

      const Value *messagingVal = Pointer("/messaging").Get(task);
      if (!messagingVal || !messagingVal->IsArray()) {
        THROW_EXC_TRC_WAR(std::logic_error, "Invalid messaging property value.");
      }
      std::list<MessagingInstance> messagingList;

      const auto arr = messagingVal->GetArray();
      for (auto itr = arr.Begin(); itr != arr.End(); ++itr) {
        auto type = Pointer("/type").Get(*itr)->GetString();
        auto instance = Pointer("/instance").Get(*itr)->GetString();
        messagingList.push_back(
          MessagingInstance(
            type,
            instance
          )
        );
      }

      const Value *messageVal = Pointer("/message").Get(task);
      if (messageVal && messageVal->IsObject()) {
        rapidjson::Document doc;
        doc.CopyFrom(*messageVal, doc.GetAllocator());
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);
        std::string msgStr = buffer.GetString();
        std::vector<uint8_t> msgVect((uint8_t*)msgStr.data(), (uint8_t*)msgStr.data() + msgStr.size());
        if (m_messageHandlerFunc) {
          for (auto messaging : messagingList) {
            m_messageHandlerFunc(messaging, msgVect);
          }
        }
      }
      else {
        THROW_EXC_TRC_WAR(std::logic_error, "Expected object: /message")
      }
    }

    void handleTaskFromScheduler(const rapidjson::Value & task)
    {
      using namespace rapidjson;

      if (task.IsObject()) {
        handleTaskObject(task);
      }
      else if (task.IsArray()) {
        for (auto it = task.Begin(); it != task.End(); it++) {
          handleTaskObject(*it);
        }
      }
      else {
        THROW_EXC_TRC_WAR(std::logic_error, "Unexpected type: /task")
      }
    }

  public:
    Imp()
    {
    }

    ~Imp()
    {
    }

    void registerMessageHandler(MessageHandlerFunc hndl)
    {
      TRC_FUNCTION_ENTER("");
      m_messageHandlerFunc = hndl;
      TRC_FUNCTION_LEAVE("")
    }

    void unregisterMessageHandler()
    {
      TRC_FUNCTION_ENTER("");
      m_messageHandlerFunc = IMessagingService::MessageHandlerFunc();
      TRC_FUNCTION_LEAVE("")
    }

    void sendMessage(const std::basic_string<uint8_t> & msg)
    {
      (void)msg; //silence -Wunused-parameter
      TRC_FUNCTION_ENTER("");
      TRC_WARNING("This function shouldn't be called");
      TRC_FUNCTION_LEAVE("")
    }

    const MessagingInstance &getMessagingInstance() const
    {
      return m_messagingInstance;
    }

    void activate(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "SchedulerMessaging instance activate" << std::endl <<
        "******************************"
      );

      std::string instanceName;
      props->getMemberAsString("instance", instanceName);
      m_messagingInstance.instance = instanceName;

      m_iSchedulerService->registerTaskHandler(m_messagingInstance.instance, [&](const rapidjson::Value & task) {
        handleTaskFromScheduler(task);
      });

      TRC_FUNCTION_LEAVE("")
    }

    void deactivate()
    {
      TRC_FUNCTION_ENTER("");

      m_iSchedulerService->unregisterTaskHandler(m_messagingInstance.instance);

      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "SchedulerMessaging instance deactivate" << std::endl <<
        "******************************"
      );
      TRC_FUNCTION_LEAVE("")
    }

    void modify(const shape::Properties *props)
    {
      (void)props; //silence -Wunused-parameter
    }

    void attachInterface(ISchedulerService* iface)
    {
      m_iSchedulerService = iface;
    }

    void detachInterface(ISchedulerService* iface)
    {
      if (m_iSchedulerService == iface) {
        m_iSchedulerService = nullptr;
      }
    }

  };

  ////////////////////////////////////
  // class SchedulerMessaging
  ////////////////////////////////////
  SchedulerMessaging::SchedulerMessaging()
  {
    m_imp = shape_new Imp();
  }

  SchedulerMessaging::~SchedulerMessaging()
  {
    delete m_imp;
  }

  void SchedulerMessaging::registerMessageHandler(MessageHandlerFunc hndl)
  {
    m_imp->registerMessageHandler(hndl);
  }

  void SchedulerMessaging::unregisterMessageHandler()
  {
    m_imp->unregisterMessageHandler();
  }

  void SchedulerMessaging::sendMessage(const MessagingInstance& messagingId, const std::basic_string<uint8_t> & msg)
  {
    (void)messagingId; //silence -Wunused-parameter
    m_imp->sendMessage(msg);
  }

  const MessagingInstance &SchedulerMessaging::getMessagingInstance() const
  {
    return m_imp->getMessagingInstance();
  }

  //////////////////////////////////////
  void SchedulerMessaging::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void SchedulerMessaging::deactivate()
  {
    m_imp->deactivate();
  }

  void SchedulerMessaging::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
  }

  void SchedulerMessaging::attachInterface(ISchedulerService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void SchedulerMessaging::detachInterface(ISchedulerService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void SchedulerMessaging::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void SchedulerMessaging::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

}
