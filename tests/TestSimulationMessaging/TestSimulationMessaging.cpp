#define IMessagingService_EXPORTS
#define ITestSimulationMessaging_EXPORTS

#include "TestSimulationMessaging.h"
#include "Trace.h"
#include "HexStringCoversion.h"

#include "rapidjson/pointer.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"

#include <queue>
#include <condition_variable>

//#include <iostream>
//#include <fstream>
//#include <iomanip>
//#include <thread>
//#include <condition_variable>
//#include <algorithm>

#include "iqrf__TestSimulationMessaging.hxx"

TRC_INIT_MNAME(iqrf::TestSimulationMessaging)

using namespace std;

namespace iqrf {

  const std::string ATTACH_IqrfDpa("ATTACH IqrfDpa");
  
  //aux class to convert from dot notation to ustring and back
  class DotMsg
  {
  public:
    DotMsg(std::basic_string<unsigned char> msg)
      :m_msg(msg)
    {}

    DotMsg(std::string dotMsg)
    {
      if (!dotMsg.empty()) {
        std::string buf = dotMsg;
        std::replace(buf.begin(), buf.end(), '.', ' ');

        std::istringstream istr(buf);

        int val;
        while (true) {
          if (!(istr >> std::hex >> val)) {
            if (istr.eof()) break;
            THROW_EXC_TRC_WAR(std::logic_error, "Unexpected format: " << PAR(dotMsg));
          }
          m_msg.push_back((uint8_t)val);
        }
      }
    }

    operator std::basic_string<unsigned char>() { return m_msg; }

    operator std::string()
    {
      std::string to;
      if (!m_msg.empty()) {
        std::ostringstream ostr;
        ostr.setf(std::ios::hex, std::ios::basefield);
        ostr.fill('0');
        long i = 0;
        for (uint8_t c : m_msg) {
          ostr << std::setw(2) << (short int)c;
          ostr << '.';
        }
        to = ostr.str();
        to.pop_back();
      }
      return to;
    }

  private:
    std::basic_string<unsigned char> m_msg;
  };

  /////////////////////////
  class TestSimulationMessaging::Imp {
  private:
    std::string m_name;
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

    void sendMessage(const std::string& messagingId, const std::basic_string<uint8_t> & msg)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_lock<std::mutex> lck(m_queueMux);
      m_outgoingMsgQueue.push(std::make_pair(messagingId, std::string((char*)msg.data(), msg.size())));
      TRC_FUNCTION_LEAVE("")
    }

    const std::string & getName() const
    {
      TRC_FUNCTION_ENTER("");
      TRC_FUNCTION_LEAVE(PAR(m_name));
      return m_name;
    }

    bool acceptAsyncMsg() const
    {
      TRC_FUNCTION_ENTER("");
      TRC_FUNCTION_LEAVE("");
      return true;
    }

    /////////////////////////////////////
    //from iqrf::ITestSimulationMessaging
    void pushIncomingMessage(const std::string& msg)
    {
      TRC_FUNCTION_ENTER(PAR(msg));
      if (m_hndl) {
        m_hndl(m_name, std::vector<uint8_t>((uint8_t*)msg.data(), (uint8_t*)msg.data() + msg.size()));
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
      m_name = val->GetString();

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

  void TestSimulationMessaging::sendMessage(const std::string& messagingId, const std::basic_string<uint8_t> & msg)
  {
    m_imp->sendMessage(messagingId, msg);
  }

  const std::string & TestSimulationMessaging::getName() const
  {
    return m_imp->getName();
  }

  bool TestSimulationMessaging::acceptAsyncMsg() const
  {
    return m_imp->acceptAsyncMsg();
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
