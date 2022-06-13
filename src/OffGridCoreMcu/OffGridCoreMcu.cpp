#include "OffGridCoreMcu.h"
#include "Trace.h"
#include "HexStringCoversion.h"
#include "DataTypes.h"

#include "rapidjson/pointer.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"

#include <iostream>
#include <atomic>
#include <functional>
#include <thread>
#include <condition_variable>
#include <cmath>
#include <future>

#include "iqrf__OffGridCoreMcu.hxx"

#ifdef SHAPE_PLATFORM_WINDOWS
#define OFFGRIDMCU_TEST
#endif

TRC_INIT_MNAME(iqrf::OffGridCoreMcu)

namespace iqrf {

  class OffGridCoreMcu::Imp
  {
  public:
    class TestCommCommand : public shape::ICommand
    {
    public:
      TestCommCommand() = delete;
      TestCommCommand(OffGridCoreMcu::Imp* imp)
        :m_imp(imp)
      {
      }

      void usage(std::ostringstream& ostr)
      {
        ostr <<
          std::left << std::setw(20) << "ts h" << "test comm help" << std::endl <<
          std::left << std::setw(20) << "ts r" << "get last raw communication" << std::endl <<
          std::left << std::setw(20) << "ts d <string>" << "send request and gets response" << std::endl <<
          std::left << std::setw(20) << "ts gRTCt" << "get RTC time" << std::endl <<
          std::left << std::setw(20) << "ts sRTCt <hh:mm:ss>" << "set RTC time" << std::endl <<
          std::left << std::setw(20) << "ts gRTCd" << "get RTC date" << std::endl <<
          std::left << std::setw(20) << "ts sRTCd <YYYY-MM-DD>" << "set RTC date" << std::endl <<
          std::left << std::setw(20) << "ts gPWOFFt" << "get power off time" << std::endl <<
          std::left << std::setw(20) << "ts sPWOFFt <hh:mm:ss>" << "set power off time" << std::endl <<
          std::left << std::setw(20) << "ts gWKUPt" << "get wakeup time" << std::endl <<
          std::left << std::setw(20) << "ts sWKUPt <hh:mm:ss>" << "set wakeup time" << std::endl <<
          std::left << std::setw(20) << "ts gVTG" << "get voltage" << std::endl <<
          std::left << std::setw(20) << "ts gCUR" << "get current" << std::endl <<
          std::left << std::setw(20) << "ts gPWR" << "get power" << std::endl <<
          std::left << std::setw(20) << "ts gTMP" << "get temperature" << std::endl <<
          std::left << std::setw(20) << "ts gRCAP" << "get RepCap" << std::endl <<
          std::left << std::setw(20) << "ts gRSOC" << "get RepSoc" << std::endl <<
          std::left << std::setw(20) << "ts gTTE" << "get Tte" << std::endl <<
          std::left << std::setw(20) << "ts gTTF" << "get Ttf" << std::endl <<
          std::left << std::setw(20) << "ts gVER" << "get ver" << std::endl <<
          std::endl;
      }

