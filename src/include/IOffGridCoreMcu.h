#pragma once

#include <string>
#include <vector>
#include <chrono>

namespace iqrf {
  class IOffGridCoreMcu
  {
  public:
    class Raw {
    public:
      std::string request;
      std::chrono::system_clock::time_point requestTs;
      std::string response;
      std::chrono::system_clock::time_point responseTs;
      std::vector<uint8_t> recBuffer;
    };

    virtual ~IOffGridCoreMcu() {};
    
    virtual std::string testCom(const std::string & dotstr) = 0;

    // Gets and invalidate last raw data info. If called without previous command it throws error.
    virtual Raw getLastRaw() = 0;

    // timeStr: "hh:mm:ss"
    virtual void setPwrOffTimeCmd(const std::string & timeStr) = 0;
    // timeStr: "hh:mm:ss"
    virtual void setWakeUpTimeCmd(const std::string & timeStr) = 0;

    // returns: "hh:mm:ss"
    virtual std::string getPwrOffTimeCmd() = 0;
    // returns: "hh:mm:ss"
    virtual std::string getWakeUpTimeCmd() = 0;
   
    // sets time part of RTC from time string "hh:mm:ss"
    virtual void setRtcTimeCmd(const std::string & timeStr) = 0;
    // sets date part of RTC from date string "YYYY:DD:MM"
    virtual void setRtcDateCmd(const std::string & dateStr) = 0;
    
    // returns: "hh:mm:ss"
    virtual std::string getRtcTimeCmd() = 0;
    // returns: "YYYY:DD:MM" - weak day is not considered
    virtual std::string getRtcDateCmd() = 0;

    virtual float getVoltageCmd() = 0;
    virtual float getCurrentCmd() = 0;
    virtual float getPowerCmd() = 0;
    virtual float getTemperatureCmd() = 0;
    virtual int getRepCapCmd() = 0;
    virtual int getRepSocCmd() = 0;
    virtual int getTteCmd() = 0;
    virtual int getTtfCmd() = 0;

  };
}
