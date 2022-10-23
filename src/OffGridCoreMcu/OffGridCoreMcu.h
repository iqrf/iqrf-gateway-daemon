#pragma once

#include "IOffGridCoreMcu.h"
#include "IIqrfChannelService.h"
#include "ICommandService.h"
#include "ShapeProperties.h"
#include "ITraceService.h"

namespace iqrf {

  class OffGridCoreMcu : public IOffGridCoreMcu
  {
  public:
    OffGridCoreMcu();
    virtual ~OffGridCoreMcu();

    std::string testCom(const std::string & dotstr) override;
   
    Raw getLastRaw() override;
    void setPwrOffTimeCmd(const std::string & timeStr) override;
    void setWakeUpTimeCmd(const std::string & timeStr) override;
    std::string getPwrOffTimeCmd() override;
    std::string getWakeUpTimeCmd() override;
    void setRtcTimeCmd(const std::string & timeStr) override;
    void setRtcDateCmd(const std::string & dateStr) override;
    std::string getRtcTimeCmd() override;
    std::string getRtcDateCmd() override;
    
    float getVoltageCmd() override;
    float getCurrentCmd() override;
    float getPowerCmd() override;
    float getTemperatureCmd() override;
    int getRepCapCmd() override;
    int getRepSocCmd() override;
    int getTteCmd() override;
    int getTtfCmd() override;

    void setLteOnCmd() override;
    void setLteOffCmd() override;
    bool getLteStateCmd() override;
    void setLoraOnCmd() override;
    void setLoraOffCmd() override;
    bool getLoraStateCmd() override;

    std::string sendLoraAtCmd(const std::string& at) override;
    std::string recieveLoraAtCmd() override;

    std::string getMcuVersionCmd() override;

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(iqrf::IIqrfChannelService* iface);
    void detachInterface(iqrf::IIqrfChannelService* iface);

    void attachInterface(shape::ICommandService* iface);
    void detachInterface(shape::ICommandService* iface);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

    class Imp;
    Imp *m_imp;
  };

}