      std::string doCmd(const std::string& params) override
      {
        TRC_FUNCTION_ENTER("");
        std::ostringstream ostr;

        try {
          std::string cmd;
          std::string subcmd;
          std::istringstream istr(params);

          istr >> cmd >> subcmd;

          TRC_DEBUG("process: " << PAR(subcmd));

          //////////////
          if (subcmd == "h") {
            usage(ostr);
          }
          //////////////
          else if (subcmd == "r") {
            traceLastRaw(ostr);
          }
          else if (subcmd == "d") {
            std::string dotstr;
            istr >> dotstr;
            std::string res = m_imp->testCom(dotstr);
            traceLastRaw(ostr);
          }
          else if (subcmd == "gRTCt") {
            std::string ret = m_imp->getRtcTimeCmd();
            ostr << "getRtcTime: " << ret << std::endl;
            traceLastRaw(ostr);
          }
          else if (subcmd == "sRTCt") {
            std::string timeStr;
            istr >> timeStr;
            m_imp->setRtcTimeCmd(timeStr);
            ostr << "setRtcTime: " << timeStr << std::endl;
            traceLastRaw(ostr);
          }
          else if (subcmd == "gRTCd") {
            std::string ret = m_imp->getRtcDateCmd();
            ostr << "getRtcDate: " << ret << std::endl;
            traceLastRaw(ostr);
          }
          else if (subcmd == "sRTCd") {
            std::string dateStr;
            istr >> dateStr;
            m_imp->setRtcDateCmd(dateStr);
            ostr << "setRtcDate: " << dateStr << std::endl;
            traceLastRaw(ostr);
          }
          else if (subcmd == "gPWOFFt") {
            std::string ret = m_imp->getPwrOffTimeCmd();
            ostr << "getPwrOffTime: " << ret << std::endl;
            traceLastRaw(ostr);
          }
          else if (subcmd == "sPWOFFt") {
            std::string timeStr;
            istr >> timeStr;
            m_imp->setPwrOffTimeCmd(timeStr);
            ostr << "setPwrOffTime: " << timeStr << std::endl;
            traceLastRaw(ostr);
          }
          else if (subcmd == "gWKUPt") {
            std::string ret = m_imp->getWakeUpTimeCmd();
            ostr << "getWakeUpTime: " << ret << std::endl;
            traceLastRaw(ostr);
          }
          else if (subcmd == "sWKUPt") {
            std::string timeStr;
            istr >> timeStr;
            m_imp->setWakeUpTimeCmd(timeStr);
            ostr << "setWakeUpTimeCmd: " << timeStr << std::endl;
            traceLastRaw(ostr);
          }
          else if (subcmd == "gVTG") {
            float ret = m_imp->getVoltageCmd();
            ostr << "getVoltageCmd: " << ret << std::endl;
            traceLastRaw(ostr);
          }
          else if (subcmd == "gCUR") {
            float ret = m_imp->getCurrentCmd();
            ostr << "getCurrentCmd: " << ret << std::endl;
            traceLastRaw(ostr);
          }
          else if (subcmd == "gPWR") {
            float ret = m_imp->getPowerCmd();
            ostr << "getPowerCmd: " << ret << std::endl;
            traceLastRaw(ostr);
          }
          else if (subcmd == "gTMP") {
            float ret = m_imp->getTemperatureCmd();
            ostr << "getTemperatureCmd: " << ret << std::endl;
            traceLastRaw(ostr);
          }
          else if (subcmd == "gRCAP") {
            int ret = m_imp->getRepCapCmd();
            ostr << "getRepCapCmd: " << ret << std::endl;
            traceLastRaw(ostr);
          }
          else if (subcmd == "gRSOC") {
            int ret = m_imp->getRepSocCmd();
            ostr << "getRepSocCmd: " << ret << std::endl;
            traceLastRaw(ostr);
          }
          else if (subcmd == "gTTE") {
            int ret = m_imp->getTteCmd();
            ostr << "getTteCmd: " << ret << std::endl;
            traceLastRaw(ostr);
          }
          else if (subcmd == "gTTF") {
            int ret = m_imp->getTtfCmd();
            ostr << "getTtfCmd: " << ret << std::endl;
            traceLastRaw(ostr);
          }
          else if (subcmd == "gVER") {
          std::string ret = m_imp->getMcuVersionCmd();
          ostr << "getVersionCmd: " << ret << std::endl;
          traceLastRaw(ostr);
          }
          else {
            ostr << "usage: " << std::endl;
            usage(ostr);
          }
        }
        catch (std::exception& e) {
          ostr << e.what();
        }

        TRC_FUNCTION_LEAVE("");
        return ostr.str();
      }

      std::string getHelp() override
      {
        return "Test comm simulation. Type h for help";
      }

      ~TestCommCommand() {}
    private:
      OffGridCoreMcu::Imp* m_imp = nullptr;
      
      std::ostringstream & traceLastRaw(std::ostringstream & os) {
        auto raw = m_imp->getLastRaw();
        os << "raw: "
          << std::endl << raw.request
          << std::endl << encodeTimestamp(raw.requestTs)
          << std::endl << raw.response
          << std::endl << encodeTimestamp(raw.responseTs)
          ;
        return os;
      }
    };

    iqrf::IIqrfChannelService* m_iIqrfChannelService = nullptr;
    shape::ICommandService* m_iCommandService = nullptr;

    std::string m_testComMsg = "22.21";

