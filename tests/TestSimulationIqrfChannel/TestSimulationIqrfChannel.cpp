#define ITestSimulationIqrfChannel_EXPORTS
#define IIqrfChannelService_EXPORTS

#include "TestSimulationIqrfChannel.h"
#include "DotMsg.h"
#include "AccessControl.h"
#include "Trace.h"
#include "HexStringCoversion.h"

#include <queue>
#include <condition_variable>

#include "iqrf__TestSimulationIqrfChannel.hxx"

TRC_INIT_MNAME(iqrf::TestSimulationIqrfChannel)

using namespace std;

namespace iqrf {

  class TestSimulationIqrfChannel::Imp {
  
  private:
    std::queue<std::string> m_incomingMsgQueue;
    std::mutex  m_queueMux;
    std::condition_variable m_cv;
    
    AccessControl<Imp> m_accessControl;

  public:

    Imp()
      :m_accessControl(this)
    {
    }

    ~Imp()
    {
    }

    //iqrf::ITestSimulationIqrfChannel
    /////////////////////////////////////
    void pushOutgoingMessage(const std::string& msg, unsigned millisToDelay)
    {
      TRC_FUNCTION_ENTER("");
      std::this_thread::sleep_for(std::chrono::milliseconds(millisToDelay));
      m_accessControl.messageHandler(DotMsg(msg));
      TRC_FUNCTION_LEAVE("")
    }

    std::string popIncomingMessage(unsigned millisToWait)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_lock<std::mutex> lck(m_queueMux);
      std::string retval;
      if (m_incomingMsgQueue.empty()) {
        while (m_cv.wait_for(lck, std::chrono::milliseconds(millisToWait)) != std::cv_status::timeout) {
          if (!m_incomingMsgQueue.empty()) break;
        }
      }

      if (!m_incomingMsgQueue.empty()) {
        retval = m_incomingMsgQueue.front();
        m_incomingMsgQueue.pop();
      }
      TRC_FUNCTION_LEAVE(PAR(retval));
      return retval;
    }

    //iqrf::IIqrfChannelService
    /////////////////////////////////////
    IIqrfChannelService::State getState() const
    {
      TRC_FUNCTION_ENTER("");
      TRC_FUNCTION_LEAVE("")
      return IIqrfChannelService::State::Ready;
    }

    std::unique_ptr<IIqrfChannelService::Accessor> getAccess(IIqrfChannelService::ReceiveFromFunc receiveFromFunc, IIqrfChannelService::AccesType access)
    {
      auto retval = m_accessControl.getAccess(receiveFromFunc, access);
      
      //IqrfDpa has registered - fire test
      m_cv.notify_all();

      return retval;
    }

    bool hasExclusiveAccess() const
    {
      TRC_FUNCTION_ENTER("");
      TRC_FUNCTION_LEAVE("")
      return false;
    }

    //Accessor
    ///////////////////////////////////
    void send(const std::basic_string<unsigned char>& message)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_lock<std::mutex> lck(m_queueMux);
      m_incomingMsgQueue.push(std::string((char*)message.data(), message.size()));
      m_cv.notify_one();

      TRC_FUNCTION_LEAVE("");
    }

    bool enterProgrammingState()
    {
      TRC_FUNCTION_ENTER("");
      TRC_FUNCTION_LEAVE("");
      return true;
    }

    IIqrfChannelService::Accessor::UploadErrorCode
      upload(
        const IIqrfChannelService::Accessor::UploadTarget target,
        const std::basic_string<uint8_t>& data,
        const uint16_t address
      )
    {
      // write data to TR module
      TRC_FUNCTION_ENTER("");
      TRC_FUNCTION_LEAVE("");
      return IIqrfChannelService::Accessor::UploadErrorCode::UPLOAD_NO_ERROR;
    }

    bool terminateProgrammingState() {
      TRC_FUNCTION_ENTER("");
      TRC_FUNCTION_LEAVE("");
      return true;
    }

    void activate(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "TestSimulationIqrfChannel instance activate" << std::endl <<
        "******************************"
      );

      TRC_FUNCTION_LEAVE("")
    }

    void deactivate()
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "TestSimulationIqrfChannel instance deactivate" << std::endl <<
        "******************************"
      );
      TRC_FUNCTION_LEAVE("")
    }

  };

  ////////////////////////////////////
  TestSimulationIqrfChannel::TestSimulationIqrfChannel()
  {
    m_imp = shape_new Imp();
  }

  TestSimulationIqrfChannel::~TestSimulationIqrfChannel()
  {
    delete m_imp;
  }

  //iqrf::ITestSimulationIqrfChannel
  /////////////////////////////////////
  void TestSimulationIqrfChannel::pushOutgoingMessage(const std::string& msg, unsigned millisToDelay)
  {
    m_imp->pushOutgoingMessage(msg, millisToDelay);
  }

  std::string TestSimulationIqrfChannel::popIncomingMessage(unsigned millisToWait)
  {
    return m_imp->popIncomingMessage(millisToWait);
  }

  //iqrf::IIqrfChannelService
  /////////////////////////////////////
  IIqrfChannelService::State TestSimulationIqrfChannel::getState() const
  {
    return m_imp->getState();
  }

  std::unique_ptr<IIqrfChannelService::Accessor> TestSimulationIqrfChannel::getAccess(ReceiveFromFunc receiveFromFunc, AccesType access)
  {
    return m_imp->getAccess(receiveFromFunc, access);
  }

  bool TestSimulationIqrfChannel::hasExclusiveAccess() const
  {
    return m_imp->hasExclusiveAccess();
  }

  /////////////////////////
  void TestSimulationIqrfChannel::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void TestSimulationIqrfChannel::deactivate()
  {
    m_imp->deactivate();
  }

  void TestSimulationIqrfChannel::modify(const shape::Properties *props)
  {
  }

  void TestSimulationIqrfChannel::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void TestSimulationIqrfChannel::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

}
