#pragma once

#include "ComBase.h"
#include <list>

namespace iqrf {
  class ComMngIqmeshWriteConfig : public ComBase
  {
  public:
    ComMngIqmeshWriteConfig() = delete;
    ComMngIqmeshWriteConfig(rapidjson::Document& doc)
      :ComBase(doc)
    {
      parse(doc);
    }

    virtual ~ComMngIqmeshWriteConfig()
    {
    }

    bool isSetRestart() {
      return m_isSetRestart;
    }

    const bool getRestart() const
    {
      return m_restart;
    }

    bool isSetRepeat() {
      return m_isSetRepeat;
    }

    const int getRepeat() const
    {
      return m_repeat;
    }

    // returns addresses of devices, to which write configuration in
    const std::list<int> getDeviceAddr() const
    {
      return m_deviceAddr;
    }

    bool isSetRfChannelA() {
      return m_isSetRfChannelA;
    }

    const int getRfChannelA() {
      return m_rfChannelA;
    }

    const int getRfChannelB() {
      return m_rfChannelB;
    }

    bool isSetRfChannelB() {
      return m_isSetRfChannelB;
    }

    bool isSetRfSubChannelA() {
      return m_isSetRfSubChannelA;
    }

    const int getRfSubChannelA() {
      return m_rfSubChannelA;
    }

    bool isSetRfSubChannelB() {
      return m_isSetRfSubChannelB;
    }

    const int getRfSubChannelB() {
      return m_rfSubChannelB;
    }

    bool isSetTxPower() {
      return m_isSetTxPower;
    }

    const int getTxPower() {
      return m_txPower;
    }

    bool isSetRxFilter() {
      return m_isSetRxFilter;
    }

    const int getRxFilter() {
      return m_rxFilter;
    }

    bool isSetLpRxTimeout() {
      return m_isSetLpRxTimeout;
    }

    const int getLpRxTimeout() {
      return m_lpRxTimeout;
    }

    bool isSetRfPgmAltChannel() {
      return m_isSetRfPgmAltChannel;
    }

    const int getRfPgmAltChannel() {
      return m_rfPgmAltChannel;
    }

    bool isSetUartBaudrate() {
      return m_isSetUartBaudrate;
    }

    const int getUartBaudrate() {
      return m_uartBaudrate;
    }

    bool isSetRfPgmEnableAfterReset() {
      return m_isSetRfPgmEnableAfterReset;
    }

    const bool getRfPgmEnableAfterReset() {
      return m_rfPgmEnableAfterReset;
    }

    bool isSetRfPgmTerminateAfter1Min() {
      return m_isSetRfPgmTerminateAfter1Min;
    }

    const bool getRfPgmTerminateAfter1Min() {
      return m_rfPgmTerminateAfter1Min;
    }

    bool isSetRfPgmTerminateMcuPin() {
      return m_isSetRfPgmTerminateMcuPin;
    }

    const bool getRfPgmTerminateMcuPin() {
      return m_rfPgmTerminateMcuPin;
    }

    bool isSetRfPgmDualChannel() {
      return m_isSetRfPgmDualChannel;
    }

    const bool getRfPgmDualChannel() {
      return m_rfPgmDualChannel;
    }

    bool isSetRfPgmLpMode() {
      return m_isSetRfPgmLpMode;
    }

    const bool getRfPgmLpMode() {
      return m_rfPgmLpMode;
    }

    bool isSetRfPgmIncorrectUpload() {
      return m_isSetRfPgmIncorrectUpload;
    }

    const bool getRfPgmIncorrectUpload() {
      return m_rfPgmIncorrectUpload;
    }

    bool isSetCustomDpaHandler() {
      return m_isSetCustomDpaHandler;
    }

    const bool getCustomDpaHandler() {
      return m_customDpaHandler;
    }

    bool isSetNodeDpaInterface() {
      return m_isSetNodeDpaInterface;
    }

