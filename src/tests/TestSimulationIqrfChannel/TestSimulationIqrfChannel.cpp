#define ITestSimulationIqrfChannel_EXPORTS
#define IIqrfChannelService_EXPORTS

#include "TestSimulationIqrfChannel.h"
#include "AccessControl.h"
#include "Trace.h"
#include "HexStringCoversion.h"

#include <queue>
#include <condition_variable>
#include <thread>

#include "iqrf__TestSimulationIqrfChannel.hxx"

TRC_INIT_MNAME(iqrf::TestSimulationIqrfChannel)

using namespace std;

namespace iqrf {

  class TestSimulationIqrfChannel::Imp {
  
  private:
    std::queue<std::string> m_incomingMsgQueue;
    std::mutex  m_queueMux;
    std::condition_variable m_cv;
    std::thread m_thd;
    
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
      if (millisToDelay > 0)
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

    void startListen()
    {
    }

    IIqrfChannelService::State getState() const
    {
      TRC_FUNCTION_ENTER("");
      TRC_FUNCTION_LEAVE("")
      return IIqrfChannelService::State::Ready;
    }

    std::unique_ptr<IIqrfChannelService::Accessor> getAccess(IIqrfChannelService::ReceiveFromFunc receiveFromFunc, IIqrfChannelService::AccesType access)
    {
      auto retval = m_accessControl.getAccess(receiveFromFunc, access);
      
      //simulate IqrfDpa activate procedure
      static bool simulateReset = true;
      if (simulateReset) {
        simulateReset = false;
        m_thd = std::thread([&]() {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
          //simulate send async response after TR reset
          //pushOutgoingMessage("00.00.ff.3f.00.00.00.00.28.02.00.fd.26.00.00.00.00.00.00.01", 0);
          pushOutgoingMessage("00.00.ff.3f.00.00.80.00.02.03.00.fd.26.00.00.00.00.00.00.01", 0);

          //simulate send OS read transaction handling
          //get OS read request
          std::string osRead = popIncomingMessage(1000);
          if (osRead == "00.00.02.00.ff.ff") {
            //OS read response
            //pushOutgoingMessage("00.00.02.80.00.00.00.00.8a.52.00.81.38.24.79.08.00.28.00.c0", 20);
            pushOutgoingMessage("00.00.02.80.00.00.00.00.8a.52.00.81.38.24.B8.08.00.28.00.c0", 20);
          }
        });
      }
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
      m_incomingMsgQueue.push(DotMsg(message));
      m_cv.notify_one();

      TRC_FUNCTION_LEAVE("");
    }

    bool enterProgrammingState()
    {
      TRC_FUNCTION_ENTER("");
      TRC_FUNCTION_LEAVE("");
      return true;
    }

    IIqrfChannelService::UploadErrorCode
      upload(
        const IIqrfChannelService::UploadTarget target,
        const std::basic_string<uint8_t>& data,
        const uint16_t address
      )
    {
      // write data to TR module
      (void)target; //silence -Wunused-parameter
      (void)data; //silence -Wunused-parameter
      (void)address; //silence -Wunused-parameter

      TRC_FUNCTION_ENTER("");
      TRC_FUNCTION_LEAVE("");
      return IIqrfChannelService::UploadErrorCode::UPLOAD_NO_ERROR;
    }

    bool terminateProgrammingState() {
      TRC_FUNCTION_ENTER("");
      TRC_FUNCTION_LEAVE("");
      return true;
    }

    IIqrfChannelService::osInfo getTrModuleInfo()
    {
      IIqrfChannelService::osInfo inf;
      return inf;
    }

    void activate(const shape::Properties *props)
    {
      (void)props; //silence -Wunused-parameter
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

      if (m_thd.joinable()) {
        m_thd.join();
      }
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
  void TestSimulationIqrfChannel::startListen()
  {
    return m_imp->startListen();
  }

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
    (void)props; //silence -Wunused-parameter
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