    std::condition_variable m_recCond;
    std::mutex m_recMux;
    std::vector<uint8_t> m_recVect;
    std::vector<uint8_t> m_recFakeVect;
    IOffGridCoreMcu::Raw m_raw;

  public:
    Imp()
    {
    }

    ~Imp()
    {
    }

    void sendAndWaitForResponse(const std::vector<uint8_t> & request)
    {
      // on response handler invoked async when response is received
      auto onResponse = [&](const std::basic_string<unsigned char> & bmsg)
      {
        std::unique_lock<std::mutex> lck(m_recMux);
        m_recVect = std::vector<uint8_t>(bmsg.begin(), bmsg.end());
        m_recCond.notify_one();
        return 0;
      };

      std::unique_lock<std::mutex> lck(m_recMux);

      std::basic_string<unsigned char> brequest(std::basic_string<unsigned char>(request.begin(), request.end()));
      m_raw.request = iqrf::DotMsg(brequest);
      m_raw.requestTs = std::chrono::system_clock::now();
      m_recVect.clear();
      m_raw.response.clear();
      m_raw.responseTs = std::chrono::system_clock::time_point();
      TRC_INFORMATION(">>>>>>>>>>>>>>>>>>" << std::endl <<
        "Send: " << m_raw.request);

#ifdef OFFGRIDMCU_TEST
      m_recVect = m_recFakeVect;
#else
      auto accessor = m_iIqrfChannelService->getAccess(onResponse, iqrf::IIqrfChannelService::AccesType::Normal);
      accessor->send(brequest);

      if (!m_recCond.wait_for(lck, std::chrono::milliseconds(1000), [this] {return !m_recVect.empty(); })) {
        THROW_EXC_TRC_WAR(std::logic_error, "!!!!!!!!!!! No response");
      }
#endif
      m_raw.recBuffer = m_recVect;

      m_raw.responseTs = std::chrono::system_clock::now();
      m_raw.response = iqrf::DotMsg(std::basic_string<unsigned char>(m_recVect.begin(), m_recVect.end()));
      TRC_INFORMATION("<<<<<<<<<<<<<<<<<<" << std::endl <<
        "Receive: " << m_raw.response);
    }

    std::string testCom(const std::string & dotstr)
    {
      TRC_FUNCTION_ENTER("");

      std::string retval;

      std::vector<uint8_t> request = iqrf::DotMsg(dotstr);
      sendAndWaitForResponse(request);

      TRC_FUNCTION_LEAVE("");
      return retval;
    }

    IOffGridCoreMcu::Raw getLastRaw()
    {
      TRC_FUNCTION_ENTER("");
      std::unique_lock<std::mutex> lck(m_recMux);
      TRC_FUNCTION_LEAVE("");
      return m_raw;
    }

    //////////////////////////
    // Timer
    //////////////////////////
    void setPwrOffTimeCmd(const std::string & timeStr)
    {
      TRC_FUNCTION_ENTER(PAR(timeStr));
      offgrid::SetPwrOffTimeCmd cmd;
      cmd.setTime(timeStr);
      
#ifdef OFFGRIDMCU_TEST
      m_recFakeVect = iqrf::DotMsg("81.01.05.00");
#endif
      sendAndWaitForResponse(cmd.encodeRequest());
      
      cmd.parseResponse(getLastRaw().recBuffer);
      TRC_FUNCTION_LEAVE("");
    }

    void setWakeUpTimeCmd(const std::string & timeStr)
    {
      TRC_FUNCTION_ENTER(PAR(timeStr));
      offgrid::SetWakeUpTimeCmd cmd;
      cmd.setTime(timeStr);
      
#ifdef OFFGRIDMCU_TEST
      m_recFakeVect = iqrf::DotMsg("81.02.05.00");
#endif
      sendAndWaitForResponse(cmd.encodeRequest());

      cmd.parseResponse(getLastRaw().recBuffer);
      TRC_FUNCTION_LEAVE("");
    }

    std::string getPwrOffTimeCmd()
    {
      TRC_FUNCTION_ENTER("");
      std::string retval;

      offgrid::GetPwrOffTimeCmd cmd;

#ifdef OFFGRIDMCU_TEST
      m_recFakeVect = iqrf::DotMsg("81.03.0B.00.09.09.16.07.09.00");
#endif
      sendAndWaitForResponse(cmd.encodeRequest());

      cmd.parseResponse(getLastRaw().recBuffer);
      retval = cmd.getDate() + "T" + cmd.getTime();

      TRC_FUNCTION_LEAVE(PAR(retval));
      return retval;
    }