    const bool getNodeDpaInterface() {
      return m_nodeDpaInterface;
    }

    bool isSetDpaAutoexec() {
      return m_isSetDpaAutoexec;
    }

    const bool getDpaAutoexec() {
      return m_dpaAutoexec;
    }

    bool isSetRoutingOff() {
      return m_isSetRoutingOff;
    }

    const bool getRoutingOff() {
      return m_routingOff;
    }

    bool isSetIoSetup() {
      return m_isSetIoSetup;
    }

    const bool getIoSetup() {
      return m_ioSetup;
    }

    bool isSetPeerToPeer() {
      return m_isSetPeerToPeer;
    }

    const bool getPeerToPeer() {
      return m_peerToPeer;
    }


    bool isSetRfBand() const {
      return m_isSetRfBand;
    }

    // returns the coordinator's RF band
    const std::string getRfBand() const {
      return m_rfBand;
    }

    // returns the security password
    bool isSetSecurityPassword() const {
      return m_isSetSecurityPassword;
    }

    // returns the security password
    const std::string getSecurityPassword() const {
      return m_securityPassword;
    }

    bool isSetSecurityUserKey() const {
      return m_isSetSecurityUserKey;
    }

    // returns the security user key
    const std::string getSecurityUserKey() const {
      return m_securityUserKey;
    }

  protected:
    void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) override
    {
      rapidjson::Pointer("/data/rsp/response")
        .Set(doc, encodeBinary(res.getResponse().DpaPacket().Buffer, res.getResponse().GetLength()));
    }


  private:
    bool m_isSetRepeat = false;
    bool m_isSetRestart = false;

    bool m_isSetRfChannelA = false;
    bool m_isSetRfChannelB = false;
    bool m_isSetRfSubChannelA = false;
    bool m_isSetRfSubChannelB = false;
    bool m_isSetTxPower = false;
    bool m_isSetRxFilter = false;
    bool m_isSetLpRxTimeout = false;
    bool m_isSetRfPgmAltChannel = false;
    bool m_isSetUartBaudrate = false;

    bool m_isSetRfPgmEnableAfterReset = false;
    bool m_isSetRfPgmTerminateAfter1Min = false;
    bool m_isSetRfPgmTerminateMcuPin = false;
    bool m_isSetRfPgmDualChannel = false;
    bool m_isSetRfPgmLpMode = false;
    bool m_isSetRfPgmIncorrectUpload = false;
    bool m_isSetCustomDpaHandler = false;
    bool m_isSetNodeDpaInterface = false;
    bool m_isSetDpaAutoexec = false;
    bool m_isSetRoutingOff = false;
    bool m_isSetIoSetup = false;
    bool m_isSetPeerToPeer = false;

    bool m_isSetRfBand = false;
    bool m_isSetSecurityPassword = false;
    bool m_isSetSecurityUserKey = false;

    int m_repeat = 0;
    bool m_restart;
    std::list<int> m_deviceAddr;
    int m_rfChannelA;
    int m_rfChannelB;
    int m_rfSubChannelA;
    int m_rfSubChannelB;
    int m_txPower;
    int m_rxFilter;
    int m_lpRxTimeout;
    int m_rfPgmAltChannel;
    int m_uartBaudrate;

    bool m_rfPgmEnableAfterReset;
    bool m_rfPgmTerminateAfter1Min;
    bool m_rfPgmTerminateMcuPin;
    bool m_rfPgmDualChannel;
    bool m_rfPgmLpMode;
    bool m_rfPgmIncorrectUpload;
    bool m_customDpaHandler;
    bool m_nodeDpaInterface;
    bool m_dpaAutoexec;
    bool m_routingOff;
    bool m_ioSetup;
    bool m_peerToPeer;

    std::string m_rfBand;
    std::string m_securityPassword;
    std::string m_securityUserKey;


