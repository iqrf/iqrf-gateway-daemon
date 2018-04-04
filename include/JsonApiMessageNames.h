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

  const std::string mType_iqrfEmbedCoordRebond = "iqrfEmbedCoordRebond";
  const std::string mType_iqrfEmbedCoordRemoveBond = "iqrfEmbedCoordRemoveBond";
  const std::string mType_iqrfEmbedCoordRestore = "iqrfEmbedCoordRestore";
  const std::string mType_iqrfEmbedCoordSetDpaParams = "iqrfEmbedCoordSetDpaParams";
  const std::string mType_iqrfEmbedCoordSetHops = "iqrfEmbedCoordSetHops";
  const std::string mType_iqrfEmbedExploreEnum = "iqrfEmbedExploreEnum";
  const std::string mType_iqrfEmbedExploreMorePerInfo = "iqrfEmbedExploreMorePerInfo";
  const std::string mType_iqrfEmbedExplorePerInfo = "iqrfEmbedExplorePerInfo";
  const std::string mType_iqrfEmbedFrcExtraResult = "iqrfEmbedFrcExtraResult";
  const std::string mType_iqrfEmbedFrcSend = "iqrfEmbedFrcSend";
  const std::string mType_iqrfEmbedFrcSendSelective = "iqrfEmbedFrcSendSelective";
  const std::string mType_iqrfEmbedFrcSetParams = "iqrfEmbedFrcSetParams";
  const std::string mType_iqrfEmbedIoDir = "iqrfEmbedIoDir";
  const std::string mType_iqrfEmbedIoGet = "iqrfEmbedIoGet";
  const std::string mType_iqrfEmbedIoSet = "iqrfEmbedIoSet";
  const std::string mType_iqrfEmbedLedgGet = "iqrfEmbedLedgGet";
  const std::string mType_iqrfEmbedLedgPulse = "iqrfEmbedLedgPulse";
  const std::string mType_iqrfEmbedLedgSet = "iqrfEmbedLedgSet";
  const std::string mType_iqrfEmbedLedrGet = "iqrfEmbedLedrGet";
  const std::string mType_iqrfEmbedLedrPulse = "iqrfEmbedLedrPulse";
  const std::string mType_iqrfEmbedLedrSet = "iqrfEmbedLedrSet";
  const std::string mType_iqrfEmbedMemoryRead = "iqrfEmbedMemoryRead";
  const std::string mType_iqrfEmbedMemoryWrite = "iqrfEmbedMemoryWrite";
  const std::string mType_iqrfEmbedNodeBackup = "iqrfEmbedNodeBackup";
  const std::string mType_iqrfEmbedNodeClearRemotelyBondedMid = "iqrfEmbedNodeClearRemotelyBondedMid";
  const std::string mType_iqrfEmbedNodeEnableRemoteBond = "iqrfEmbedNodeEnableRemoteBond";
  const std::string mType_iqrfEmbedNodeRead = "iqrfEmbedNodeRead";
  const std::string mType_iqrfEmbedNodeReadRemotelyBondedMid = "iqrfEmbedNodeReadRemotelyBondedMid";
  const std::string mType_iqrfEmbedNodeRemoveBond = "iqrfEmbedNodeRemoveBond";
  const std::string mType_iqrfEmbedNodeRestore = "iqrfEmbedNodeRestore";
  const std::string mType_iqrfEmbedOsBatch = "iqrfEmbedOsBatch";
  const std::string mType_iqrfEmbedOsInitRR = "iqrfEmbedOsInitRR";
  const std::string mType_iqrfEmbedOsLoadCode = "iqrfEmbedOsLoadCode";
  const std::string mType_iqrfEmbedOsRead = "iqrfEmbedOsRead";
  const std::string mType_iqrfEmbedOsReadCfg = "iqrfEmbedOsReadCfg";
  const std::string mType_iqrfEmbedOsRunRfpgm = "iqrfEmbedOsRunRfpgm";
  const std::string mType_iqrfEmbedOsSelectiveBatch = "iqrfEmbedOsSelectiveBatch";
  const std::string mType_iqrfEmbedOsSetSecurity = "iqrfEmbedOsSetSecurity";
  const std::string mType_iqrfEmbedOsSleep = "iqrfEmbedOsSleep";
  const std::string mType_iqrfEmbedOsWriteCfg = "iqrfEmbedOsWriteCfg";
  const std::string mType_iqrfEmbedOsWriteCfgByte = "iqrfEmbedOsWriteCfgByte";
  const std::string mType_iqrfEmbedSpiWriteRead = "iqrfEmbedSpiWriteRead";
  const std::string mType_iqrfEmbedThermometerRead = "iqrfEmbedThermometerRead";
  const std::string mType_iqrfEmbedUartClearWriteRead = "iqrfEmbedUartClearWriteRead";
  const std::string mType_iqrfEmbedUartClose = "iqrfEmbedUartClose";
  const std::string mType_iqrfEmbedUartOpen = "iqrfEmbedUartOpen";
  const std::string mType_iqrfEmbedUartWriteRead = "iqrfEmbedUartWriteRead";
  const std::string mType_iqrfRaw = "iqrfRaw";
  const std::string mType_iqrfRawHdp = "iqrfRawHdp";
  const std::string mType_iqrfSdevBinaryOutputEnum = "iqrfSdevBinaryOutputEnum";
  const std::string mType_iqrfSdevBinaryOutputSetOutput = "iqrfSdevBinaryOutputSetOutput";
  const std::string mType_iqrfSdevLightDecrementPower = "iqrfSdevLightDecrementPower";
  const std::string mType_iqrfSdevLightEnum = "iqrfSdevLightEnum";
  const std::string mType_iqrfSdevLightIncrementPower = "iqrfSdevLightIncrementPower";
  const std::string mType_iqrfSdevLightSetPower = "iqrfSdevLightSetPower";
  const std::string mType_iqrfSdevSensorEnum = "iqrfSdevSensorEnum";
  const std::string mType_iqrfSdevSensorFrc = "iqrfSdevSensorFrc";
  const std::string mType_iqrfSdevSensorReadwt = "iqrfSdevSensorReadwt";

}