    std::string getWakeUpTimeCmd()
    {
      TRC_FUNCTION_ENTER("");
      std::string retval;

      offgrid::GetWakeUpTimeCmd cmd;

#ifdef OFFGRIDMCU_TEST
      m_recFakeVect = iqrf::DotMsg("81.04.0B.00.09.09.16.09.09.00");
#endif
      sendAndWaitForResponse(cmd.encodeRequest());

      cmd.parseResponse(getLastRaw().recBuffer);
      retval = cmd.getDate() + "T" + cmd.getTime();

      TRC_FUNCTION_LEAVE(PAR(retval));
      return retval;
    }

    //////////////////////////
    // RTC
    //////////////////////////
    void setRtcTimeCmd(const std::string & timeStr)
    {
      //TRC_FUNCTION_ENTER(NAME_PAR(tp, encodeTimestamp(tp)));
      TRC_FUNCTION_ENTER(PAR(timeStr));
      offgrid::SetRTCTimeCmd cmd;
      cmd.setTime(timeStr);
      
#ifdef OFFGRIDMCU_TEST
      m_recFakeVect = iqrf::DotMsg("82.01.05.00");
#endif
      sendAndWaitForResponse(cmd.encodeRequest());
      
      cmd.parseResponse(getLastRaw().recBuffer);
      TRC_FUNCTION_LEAVE("");
    }

    void setRtcDateCmd(const std::string & dateStr)
    {
      //TRC_FUNCTION_ENTER(NAME_PAR(tp, encodeTimestamp(tp)));
      TRC_FUNCTION_ENTER(PAR(dateStr));

      offgrid::SetRTCDateCmd cmd;
      cmd.setDate(dateStr);

#ifdef OFFGRIDMCU_TEST
      m_recFakeVect = iqrf::DotMsg("82.02.05.00");
#endif
      sendAndWaitForResponse(cmd.encodeRequest());

      cmd.parseResponse(getLastRaw().recBuffer);
      TRC_FUNCTION_LEAVE("");
    }

    std::string getRtcTimeCmd()
    {
      TRC_FUNCTION_ENTER("");
      std::string retval;

      offgrid::GetRTCTimeCmd cmd;

#ifdef OFFGRIDMCU_TEST
      m_recFakeVect = iqrf::DotMsg("82.03.08.00.08.07.06");
#endif
      sendAndWaitForResponse(cmd.encodeRequest());

      cmd.parseResponse(getLastRaw().recBuffer);
      retval = cmd.getTime();

      TRC_FUNCTION_LEAVE(PAR(retval));
      return retval;
    }

    std::string getRtcDateCmd()
    {
      TRC_FUNCTION_ENTER("");
      std::string retval;

      offgrid::GetRTCDateCmd cmd;

#ifdef OFFGRIDMCU_TEST
      m_recFakeVect = iqrf::DotMsg("82.04.09.00.0A.06.16.05");
#endif
      sendAndWaitForResponse(cmd.encodeRequest());

      cmd.parseResponse(getLastRaw().recBuffer);
      retval = cmd.getDate();

      TRC_FUNCTION_LEAVE(PAR(retval));
      return retval;
    }

    //////////////////////////
    // Charger
    //////////////////////////
    float getVoltageCmd()
    {
      TRC_FUNCTION_ENTER("");
      float retval;
      offgrid::GetVoltageCmd cmd;

#ifdef OFFGRIDMCU_TEST
      m_recFakeVect = iqrf::DotMsg("83.01.07.00.01.02");
#endif
      sendAndWaitForResponse(cmd.encodeRequest());

      cmd.parseResponse(getLastRaw().recBuffer);
      retval = cmd.getVoltage();
      TRC_FUNCTION_LEAVE(PAR(retval));
      return retval;
    }

    float getCurrentCmd()
    {
      TRC_FUNCTION_ENTER("");
      float retval;
      offgrid::GetCurrentCmd cmd;

#ifdef OFFGRIDMCU_TEST
      m_recFakeVect = iqrf::DotMsg("83.02.07.00.02.03");
#endif
      sendAndWaitForResponse(cmd.encodeRequest());

      cmd.parseResponse(getLastRaw().recBuffer);
      retval = cmd.getCurrent();
      TRC_FUNCTION_LEAVE(PAR(retval));
      return retval;
    }