    void parseDeviceAddr(rapidjson::Document& doc) {
      // mandatory field
      if (rapidjson::Pointer("/data/req/deviceAddr").IsValid() == false) {
        throw std::logic_error("Invalid /data/req/deviceAddr");
      }

      rapidjson::Value* deviceAddrJson = rapidjson::Pointer("/data/req/deviceAddr").Get(doc);
      if (deviceAddrJson->IsArray()) {
        for (rapidjson::SizeType i = 0; i < deviceAddrJson->Size(); i++) {
          m_deviceAddr.push_back(deviceAddrJson[i].GetInt());
        }
      }
    }

    // parses configuration bytes
    void parseConfigBytes(rapidjson::Document& doc) 
    {      
      if (rapidjson::Pointer("/data/req/rfChannelA").IsValid()) {
        m_isSetRfChannelA = true;
        m_rfChannelA = rapidjson::Pointer("/data/req/rfChannelA").Get(doc)->GetInt();
      }

      if (rapidjson::Pointer("/data/req/rfChannelB").IsValid()) {
        m_isSetRfChannelB = true;
        m_rfChannelB = rapidjson::Pointer("/data/req/rfChannelB").Get(doc)->GetInt();
      }

      if (rapidjson::Pointer("/data/req/rfSubChannelA").IsValid()) {
        m_isSetRfSubChannelA = true;
        m_rfSubChannelA = rapidjson::Pointer("/data/req/rfSubChannelA").Get(doc)->GetInt();
      }

      if (rapidjson::Pointer("/data/req/rfSubChannelB").IsValid()) {
        m_isSetRfSubChannelB = true;
        m_rfSubChannelB = rapidjson::Pointer("/data/req/rfSubChannelB").Get(doc)->GetInt();
      }

      if (rapidjson::Pointer("/data/req/txPower").IsValid()) {
        m_isSetTxPower = true;
        m_txPower = rapidjson::Pointer("/data/req/txPower").Get(doc)->GetInt();
      }

      if (rapidjson::Pointer("/data/req/rxFilter").IsValid()) {
        m_isSetRxFilter = true;
        m_rxFilter = rapidjson::Pointer("/data/req/rxFilter").Get(doc)->GetInt();
      }

      if (rapidjson::Pointer("/data/req/lpRxTimeout").IsValid()) {
        m_isSetLpRxTimeout = true;
        m_lpRxTimeout = rapidjson::Pointer("/data/req/lpRxTimeout").Get(doc)->GetInt();
      }
     
      if (rapidjson::Pointer("/data/req/rfPgmAltChannel").IsValid()) {
        m_isSetRfPgmAltChannel = true;
        m_rfPgmAltChannel = rapidjson::Pointer("/data/req/rfPgmAltChannel").Get(doc)->GetInt();
      }
      
      if (rapidjson::Pointer("/data/req/uartBaudrate").IsValid()) {
        m_isSetUartBaudrate = true;
        m_uartBaudrate = rapidjson::Pointer("/data/req/uartBaudrate").Get(doc)->GetInt();
      }



      if (rapidjson::Pointer("/data/req/customDpaHandler").IsValid()) {
        m_isSetCustomDpaHandler = true;
        m_customDpaHandler = rapidjson::Pointer("/data/req/customDpaHandler").Get(doc)->GetBool();
      }

      if (rapidjson::Pointer("/data/req/nodeDpaInterface").IsValid()) {
        m_isSetNodeDpaInterface = true;
       m_nodeDpaInterface = rapidjson::Pointer("/data/req/nodeDpaInterface").Get(doc)->GetBool();
      }

      if (rapidjson::Pointer("/data/req/dpaAutoexec").IsValid()) {
        m_isSetDpaAutoexec = true;
        m_dpaAutoexec = rapidjson::Pointer("/data/req/dpaAutoexec").Get(doc)->GetBool();
      }

      if (rapidjson::Pointer("/data/req/routingOff").IsValid()) {
        m_isSetRoutingOff = true;
        m_routingOff = rapidjson::Pointer("/data/req/routingOff").Get(doc)->GetBool();
      }

      if (rapidjson::Pointer("/data/req/ioSetup").IsValid()) {
        m_isSetIoSetup = true;
        m_ioSetup = rapidjson::Pointer("/data/req/ioSetup").Get(doc)->GetBool();
      }

      if (rapidjson::Pointer("/data/req/peerToPeer").IsValid()) {
        m_isSetPeerToPeer = true;
        m_peerToPeer = rapidjson::Pointer("/data/req/peerToPeer").Get(doc)->GetBool();
      }


      // RFPGM configuration bits
      if (rapidjson::Pointer("/data/req/rfPgmDualChannel").IsValid()) {
        m_isSetRfPgmDualChannel = true;
        m_rfPgmDualChannel = rapidjson::Pointer("/data/req/rfPgmDualChannel").Get(doc)->GetBool();
      }

      if (rapidjson::Pointer("/data/req/rfPgmLpMode").IsValid()) {
        m_isSetRfPgmLpMode = true;
        m_rfPgmLpMode = rapidjson::Pointer("/data/req/rfPgmLpMode").Get(doc)->GetBool();
      }

      if (rapidjson::Pointer("/data/req/rfPgmEnableAfterReset").IsValid()) {
        m_isSetRfPgmEnableAfterReset = true;
        m_rfPgmEnableAfterReset = rapidjson::Pointer("/data/req/rfPgmEnableAfterReset").Get(doc)->GetBool();
      }

      if (rapidjson::Pointer("/data/req/rfPgmTerminateAfter1Min").IsValid()) {
        m_isSetRfPgmTerminateAfter1Min = true;
        m_rfPgmTerminateAfter1Min = rapidjson::Pointer("/data/req/rfPgmTerminateAfter1Min").Get(doc)->GetBool();
      }

      if (rapidjson::Pointer("/data/req/rfPgmTerminateMcuPin").IsValid()) {
        m_isSetRfPgmTerminateMcuPin = true;
        m_rfPgmTerminateMcuPin = rapidjson::Pointer("/data/req/rfPgmTerminateMcuPin").Get(doc)->GetBool();
      }
    }

