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

    const int getRepeat() const
    {
      return m_repeat;
    }

    bool isSetDeviceAddr() const {
      return m_isSetDeviceAddr;
    }

    // returns address of device, to which write configuration in
    const int getDeviceAddr() const
    {
      return m_deviceAddr;
    }

    bool isSetHwpId() const {
      return m_isSetHwpId;
    }

    const int getHwpId() const
    {
      return m_hwpId;
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

    bool isSetRfAltDsmChannel() {
      return m_isSetRfAltDsmChannel;
    }

    const int getRfAltDsmChannel() {
      return m_rfAltDsmChannel;
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

    // only for DPA 3.03 onwards
    bool isSetNeverSleep() {
      return m_isSetNeverSleep;
    }

    const bool getNeverSleep() {
      return m_neverSleep;
    }

    // only for DPA 4.00 onwards
    bool isSetStdAndLpNetwork() {
      return m_isSetStdAndLpNetwork;
    }

    const bool getStdAndLpNetwork() {
      return m_stdAndLpNetwork;
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


    // first 4 bytes of configuration - enabled/disabled peripherals 
    bool isSetCoordinator() {
      return m_isSetCoordinator;
    }

    const bool getCoordinator() {
      return m_coordinator;
    }

    bool isSetNode() {
      return m_isSetNode;
    }

    const bool getNode() {
      return m_node;
    }

    bool isSetOs() {
      return m_isSetOs;
    }

    const bool getOs() {
      return m_os;
    }

    bool isSetEeprom() {
      return m_isSetEeprom;
    }

    const bool getEeprom() {
      return m_eeprom;
    }

    bool isSetEeeprom() {
      return m_isSetEeeprom;
    }

    const bool getEeeprom() {
      return m_eeeprom;
    }

    bool isSetRam() {
      return m_isSetRam;
    }

    const bool getRam() {
      return m_ram;
    }

    bool isSetLedr() {
      return m_isSetLedr;
    }

    const bool getLedr() {
      return m_ledr;
    }

    bool isSetLedg() {
      return m_isSetLedg;
    }

    const bool getLedg() {
      return m_ledg;
    }

    bool isSetSpi() {
      return m_isSetSpi;
    }

    const bool getSpi() {
      return m_spi;
    }

    bool isSetIo() {
      return m_isSetIo;
    }

    const bool getIo() {
      return m_io;
    }

    bool isSetThermometer() {
      return m_isSetThermometer;
    }

    const bool getThermometer() {
      return m_thermometer;
    }

    bool isSetPwm() {
      return m_isSetPwm;
    }

    const bool getPwm() {
      return m_pwm;
    }

    bool isSetUart() {
      return m_isSetUart;
    }

    const bool getUart() {
      return m_uart;
    }

    bool isSetFrc() {
      return m_isSetFrc;
    }

    const bool getFrc() {
      return m_frc;
    }


  protected:
    void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) override
    {
      rapidjson::Pointer("/data/rsp/response")
        .Set(doc, encodeBinary(res.getResponse().DpaPacket().Buffer, res.getResponse().GetLength()));
    }


  private:
    bool m_isSetDeviceAddr = false;
    bool m_isSetHwpId = false;

    bool m_isSetRfChannelA = false;
    bool m_isSetRfChannelB = false;
    bool m_isSetRfSubChannelA = false;
    bool m_isSetRfSubChannelB = false;
    bool m_isSetTxPower = false;
    bool m_isSetRxFilter = false;
    bool m_isSetLpRxTimeout = false;
    bool m_isSetRfAltDsmChannel = false;
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
    bool m_isSetNeverSleep = false;
    bool m_isSetStdAndLpNetwork = false;

    bool m_isSetRfBand = false;
    bool m_isSetSecurityPassword = false;
    bool m_isSetSecurityUserKey = false;

    int m_repeat = 0;
    int m_deviceAddr;
    int m_hwpId;

    int m_rfChannelA;
    int m_rfChannelB;
    int m_rfSubChannelA;
    int m_rfSubChannelB;
    int m_txPower;
    int m_rxFilter;
    int m_lpRxTimeout;
    int m_rfAltDsmChannel;
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
    bool m_neverSleep;
    bool m_stdAndLpNetwork;

    std::string m_rfBand;
    std::string m_securityPassword;
    std::string m_securityUserKey;

    // periherals presence in config bytes
    bool m_coordinator;
    bool m_node;
    bool m_os;
    bool m_eeprom;
    bool m_eeeprom;
    bool m_ram;
    bool m_ledr;
    bool m_ledg;
    bool m_spi;
    bool m_io;
    bool m_thermometer;
    bool m_pwm;
    bool m_uart;
    bool m_frc;

    bool m_isSetCoordinator = false;
    bool m_isSetNode = false;
    bool m_isSetOs = false;
    bool m_isSetEeprom = false;
    bool m_isSetEeeprom = false;
    bool m_isSetRam = false;
    bool m_isSetLedr = false;
    bool m_isSetLedg = false;
    bool m_isSetSpi = false;
    bool m_isSetIo = false;
    bool m_isSetThermometer = false;
    bool m_isSetPwm = false;
    bool m_isSetUart = false;
    bool m_isSetFrc = false;



    void parseDeviceAddr(rapidjson::Document& doc) {
      if (rapidjson::Value* deviceAddrJsonVal = rapidjson::Pointer("/data/req/deviceAddr").Get(doc)) {
        m_deviceAddr = deviceAddrJsonVal->GetInt();
        m_isSetDeviceAddr = true;
      }
    }

    void parseHwpId(rapidjson::Document& doc) {
      if (rapidjson::Value* hwpIdJsonVal = rapidjson::Pointer("/data/req/hwpId").Get(doc)) {
        m_hwpId = hwpIdJsonVal->GetInt();
        m_isSetHwpId = true;
      }
    }

    void parseEmbPeripherals(rapidjson::Document& doc) {
      if (rapidjson::Value* coordinatorJsonVal = rapidjson::Pointer("/data/req/embPers/coordinator").Get(doc)) {
        m_coordinator = coordinatorJsonVal->GetBool();
        m_isSetCoordinator = true;
      }

      if (rapidjson::Value* nodeJsonVal = rapidjson::Pointer("/data/req/embPers/node").Get(doc)) {
        m_node = nodeJsonVal->GetBool();
        m_isSetNode = true;
      }

      if (rapidjson::Value* OsJsonVal = rapidjson::Pointer("/data/req/embPers/os").Get(doc)) {
        m_os = OsJsonVal->GetBool();
        m_isSetOs = true;
      }

      if (rapidjson::Value* eepromJsonVal = rapidjson::Pointer("/data/req/embPers/eeprom").Get(doc)) {
        m_eeprom = eepromJsonVal->GetBool();
        m_isSetEeprom = true;
      }

      if (rapidjson::Value* eeepromJsonVal = rapidjson::Pointer("/data/req/embPers/eeeprom").Get(doc)) {
        m_eeeprom = eeepromJsonVal->GetBool();
        m_isSetEeeprom = true;
      }

      if (rapidjson::Value* ramJsonVal = rapidjson::Pointer("/data/req/embPers/ram").Get(doc)) {
        m_ram = ramJsonVal->GetBool();
        m_isSetRam = true;
      }

      if (rapidjson::Value* ledrJsonVal = rapidjson::Pointer("/data/req/embPers/ledr").Get(doc)) {
        m_ledr = ledrJsonVal->GetBool();
        m_isSetLedr = true;
      }

      if (rapidjson::Value* ledgJsonVal = rapidjson::Pointer("/data/req/embPers/ledg").Get(doc)) {
        m_ledg = ledgJsonVal->GetBool();
        m_isSetLedg = true;
      }

      if (rapidjson::Value* spiJsonVal = rapidjson::Pointer("/data/req/embPers/spi").Get(doc)) {
        m_spi = spiJsonVal->GetBool();
        m_isSetSpi = true;
      }

      if (rapidjson::Value* ioJsonVal = rapidjson::Pointer("/data/req/embPers/io").Get(doc)) {
        m_io = ioJsonVal->GetBool();
        m_isSetIo = true;
      }

      if (rapidjson::Value* thermometerJsonVal = rapidjson::Pointer("/data/req/embPers/thermometer").Get(doc)) {
        m_thermometer = thermometerJsonVal->GetBool();
        m_isSetThermometer = true;
      }

      if (rapidjson::Value* pwmJsonVal = rapidjson::Pointer("/data/req/embPers/pwm").Get(doc)) {
        m_pwm = pwmJsonVal->GetBool();
        m_isSetPwm = true;
      }

      if (rapidjson::Value* uartJsonVal = rapidjson::Pointer("/data/req/embPers/uart").Get(doc)) {
        m_uart = uartJsonVal->GetBool();
        m_isSetUart = true;
      }

      if (rapidjson::Value* frcJsonVal = rapidjson::Pointer("/data/req/embPers/frc").Get(doc)) {
        m_frc = frcJsonVal->GetBool();
        m_isSetFrc = true;
      }
    }

    // parses configuration bytes
    void parseConfigBytes(rapidjson::Document& doc) 
    {      
      parseEmbPeripherals(doc);
      
      if (rapidjson::Value* rfChannelAJsonVal = rapidjson::Pointer("/data/req/rfChannelA").Get(doc)) {
        m_rfChannelA = rfChannelAJsonVal->GetInt();
        m_isSetRfChannelA = true;
      }
      
      if (rapidjson::Value* rfChannelBJsonVal = rapidjson::Pointer("/data/req/rfChannelB").Get(doc)) {
        m_rfChannelB = rfChannelBJsonVal->GetInt();
        m_isSetRfChannelB = true;
      }

      if (rapidjson::Value* rfSubChannelAJsonVal = rapidjson::Pointer("/data/req/rfSubChannelA").Get(doc)) {
        m_rfSubChannelA = rfSubChannelAJsonVal->GetInt();
        m_isSetRfSubChannelA = true;
      }

      if (rapidjson::Value* rfSubChannelBJsonVal = rapidjson::Pointer("/data/req/rfSubChannelB").Get(doc)) {
        m_rfSubChannelB = rfSubChannelBJsonVal->GetInt();
        m_isSetRfSubChannelB = true;
      }

      if (rapidjson::Value* txPowerJsonVal = rapidjson::Pointer("/data/req/txPower").Get(doc)) {
        m_txPower = txPowerJsonVal->GetInt();
        m_isSetTxPower = true;
      }

      if (rapidjson::Value* rxFilterJsonVal = rapidjson::Pointer("/data/req/rxFilter").Get(doc)) {
        m_rxFilter = rxFilterJsonVal->GetInt();
        m_isSetRxFilter = true;
      }

      if (rapidjson::Value* lpRxTimeoutJsonVal = rapidjson::Pointer("/data/req/lpRxTimeout").Get(doc)) {
        m_lpRxTimeout = lpRxTimeoutJsonVal->GetInt();
        m_isSetLpRxTimeout = true;
      }

      if (rapidjson::Value* rfPgmAltChannelJsonVal = rapidjson::Pointer("/data/req/rfAltDsmChannel").Get(doc)) {
        m_rfAltDsmChannel = rfPgmAltChannelJsonVal->GetInt();
        m_isSetRfAltDsmChannel = true;
      }

      if (rapidjson::Value* uartBaudrateJsonVal = rapidjson::Pointer("/data/req/uartBaudrate").Get(doc)) {
        m_uartBaudrate = uartBaudrateJsonVal->GetInt();
        m_isSetUartBaudrate = true;
      }


      if (rapidjson::Value* customDpaHandlerJsonVal = rapidjson::Pointer("/data/req/customDpaHandler").Get(doc)) {
        m_customDpaHandler = customDpaHandlerJsonVal->GetBool();
        m_isSetCustomDpaHandler = true;
      }

      if (rapidjson::Value* nodeDpaInterfaceJsonVal = rapidjson::Pointer("/data/req/nodeDpaInterface").Get(doc)) {
        m_nodeDpaInterface = nodeDpaInterfaceJsonVal->GetBool();
        m_isSetNodeDpaInterface = true;
      }

      if (rapidjson::Value* dpaAutoexecJsonVal = rapidjson::Pointer("/data/req/dpaAutoexec").Get(doc)) {
        m_dpaAutoexec = dpaAutoexecJsonVal->GetBool();
        m_isSetDpaAutoexec = true;
      }

      if (rapidjson::Value* routingOffJsonVal = rapidjson::Pointer("/data/req/routingOff").Get(doc)) {
        m_routingOff = routingOffJsonVal->GetBool();
        m_isSetRoutingOff = true;
      }

      if (rapidjson::Value* ioSetupJsonVal = rapidjson::Pointer("/data/req/ioSetup").Get(doc)) {
        m_ioSetup = ioSetupJsonVal->GetBool();
        m_isSetIoSetup = true;
      }

      if (rapidjson::Value* peerToPeerJsonVal = rapidjson::Pointer("/data/req/peerToPeer").Get(doc)) {
        m_peerToPeer = peerToPeerJsonVal->GetBool();
        m_isSetPeerToPeer = true;
      }

      if (rapidjson::Value* neverSleepJsonVal = rapidjson::Pointer("/data/req/neverSleep").Get(doc)) {
        m_neverSleep = neverSleepJsonVal->GetBool();
        m_isSetNeverSleep = true;
      }

      if (rapidjson::Value* stdAndLpNetworkJsonVal = rapidjson::Pointer("/data/req/stdAndLpNetwork").Get(doc)) {
        m_stdAndLpNetwork = stdAndLpNetworkJsonVal->GetBool();
        m_isSetStdAndLpNetwork = true;
      }

      // RFPGM configuration bits
      if (rapidjson::Value* rfPgmDualChannelJsonVal = rapidjson::Pointer("/data/req/rfPgmDualChannel").Get(doc)) {
        m_rfPgmDualChannel = rfPgmDualChannelJsonVal->GetBool();
        m_isSetRfPgmDualChannel = true;
      }

      if (rapidjson::Value* rfPgmLpModeJsonVal = rapidjson::Pointer("/data/req/rfPgmLpMode").Get(doc)) {
        m_rfPgmLpMode = rfPgmLpModeJsonVal->GetBool();
        m_isSetRfPgmLpMode = true;
      }

      if (rapidjson::Value* rfPgmEnableAfterResetJsonVal = rapidjson::Pointer("/data/req/rfPgmEnableAfterReset").Get(doc)) {
        m_rfPgmEnableAfterReset = rfPgmEnableAfterResetJsonVal->GetBool();
        m_isSetRfPgmEnableAfterReset = true;
      }

      if (rapidjson::Value* rfPgmTerminateAfter1MinJsonVal = rapidjson::Pointer("/data/req/rfPgmTerminateAfter1Min").Get(doc)) {
        m_rfPgmTerminateAfter1Min = rfPgmTerminateAfter1MinJsonVal->GetBool();
        m_isSetRfPgmTerminateAfter1Min = true;
      }

      if (rapidjson::Value* rfPgmTerminateMcuPinJsonVal = rapidjson::Pointer("/data/req/rfPgmTerminateMcuPin").Get(doc)) {
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

    // parses document into data fields
    void parse(rapidjson::Document& doc) {
      parseRepeat(doc);
      parseDeviceAddr(doc);
      parseHwpId(doc);
      parseConfigBytes(doc);
      parseBand(doc);
      parseSecuritySettings(doc);
    }
  };
}