    float getPowerCmd()
    {
      TRC_FUNCTION_ENTER("");
      float retval;
      offgrid::GetPowerCmd cmd;

#ifdef OFFGRIDMCU_TEST
      m_recFakeVect = iqrf::DotMsg("83.03.07.00.03.04");
#endif
      sendAndWaitForResponse(cmd.encodeRequest());

      cmd.parseResponse(getLastRaw().recBuffer);
      retval = cmd.getPower();
      TRC_FUNCTION_LEAVE(PAR(retval));
      return retval;
    }

    float getTemperatureCmd()
    {
      TRC_FUNCTION_ENTER("");
      float retval;
      offgrid::GetTemperatureCmd cmd;

#ifdef OFFGRIDMCU_TEST
      m_recFakeVect = iqrf::DotMsg("83.04.07.00.04.05");
#endif
      sendAndWaitForResponse(cmd.encodeRequest());

      cmd.parseResponse(getLastRaw().recBuffer);
      retval = cmd.getTemperature();
      TRC_FUNCTION_LEAVE(PAR(retval));
      return retval;
    }
    int getRepCapCmd()
    {
      TRC_FUNCTION_ENTER("");
      int retval;
      offgrid::GetRepCapCmd cmd;

#ifdef OFFGRIDMCU_TEST
      m_recFakeVect = iqrf::DotMsg("83.05.07.00.05.06");
#endif
      sendAndWaitForResponse(cmd.encodeRequest());

      cmd.parseResponse(getLastRaw().recBuffer);
      retval = cmd.getRepCap();
      TRC_FUNCTION_LEAVE(PAR(retval));
      return retval;
    }

    int getRepSocCmd()
    {
      TRC_FUNCTION_ENTER("");
      int retval;
      offgrid::GetRepSocCmd cmd;

#ifdef OFFGRIDMCU_TEST
      m_recFakeVect = iqrf::DotMsg("83.06.07.00.06.07");
#endif
      sendAndWaitForResponse(cmd.encodeRequest());

      cmd.parseResponse(getLastRaw().recBuffer);
      retval = cmd.getRepSoc();
      TRC_FUNCTION_LEAVE(PAR(retval));
      return retval;
    }

    int getTteCmd()
    {
      TRC_FUNCTION_ENTER("");
      int retval;
      offgrid::GetTteCmd cmd;

#ifdef OFFGRIDMCU_TEST
      m_recFakeVect = iqrf::DotMsg("83.07.07.00.06.07");
#endif
      sendAndWaitForResponse(cmd.encodeRequest());

      cmd.parseResponse(getLastRaw().recBuffer);
      retval = cmd.getSeconds();
      TRC_FUNCTION_LEAVE(PAR(retval));
      return retval;
    }

    int getTtfCmd()
    {
      TRC_FUNCTION_ENTER("");
      int retval;
      offgrid::GetTtfCmd cmd;

#ifdef OFFGRIDMCU_TEST
      m_recFakeVect = iqrf::DotMsg("83.08.07.00.07.08");
#endif
      sendAndWaitForResponse(cmd.encodeRequest());

      cmd.parseResponse(getLastRaw().recBuffer);
      retval = cmd.getSeconds();
      TRC_FUNCTION_LEAVE(PAR(retval));
      return retval;
    }

    //////////////////////////
    // Devices Power control
    //////////////////////////
    void setLteOnCmd()
    {
      TRC_FUNCTION_ENTER("");
      int retval;
      offgrid::SetLteOnCmd cmd;

#ifdef OFFGRIDMCU_TEST
      m_recFakeVect = iqrf::DotMsg("84.01.05.00");
#endif
      sendAndWaitForResponse(cmd.encodeRequest());

      cmd.parseResponse(getLastRaw().recBuffer);
      TRC_FUNCTION_LEAVE("");
    }

    void setLteOffCmd()
    {
      TRC_FUNCTION_ENTER("");
      int retval;
      offgrid::SetLteOffCmd cmd;

#ifdef OFFGRIDMCU_TEST
      m_recFakeVect = iqrf::DotMsg("84.02.05.00");
#endif
      sendAndWaitForResponse(cmd.encodeRequest());

      cmd.parseResponse(getLastRaw().recBuffer);
      TRC_FUNCTION_LEAVE("");
    }