    void parseBand(rapidjson::Document& doc) {
      if (rapidjson::Pointer("/data/req/rfBand").IsValid()) {
        m_isSetRfBand = true;
        m_rfBand = rapidjson::Pointer("/data/req/rfBand").Get(doc)->GetString();
      }
    }

    void parseSecuritySettings(rapidjson::Document& doc) {
      if (rapidjson::Pointer("/data/req/securityPassword").IsValid()) {
        m_isSetSecurityPassword = true;
        m_securityPassword = rapidjson::Pointer("/data/req/securityPassword").Get(doc)->GetString();
      }
      
      if (rapidjson::Pointer("/data/req/securityUserKey").IsValid()) {
        m_isSetSecurityUserKey = true;
        m_securityUserKey = rapidjson::Pointer("/data/req/securityUserKey").Get(doc)->GetString();
      }
    }

    void parseRepeat(rapidjson::Document& doc) {
      if (rapidjson::Pointer("/data/repeat").IsValid()) {
        m_repeat = rapidjson::Pointer("/data/repeat").Get(doc)->GetInt();
        m_isSetRepeat = true;
      }
    }

    void parseRestart(rapidjson::Document& doc) {
      if (rapidjson::Pointer("/data/restart").IsValid()) {
        m_restart = rapidjson::Pointer("/data/restart").Get(doc)->GetBool();
        m_isSetRestart = true;
      }
    }

    // parses document into data fields
    void parse(rapidjson::Document& doc) {
      parseRepeat(doc);
      parseRestart(doc);
      parseDeviceAddr(doc);
      parseConfigBytes(doc);
      parseBand(doc);
      parseSecuritySettings(doc);
    }
  };
}
