#pragma once

#include <string>

namespace iqrf {
  enum class RepoDevice {
    Undef = -16,
    Library = -1,
    EperCoord = 0,
    EperNode = 1,
    EperOs = 2,
    EperEeprom = 3,
    EperEeeprom = 4,
    EperMemory = 5,
    EperRLed = 6,
    EperGLed = 7,
    EperSpi = 8,
    EperIo = 9,
    EperTherm = 10,
    EperPwm = 11,
    EperUart = 12,
    EperFrc = 13,
    SdevBinary = 75,
    SdevSensor = 94,
    SdevLight = 113,
    EperDpa = 255
  };

  // driver mapping
  class RepoDeviceMethod
  {
  public:
    RepoDeviceMethod() = delete;
    RepoDeviceMethod(const std::string& methodRequestName, const std::string& methodResponseName)
      :m_methodRequestName(methodRequestName), m_methodResponseName(methodResponseName)
    {}
    std::string m_methodRequestName;
    std::string m_methodResponseName;
  };

  //const std::string repoName_DrvLib = "Driver Library";
  //  const std::string 	0 name	"Embed: Coordinator"
  //  const std::string 	1 name	"Embed: Node"
  //  const std::string 	2 name	"Embed: OS"
  //  const std::string 	3 name	"Embed: EEPROM"
  //  const std::string 	4 name	"Embed: EEEPROM"
  //  const std::string 	5 name	"Embed: RAM"
  //  const std::string 	6 name	"Embed: Red LED"
  //  const std::string 	7 name	"Embed: Green LED"
  //  const std::string 	8 name	"Embed: SPI"
  //  const std::string 	9 name	"Embed: IO"
  //  const std::string 	10 name	"Embed: Thermometer"
  //  const std::string 	11 name	"Embed: PWM"
  //  const std::string 	12 name	"Embed: UART"
  //  const std::string 	13 name	"Embed: FRC"
  //  const std::string 	75 name	"Binary Output"
  //  const std::string 	94 name	"Sensor"
  //  const std::string 	113 name	"Light"
  //  const std::string 	255 name	"Embed: DPA exploration"

  const std::string mType_comEperCoordRebond = "comEperCoordRebond";
  const std::string mType_comEperCoordRemoveBond = "comEperCoordRemoveBond";
  const std::string mType_comEperCoordRestore = "comEperCoordRestore";
  const std::string mType_comEperCoordSetDpaParams = "comEperCoordSetDpaParams";
  const std::string mType_comEperCoordSetHops = "comEperCoordSetHops";
  const std::string mType_comEperExploreEnum = "comEperExploreEnum";
  const std::string mType_comEperExploreMorePerInfo = "comEperExploreMorePerInfo";
  const std::string mType_comEperExplorePerInfo = "comEperExplorePerInfo";
  const std::string mType_comEperFrcExtraResult = "comEperFrcExtraResult";
  const std::string mType_comEperFrcSend = "comEperFrcSend";
  const std::string mType_comEperFrcSendSelective = "comEperFrcSendSelective";
  const std::string mType_comEperFrcSetParams = "comEperFrcSetParams";
  const std::string mType_comEperIoDir = "comEperIoDir";
  const std::string mType_comEperIoGet = "comEperIoGet";
  const std::string mType_comEperIoSet = "comEperIoSet";
  const std::string mType_comEperLedgGet = "comEperLedgGet";
  const std::string mType_comEperLedgPulse = "comEperLedgPulse";
  const std::string mType_comEperLedgSet = "comEperLedgSet";
  const std::string mType_comEperLedrGet = "comEperLedrGet";
  const std::string mType_comEperLedrPulse = "comEperLedrPulse";
  const std::string mType_comEperLedrSet = "comEperLedrSet";
  const std::string mType_comEperMemoryRead = "comEperMemoryRead";
  const std::string mType_comEperMemoryWrite = "comEperMemoryWrite";
  const std::string mType_comEperNodeBackup = "comEperNodeBackup";
  const std::string mType_comEperNodeClearRemotelyBondedMid = "comEperNodeClearRemotelyBondedMid";
  const std::string mType_comEperNodeEnableRemoteBond = "comEperNodeEnableRemoteBond";
  const std::string mType_comEperNodeRead = "comEperNodeRead";
  const std::string mType_comEperNodeReadRemotelyBondedMid = "comEperNodeReadRemotelyBondedMid";
  const std::string mType_comEperNodeRemoveBond = "comEperNodeRemoveBond";
  const std::string mType_comEperNodeRestore = "comEperNodeRestore";
  const std::string mType_comEperOsBatch = "comEperOsBatch";
  const std::string mType_comEperOsInitRR = "comEperOsInitRR";
  const std::string mType_comEperOsLoadCode = "comEperOsLoadCode";
  const std::string mType_comEperOsRead = "comEperOsRead";
  const std::string mType_comEperOsReadCfg = "comEperOsReadCfg";
  const std::string mType_comEperOsRunRfpgm = "comEperOsRunRfpgm";
  const std::string mType_comEperOsSelectiveBatch = "comEperOsSelectiveBatch";
  const std::string mType_comEperOsSetSecurity = "comEperOsSetSecurity";
  const std::string mType_comEperOsSleep = "comEperOsSleep";
  const std::string mType_comEperOsWriteCfg = "comEperOsWriteCfg";
  const std::string mType_comEperOsWriteCfgByte = "comEperOsWriteCfgByte";
  const std::string mType_comEperSpiWriteRead = "comEperSpiWriteRead";
  const std::string mType_comEperThermometerRead = "comEperThermometerRead";
  const std::string mType_comEperUartClearWriteRead = "comEperUartClearWriteRead";
  const std::string mType_comEperUartClose = "comEperUartClose";
  const std::string mType_comEperUartOpen = "comEperUartOpen";
  const std::string mType_comEperUartWriteRead = "comEperUartWriteRead";
  const std::string mType_comRaw = "comRaw";
  const std::string mType_comRawHdp = "comRawHdp";
  const std::string mType_comSdevBinaryOutputEnum = "comSdevBinaryOutputEnum";
  const std::string mType_comSdevBinaryOutputSetOutput = "comSdevBinaryOutputSetOutput";
  const std::string mType_comSdevLightDecrementPower = "comSdevLightDecrementPower";
  const std::string mType_comSdevLightEnum = "comSdevLightEnum";
  const std::string mType_comSdevLightIncrementPower = "comSdevLightIncrementPower";
  const std::string mType_comSdevLightSetPower = "comSdevLightSetPower";
  const std::string mType_comSdevSensorEnum = "comSdevSensorEnum";
  const std::string mType_comSdevSensorFrc = "comSdevSensorFrc";
  const std::string mType_comSdevSensorReadwt = "comSdevSensorReadwt";

}