    bool getLteStateCmd()
    {
      TRC_FUNCTION_ENTER("");
      bool retval;
      offgrid::GetLteStateCmd cmd;

#ifdef OFFGRIDMCU_TEST
      m_recFakeVect = iqrf::DotMsg("84.03.06.00.01");
//#endif
      //TODO not implemented by MCU yet => it may differ
      sendAndWaitForResponse(cmd.encodeRequest());

      cmd.parseResponse(getLastRaw().recBuffer);

      retval = cmd.getState();
#else
      retval = true;
#endif
      TRC_FUNCTION_LEAVE(PAR(retval));
      return retval;
    }

    void setLoraOnCmd()
    {
      TRC_FUNCTION_ENTER("");
      int retval;
      offgrid::SetLoraOnCmd cmd;

#ifdef OFFGRIDMCU_TEST
      m_recFakeVect = iqrf::DotMsg("84.04.05.00");
//#endif
      //TODO not implemented by MCU yet => it may differ
      sendAndWaitForResponse(cmd.encodeRequest());

      cmd.parseResponse(getLastRaw().recBuffer);
#else
#endif
      TRC_FUNCTION_LEAVE("");
    }

    void setLoraOffCmd()
    {
      TRC_FUNCTION_ENTER("");
      int retval;
      offgrid::SetLoraOffCmd cmd;

#ifdef OFFGRIDMCU_TEST
      m_recFakeVect = iqrf::DotMsg("84.05.05.00");
//#endif
      //TODO not implemented by MCU yet => it may differ
      sendAndWaitForResponse(cmd.encodeRequest());
#else
#endif
      cmd.parseResponse(getLastRaw().recBuffer);
      TRC_FUNCTION_LEAVE("");
    }

    bool getLoraStateCmd()
    {
      TRC_FUNCTION_ENTER("");
      int retval;
      offgrid::GetLteStateCmd cmd;

#ifdef OFFGRIDMCU_TEST
      m_recFakeVect = iqrf::DotMsg("84.06.06.00.01");
//#endif
      //sendAndWaitForResponse(cmd.encodeRequest());

      cmd.parseResponse(getLastRaw().recBuffer);
      retval = cmd.getState();
#else
      retval = true;
#endif
      TRC_FUNCTION_LEAVE(PAR(retval));
      return retval;
    }

    //////////////////////////
    // Other
    //////////////////////////
    std::string getMcuVersionCmd()
    {
      TRC_FUNCTION_ENTER("");
      std::string retval;
      offgrid::GetVerCmd cmd;

#ifdef OFFGRIDMCU_TEST
      m_recFakeVect = iqrf::DotMsg("A0.01.07.00.04.01");
#endif
      sendAndWaitForResponse(cmd.encodeRequest());

      cmd.parseResponse(getLastRaw().recBuffer);
      retval = cmd.getVersion();
      TRC_FUNCTION_LEAVE(PAR(retval));
      return retval;
    }

    void activate(const shape::Properties *props)
    {
      (void)props; //silence -Wunused-parameter
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "OffGridCoreMcu instance activate" << std::endl <<
        "******************************"
      );

      using namespace rapidjson;

      const Document& doc = props->getAsJson();

      m_iIqrfChannelService->startListen();

      TRC_FUNCTION_LEAVE("")
    }

