/**
 * Copyright 2015-2025 IQRF Tech s.r.o.
 * Copyright 2019-2025 MICRORISC s.r.o.
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

#define IMessagingService_EXPORTS
#define ITestSimulationMessaging_EXPORTS

#include "TestSimulationMessaging.h"
#include "Trace.h"
#include "HexStringCoversion.h"

#include "rapidjson/pointer.h"

#include <queue>
#include <condition_variable>

#include "iqrf__TestSimulationMessaging.hxx"

TRC_INIT_MNAME(iqrf::TestSimulationMessaging)

using namespace std;

namespace iqrf {

  class TestSimulationMessaging::Imp {
  private:
    bool m_acceptAsyncMsg = false;
    MessagingInstance m_messagingInstance = MessagingInstance(MessagingType::TEST);
    MessageHandlerFunc m_hndl;
    std::queue<std::pair<std::string, std::string>> m_outgoingMsgQueue;
    std::mutex  m_queueMux;
    std::condition_variable m_cv;

  public:

    Imp()
    {
    }

    ~Imp()
    {
    }

    /////////////////////////////////////
    //from iqrf::IMessagingService
    void registerMessageHandler(MessageHandlerFunc hndl)
    {
      TRC_FUNCTION_ENTER("");
      m_hndl = hndl;
      TRC_FUNCTION_LEAVE("")
    }

    void unregisterMessageHandler()
    {
      TRC_FUNCTION_ENTER("");
      m_hndl = nullptr;
      TRC_FUNCTION_LEAVE("")
    }

    void sendMessage(const MessagingInstance& messaging, const std::basic_string<uint8_t> & msg)
    {
      TRC_FUNCTION_ENTER("");
      {
        std::unique_lock<std::mutex> lck(m_queueMux);
        m_outgoingMsgQueue.push(std::make_pair(messaging.instance, std::string((char*)msg.data(), msg.size())));
      }
      m_cv.notify_one();
      TRC_FUNCTION_LEAVE("")
    }

    bool acceptAsyncMsg() const {
      return m_acceptAsyncMsg;
    }

    const MessagingInstance &getMessagingInstance() const
    {
      return m_messagingInstance;
    }

    /////////////////////////////////////
    //from iqrf::ITestSimulationMessaging
    void pushIncomingMessage(const std::string& msg)
    {
      TRC_FUNCTION_ENTER(PAR(msg));
      if (m_hndl) {
        m_hndl(m_messagingInstance, std::vector<uint8_t>((uint8_t*)msg.data(), (uint8_t*)msg.data() + msg.size()));
      }
      else {
        TRC_WARNING("message handler is not registered");
      }
      TRC_FUNCTION_LEAVE("")
    }

    std::string popOutgoingMessage(unsigned millisToWait)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_lock<std::mutex> lck(m_queueMux);
      std::string retval;
      if (m_outgoingMsgQueue.empty()) {
        while (m_cv.wait_for(lck, std::chrono::milliseconds(millisToWait)) != std::cv_status::timeout) {
          if (!m_outgoingMsgQueue.empty()) break;
        }
      }

      if (!m_outgoingMsgQueue.empty()) {
        retval = m_outgoingMsgQueue.front().second;
        m_outgoingMsgQueue.pop();
      }
      TRC_FUNCTION_LEAVE(PAR(retval));
      return retval;
    }

    void activate(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "TestSimulationMessaging instance activate" << std::endl <<
        "******************************"
      );

      const rapidjson::Value* val = rapidjson::Pointer("/instance").Get(props->getAsJson());
      m_messagingInstance.instance = val->GetString();

      TRC_FUNCTION_LEAVE("")
    }

    void deactivate()
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "TestSimulationMessaging instance deactivate" << std::endl <<
        "******************************"
      );
      TRC_FUNCTION_LEAVE("")
    }


  };

  ////////////////////////////////////
  TestSimulationMessaging::TestSimulationMessaging()
  {
    m_imp = shape_new Imp();
  }

  TestSimulationMessaging::~TestSimulationMessaging()
  {
    delete m_imp;
  }

  void TestSimulationMessaging::registerMessageHandler(MessageHandlerFunc hndl)
  {
    m_imp->registerMessageHandler(hndl);
  }

  void TestSimulationMessaging::unregisterMessageHandler()
  {
    m_imp->unregisterMessageHandler();
  }

  void TestSimulationMessaging::sendMessage(const MessagingInstance& messaging, const std::basic_string<uint8_t> & msg)
  {
    m_imp->sendMessage(messaging, msg);
  }

  bool TestSimulationMessaging::acceptAsyncMsg() const {
    return m_imp->acceptAsyncMsg();
  }

  const iqrf::MessagingInstance &TestSimulationMessaging::getMessagingInstance() const
  {
    return m_imp->getMessagingInstance();
  }

  void TestSimulationMessaging::pushIncomingMessage(const std::string& msg)
  {
    m_imp->pushIncomingMessage(msg);
  }

  std::string TestSimulationMessaging::popOutgoingMessage(unsigned millisToWait)
  {
    return m_imp->popOutgoingMessage(millisToWait);
  }

  /////////////////////////
  void TestSimulationMessaging::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void TestSimulationMessaging::deactivate()
  {
    m_imp->deactivate();
  }

  void TestSimulationMessaging::modify(const shape::Properties *props)
  {
    (void)props; //silence -Wunused-parameter
  }

  void TestSimulationMessaging::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void TestSimulationMessaging::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

}
