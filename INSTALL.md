# How to install the gateway-daemon (Beta)

[https://repos.iqrfsdk.org/testing](https://repos.iqrfsdk.org/testing)

-   iqrf-gateway-daemon_2.0.0-x_amd64.deb
-   iqrf-gateway-daemon_2.0.0-x_armhf.deb
-   iqrf-gateway-daemon_2.0.0-x_arm64.deb

## Download public key to verify the packages from the repository

```Bash
sudo apt-get install dirmngr
sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 9C076FCC7AB8F2E43C2AB0E73241B9B7B4BD8F8E
```

## Add repository to the source list

-	For Debian 9 (amd64, armhf, arm64)

```Bash
echo "deb https://repos.iqrfsdk.org/testing/debian stretch testing" | sudo tee -a /etc/apt/sources.list
sudo apt-get update
```

## Stop and disable daemon v1

```Bash
sudo systemctl stop iqrf-daemon.service
sudo systemctl disable iqrf-daemon.service
```

## Install or update the daemon

```Bash
sudo apt-get install iqrf-gateway-daemon
```
or

```Bash
sudo apt-get update
sudo apt-get install --only-upgrade iqrf-gateway-daemon
```
and

```Bash
sudo systemctl daemon-reload
sudo systemctl enable iqrfgd2.service
sudo systemctl restart iqrfgd2.service
```

## Check the daemon service status

```Bash
sudo systemctl status iqrfgd2.service

● iqrfgd2.service - IQRF Gateway Daemon
   Loaded: loaded (/lib/systemd/system/iqrfgd2.service; disabled; vendor preset: enabled)
   Active: active (running) since Thu 2018-07-26 17:40:15 CEST; 18s ago
 Main PID: 19638 (iqrfgd2)
    Tasks: 15 (limit: 4915)
   CGroup: /system.slice/iqrfgd2.service
           ââ19638 /usr/bin/iqrfgd2 /etc/iqrfgd2/config.json

Jul 26 17:40:18 ubilinux4 iqrfgd2[19638]: vhost match to default based on port 1338
Jul 26 17:40:18 ubilinux4 iqrfgd2[19638]: Upgrade to ws
Jul 26 17:40:18 ubilinux4 iqrfgd2[19638]: defaulting to prot handler 0
Jul 26 17:40:18 ubilinux4 iqrfgd2[19638]: lws_handshake_server: 0x7f91380060a0: inheriting ws ah (rxpos:0, rxlen:223)
Jul 26 17:40:18 ubilinux4 iqrfgd2[19638]: lws_header_table_detach: wsi 0x7f91380060a0: ah 0x7f91380062f0 (tsi=0, count = 1)
Jul 26 17:40:18 ubilinux4 iqrfgd2[19638]: lws_header_table_detach: nobody usable waiting
Jul 26 17:40:18 ubilinux4 iqrfgd2[19638]: _lws_destroy_ah: freed ah 0x7f91380062f0 : pool length 0
Jul 26 17:40:18 ubilinux4 iqrfgd2[19638]: lws_header_table_detach: wsi 0x7f91380060a0: ah 0x7f91380062f0 (tsi=0, count = 0)
Jul 26 17:40:18 ubilinux4 iqrfgd2[19638]: lws_read: read_ok, used 223
Jul 26 17:40:18 ubilinux4 iqrfgd2[19638]: Loading cache success           
```

## Set your configuration 

CK-USB-04A or GW-USB-06 devices must be switched to USB CDC IQRF mode using IDE
menu: Tools/USB Classes/Switch to CDC IQRF mode

Configure/check mainly following components:

folder /etc/iqrfgd2: 

- config.json           (tip: check/enable/disable daemon components)
  - select either SPI or CDC

- iqrf__IqrfSpi.json    (tip: configure your IQRF interface - SPI)
  - Raspberry Pi: /dev/spidev0.0
  - Orange Pi Zero: /dev/spidev1.0
  - UP board: /dev/spidev2.0
  - UP2 board: /dev/spidev1.0

- iqrf__IqrfCdc.json    (tip: check/configure your IQRF interface - CDC)
  - Interface: /dev/ttyACMx {x=0...y}

- iqrf__IqrfDpa.json    (tip: check/configure your DPA params)
  - Mode: LP/STD
  - Bonded/discovered devices
  - FRC response time 

- iqrf__MqttMessaging.json    (tip: check/configure your MQTT broker)
  - Broker IP
  - Broker port
  - Client topics
  - Accept async msgs

- cat iqrf__MqMessaging.json   (tip: check/configure your MQ names) 
  - Remote MQ name
  - Local MQ name
  - Accept async msgs

- iqrf__UdpMessaging.json   (tip: check/configure your UDP ports)
  - Remote port
  - Local port

- iqrf__WebsocketMessaging.json   (tip: check/configure your Websocket msgs)
  - Accept async msgs

- shape__WebsocketService.json    (tip: check/configure your Websocket ports)
  - Websocket port

- iqrf__JsCache.json    (tip: check/configure IQRF repository cache update period)
  - Cache update period

- iqrf__OtaUploadService.json   (tip: check/configure location of HEX, IQRF files)
  - Upload path

folder /var/cache/iqrfgd2/scheduler:

- Tasks.json        (tip: configure your regural DPA tasks if any)

and restart the service:

```Bash
sudo systemctl restart iqrfgd2.service
```

## Content of iqrf-gateway-daemon package

```Bash
dpkg -L iqrf-gateway-daemon

/.
/usr
/usr/lib
/usr/lib/iqrfgd2
/usr/lib/iqrfgd2/libTraceFileService.so
/usr/lib/iqrfgd2/libSchedulerMessaging.so
/usr/lib/iqrfgd2/libSmartConnectService.so
/usr/lib/iqrfgd2/liblauncher.a
/usr/lib/iqrfgd2/libLegacyApiSupport.so
/usr/lib/iqrfgd2/libScheduler.so
/usr/lib/iqrfgd2/libIqrfRepoCache.so
/usr/lib/iqrfgd2/libUdpMessaging.so
/usr/lib/iqrfgd2/libTraceFormatService.so
/usr/lib/iqrfgd2/libDaemonController.so
/usr/lib/iqrfgd2/libWebsocketMessaging.so
/usr/lib/iqrfgd2/libWriteTrConfService.so
/usr/lib/iqrfgd2/libIqrfDpa.so
/usr/lib/iqrfgd2/libJsonDpaApiIqrfStdExt.so
/usr/lib/iqrfgd2/libIqrfCdc.so
/usr/lib/iqrfgd2/libEnumerateDeviceService.so
/usr/lib/iqrfgd2/libCppRestService.so
/usr/lib/iqrfgd2/libJsonSplitter.so
/usr/lib/iqrfgd2/libJsonCfgApi.so
/usr/lib/iqrfgd2/libJsonDpaApiRaw.so
/usr/lib/iqrfgd2/libWebsocketService.so
/usr/lib/iqrfgd2/libJsonDpaApiIqrfStandard.so
/usr/lib/iqrfgd2/libIqrfSpi.so
/usr/lib/iqrfgd2/libMqMessaging.so
/usr/lib/iqrfgd2/libIdeCounterpart.so
/usr/lib/iqrfgd2/libJsRenderDuktape.so
/usr/lib/iqrfgd2/libReadTrConfService.so
/usr/lib/iqrfgd2/libJsonMngApi.so
/usr/lib/iqrfgd2/libOtaUploadService.so
/usr/lib/iqrfgd2/libMqttMessaging.so
/usr/lib/iqrfgd2/libBondNodeLocalService.so
/usr/bin
/usr/bin/iqrfgd2
/usr/local
/usr/local/lib
/usr/local/lib/libpaho-mqtt3c.so.1.2.0
/usr/local/lib/libpaho-mqtt3as.so.1.2.0
/usr/local/lib/libpaho-mqtt3a.so.1.2.0
/usr/local/lib/libpaho-mqtt3cs.so.1.2.0
/usr/share
/usr/share/iqrfgd2
/usr/share/iqrfgd2/apiSchemas
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedOs_Rfpgm-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedOs_WriteCfgByte-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedCoordinator_SetHops-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedCoordinator_SetDpaParams-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqmeshNetwork_SmartConnect-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/mngScheduler_PeriodicTask-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedCoordinator_ClearRemotelyBondedMid-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedCoordinator_Restore-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedCoordinator_AddrInfo-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/mngScheduler_GetTask-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedCoordinator_ReadRemotelyBondedMid-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedEeprom_Write-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedCoordinator_Backup-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqmeshNetwork_ReadTrConf-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedIo_Set-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedNode_EnableRemoteBonding-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedNode_ReadRemotelyBondedMid-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqmeshNetwork_WriteTrConf-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedCoordinator_DiscoveredDevices-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/mngScheduler_SchedulerMessagingTask-object-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedLedr_Pulse-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedNode_ClearRemotelyBondedMid-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedCoordinator_ClearAllBonds-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedEeprom_Read-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedOs_Read-response-1-1-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedEeeprom_Write-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedOs_Batch-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqmeshNetwork_BondNodeLocal-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedCoordinator_BondedDevices-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/mngScheduler_GetTask-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedIo_Direction-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedOs_SetSecurity-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqmeshNetwork_EnumerateDevice-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqmeshNetwork_OtaUpload-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedOs_WriteCfgTriplet-object-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/mngScheduler_List-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedExplore_MorePeripheralsInformation-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedCoordinator_SetHops-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedNode_RemoveBond-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfLight_SetPower-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedFrc_SetParams-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedCoordinator_SmartConnect-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedUart_WriteRead-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedExplore_Enumerate-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedLedr_Get-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedLedg_Pulse-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfBinaryoutput_SetOutput-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfBinaryoutput_Enumerate-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedFrc_ExtraResult-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqmeshNetwork_OtaUpload-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedOs_Read-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/mngScheduler_PeriodicTask-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedLedr_Get-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedOs_SetSecurity-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfSensor_Frc-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedOs_Sleep-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfRawHdp-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedFrc_SendSelective-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedCoordinator_RemoveBond-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/mngScheduler_List-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/mngScheduler_RemoveAll-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedCoordinator_SmartConnect-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedEeprom_Read-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedNode_EnableRemoteBonding-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfSensor_Frc-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedLedg_Set-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfSensor_ReadSensorsWithTypes-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedFrc_ExtraResult-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedNode_Read-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfSensor_Enumerate-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/cfgDaemon_Component-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedOs_Restart-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedEeeprom_Read-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedOs_Batch-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfLight_SetPower-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedNode_Restore-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedLedg_Get-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedOs_ReadCfg-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfRaw-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedNode_Restore-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedCoordinator_AuthorizeBond-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedOs_Sleep-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfLight_IncrementPower-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/mngScheduler_RemoveAll-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedCoordinator_DiscoveredDevices-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedExplore_PeripheralInformation-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedLedr_Set-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedNode_ReadRemotelyBondedMid-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedNode_Backup-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfRawHdp-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedFrc_Send-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfLight_IncrementPower-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedOs_SelectiveBatch-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedCoordinator_RebondNode-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/mngDaemon_Mode-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/messageError-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedOs_WriteCfg-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedCoordinator_BondedDevices-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedNode_RemoveBond-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedUart_Open-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfBinaryoutput_Enumerate-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfSensor_Enumerate-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedFrc_SetParams-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedLedg_Set-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedOs_Reset-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/mngDaemon_Mode-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqmeshNetwork_SmartConnect-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedCoordinator_ReadRemotelyBondedMid-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedIo_Get-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedFrc_SendSelective-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/mngDaemon_Upload-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedOs_BatchRequest-object-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfSensor_ReadSensorsWithTypes-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedOs_LoadCode-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedCoordinator_BondNode-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedOs_SelectiveBatch-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedCoordinator_ClearRemotelyBondedMid-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedCoordinator_AuthorizeBond-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedUart_Close-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedNode_Read-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedCoordinator_EnableRemoteBonding-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedCoordinator_ClearAllBonds-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/mngDaemon_Exit-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedRam_Read-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedCoordinator_DiscoveryData-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedCoordinator_Restore-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedCoordinator_EnableRemoteBonding-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedOs_LoadCode-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedIo_Set-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedCoordinator_Discovery-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedLedr_Set-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfLight_DecrementPower-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedUart_ClearWriteRead-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedOs_ReadCfg-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedCoordinator_SetDpaParams-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/mngDaemon_Exit-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedThermometer_Read-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedFrc_Send-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedRam_Read-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedThermometer_Read-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfLight_Enumerate-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedExplore_PeripheralInformation-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedOs_WriteCfg-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedLedr_Pulse-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedCoordinator_RemoveBond-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedRam_Write-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/mngScheduler_RemoveTask-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqmeshNetwork_WriteTrConf-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedOs_Rfpgm-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/mngScheduler_AddTask-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedExplore_MorePeripheralsInformation-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedLedg_Get-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/mngScheduler_RemoveTask-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedSpi_WriteRead-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedCoordinator_Backup-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/cfgDaemon_Component-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedCoordinator_AddrInfo-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedEeeprom_Read-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqmeshNetwork_BondNodeLocal-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedNode_Backup-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqmeshNetwork_ReadTrConf-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedUart_ClearWriteRead-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedNode_ClearRemotelyBondedMid-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedOs_Restart-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedCoordinator_Discovery-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedCoordinator_BondNode-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedCoordinator_DiscoveryData-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedEeprom_Write-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedEeeprom_Write-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedOs_Read-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedUart_WriteRead-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/mngScheduler_AddTask-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfBinaryoutput_SetOutput-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfRaw-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedIo_Direction-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedUart_Close-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedOs_Reset-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfLight_DecrementPower-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfLight_Enumerate-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedRam_Write-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedIo_Get-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedExplore_Enumerate-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedCoordinator_RebondNode-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedLedg_Pulse-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/mngDaemon_Upload-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedSpi_WriteRead-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedUart_Open-request-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqmeshNetwork_EnumerateDevice-response-1-0-0.json
/usr/share/iqrfgd2/apiSchemas/iqrfEmbedOs_WriteCfgByte-response-1-0-0.json
/usr/share/iqrfgd2/javaScript
/usr/share/iqrfgd2/javaScript/DaemonWrapper.js
/usr/share/doc
/usr/share/doc/iqrf-gateway-daemon
/usr/share/doc/iqrf-gateway-daemon/changelog.gz
/lib
/lib/systemd
/lib/systemd/system
/lib/systemd/system/iqrfgd2.service
/etc
/etc/iqrfgd2
/etc/iqrfgd2/shape__TraceFileService.json
/etc/iqrfgd2/iqrf__DaemonController.json
/etc/iqrfgd2/shape__TraceFileService_JsCache.json
/etc/iqrfgd2/shape__ConfigurationService.json
/etc/iqrfgd2/iqrf__IqrfSpi.json
/etc/iqrfgd2/iqrf__JsonMngApi.json
/etc/iqrfgd2/iqrf__UdpMessaging.json
/etc/iqrfgd2/iqrf__EnumerateDeviceService.json
/etc/iqrfgd2/iqrf__IqrfDpa.json
/etc/iqrfgd2/iqrf__SmartConnectService.json
/etc/iqrfgd2/config.json
/etc/iqrfgd2/iqrf__IqrfRepoCache.json
/etc/iqrfgd2/shape__TraceFormatService.json
/etc/iqrfgd2/iqrf__JsonSplitter.json
/etc/iqrfgd2/iqrf__IdeCounterpart.json
/etc/iqrfgd2/shape__LauncherService.json
/etc/iqrfgd2/iqrf__MqMessaging.json
/etc/iqrfgd2/iqrf__JsonDpaApiRaw.json
/etc/iqrfgd2/iqrf__BondNodeLocalService.json
/etc/iqrfgd2/iqrf__MqttMessaging1.json
/etc/iqrfgd2/iqrf__WriteTrConfService.json
/etc/iqrfgd2/iqrf__SchedulerMessaging.json
/etc/iqrfgd2/iqrf__ReadTrConfService.json
/etc/iqrfgd2/iqrf__JsonDpaApiIqrfStdExt.json
/etc/iqrfgd2/iqrf__IqrfCdc.json
/etc/iqrfgd2/iqrf__LegacyApiSupport.json
/etc/iqrfgd2/iqrf__JsonCfgApi.json
/etc/iqrfgd2/shape__WebsocketService.json
/etc/iqrfgd2/iqrf__WebsocketMessaging.json
/etc/iqrfgd2/iqrf__OtaUploadService.json
/etc/iqrfgd2/iqrf__JsonDpaApiIqrfStandard.json
/etc/iqrfgd2/shape__CppRestService.json
/etc/iqrfgd2/iqrf__Scheduler.json
/etc/iqrfgd2/iqrf__JsRenderDuktape.json
/etc/iqrfgd2/cfgSchemas
/etc/iqrfgd2/cfgSchemas/schema__iqrf__Ide4Counterpart.json
/etc/iqrfgd2/cfgSchemas/schema__iqrf__BondNodeLocalService.json
/etc/iqrfgd2/cfgSchemas/schema__iqrf__EnumerateDeviceService.json
/etc/iqrfgd2/cfgSchemas/schema__iqrf__MqMessaging.json
/etc/iqrfgd2/cfgSchemas/schema__shape__CppRestService.json
/etc/iqrfgd2/cfgSchemas/schema__iqrf__JsonCfgApi.json
/etc/iqrfgd2/cfgSchemas/schema__iqrf__JsonDpaApiIqrfStandard.json
/etc/iqrfgd2/cfgSchemas/schema__iqrf__LegacyApiSupport.json
/etc/iqrfgd2/cfgSchemas/schema__shape__LauncherService.json
/etc/iqrfgd2/cfgSchemas/schema__iqrf__Scheduler.json
/etc/iqrfgd2/cfgSchemas/schema__iqrf__WriteTrConfService.json
/etc/iqrfgd2/cfgSchemas/schema__iqrf__JsonMngApi.json
/etc/iqrfgd2/cfgSchemas/schema__iqrf__UdpMessaging.json
/etc/iqrfgd2/cfgSchemas/schema__shape__TraceFileService.json
/etc/iqrfgd2/cfgSchemas/schema__iqrf__IqrfCdc.json
/etc/iqrfgd2/cfgSchemas/schema__shape__TraceFormatService.json
/etc/iqrfgd2/cfgSchemas/schema__iqrf__OtaUploadService.json
/etc/iqrfgd2/cfgSchemas/schema__iqrf__SchedulerMessaging.json
/etc/iqrfgd2/cfgSchemas/schema__iqrf__IqrfSpi.json
/etc/iqrfgd2/cfgSchemas/schema__iqrf__WebsocketMessaging.json
/etc/iqrfgd2/cfgSchemas/schema__iqrf__ReadTrConfService.json
/etc/iqrfgd2/cfgSchemas/schema__iqrf__JsonSplitter.json
/etc/iqrfgd2/cfgSchemas/schema__iqrf__IqrfDpa.json
/etc/iqrfgd2/cfgSchemas/schema__shape__ConfigurationService.json
/etc/iqrfgd2/cfgSchemas/schema__iqrf__JsonDpaApiRaw.json
/etc/iqrfgd2/cfgSchemas/schema__iqrf__SmartConnectService.json
/etc/iqrfgd2/cfgSchemas/schema__iqrf__JsCache.json
/etc/iqrfgd2/cfgSchemas/schema__iqrf__MqttMessaging.json
/etc/iqrfgd2/cfgSchemas/schema__iqrf__DaemonController.json
/etc/iqrfgd2/cfgSchemas/schema__shape__WebsocketService.json
/var
/var/cache
/var/cache/iqrfgd2
/var/cache/iqrfgd2/Scheduler
/var/cache/iqrfgd2/Scheduler/Tasks.json
/var/cache/iqrfgd2/iqrfRepoCache
/var/cache/iqrfgd2/iqrfRepoCache/products
/var/cache/iqrfgd2/iqrfRepoCache/products/data.json
/var/cache/iqrfgd2/iqrfRepoCache/companies
/var/cache/iqrfgd2/iqrfRepoCache/companies/data.json
/var/cache/iqrfgd2/iqrfRepoCache/osdpa
/var/cache/iqrfgd2/iqrfRepoCache/osdpa/data.json
/var/cache/iqrfgd2/iqrfRepoCache/server
/var/cache/iqrfgd2/iqrfRepoCache/server/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages
/var/cache/iqrfgd2/iqrfRepoCache/packages/28
/var/cache/iqrfgd2/iqrfRepoCache/packages/28/handler.hex
/var/cache/iqrfgd2/iqrfRepoCache/packages/28/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/47
/var/cache/iqrfgd2/iqrfRepoCache/packages/47/handler.hex
/var/cache/iqrfgd2/iqrfRepoCache/packages/47/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/6
/var/cache/iqrfgd2/iqrfRepoCache/packages/6/handler.hex
/var/cache/iqrfgd2/iqrfRepoCache/packages/6/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/59
/var/cache/iqrfgd2/iqrfRepoCache/packages/59/handler.hex
/var/cache/iqrfgd2/iqrfRepoCache/packages/59/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/19
/var/cache/iqrfgd2/iqrfRepoCache/packages/19/handler.hex
/var/cache/iqrfgd2/iqrfRepoCache/packages/19/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/48
/var/cache/iqrfgd2/iqrfRepoCache/packages/48/handler.hex
/var/cache/iqrfgd2/iqrfRepoCache/packages/48/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/43
/var/cache/iqrfgd2/iqrfRepoCache/packages/43/handler.hex
/var/cache/iqrfgd2/iqrfRepoCache/packages/43/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/57
/var/cache/iqrfgd2/iqrfRepoCache/packages/57/handler.hex
/var/cache/iqrfgd2/iqrfRepoCache/packages/57/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/9
/var/cache/iqrfgd2/iqrfRepoCache/packages/9/handler.hex
/var/cache/iqrfgd2/iqrfRepoCache/packages/9/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/32
/var/cache/iqrfgd2/iqrfRepoCache/packages/32/handler.hex
/var/cache/iqrfgd2/iqrfRepoCache/packages/32/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/39
/var/cache/iqrfgd2/iqrfRepoCache/packages/39/handler.hex
/var/cache/iqrfgd2/iqrfRepoCache/packages/39/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/27
/var/cache/iqrfgd2/iqrfRepoCache/packages/27/handler.hex
/var/cache/iqrfgd2/iqrfRepoCache/packages/27/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/54
/var/cache/iqrfgd2/iqrfRepoCache/packages/54/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/10
/var/cache/iqrfgd2/iqrfRepoCache/packages/10/handler.hex
/var/cache/iqrfgd2/iqrfRepoCache/packages/10/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/13
/var/cache/iqrfgd2/iqrfRepoCache/packages/13/handler.hex
/var/cache/iqrfgd2/iqrfRepoCache/packages/13/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/42
/var/cache/iqrfgd2/iqrfRepoCache/packages/42/handler.hex
/var/cache/iqrfgd2/iqrfRepoCache/packages/42/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/61
/var/cache/iqrfgd2/iqrfRepoCache/packages/61/handler.hex
/var/cache/iqrfgd2/iqrfRepoCache/packages/61/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/58
/var/cache/iqrfgd2/iqrfRepoCache/packages/58/handler.hex
/var/cache/iqrfgd2/iqrfRepoCache/packages/58/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/29
/var/cache/iqrfgd2/iqrfRepoCache/packages/29/handler.hex
/var/cache/iqrfgd2/iqrfRepoCache/packages/29/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/16
/var/cache/iqrfgd2/iqrfRepoCache/packages/16/handler.hex
/var/cache/iqrfgd2/iqrfRepoCache/packages/16/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/25
/var/cache/iqrfgd2/iqrfRepoCache/packages/25/handler.hex
/var/cache/iqrfgd2/iqrfRepoCache/packages/25/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/38
/var/cache/iqrfgd2/iqrfRepoCache/packages/38/handler.hex
/var/cache/iqrfgd2/iqrfRepoCache/packages/38/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/37
/var/cache/iqrfgd2/iqrfRepoCache/packages/37/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/26
/var/cache/iqrfgd2/iqrfRepoCache/packages/26/handler.hex
/var/cache/iqrfgd2/iqrfRepoCache/packages/26/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/51
/var/cache/iqrfgd2/iqrfRepoCache/packages/51/handler.hex
/var/cache/iqrfgd2/iqrfRepoCache/packages/51/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/11
/var/cache/iqrfgd2/iqrfRepoCache/packages/11/handler.hex
/var/cache/iqrfgd2/iqrfRepoCache/packages/11/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/7
/var/cache/iqrfgd2/iqrfRepoCache/packages/7/handler.hex
/var/cache/iqrfgd2/iqrfRepoCache/packages/7/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/4
/var/cache/iqrfgd2/iqrfRepoCache/packages/4/handler.hex
/var/cache/iqrfgd2/iqrfRepoCache/packages/4/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/55
/var/cache/iqrfgd2/iqrfRepoCache/packages/55/handler.hex
/var/cache/iqrfgd2/iqrfRepoCache/packages/55/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/5
/var/cache/iqrfgd2/iqrfRepoCache/packages/5/handler.hex
/var/cache/iqrfgd2/iqrfRepoCache/packages/5/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/36
/var/cache/iqrfgd2/iqrfRepoCache/packages/36/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/3
/var/cache/iqrfgd2/iqrfRepoCache/packages/3/handler.hex
/var/cache/iqrfgd2/iqrfRepoCache/packages/3/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/60
/var/cache/iqrfgd2/iqrfRepoCache/packages/60/handler.hex
/var/cache/iqrfgd2/iqrfRepoCache/packages/60/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/31
/var/cache/iqrfgd2/iqrfRepoCache/packages/31/handler.hex
/var/cache/iqrfgd2/iqrfRepoCache/packages/31/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/24
/var/cache/iqrfgd2/iqrfRepoCache/packages/24/handler.hex
/var/cache/iqrfgd2/iqrfRepoCache/packages/24/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/17
/var/cache/iqrfgd2/iqrfRepoCache/packages/17/handler.hex
/var/cache/iqrfgd2/iqrfRepoCache/packages/17/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/45
/var/cache/iqrfgd2/iqrfRepoCache/packages/45/handler.hex
/var/cache/iqrfgd2/iqrfRepoCache/packages/45/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/30
/var/cache/iqrfgd2/iqrfRepoCache/packages/30/handler.hex
/var/cache/iqrfgd2/iqrfRepoCache/packages/30/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/8
/var/cache/iqrfgd2/iqrfRepoCache/packages/8/handler.hex
/var/cache/iqrfgd2/iqrfRepoCache/packages/8/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/22
/var/cache/iqrfgd2/iqrfRepoCache/packages/22/handler.hex
/var/cache/iqrfgd2/iqrfRepoCache/packages/22/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/41
/var/cache/iqrfgd2/iqrfRepoCache/packages/41/handler.hex
/var/cache/iqrfgd2/iqrfRepoCache/packages/41/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/33
/var/cache/iqrfgd2/iqrfRepoCache/packages/33/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/21
/var/cache/iqrfgd2/iqrfRepoCache/packages/21/handler.hex
/var/cache/iqrfgd2/iqrfRepoCache/packages/21/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/56
/var/cache/iqrfgd2/iqrfRepoCache/packages/56/handler.hex
/var/cache/iqrfgd2/iqrfRepoCache/packages/56/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/40
/var/cache/iqrfgd2/iqrfRepoCache/packages/40/handler.hex
/var/cache/iqrfgd2/iqrfRepoCache/packages/40/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/20
/var/cache/iqrfgd2/iqrfRepoCache/packages/20/handler.hex
/var/cache/iqrfgd2/iqrfRepoCache/packages/20/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/50
/var/cache/iqrfgd2/iqrfRepoCache/packages/50/handler.hex
/var/cache/iqrfgd2/iqrfRepoCache/packages/50/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/18
/var/cache/iqrfgd2/iqrfRepoCache/packages/18/handler.hex
/var/cache/iqrfgd2/iqrfRepoCache/packages/18/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/35
/var/cache/iqrfgd2/iqrfRepoCache/packages/35/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/44
/var/cache/iqrfgd2/iqrfRepoCache/packages/44/handler.hex
/var/cache/iqrfgd2/iqrfRepoCache/packages/44/data.json
/var/cache/iqrfgd2/iqrfRepoCache/packages/46
/var/cache/iqrfgd2/iqrfRepoCache/packages/46/handler.hex
/var/cache/iqrfgd2/iqrfRepoCache/packages/46/data.json
/var/cache/iqrfgd2/iqrfRepoCache/standards
/var/cache/iqrfgd2/iqrfRepoCache/standards/6
/var/cache/iqrfgd2/iqrfRepoCache/standards/6/0
/var/cache/iqrfgd2/iqrfRepoCache/standards/6/0/data.json
/var/cache/iqrfgd2/iqrfRepoCache/standards/6/data.json
/var/cache/iqrfgd2/iqrfRepoCache/standards/2
/var/cache/iqrfgd2/iqrfRepoCache/standards/2/1
/var/cache/iqrfgd2/iqrfRepoCache/standards/2/1/data.json
/var/cache/iqrfgd2/iqrfRepoCache/standards/2/0
/var/cache/iqrfgd2/iqrfRepoCache/standards/2/0/data.json
/var/cache/iqrfgd2/iqrfRepoCache/standards/2/data.json
/var/cache/iqrfgd2/iqrfRepoCache/standards/9
/var/cache/iqrfgd2/iqrfRepoCache/standards/9/0
/var/cache/iqrfgd2/iqrfRepoCache/standards/9/0/data.json
/var/cache/iqrfgd2/iqrfRepoCache/standards/9/data.json
/var/cache/iqrfgd2/iqrfRepoCache/standards/10
/var/cache/iqrfgd2/iqrfRepoCache/standards/10/0
/var/cache/iqrfgd2/iqrfRepoCache/standards/10/0/data.json
/var/cache/iqrfgd2/iqrfRepoCache/standards/10/data.json
/var/cache/iqrfgd2/iqrfRepoCache/standards/13
/var/cache/iqrfgd2/iqrfRepoCache/standards/13/0
/var/cache/iqrfgd2/iqrfRepoCache/standards/13/0/data.json
/var/cache/iqrfgd2/iqrfRepoCache/standards/13/data.json
/var/cache/iqrfgd2/iqrfRepoCache/standards/11
/var/cache/iqrfgd2/iqrfRepoCache/standards/11/0
/var/cache/iqrfgd2/iqrfRepoCache/standards/11/0/data.json
/var/cache/iqrfgd2/iqrfRepoCache/standards/11/data.json
/var/cache/iqrfgd2/iqrfRepoCache/standards/94
/var/cache/iqrfgd2/iqrfRepoCache/standards/94/14
/var/cache/iqrfgd2/iqrfRepoCache/standards/94/14/data.json
/var/cache/iqrfgd2/iqrfRepoCache/standards/94/data.json
/var/cache/iqrfgd2/iqrfRepoCache/standards/94/15
/var/cache/iqrfgd2/iqrfRepoCache/standards/94/15/data.json
/var/cache/iqrfgd2/iqrfRepoCache/standards/7
/var/cache/iqrfgd2/iqrfRepoCache/standards/7/0
/var/cache/iqrfgd2/iqrfRepoCache/standards/7/0/data.json
/var/cache/iqrfgd2/iqrfRepoCache/standards/7/data.json
/var/cache/iqrfgd2/iqrfRepoCache/standards/4
/var/cache/iqrfgd2/iqrfRepoCache/standards/4/0
/var/cache/iqrfgd2/iqrfRepoCache/standards/4/0/data.json
/var/cache/iqrfgd2/iqrfRepoCache/standards/4/data.json
/var/cache/iqrfgd2/iqrfRepoCache/standards/5
/var/cache/iqrfgd2/iqrfRepoCache/standards/5/0
/var/cache/iqrfgd2/iqrfRepoCache/standards/5/0/data.json
/var/cache/iqrfgd2/iqrfRepoCache/standards/5/data.json
/var/cache/iqrfgd2/iqrfRepoCache/standards/3
/var/cache/iqrfgd2/iqrfRepoCache/standards/3/0
/var/cache/iqrfgd2/iqrfRepoCache/standards/3/0/data.json
/var/cache/iqrfgd2/iqrfRepoCache/standards/3/data.json
/var/cache/iqrfgd2/iqrfRepoCache/standards/12
/var/cache/iqrfgd2/iqrfRepoCache/standards/12/0
/var/cache/iqrfgd2/iqrfRepoCache/standards/12/0/data.json
/var/cache/iqrfgd2/iqrfRepoCache/standards/12/data.json
/var/cache/iqrfgd2/iqrfRepoCache/standards/1
/var/cache/iqrfgd2/iqrfRepoCache/standards/1/0
/var/cache/iqrfgd2/iqrfRepoCache/standards/1/0/data.json
/var/cache/iqrfgd2/iqrfRepoCache/standards/1/data.json
/var/cache/iqrfgd2/iqrfRepoCache/standards/8
/var/cache/iqrfgd2/iqrfRepoCache/standards/8/0
/var/cache/iqrfgd2/iqrfRepoCache/standards/8/0/data.json
/var/cache/iqrfgd2/iqrfRepoCache/standards/8/data.json
/var/cache/iqrfgd2/iqrfRepoCache/standards/0
/var/cache/iqrfgd2/iqrfRepoCache/standards/0/1
/var/cache/iqrfgd2/iqrfRepoCache/standards/0/1/data.json
/var/cache/iqrfgd2/iqrfRepoCache/standards/0/0
/var/cache/iqrfgd2/iqrfRepoCache/standards/0/0/data.json
/var/cache/iqrfgd2/iqrfRepoCache/standards/0/data.json
/var/cache/iqrfgd2/iqrfRepoCache/standards/data.json
/var/cache/iqrfgd2/iqrfRepoCache/standards/113
/var/cache/iqrfgd2/iqrfRepoCache/standards/113/5
/var/cache/iqrfgd2/iqrfRepoCache/standards/113/5/data.json
/var/cache/iqrfgd2/iqrfRepoCache/standards/113/data.json
/var/cache/iqrfgd2/iqrfRepoCache/standards/255
/var/cache/iqrfgd2/iqrfRepoCache/standards/255/0
/var/cache/iqrfgd2/iqrfRepoCache/standards/255/0/data.json
/var/cache/iqrfgd2/iqrfRepoCache/standards/255/data.json
/var/cache/iqrfgd2/iqrfRepoCache/standards/-1
/var/cache/iqrfgd2/iqrfRepoCache/standards/-1/0
/var/cache/iqrfgd2/iqrfRepoCache/standards/-1/0/data.json
/var/cache/iqrfgd2/iqrfRepoCache/standards/-1/data.json
/var/cache/iqrfgd2/iqrfRepoCache/standards/75
/var/cache/iqrfgd2/iqrfRepoCache/standards/75/4
/var/cache/iqrfgd2/iqrfRepoCache/standards/75/4/data.json
/var/cache/iqrfgd2/iqrfRepoCache/standards/75/data.json
/var/cache/iqrfgd2/iqrfRepoCache/manufacturers
/var/cache/iqrfgd2/iqrfRepoCache/manufacturers/data.json
```