    void deactivate()
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "OffGridCoreMcu instance deactivate" << std::endl <<
        "******************************"
      );

      TRC_FUNCTION_LEAVE("")
    }

    void attachInterface(iqrf::IIqrfChannelService* iface)
    {
      m_iIqrfChannelService = iface;
    }

    void detachInterface(iqrf::IIqrfChannelService* iface)
    {
      if (m_iIqrfChannelService == iface) {
        m_iIqrfChannelService = nullptr;
      }
    }

    void attachInterface(shape::ICommandService* iface)
    {
      m_iCommandService = iface;
      m_iCommandService->addCommand("ts", std::shared_ptr<shape::ICommand>(shape_new TestCommCommand(this)));
    }

    void detachInterface(shape::ICommandService* iface)
    {
      if (m_iCommandService == iface) {
        m_iCommandService->removeCommand("ts");
        m_iCommandService = nullptr;
      }
    }

  };

  ////////////////////////////////////
  OffGridCoreMcu::OffGridCoreMcu()
  {
    m_imp = shape_new Imp();
  }

  OffGridCoreMcu::~OffGridCoreMcu()
  {
    delete m_imp;
  }

  std::string OffGridCoreMcu::testCom(const std::string & dotstr)
  {
    return m_imp->testCom(dotstr);
  }

  IOffGridCoreMcu::Raw OffGridCoreMcu::getLastRaw()
  {
    return m_imp->getLastRaw();
  }

  void OffGridCoreMcu::setPwrOffTimeCmd(const std::string & timeStr)
  {
    m_imp->setPwrOffTimeCmd(timeStr);
  }

  void OffGridCoreMcu::setWakeUpTimeCmd(const std::string & timeStr)
  {
    return m_imp->setWakeUpTimeCmd(timeStr);
  }

  std::string OffGridCoreMcu::getPwrOffTimeCmd()
  {
    return m_imp->getPwrOffTimeCmd();
  }

  std::string OffGridCoreMcu::getWakeUpTimeCmd()
  {
    return m_imp->getWakeUpTimeCmd();
  }

  void OffGridCoreMcu::setRtcTimeCmd(const std::string & timeStr)
  {
    m_imp->setRtcTimeCmd(timeStr);
  }

  void OffGridCoreMcu::setRtcDateCmd(const std::string & dateStr)
  {
    m_imp->setRtcDateCmd(dateStr);
  }

  std::string OffGridCoreMcu::getRtcTimeCmd()
  {
    return m_imp->getRtcTimeCmd();
  }

  std::string OffGridCoreMcu::getRtcDateCmd()
  {
    return m_imp->getRtcDateCmd();
  }

  float OffGridCoreMcu::getVoltageCmd()
  {
    return m_imp->getVoltageCmd();
  }

  float OffGridCoreMcu::getCurrentCmd()
  {
    return m_imp->getCurrentCmd();
  }

  float OffGridCoreMcu::getPowerCmd()
  {
    return m_imp->getPowerCmd();
  }

  float OffGridCoreMcu::getTemperatureCmd()
  {
    return m_imp->getTemperatureCmd();
  }

  int OffGridCoreMcu::getRepCapCmd()
  {
    return m_imp->getRepCapCmd();
  }

  int OffGridCoreMcu::getRepSocCmd()
  {
    return m_imp->getRepSocCmd();
  }

  int OffGridCoreMcu::getTteCmd()
  {
    return m_imp->getTteCmd();
  }

  int OffGridCoreMcu::getTtfCmd()
  {
    return m_imp->getTtfCmd();
  }

  void OffGridCoreMcu::setLteOnCmd()
  {
    m_imp->setLteOnCmd();
  }

  void OffGridCoreMcu::setLteOffCmd()
  {
    m_imp->setLteOffCmd();
  }

  bool OffGridCoreMcu::getLteStateCmd()
  {
    return m_imp->getLteStateCmd();
  }

  void OffGridCoreMcu::setLoraOnCmd()
  {
    m_imp->setLoraOnCmd();
  }

  void OffGridCoreMcu::setLoraOffCmd()
  {
    m_imp->setLoraOffCmd();
  }

  bool OffGridCoreMcu::getLoraStateCmd()
  {
    return m_imp->getLoraStateCmd();
  }

  std::string OffGridCoreMcu::getMcuVersionCmd()
  {
    return m_imp->getMcuVersionCmd();
  }

  /////////////////////////
  void OffGridCoreMcu::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void OffGridCoreMcu::deactivate()
  {
    m_imp->deactivate();
  }

  void OffGridCoreMcu::modify(const shape::Properties *props)
  {
    (void)props; //silence -Wunused-parameter
  }

  void OffGridCoreMcu::attachInterface(iqrf::IIqrfChannelService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void OffGridCoreMcu::detachInterface(iqrf::IIqrfChannelService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void OffGridCoreMcu::attachInterface(shape::ICommandService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void OffGridCoreMcu::detachInterface(shape::ICommandService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void OffGridCoreMcu::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void OffGridCoreMcu::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

}
