#pragma once

#include "ComBase.h"
#include <vector>
#include "JsonUtils.h"

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

    const int getRepeat() const
    {
      return m_repeat;
    }

    bool isSetDeviceAddr() const {
      return m_isSetDeviceAddr;
    }

    // returns addresses of devices, to which write configuration in
    const std::vector<int> getDeviceAddr() const
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
    bool m_isSetRestart = false;
    bool m_isSetDeviceAddr = false;

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
    std::vector<int> m_deviceAddr;
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
      if (rapidjson::Value* deviceAddrJsonVal = rapidjson::Pointer("/data/req").Get(doc)) {
        m_deviceAddr = jutils::getMemberAsVector<int>("deviceAddr", *deviceAddrJsonVal);
        m_isSetDeviceAddr = true;
      }
    }

    // parses configuration bytes
    void parseConfigBytes(rapidjson::Document& doc) 
    {      
      if (rapidjson::Value* rfChannelAJsonVal = rapidjson::Pointer("/data/rfChannelA").Get(doc)) {
        m_rfChannelA = rfChannelAJsonVal->GetInt();
        m_isSetRfChannelA = true;
      }
      
      if (rapidjson::Value* rfChannelBJsonVal = rapidjson::Pointer("/data/rfChannelB").Get(doc)) {
        m_rfChannelB = rfChannelBJsonVal->GetInt();
        m_isSetRfChannelB = true;
      }

      if (rapidjson::Value* rfSubChannelAJsonVal = rapidjson::Pointer("/data/rfSubChannelA").Get(doc)) {
        m_rfSubChannelA = rfSubChannelAJsonVal->GetInt();
        m_isSetRfSubChannelA = true;
      }

      if (rapidjson::Value* rfSubChannelBJsonVal = rapidjson::Pointer("/data/rfSubChannelB").Get(doc)) {
        m_rfSubChannelB = rfSubChannelBJsonVal->GetInt();
        m_isSetRfSubChannelB = true;
      }

      if (rapidjson::Value* txPowerJsonVal = rapidjson::Pointer("/data/txPower").Get(doc)) {
        m_txPower = txPowerJsonVal->GetInt();
        m_isSetTxPower = true;
      }

      if (rapidjson::Value* rxFilterJsonVal = rapidjson::Pointer("/data/rxFilter").Get(doc)) {
        m_rxFilter = rxFilterJsonVal->GetInt();
        m_isSetRxFilter = true;
      }

      if (rapidjson::Value* lpRxTimeoutJsonVal = rapidjson::Pointer("/data/lpRxTimeout").Get(doc)) {
        m_lpRxTimeout = lpRxTimeoutJsonVal->GetInt();
        m_isSetLpRxTimeout = true;
      }

      if (rapidjson::Value* rfPgmAltChannelJsonVal = rapidjson::Pointer("/data/rfPgmAltChannel").Get(doc)) {
        m_rfPgmAltChannel = rfPgmAltChannelJsonVal->GetInt();
        m_isSetRfPgmAltChannel = true;
      }

      if (rapidjson::Value* uartBaudrateJsonVal = rapidjson::Pointer("/data/uartBaudrate").Get(doc)) {
        m_uartBaudrate = uartBaudrateJsonVal->GetInt();
        m_isSetUartBaudrate = true;
      }


      if (rapidjson::Value* customDpaHandlerJsonVal = rapidjson::Pointer("/data/customDpaHandler").Get(doc)) {
        m_customDpaHandler = customDpaHandlerJsonVal->GetBool();
        m_isSetCustomDpaHandler = true;
      }

      if (rapidjson::Value* nodeDpaInterfaceJsonVal = rapidjson::Pointer("/data/nodeDpaInterface").Get(doc)) {
        m_nodeDpaInterface = nodeDpaInterfaceJsonVal->GetBool();
        m_isSetNodeDpaInterface = true;
      }

      if (rapidjson::Value* dpaAutoexecJsonVal = rapidjson::Pointer("/data/dpaAutoexec").Get(doc)) {
        m_dpaAutoexec = dpaAutoexecJsonVal->GetBool();
        m_isSetDpaAutoexec = true;
      }

      if (rapidjson::Value* routingOffJsonVal = rapidjson::Pointer("/data/routingOff").Get(doc)) {
        m_routingOff = routingOffJsonVal->GetBool();
        m_isSetRoutingOff = true;
      }

      if (rapidjson::Value* ioSetupJsonVal = rapidjson::Pointer("/data/ioSetup").Get(doc)) {
        m_ioSetup = ioSetupJsonVal->GetBool();
        m_isSetIoSetup = true;
      }

      if (rapidjson::Value* peerToPeerJsonVal = rapidjson::Pointer("/data/peerToPeer").Get(doc)) {
        m_peerToPeer = peerToPeerJsonVal->GetBool();
        m_isSetPeerToPeer = true;
      }


      // RFPGM configuration bits
      if (rapidjson::Value* rfPgmDualChannelJsonVal = rapidjson::Pointer("/data/rfPgmDualChannel").Get(doc)) {
        m_rfPgmDualChannel = rfPgmDualChannelJsonVal->GetBool();
        m_isSetRfPgmDualChannel = true;
      }

      if (rapidjson::Value* rfPgmLpModeJsonVal = rapidjson::Pointer("/data/rfPgmLpMode").Get(doc)) {
        m_rfPgmLpMode = rfPgmLpModeJsonVal->GetBool();
        m_isSetRfPgmLpMode = true;
      }

      if (rapidjson::Value* rfPgmEnableAfterResetJsonVal = rapidjson::Pointer("/data/rfPgmEnableAfterReset").Get(doc)) {
        m_rfPgmEnableAfterReset = rfPgmEnableAfterResetJsonVal->GetBool();
        m_isSetRfPgmEnableAfterReset = true;
      }

      if (rapidjson::Value* rfPgmTerminateAfter1MinJsonVal = rapidjson::Pointer("/data/rfPgmTerminateAfter1Min").Get(doc)) {
        m_rfPgmTerminateAfter1Min = rfPgmTerminateAfter1MinJsonVal->GetBool();
        m_isSetRfPgmTerminateAfter1Min = true;
      }

      if (rapidjson::Value* rfPgmTerminateMcuPinJsonVal = rapidjson::Pointer("/data/rfPgmTerminateMcuPin").Get(doc)) {
        m_rfPgmTerminateMcuPin = rfPgmTerminateMcuPinJsonVal->GetBool();
        m_isSetRfPgmTerminateMcuPin = true;
      }
    }

    void parseBand(rapidjson::Document& doc) {
      if (rapidjson::Value* rfBandJsonVal = rapidjson::Pointer("/data/req/rfBand").Get(doc)) {
        m_rfBand = rfBandJsonVal->GetString();
        m_isSetRfBand = true;
      }
    }

    void parseSecuritySettings(rapidjson::Document& doc) {
      if (rapidjson::Value* securityPasswordJsonVal = rapidjson::Pointer("/data/req/securityPassword").Get(doc)) {
        m_securityPassword = securityPasswordJsonVal->GetString();
        m_isSetSecurityPassword = true;
      }

      if (rapidjson::Value* securityUserKeyJsonVal = rapidjson::Pointer("/data/req/securityUserKey").Get(doc)) {
        m_securityUserKey = securityUserKeyJsonVal->GetString();
        m_isSetSecurityUserKey = true;
      }
    }

    void parseRepeat(rapidjson::Document& doc) {
      if (rapidjson::Value* repeatJsonVal = rapidjson::Pointer("/data/repeat").Get(doc)) {
        m_repeat = repeatJsonVal->GetInt();
      }
    }

    void parseRestart(rapidjson::Document& doc) {
      if (rapidjson::Value* restartJsonVal = rapidjson::Pointer("/data/restart").Get(doc)) {
        m_restart = restartJsonVal->GetBool();
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
