namespace TestApiApp
{
  #region usings

  using System;
  using System.Collections.Generic;
  using System.Dynamic;
  using System.Net;
  using System.Text;
  using System.Threading;
  using System.IO;
  using System.Reflection;

  using WebSocketSharp;
  using WebSocketSharp.Net;

  using Newtonsoft.Json;
  using Newtonsoft.Json.Converters;

  using uPLibrary.Networking.M2Mqtt;
  using uPLibrary.Networking.M2Mqtt.Messages;

  using NJsonSchema;
  using NJsonSchema.CodeGeneration.CSharp;

  //IqrfRaw APIs 
  using iqrfRaw_Request;
  using iqrfRaw_Response;
  using iqrfRawHdp_Request;
  using iqrfRawHdp_Response;

  //IqrfEmbed APIs 
  using iqrfEmbedExplore_Enumerate_Request;
  using iqrfEmbedExplore_Enumerate_Response;
  using iqrfEmbedExplore_PeripheralInformation_Request;
  using iqrfEmbedExplore_PeripheralInformation_Response;
  using iqrfEmbedExplore_MorePeripheralsInformation_Request;
  using iqrfEmbedExplore_MorePeripheralsInformation_Response;

  using iqrfEmbedCoordinator_AddrInfo_Request;
  using iqrfEmbedCoordinator_AddrInfo_Response;
  using iqrfEmbedCoordinator_AuthorizeBond_Request;
  using iqrfEmbedCoordinator_AuthorizeBond_Response;
  using iqrfEmbedCoordinator_Backup_Request;
  using iqrfEmbedCoordinator_Backup_Response;
  using iqrfEmbedCoordinator_BondedDevices_Request;
  using iqrfEmbedCoordinator_BondedDevices_Response;
  using iqrfEmbedCoordinator_BondNode_Request;
  using iqrfEmbedCoordinator_BondNode_Response;
  using iqrfEmbedCoordinator_ClearAllBonds_Request;
  using iqrfEmbedCoordinator_ClearAllBonds_Response;
  using iqrfEmbedCoordinator_ClearRemotelyBondedMid_Request;
  using iqrfEmbedCoordinator_ClearRemotelyBondedMid_Response;
  using iqrfEmbedCoordinator_DiscoveredDevices_Request;
  using iqrfEmbedCoordinator_DiscoveredDevices_Response;
  using iqrfEmbedCoordinator_Discovery_Request;
  using iqrfEmbedCoordinator_Discovery_Response;
  using iqrfEmbedCoordinator_DiscoveryData_Request;
  using iqrfEmbedCoordinator_DiscoveryData_Response;
  using iqrfEmbedCoordinator_EnableRemoteBonding_Request;
  using iqrfEmbedCoordinator_EnableRemoteBonding_Response;
  using iqrfEmbedCoordinator_ReadRemotelyBondedMid_Request;
  using iqrfEmbedCoordinator_ReadRemotelyBondedMid_Response;
  using iqrfEmbedCoordinator_RebondNode_Request;
  using iqrfEmbedCoordinator_RebondNode_Response;
  using iqrfEmbedCoordinator_RemoveBond_Request;
  using iqrfEmbedCoordinator_RemoveBond_Response;
  using iqrfEmbedCoordinator_Restore_Request;
  using iqrfEmbedCoordinator_Restore_Response;
  using iqrfEmbedCoordinator_SetDpaParams_Request;
  using iqrfEmbedCoordinator_SetDpaParams_Response;
  using iqrfEmbedCoordinator_SetHops_Request;
  using iqrfEmbedCoordinator_SetHops_Response;
  using iqrfEmbedCoordinator_SmartConnect_Request;
  using iqrfEmbedCoordinator_SmartConnect_Response;

  using iqrfEmbedNode_Read_Request;
  using iqrfEmbedNode_Read_Response;
  using iqrfEmbedNode_Backup_Request;
  using iqrfEmbedNode_Backup_Response;
  using iqrfEmbedNode_Restore_Request;
  using iqrfEmbedNode_Restore_Response;
  using iqrfEmbedNode_ReadRemotelyBondedMid_Request;
  using iqrfEmbedNode_ReadRemotelyBondedMid_Response;
  using iqrfEmbedNode_ClearRemotelyBondedMid_Request;
  using iqrfEmbedNode_ClearRemotelyBondedMid_Response;
  using iqrfEmbedNode_EnableRemoteBonding_Request;
  using iqrfEmbedNode_EnableRemoteBonding_Response;
  using iqrfEmbedNode_RemoveBond_Request;
  using iqrfEmbedNode_RemoveBond_Response;

  using iqrfEmbedOs_Batch_Request;
  using iqrfEmbedOs_Batch_Response;
  using iqrfEmbedOs_BatchRequest;
  using iqrfEmbedOs_LoadCode_Request;
  using iqrfEmbedOs_LoadCode_Response;
  using iqrfEmbedOs_Read_Request;
  using iqrfEmbedOs_Read_Response;
  using iqrfEmbedOs_ReadCfg_Request;
  using iqrfEmbedOs_ReadCfg_Response;
  using iqrfEmbedOs_Reset_Request;
  using iqrfEmbedOs_Reset_Response;
  using iqrfEmbedOs_Restart_Request;
  using iqrfEmbedOs_Restart_Response;
  using iqrfEmbedOs_Rfpgm_Request;
  using iqrfEmbedOs_Rfpgm_Response;
  using iqrfEmbedOs_SelectiveBatch_Request;
  using iqrfEmbedOs_SelectiveBatch_Response;
  using iqrfEmbedOs_SetSecurity_Request;
  using iqrfEmbedOs_SetSecurity_Response;
  using iqrfEmbedOs_Sleep_Request;
  using iqrfEmbedOs_Sleep_Response;
  using iqrfEmbedOs_WriteCfg_Request;
  using iqrfEmbedOs_WriteCfg_Response;
  using iqrfEmbedOs_WriteCfgByte_Request;
  using iqrfEmbedOs_WriteCfgByte_Response;
  using iqrfEmbedOs_WriteCfgTriplet;

  using iqrfEmbedEeprom_Read_Request;
  using iqrfEmbedEeprom_Read_Response;
  using iqrfEmbedEeprom_Write_Request;
  using iqrfEmbedEeprom_Write_Response;

  using iqrfEmbedEeeprom_Read_Request;
  using iqrfEmbedEeeprom_Read_Response;
  using iqrfEmbedEeeprom_Write_Request;
  using iqrfEmbedEeeprom_Write_Response;

  using iqrfEmbedRam_Read_Request;
  using iqrfEmbedRam_Read_Response;
  using iqrfEmbedRam_Write_Request;
  using iqrfEmbedRam_Write_Response;

  using iqrfEmbedLedr_Set_Request;
  using iqrfEmbedLedr_Set_Response;
  using iqrfEmbedLedr_Get_Request;
  using iqrfEmbedLedr_Get_Response;
  using iqrfEmbedLedr_Pulse_Request;
  using iqrfEmbedLedr_Pulse_Response;
  using iqrfEmbedLedg_Set_Request;
  using iqrfEmbedLedg_Set_Response;
  using iqrfEmbedLedg_Get_Request;
  using iqrfEmbedLedg_Get_Response;
  using iqrfEmbedLedg_Pulse_Request;
  using iqrfEmbedLedg_Pulse_Response;

  using iqrfEmbedThermometer_Read_Request;
  using iqrfEmbedThermometer_Read_Response;

  using iqrfEmbedSpi_WriteRead_Request;
  using iqrfEmbedSpi_WriteRead_Response;

  using iqrfEmbedUart_Open_Request;
  using iqrfEmbedUart_Open_Response;
  using iqrfEmbedUart_Close_Request;
  using iqrfEmbedUart_Close_Response;
  using iqrfEmbedUart_WriteRead_Request;
  using iqrfEmbedUart_WriteRead_Response;
  using iqrfEmbedUart_ClearWriteRead_Request;
  using iqrfEmbedUart_ClearWriteRead_Response;

  using iqrfEmbedIo_Direction_Request;
  using iqrfEmbedIo_Direction_Response;
  using iqrfEmbedIo_Set_Request;
  using iqrfEmbedIo_Set_Response;
  using iqrfEmbedIo_Get_Request;
  using iqrfEmbedIo_Get_Response;

  using iqrfEmbedFrc_Send_Request;
  using iqrfEmbedFrc_Send_Response;
  using iqrfEmbedFrc_SendSelective_Request;
  using iqrfEmbedFrc_SendSelective_Response;
  using iqrfEmbedFrc_ExtraResult_Request;
  using iqrfEmbedFrc_ExtraResult_Response;
  using iqrfEmbedFrc_SetParams_Request;
  using iqrfEmbedFrc_SetParams_Response;

  using iqrfSensor_Enumerate_Request;
  using iqrfSensor_Enumerate_Response;
  using iqrfSensor_ReadSensorsWithTypes_Request;
  using iqrfSensor_ReadSensorsWithTypes_Response;
  using iqrfSensor_Frc_Request;
  using iqrfSensor_Frc_Response;

  using iqrfBinaryoutput_Enumerate_Request;
  using iqrfBinaryoutput_Enumerate_Response;
  using iqrfBinaryoutput_SetOutput_Request;
  using iqrfBinaryoutput_SetOutput_Response;

  using iqrfLight_Enumerate_Request;
  using iqrfLight_Enumerate_Response;
  using iqrfLight_SetPower_Request;
  using iqrfLight_SetPower_Response;
  using iqrfLight_IncrementPower_Request;
  using iqrfLight_IncrementPower_Response;
  using iqrfLight_DecrementPower_Request;
  using iqrfLight_DecrementPower_Response;

  // IqmeshNetwork APIs
  using iqmeshNetwork_BondNodeLocal_Request;
  using iqmeshNetwork_BondNodeLocal_Response;
  using iqmeshNetwork_SmartConnect_Request;
  using iqmeshNetwork_SmartConnect_Response;
  using iqmeshNetwork_EnumerateDevice_Request;
  using iqmeshNetwork_EnumerateDevice_Response;
  using iqmeshNetwork_OtaUpload_Request;
  using iqmeshNetwork_OtaUpload_Response;
  using iqmeshNetwork_ReadTrConf_Request;
  using iqmeshNetwork_ReadTrConf_Response;
  using iqmeshNetwork_WriteTrConf_Request;
  using iqmeshNetwork_WriteTrConf_Response;

  // Management APIs
  using mngDaemon_Mode_Request;
  using mngDaemon_Mode_Response;
  using mngDaemon_Exit_Request;
  using mngDaemon_Exit_Response;

  // Scheduler APIs
  using mngScheduler_List_Request;
  using mngScheduler_List_Response;
  using mngScheduler_AddTask_Request;
  using mngScheduler_AddTask_Response;
  using mngScheduler_PeriodicTask_Request;
  using mngScheduler_PeriodicTask_Response;
  using mngScheduler_RemoveTask_Request;
  using mngScheduler_RemoveTask_Response;
  using mngScheduler_SchedulerMessagingTask;
  #endregion

  class Program
  {
    /// <summary>
    /// MQTT and WebSocket timeout [ms]
    /// </summary>
    const int Timeout = 20 * 1000;

    /// <summary>
    /// GW IP address
    /// </summary>
    //const string IqrfGwDeamonIP = "192.168.1.100";
    const string IqrfGwDeamonIP = "10.11.16.79";


    /// <summary>
    /// Log transaction
    /// </summary>
    /// <param name="mType"></param>
    /// <param name="data"></param>
    private static void logTransaction(string mType, string prefix, string data)
    {
      System.IO.File.AppendAllText(@"..\..\log\" + mType + ".log", prefix + DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss.FFF") + "\r\n" + data + "\r\n");
    }

    /// <summary>
    /// Make example file
    /// </summary>
    /// <param name="mType"></param>
    /// <param name="direction"></param>
    /// <param name="data"></param>
    private static void logExample(string mType, string direction, string data)
    {
      System.IO.File.WriteAllText(@"..\..\log\" + mType + "-" + direction + "-1-0-0-example.json", data);
    }

    /// <summary>
    /// WebSocket request to iqrf-daemon
    /// </summary>
    /// <param name="url"></param>
    /// <param name="request"></param>
    /// <returns></returns>
    private static List<string> WebSocketRequest(string iqrfGwDaemonIP, dynamic request, int timeoutMs)
    {
      // Create WebSocket client
      WebSocket ws = new WebSocket("ws://" + iqrfGwDaemonIP + ":1338", new string[] { "iqrf" });
      try
      {
        int timeout = timeoutMs / 100;
        List<string> response = new List<string>();

        // Set MsgId
        string mtype = "";
        string strRequest = "";
        if (request is string)
        {
          // Get mType
          int startIndex = request.IndexOf("\"mType\": \"", 0) + "\"mType\": \"".Length;
          int length = request.IndexOf("\"", startIndex) - startIndex;
          mtype = request.Substring(startIndex, length);
          strRequest = request;
        }
        else
        {
          request.Data.MsgId = Guid.NewGuid().ToString();
          strRequest = ((object)request).JsonSerialize();
          mtype = request.MType.ToString();
          mtype = Char.ToLowerInvariant(mtype[0]) + mtype.Substring(1);
        }
        // Log the request
        logTransaction(mtype, "==================================\r\nSending to WebSocket: ", strRequest);
        logExample(mtype, "request", strRequest);

        // Add event to receive MQTT message
        ws.OnMessage += (sender, e) =>
        {
          // Log response
          logTransaction(mtype, "Received from WebSocket: ", e.Data);
          logExample(mtype, "response", e.Data);
          response.Add(e.Data);
          // Preset timeout for next possible response to 0.5 sec (5 * 100 ms)
          timeout = 500 / 100;
        };

        // Connect to iqrf-daemon
        ws.Connect();
        // Send the request
        ws.Send(strRequest);
        // Wait for response 
        do
        {
          Thread.Sleep(100);
        } while (--timeout != 0);

        // Log end of the transction
        logTransaction(mtype, "", "==================================\r\n");
        return response;
      }
      finally
      {
        ws.Close();
      }
    }

    /// <summary>
    /// MQTT request to iqrf-daemon
    /// </summary>
    /// <param name="iqrfGwDaemon"></param>
    /// <param name="request"></param>
    /// <returns></returns>
    private static List<string> MQTTRequest(string iqrfGwDaemon, dynamic request, int timeoutMs)
    {
      // Create MQTT client
      MqttClient mqttClient = new MqttClient(iqrfGwDaemon);
      // Connect to the MQTT broker
      mqttClient.Connect(Guid.NewGuid().ToString());
      try
      {
        // Initial timeout
        int timeout = timeoutMs / 100;
        List<string> response = new List<string>();

        // Set MsgId
        string mtype = "";
        string strRequest = "";
        if (request is string)
        {
          // Get mType
          int startIndex = request.IndexOf("\"mType\": \"", 0) + "\"mType\": \"".Length;
          int length = request.IndexOf("\"", startIndex) - startIndex;
          mtype = request.Substring(startIndex, length);
          strRequest = request;
        }
        else
        {
          request.Data.MsgId = Guid.NewGuid().ToString();
          strRequest = ((object)request).JsonSerialize();
          mtype = request.MType.ToString();
          mtype = Char.ToLowerInvariant(mtype[0]) + mtype.Substring(1);
        }

        // Log the request
        logTransaction(mtype, "==================================\r\nSending to MQTT: ", strRequest);
        logExample(mtype, "request", strRequest);

        // Add event to receive MQTT message
        mqttClient.MqttMsgPublishReceived += (sender, e) =>
        {
          // Log response
          string rsp = Encoding.UTF8.GetString(e.Message);
          logTransaction(mtype, "Received from MQTT: ", rsp);
          logExample(mtype, "response", rsp);
          response.Add(rsp);
          // Preset timeout for next possible response to 0.5 sec (5 * 100 ms)
          timeout = 500 / 100;
        };

        // Subscribe IQRF/DpaResponse at MQTT broker
        mqttClient.Subscribe(new string[] { "Iqrf/DpaResponse" }, new byte[] { MqttMsgBase.QOS_LEVEL_EXACTLY_ONCE });
        mqttClient.Publish("Iqrf/DpaRequest", Encoding.UTF8.GetBytes(strRequest), MqttMsgBase.QOS_LEVEL_EXACTLY_ONCE, true);
        // Wait for response 
        do
        {
          Thread.Sleep(100);
        } while (--timeout != 0);

        // Log end of the transction
        logTransaction(mtype, "", "==================================\r\n");
        return response;
      }
      finally
      {
        mqttClient.Disconnect();
      }
    }

    /// <summary>
    /// Main function
    /// </summary>
    static void Main ()
    {    
      try
      {
        // Create request 

        /* RAW */
        /*
        IqrfRaw_Request request = new IqrfRaw_Request();
        request.Data.MsgId = "testRaw";
        request.Data.Req.RData = "00.00.06.03.FF.FF";
        request.Data.Timeout = 1000;
        request.Data.ReturnVerbose = true;
        */
        /* RAWHDP */

        /*
        IqrfRawHdp_Request request = new IqrfRawHdp_Request();
        request.Data.MsgId = "testRawHdp";
        request.Data.Timeout = 1000;
        request.Data.Req.NAdr = 1;
        request.Data.Req.PNum = 6;
        request.Data.Req.PCmd = 3;
        request.Data.Req.PData = new System.Collections.ObjectModel.ObservableCollection<int> {};
        request.Data.ReturnVerbose = true;
        */

        /* EXPLORE */

        /*
        IqrfEmbedExplore_Enumerate_Request request = new IqrfEmbedExplore_Enumerate_Request();
        request.Data.MsgId = "testEmbedExplore";
        request.Data.Timeout = 1000;
        request.Data.Req.NAdr = 1;
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedExplore_PeripheralInformation_Request request = new IqrfEmbedExplore_PeripheralInformation_Request();
        request.Data.MsgId = "testEmbedExplore";
        request.Data.Timeout = 1000;
        request.Data.Req.NAdr = 1;
        request.Data.Req.Param.Per = 1;
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedExplore_MorePeripheralsInformation_Request request = new IqrfEmbedExplore_MorePeripheralsInformation_Request();
        request.Data.MsgId = "testEmbedExplore";
        request.Data.Timeout = 1000;
        request.Data.Req.NAdr = 1;
        request.Data.Req.Param.Per = 1;
        request.Data.ReturnVerbose = true;
        */

        /* COORDINATOR */

        /*
        IqrfEmbedCoordinator_AddrInfo_Request request = new IqrfEmbedCoordinator_AddrInfo_Request();
        request.Data.MsgId = "testEmbedCoordinator";
        request.Data.Req.NAdr = 0;
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedCoordinator_AuthorizeBond_Request request = new IqrfEmbedCoordinator_AuthorizeBond_Request();
        request.Data.MsgId = "testEmbedCoordinator";
        request.Data.Req.NAdr = 0;
        request.Data.Req.Param.ReqAddr = 3;
        request.Data.Req.Param.Mid = 0x00000000;
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedCoordinator_Backup_Request request = new IqrfEmbedCoordinator_Backup_Request();
        request.Data.MsgId = "testEmbedCoordinator";
        request.Data.Req.NAdr = 0;
        request.Data.Req.Param.Index = 0;
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedCoordinator_BondedDevices_Request request = new IqrfEmbedCoordinator_BondedDevices_Request();
        request.Data.MsgId = "testEmbedCoordinator";
        request.Data.Req.NAdr = 0;
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedCoordinator_BondNode_Request request = new IqrfEmbedCoordinator_BondNode_Request();
        request.Data.MsgId = "testEmbedCoordinator";
        request.Data.Timeout = 11000;
        request.Data.Req.NAdr = 0;
        request.Data.Req.Param.ReqAddr = 1;
        request.Data.Req.Param.BondingMask = 0;
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedCoordinator_ClearAllBonds_Request request = new IqrfEmbedCoordinator_ClearAllBonds_Request();
        request.Data.MsgId = "testEmbedCoordinator";
        request.Data.Req.NAdr = 0;
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedCoordinator_ClearRemotelyBondedMid_Request request = new IqrfEmbedCoordinator_ClearRemotelyBondedMid_Request();
        request.Data.MsgId = "testEmbedCoordinator";
        request.Data.Req.NAdr = 0;
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedCoordinator_DiscoveredDevices_Request request = new IqrfEmbedCoordinator_DiscoveredDevices_Request();
        request.Data.MsgId = "testEmbedCoordinator";
        request.Data.Req.NAdr = 0;
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedCoordinator_Discovery_Request request = new IqrfEmbedCoordinator_Discovery_Request();
        request.Data.MsgId = "testEmbedCoordinator";
        //request.Data.Timeout = 0;
        request.Data.Req.NAdr = 0;
        request.Data.Req.Param.MaxAddr = 0;
        request.Data.Req.Param.TxPower = 6;
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedCoordinator_DiscoveryData_Request request = new IqrfEmbedCoordinator_DiscoveryData_Request();
        request.Data.MsgId = "testEmbedCoordinator";
        request.Data.Req.NAdr = 0;
        request.Data.Req.Param.Address = 0;
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedCoordinator_EnableRemoteBonding_Request request = new IqrfEmbedCoordinator_EnableRemoteBonding_Request();
        request.Data.MsgId = "testEmbedCoordinator";
        request.Data.Req.NAdr = 0;
        request.Data.Req.Param.BondingMask = 0;
        request.Data.Req.Param.Control = 0;
        request.Data.Req.Param.UserData = new System.Collections.ObjectModel.ObservableCollection<int> { 0, 1, 2, 3 };
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedCoordinator_ReadRemotelyBondedMid_Request request = new IqrfEmbedCoordinator_ReadRemotelyBondedMid_Request();
        request.Data.MsgId = "testEmbedCoordinator";
        request.Data.Req.NAdr = 0;
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedCoordinator_RebondNode_Request request = new IqrfEmbedCoordinator_RebondNode_Request();
        request.Data.MsgId = "testEmbedCoordinator";
        request.Data.Req.NAdr = 0;
        request.Data.Req.Param.BondAddr = 6;
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedCoordinator_RemoveBond_Request request = new IqrfEmbedCoordinator_RemoveBond_Request();
        request.Data.MsgId = "testEmbedCoordinator";
        request.Data.Req.NAdr = 0;
        request.Data.Req.Param.BondAddr = 1;
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedCoordinator_Restore_Request request = new IqrfEmbedCoordinator_Restore_Request();
        request.Data.MsgId = "testEmbedCoordinator";
        request.Data.Req.NAdr = 0;
        request.Data.Req.Param.NetworkData = new System.Collections.ObjectModel.ObservableCollection<int> { 237, 96, 20, 81, 130, 222, 124, 114, 222, 248, 144, 163, 238, 130, 117, 237, 144, 84, 167, 183, 129, 51, 202, 101, 112, 7, 123, 36, 3, 67, 8, 30, 130, 98, 206, 11, 16, 43, 35, 67, 163, 88, 214, 166, 140, 145, 17, 104, 168 };
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedCoordinator_SetDpaParams_Request request = new IqrfEmbedCoordinator_SetDpaParams_Request();
        request.Data.MsgId = "testEmbedCoordinator";
        request.Data.Req.NAdr = 0;
        request.Data.Req.Param.DpaParam = 0;
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedCoordinator_SetHops_Request request = new IqrfEmbedCoordinator_SetHops_Request();
        request.Data.MsgId = "testEmbedCoordinator";
        request.Data.Req.NAdr = 0;
        request.Data.Req.Param.RequestHops = 0xff;
        request.Data.Req.Param.ResponseHops = 0xff;
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedCoordinator_SmartConnect_Request request = new IqrfEmbedCoordinator_SmartConnect_Request();
        request.Data.MsgId = "testEmbedCoordinator";
        request.Data.Req.NAdr = 0;
        request.Data.Req.Param.ReqAddr = 1;
        request.Data.Req.Param.BondingTestRetries = 1;
        request.Data.Req.Param.Ibk = new System.Collections.ObjectModel.ObservableCollection<int> { 0x9A,  0x1E, 0x79, 0xB2, 0x3C, 0xDC, 0x39, 0xD6, 0xCC, 0x5E, 0x64, 0xC0, 0x6B, 0xBF, 0xFD, 0xA7 };
        request.Data.Req.Param.Mid = new System.Collections.ObjectModel.ObservableCollection<int> { 0x81, 0x10, 0x2D, 0x54 };
        request.Data.Req.Param.VirtualDeviceAddress = 0xFF;
        request.Data.Req.Param.UserData = new System.Collections.ObjectModel.ObservableCollection<int> {};
        request.Data.ReturnVerbose = true;
        */

        /* NODE */

        /*
        IqrfEmbedNode_Read_Request request = new IqrfEmbedNode_Read_Request();
        request.Data.MsgId = "testEmbedNode";
        request.Data.Req.NAdr = 1;
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedNode_Backup_Request request = new IqrfEmbedNode_Backup_Request();
        request.Data.MsgId = "testEmbedNode";
        request.Data.Req.NAdr = 1;
        request.Data.Req.Param.Index = 0;
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedNode_Restore_Request request = new IqrfEmbedNode_Restore_Request();
        request.Data.MsgId = "testEmbedNode";
        request.Data.Req.NAdr = 1;
        request.Data.Req.Param.BackupData = new System.Collections.ObjectModel.ObservableCollection<int> { 141, 108, 81, 171, 106, 240, 247, 200, 61, 80, 54, 94, 38, 225, 61, 90, 70, 131, 32, 188, 213, 35, 98, 116, 0, 245, 128, 226, 180, 99, 166, 173, 223, 184, 90, 121, 173, 244, 18, 91, 85, 6, 169, 23, 234, 27, 225, 41, 6 };
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedNode_ReadRemotelyBondedMid_Request request = new IqrfEmbedNode_ReadRemotelyBondedMid_Request();
        request.Data.MsgId = "testEmbedNode";
        request.Data.Req.NAdr = 1;
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedNode_ClearRemotelyBondedMid_Request request = new IqrfEmbedNode_ClearRemotelyBondedMid_Request();
        request.Data.MsgId = "testEmbedNode";
        request.Data.Req.NAdr = 1;
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedNode_EnableRemoteBonding_Request request = new IqrfEmbedNode_EnableRemoteBonding_Request();
        request.Data.MsgId = "testEmbedNode";
        request.Data.Req.NAdr = 1;
        request.Data.Req.Param.BondingMask = 0;
        request.Data.Req.Param.Control = 0;
        request.Data.Req.Param.UserData = new System.Collections.ObjectModel.ObservableCollection<int> { 0, 1, 2, 3 };
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedNode_RemoveBond_Request request = new IqrfEmbedNode_RemoveBond_Request();
        request.Data.MsgId = "testEmbedNode";
        request.Data.Req.NAdr = 1;
        request.Data.ReturnVerbose = true;
        */

        /* OS */

        /*
        IqrfEmbedOs_BatchRequest requestLedrOn = new IqrfEmbedOs_BatchRequest();
        requestLedrOn.Pnum = "06";
        requestLedrOn.Pcmd = "03";
        requestLedrOn.Hwpid = "ffff";
        
        IqrfEmbedOs_Batch_Request request = new IqrfEmbedOs_Batch_Request();
        request.Data.MsgId = "testEmbedOs";
        request.Data.Req.NAdr = 0;
        request.Data.Req.Param.Requests = new System.Collections.ObjectModel.ObservableCollection<object> { requestLedrPulse };
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedOs_LoadCode_Request request = new IqrfEmbedOs_LoadCode_Request();
        request.Data.MsgId = "testEmbedOs";
        request.Data.Req.NAdr = 1;
        request.Data.Req.Param.Flags = 0;
        request.Data.Req.Param.Address = 0;
        // Taken from example DPA techdoc 11.3
        request.Data.Req.Param.Length = 0x40;
        request.Data.Req.Param.CheckSum = 0xEA3A;
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedOs_Read_Request request = new IqrfEmbedOs_Read_Request();
        request.Data.MsgId = "testEmbedOs";
        request.Data.Req.NAdr = 0;
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedOs_ReadCfg_Request request = new IqrfEmbedOs_ReadCfg_Request();
        request.Data.MsgId = "testEmbedOs";
        request.Data.Req.NAdr = 0;
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedOs_Reset_Request request = new IqrfEmbedOs_Reset_Request();
        request.Data.MsgId = "testEmbedOs";
        request.Data.Req.NAdr = 1;
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedOs_Restart_Request request = new IqrfEmbedOs_Restart_Request();
        request.Data.MsgId = "testEmbedOs";
        request.Data.Req.NAdr = 1;
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedOs_Rfpgm_Request request = new IqrfEmbedOs_Rfpgm_Request();
        request.Data.MsgId = "testEmbedOs";
        request.Data.Req.NAdr = 1;
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedOs_BatchRequest requestLedrOn = new IqrfEmbedOs_BatchRequest();
        requestLedrOn.Pnum = "06";
        requestLedrOn.Pcmd = "01";
        requestLedrOn.Hwpid = "ffff";

        IqrfEmbedOs_SelectiveBatch_Request request = new IqrfEmbedOs_SelectiveBatch_Request();
        request.Data.MsgId = "testEmbedOs";
        request.Data.Req.NAdr = 0xff;
        request.Data.Req.Param.SelectedNodes = new System.Collections.ObjectModel.ObservableCollection<int> { 1, 2 };
        request.Data.Req.Param.Requests = new System.Collections.ObjectModel.ObservableCollection<object> { requestLedrOn };
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedOs_SetSecurity_Request request = new IqrfEmbedOs_SetSecurity_Request();
        request.Data.MsgId = "testEmbedOs";
        request.Data.Req.NAdr = 1;
        // 0 ... access password, 1 ... user key
        request.Data.Req.Param.Type = 1;
        request.Data.Req.Param.Data = new System.Collections.ObjectModel.ObservableCollection<int> { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedOs_Sleep_Request request = new IqrfEmbedOs_Sleep_Request();
        request.Data.MsgId = "testEmbedOs";
        request.Data.Req.NAdr = 2;
        request.Data.Req.Param.Time = 1;
        request.Data.Req.Param.Control = 0;
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedOs_WriteCfg_Request request = new IqrfEmbedOs_WriteCfg_Request();
        request.Data.MsgId = "testEmbedOs";
        request.Data.Req.NAdr = 0;
        // From ReadCfg
        request.Data.Req.Param.Checksum = 129;
        request.Data.Req.Param.Configuration = new System.Collections.ObjectModel.ObservableCollection<int> { 201, 18, 52, 52, 52, 30, 52, 55, 49, 50, 55, 52, 52, 52, 52, 52, 45, 54, 52, 52, 52, 52, 52, 52, 52, 52, 52, 52, 52, 55, 52 };
        request.Data.Req.Param.Rfpgm = 195;
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedOs_WriteCfgTriplet cfgTriplet = new IqrfEmbedOs_WriteCfgTriplet();
        cfgTriplet.Address = 0x08;
        cfgTriplet.Value = 0x03;
        cfgTriplet.Mask = 0x07;

        IqrfEmbedOs_WriteCfgByte_Request request = new IqrfEmbedOs_WriteCfgByte_Request();
        request.Data.MsgId = "testEmbedOs";
        request.Data.Req.NAdr = 0;
        request.Data.Req.Param.Bytes = new System.Collections.ObjectModel.ObservableCollection<object> { cfgTriplet };
        request.Data.ReturnVerbose = true;
        */

        /* EEPROM */

        /*
        IqrfEmbedEeprom_Write_Request request = new IqrfEmbedEeprom_Write_Request();
        request.Data.MsgId = "testEmbedEeprom";
        request.Data.Req.NAdr = 1;
        request.Data.Req.Param.Address = 0;
        request.Data.Req.Param.PData = new System.Collections.ObjectModel.ObservableCollection<int> { 0, 1, 2, 3, 4 };
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedEeprom_Read_Request request = new IqrfEmbedEeprom_Read_Request();
        request.Data.MsgId = "testEmbedEeprom";
        request.Data.Req.NAdr = 1;
        request.Data.Req.Param.Address= 0;
        request.Data.Req.Param.Len = 10;
        request.Data.ReturnVerbose = true;
        */

        /* EEEPROM */

        /*
        IqrfEmbedEeeprom_Write_Request request = new IqrfEmbedEeeprom_Write_Request();
        request.Data.MsgId = "testEmbedEeeprom";
        request.Data.Req.NAdr = 1;
        request.Data.Req.Param.Address = 0;
        request.Data.Req.Param.PData = new System.Collections.ObjectModel.ObservableCollection<int> { 0, 1, 2, 3, 4 };
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedEeeprom_Read_Request request = new IqrfEmbedEeeprom_Read_Request();
        request.Data.MsgId = "testEmbedEeeprom";
        request.Data.Req.NAdr = 1;
        request.Data.Req.Param.Address= 0;
        request.Data.Req.Param.Len = 10;
        request.Data.ReturnVerbose = true;
        */

        /* RAM */

        /*
        IqrfEmbedRam_Write_Request request = new IqrfEmbedRam_Write_Request();
        request.Data.MsgId = "testEmbedRam";
        request.Data.Req.NAdr = 1;
        request.Data.Req.Param.Address = 0;
        request.Data.Req.Param.PData = new System.Collections.ObjectModel.ObservableCollection<int> { 0, 1, 2, 3, 4 };
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedRam_Read_Request request = new IqrfEmbedRam_Read_Request();
        request.Data.MsgId = "testEmbedRam";
        request.Data.Req.NAdr = 1;
        request.Data.Req.Param.Address= 0;
        request.Data.Req.Param.Len = 10;
        request.Data.ReturnVerbose = true;
        */

        /* LED */

        /*
        IqrfEmbedLedr_Set_Request request = new IqrfEmbedLedr_Set_Request();
        request.Data.MsgId = "testEmbedLedr";
        request.Data.Req.NAdr = 1;
        request.Data.Req.Param.OnOff = true;
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedLedr_Get_Request request = new IqrfEmbedLedr_Get_Request();
        request.Data.MsgId = "testEmbedLedr";
        request.Data.Req.NAdr = 1;
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedLedr_Pulse_Request request = new IqrfEmbedLedr_Pulse_Request();
        request.Data.MsgId = "testEmbedLedr";
        request.Data.Req.NAdr = 1;
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedLedg_Set_Request request = new IqrfEmbedLedg_Set_Request();
        request.Data.MsgId = "testEmbedLedg";
        request.Data.Req.NAdr = 1;
        request.Data.Req.Param.OnOff = true;
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedLedg_Get_Request request = new IqrfEmbedLedg_Get_Request();
        request.Data.MsgId = "testEmbedLedg";
        request.Data.Req.NAdr = 1;
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedLedg_Pulse_Request request = new IqrfEmbedLedg_Pulse_Request();
        request.Data.MsgId = "testEmbedLedg";
        request.Data.Req.NAdr = 1;
        request.Data.ReturnVerbose = true;
        */

        /* THERMOMETER */

        /*
        IqrfEmbedThermometer_Read_Request request = new IqrfEmbedThermometer_Read_Request();
        request.Data.MsgId = "testEmbedThermometer";
        request.Data.Req.NAdr = 3;
        request.Data.ReturnVerbose = true;
        */

        /* SPI */

        /*
        IqrfEmbedSpi_WriteRead_Request request = new IqrfEmbedSpi_WriteRead_Request();
        request.Data.MsgId = "testEmbedSpi";
        request.Data.Req.NAdr = 3;
        request.Data.Req.Param.ReadTimeout = 1;
        request.Data.Req.Param.WrittenData = new System.Collections.ObjectModel.ObservableCollection<int> { 0, 1, 2, 3, 4 };
        request.Data.ReturnVerbose = true;
        */

        /* UART */

        /*
        IqrfEmbedUart_Open_Request request = new IqrfEmbedUart_Open_Request();
        request.Data.MsgId = "testEmbedUart";
        request.Data.Req.NAdr = 3;
        request.Data.Req.Param.BaudRate = 0x04;
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedUart_WriteRead_Request request = new IqrfEmbedUart_WriteRead_Request();
        request.Data.MsgId = "testEmbedUart";
        request.Data.Req.NAdr = 3;
        request.Data.Req.Param.ReadTimeout = 1;
        request.Data.Req.Param.WrittenData = new System.Collections.ObjectModel.ObservableCollection<int> { 0, 1, 2, 3, 4 };
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedUart_ClearWriteRead_Request request = new IqrfEmbedUart_ClearWriteRead_Request();
        request.Data.MsgId = "testEmbedUart";
        request.Data.Req.NAdr = 3;
        request.Data.Req.Param.ReadTimeout = 1;
        request.Data.Req.Param.WrittenData = new System.Collections.ObjectModel.ObservableCollection<int> { 0, 1, 2, 3, 4 };
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedUart_Close_Request request = new IqrfEmbedUart_Close_Request();
        request.Data.MsgId = "testEmbedUart";
        request.Data.Req.NAdr = 3;
        request.Data.ReturnVerbose = true;
        */

        /* IO */

        /*
        IqrfEmbedIo_Direction_Request request = new IqrfEmbedIo_Direction_Request();
        request.Data.MsgId = "testEmbedIo";
        request.Data.Req.NAdr = 1;
        iqrfEmbedIo_Direction_Request.Anonymous port = new iqrfEmbedIo_Direction_Request.Anonymous();
        port.Port = 0x00;  // trisa
        port.Mask = 0x04;  // pin.2
        port.Value = 0x00; // output
        request.Data.Req.Param.Ports = new System.Collections.ObjectModel.ObservableCollection<iqrfEmbedIo_Direction_Request.Anonymous> { port };
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedIo_Set_Request request = new IqrfEmbedIo_Set_Request();
        request.Data.MsgId = "testEmbedIo";
        request.Data.Req.NAdr = 1;
        iqrfEmbedIo_Set_Request.Anonymous port = new iqrfEmbedIo_Set_Request.Anonymous();
        port.Port = 0x00;  // porta
        port.Mask = 0x04;  // pin.2
        port.Value = 0x04; // log.1
        request.Data.Req.Param.Ports = new System.Collections.ObjectModel.ObservableCollection<iqrfEmbedIo_Set_Request.Anonymous> { port };
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedIo_Get_Request request = new IqrfEmbedIo_Get_Request();
        request.Data.MsgId = "testEmbedIo";
        request.Data.Req.NAdr = 1;
        request.Data.ReturnVerbose = true;
        */

        /* FRC */

        /*
        IqrfEmbedFrc_Send_Request request = new IqrfEmbedFrc_Send_Request();
        request.Data.MsgId = "testEmbedFrc";
        //request.Data.Timeout = 5000;
        request.Data.Req.NAdr = 0;
        request.Data.Req.Param.FrcCommand = 0x80;
        request.Data.Req.Param.UserData = new System.Collections.ObjectModel.ObservableCollection<int> { 0, 0 };
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedFrc_ExtraResult_Request request = new IqrfEmbedFrc_ExtraResult_Request();
        request.Data.MsgId = "testEmbedFrc";
        //request.Data.Timeout = 5000;
        request.Data.Req.NAdr = 0;
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedFrc_SendSelective_Request request = new IqrfEmbedFrc_SendSelective_Request();
        request.Data.MsgId = "testEmbedFrc";
        //request.Data.Timeout = 5000;
        request.Data.Req.NAdr = 0;
        request.Data.Req.Param.SelectedNodes = new System.Collections.ObjectModel.ObservableCollection<int> { 1, 2, 3 };
        request.Data.Req.Param.FrcCommand = 0x80;
        request.Data.Req.Param.UserData = new System.Collections.ObjectModel.ObservableCollection<int> { 0, 0 };
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedFrc_SetParams_Request request = new IqrfEmbedFrc_SetParams_Request();
        request.Data.MsgId = "testEmbedFrc";
        request.Data.Req.NAdr = 0;
        request.Data.Req.Param.FrcResponseTime = 0x00;
        request.Data.ReturnVerbose = true;
        */

        /* SENSOR */

        /*
        IqrfSensor_Enumerate_Request request = new IqrfSensor_Enumerate_Request();
        request.Data.MsgId = "testEmbedSensor";
        request.Data.Req.NAdr = 1;
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfSensor_ReadSensorsWithTypes_Request request = new IqrfSensor_ReadSensorsWithTypes_Request();
        request.Data.MsgId = "testEmbedSensor";
        request.Data.Req.NAdr = 1;
        //request.Data.Req.Param.SensorIndexes = -1;
        request.Data.Req.Param.SensorIndexes = new System.Collections.ObjectModel.ObservableCollection<int> { 0, 1 };
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfSensor_Frc_Request request = new IqrfSensor_Frc_Request();
        request.Data.MsgId = "testEmbedSensor";
        request.Data.Req.NAdr = 0;
        // refer to standard sensor documentation https://www.iqrfalliance.org/techDocs/
        request.Data.Req.Param.SensorType = 0x01;
        request.Data.Req.Param.SensorIndex = 0x01;
        // 2bits = 0x10, 1B = 0x90, 2B = 0xE0
        request.Data.Req.Param.FrcCommand = 0x90;

        //request.Data.Req.Param.SelectedNodes = new System.Collections.ObjectModel.ObservableCollection<int> { };
        SleepAfterFrc sleep = new SleepAfterFrc();
        // refer to Os sleep
        //sleep.Control = 0;
        //sleep.Time = 0;
        //request.Data.Req.Param.SleepAfterFrc = sleep;
        
        request.Data.ReturnVerbose = true;
        */

        /* BINARYOUTPUT */

        /*
        IqrfBinaryoutput_Enumerate_Request request = new IqrfBinaryoutput_Enumerate_Request();
        request.Data.MsgId = "testEmbedBout";
        request.Data.Req.NAdr = 2;
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfBinaryoutput_SetOutput_Request request = new IqrfBinaryoutput_SetOutput_Request();
        request.Data.MsgId = "testEmbedBout";
        request.Data.Req.NAdr = 2;

        iqrfBinaryoutput_SetOutput_Request.Anonymous binOuts = new iqrfBinaryoutput_SetOutput_Request.Anonymous();
        binOuts.Index = 0;
        binOuts.State = true;
        //binOuts.Time = 10;

        request.Data.Req.Param.BinOuts = new System.Collections.ObjectModel.ObservableCollection<iqrfBinaryoutput_SetOutput_Request.Anonymous> { binOuts };
        request.Data.ReturnVerbose = true;
        */

        /* LIGHT */

        /*
        IqrfLight_Enumerate_Request request = new IqrfLight_Enumerate_Request();
        request.Data.MsgId = "testEmbedLight";
        request.Data.Req.NAdr = 4;
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfLight_SetPower_Request request = new IqrfLight_SetPower_Request();
        request.Data.MsgId = "testEmbedLight";
        request.Data.Req.NAdr = 4;

        iqrfLight_SetPower_Request.Anonymous light = new iqrfLight_SetPower_Request.Anonymous();
        light.Index = 0;
        light.Power = 100;
        //light.Time = 10;

        request.Data.Req.Param.Lights = new System.Collections.ObjectModel.ObservableCollection<iqrfLight_SetPower_Request.Anonymous> { light };
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfLight_IncrementPower_Request request = new IqrfLight_IncrementPower_Request();
        request.Data.MsgId = "testEmbedLight";
        request.Data.Req.NAdr = 4;

        iqrfLight_IncrementPower_Request.Anonymous light = new iqrfLight_IncrementPower_Request.Anonymous();
        light.Index = 0;
        light.Power = 10;
        //light.Time = 10;

        request.Data.Req.Param.Lights = new System.Collections.ObjectModel.ObservableCollection<iqrfLight_IncrementPower_Request.Anonymous> { light };
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfLight_DecrementPower_Request request = new IqrfLight_DecrementPower_Request();
        request.Data.MsgId = "testEmbedLight";
        request.Data.Req.NAdr = 4;

        iqrfLight_DecrementPower_Request.Anonymous light = new iqrfLight_DecrementPower_Request.Anonymous();
        light.Index = 0;
        light.Power = 10;
        //light.Time = 10;

        request.Data.Req.Param.Lights = new System.Collections.ObjectModel.ObservableCollection<iqrfLight_DecrementPower_Request.Anonymous> { light };
        request.Data.ReturnVerbose = true;
        */

        /* IQMESH */

        /*
        IqmeshNetwork_BondNodeLocal_Request request = new IqmeshNetwork_BondNodeLocal_Request();
        request.Data.MsgId = "testIqmeshBondNodeLocal";
        request.Data.Repeat = 1;
        request.Data.Req.DeviceAddr = 4;
        request.Data.ReturnVerbose = true;
        */

        /*
        IqmeshNetwork_SmartConnect_Request request = new IqmeshNetwork_SmartConnect_Request();
        request.Data.MsgId = "testIqmeshBondNodeLocal";
        request.Data.Repeat = 1;
        request.Data.Req.DeviceAddr = 4;
        request.Data.ReturnVerbose = true;
        */

        /*
        IqmeshNetwork_EnumerateDevice_Request request = new IqmeshNetwork_EnumerateDevice_Request();
        request.Data.MsgId = "testIqmeshEnumerate";
        request.Data.Repeat = 1;
        request.Data.Req.DeviceAddr = 1;
        request.Data.ReturnVerbose = true;
        */

        /*
        IqmeshNetwork_ReadTrConf_Request request = new IqmeshNetwork_ReadTrConf_Request();
        request.Data.MsgId = "testIqmeshRead";
        request.Data.Repeat = 1;
        request.Data.Req.DeviceAddr = 1;
        request.Data.ReturnVerbose = true;
        */

        /*
        IqmeshNetwork_WriteTrConf_Request request = new IqmeshNetwork_WriteTrConf_Request();
        request.Data.MsgId = "testIqmeshWrite";
        request.Data.Repeat = 1;
        request.Data.Req.DeviceAddr = 1;
        request.Data.Req.TxPower = 3;
        request.Data.Req.RxFilter = 5;
        //request.Data.Req.SecurityPassword = "";
        //request.Data.Req.SecurityUserKey = "";
        request.Data.ReturnVerbose = true;
        */

        /*
        IqmeshNetwork_OtaUpload_Request request = new IqmeshNetwork_OtaUpload_Request();
        request.Data.MsgId = "testIqmeshOta";
        request.Data.Repeat = 1;
        request.Data.Req.DeviceAddr = 3;
        request.Data.Req.FileName = "CustomDpaHandler-LED-Red-On-7xD-V302-171116.hex";
        request.Data.Req.LoadingAction = "WithCodeLoading";
        request.Data.Req.StartMemAddr = 0;
        request.Data.ReturnVerbose = true;
        */

        /* MNG */

        /*
        MngDaemon_Mode_Request request = new MngDaemon_Mode_Request();
        request.Data.MsgId = "testMngMode";
        request.Data.Req.OperMode = ReqOperMode.Service;
        request.Data.ReturnVerbose = true;
        */

        MngDaemon_Exit_Request request = new MngDaemon_Exit_Request();
        request.Data.MsgId = "testMngExit";
        request.Data.Req.TimeToExit = 10000;
        request.Data.ReturnVerbose = true;

        /* SCHEDULER */

        /*
        MngScheduler_List_Request request = new MngScheduler_List_Request();
        request.Data.MsgId = "testSchedulerList";
        request.Data.Req.ClientId = "SchedulerMessaging";
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfRaw_Request rawReq = new IqrfRaw_Request();
        rawReq.Data.MsgId = "testRaw";
        rawReq.Data.Req.RData = "00.00.06.03.FF.FF";
        rawReq.Data.Timeout = 1000;
        rawReq.Data.ReturnVerbose = true;

        MngScheduler_SchedulerMessagingTask task = new MngScheduler_SchedulerMessagingTask();
        task.Messaging = MngScheduler_SchedulerMessagingTaskMessaging.WebsocketMessaging;
        task.Message = rawReq;

        MngScheduler_PeriodicTask_Request request = new MngScheduler_PeriodicTask_Request();
        request.Data.MsgId = "testSchedulerPeriodic";
        request.Data.Req.ClientId = "SchedulerMessaging";
        request.Data.Req.Task = task;
        request.Data.Req.TimePeriod = 10000;
        request.Data.ReturnVerbose = true;
        */

        /*
        MngScheduler_AddTask_Request request = new MngScheduler_AddTask_Request();
        request.Data.MsgId = "testSchedulerAdd";
        request.Data.Req.ClientId = "SchedulerMessaging";
        request.Data.Req.Task = task;
        request.Data.Req.CronTime = new System.Collections.ObjectModel.ObservableCollection<string> { "*\/10", "*", "*", "*", "*", "*", "*" };
        request.Data.ReturnVerbose = true;
        */

        /*
        MngScheduler_RemoveTask_Request request = new MngScheduler_RemoveTask_Request();
        request.Data.MsgId = "testSchedulerRemove";
        request.Data.Req.ClientId = "SchedulerMessaging";
        request.Data.Req.TaskId = 2146077740;
        request.Data.ReturnVerbose = true;
        */

        // Create response
        List< string> responseStr = new List<string>();

        // Send the request to iqrf-daemon via MQTT
        //responseStr = MQTTRequest( IqrfGwDeamonIP, request.ToJson(), Timeout );

        // Send the request to iqrf-daemon via WebSocket
        responseStr = WebSocketRequest( IqrfGwDeamonIP, request.JsonSerialize(), Timeout);

        // Get response

        /* RAW */

        //IqrfRaw_Response response = IqrfRaw_Response.FromJson( responseStr[0] );
        //IqrfRawHdp_Response response = IqrfRawHdp_Response.FromJson ( responseStr[0] );

        /* EXPLORE */

        //IqrfEmbedExplore_Enumerate_Response response = IqrfEmbedExplore_Enumerate_Response.FromJson( responseStr[0] );
        //IqrfEmbedExplore_PeripheralInformation_Response response = IqrfEmbedExplore_PeripheralInformation_Response.FromJson( responseStr[0] );
        //IqrfEmbedExplore_MorePeripheralsInformation_Response response = IqrfEmbedExplore_MorePeripheralsInformation_Response.FromJson( responseStr[0] );

        /* COORDINATOR */

        //IqrfEmbedCoordinator_AddrInfo_Response response = IqrfEmbedCoordinator_AddrInfo_Response.FromJson(responseStr[0]);
        //IqrfEmbedCoordinator_AuthorizeBond_Response response = IqrfEmbedCoordinator_AuthorizeBond_Response.FromJson(responseStr[0]);
        //IqrfEmbedCoordinator_Backup_Response response = IqrfEmbedCoordinator_Backup_Response.FromJson(responseStr[0]);
        //IqrfEmbedCoordinator_BondedDevices_Response response = IqrfEmbedCoordinator_BondedDevices_Response.FromJson(responseStr[0]);
        //IqrfEmbedCoordinator_BondNode_Response response = IqrfEmbedCoordinator_BondNode_Response.FromJson(responseStr[0]);
        //IqrfEmbedCoordinator_ClearAllBonds_Response response = IqrfEmbedCoordinator_ClearAllBonds_Response.FromJson(responseStr[0]);
        //IqrfEmbedCoordinator_ClearRemotelyBondedMid_Response response = IqrfEmbedCoordinator_ClearRemotelyBondedMid_Response.FromJson(responseStr[0]);
        //IqrfEmbedCoordinator_DiscoveredDevices_Response response = IqrfEmbedCoordinator_DiscoveredDevices_Response.FromJson(responseStr[0]);
        //IqrfEmbedCoordinator_Discovery_Response response = IqrfEmbedCoordinator_Discovery_Response.FromJson(responseStr[0]);
        //IqrfEmbedCoordinator_DiscoveryData_Response response = IqrfEmbedCoordinator_DiscoveryData_Response.FromJson(responseStr[0]);
        //IqrfEmbedCoordinator_EnableRemoteBonding_Response response = IqrfEmbedCoordinator_EnableRemoteBonding_Response.FromJson(responseStr[0]);
        //IqrfEmbedCoordinator_ReadRemotelyBondedMid_Response response = IqrfEmbedCoordinator_ReadRemotelyBondedMid_Response.FromJson(responseStr[0]);
        //IqrfEmbedCoordinator_RebondNode_Response response = IqrfEmbedCoordinator_RebondNode_Response.FromJson(responseStr[0]);
        //IqrfEmbedCoordinator_RemoveBond_Response response = IqrfEmbedCoordinator_RemoveBond_Response.FromJson(responseStr[0]);
        //IqrfEmbedCoordinator_Restore_Response response = IqrfEmbedCoordinator_Restore_Response.FromJson(responseStr[0]);
        //IqrfEmbedCoordinator_SetDpaParams_Response response = IqrfEmbedCoordinator_SetDpaParams_Response.FromJson(responseStr[0]);
        //IqrfEmbedCoordinator_SetHops_Response response = IqrfEmbedCoordinator_SetHops_Response.FromJson(responseStr[0]);
        //IqrfEmbedCoordinator_SmartConnect_Response response = IqrfEmbedCoordinator_SmartConnect_Response.FromJson(responseStr[0]);

        /* NODE */

        //IqrfEmbedNode_Read_Response response = IqrfEmbedNode_Read_Response.FromJson(responseStr[0]);
        //IqrfEmbedNode_Backup_Response response = IqrfEmbedNode_Backup_Response.FromJson(responseStr[0]);
        //IqrfEmbedNode_Restore_Response response = IqrfEmbedNode_Restore_Response.FromJson(responseStr[0]);
        //IqrfEmbedNode_ReadRemotelyBondedMid_Response response = IqrfEmbedNode_ReadRemotelyBondedMid_Response.FromJson(responseStr[0]);
        //IqrfEmbedNode_ClearRemotelyBondedMid_Response response = IqrfEmbedNode_ClearRemotelyBondedMid_Response.FromJson(responseStr[0]);
        //IqrfEmbedNode_EnableRemoteBonding_Response response = IqrfEmbedNode_EnableRemoteBonding_Response.FromJson(responseStr[0]);
        //IqrfEmbedNode_RemoveBond_Response response = IqrfEmbedNode_RemoveBond_Response.FromJson(responseStr[0]);

        /* OS */

        //IqrfEmbedOs_Batch_Response response = IqrfEmbedOs_Batch_Response.FromJson(responseStr[0]);
        //IqrfEmbedOs_LoadCode_Response response = IqrfEmbedOs_LoadCode_Response.FromJson(responseStr[0]);
        //IqrfEmbedOs_Read_Response response = IqrfEmbedOs_Read_Response.FromJson(responseStr[0]);
        //IqrfEmbedOs_ReadCfg_Response response = IqrfEmbedOs_ReadCfg_Response.FromJson(responseStr[0]);
        //IqrfEmbedOs_Reset_Response response = IqrfEmbedOs_Reset_Response.FromJson(responseStr[0]);
        //IqrfEmbedOs_Restart_Response response = IqrfEmbedOs_Restart_Response.FromJson(responseStr[0]);
        //IqrfEmbedOs_Rfpgm_Response response = IqrfEmbedOs_Rfpgm_Response.FromJson(responseStr[0]);
        //IqrfEmbedOs_SelectiveBatch_Response response = IqrfEmbedOs_SelectiveBatch_Response.FromJson(responseStr[0]);
        //IqrfEmbedOs_SetSecurity_Response response = IqrfEmbedOs_SetSecurity_Response.FromJson(responseStr[0]);
        //IqrfEmbedOs_Sleep_Response response = IqrfEmbedOs_Sleep_Response.FromJson(responseStr[0]);
        //IqrfEmbedOs_WriteCfg_Response response = IqrfEmbedOs_WriteCfg_Response.FromJson(responseStr[0]);
        //IqrfEmbedOs_WriteCfgByte_Response response = IqrfEmbedOs_WriteCfgByte_Response.FromJson(responseStr[0]);

        /* EEPROM */

        //IqrfEmbedEeprom_Write_Response response = IqrfEmbedEeprom_Write_Response.FromJson( responseStr[0] );
        //IqrfEmbedEeprom_Read_Response response = IqrfEmbedEeprom_Read_Response.FromJson( responseStr[0] );

        /* EEEPROM */

        //IqrfEmbedEeeprom_Write_Response response = IqrfEmbedEeeprom_Write_Response.FromJson(responseStr[0]);
        //IqrfEmbedEeeprom_Read_Response response = IqrfEmbedEeeprom_Read_Response.FromJson( responseStr[0] );

        /* RAM */

        //IqrfEmbedRam_Write_Response response = IqrfEmbedRam_Write_Response.FromJson(responseStr[0]);
        //IqrfEmbedRam_Read_Response response = IqrfEmbedRam_Read_Response.FromJson( responseStr[0] );

        /* LED */

        //IqrfEmbedLedr_Set_Response response = IqrfEmbedLedr_Set_Response.FromJson(responseStr[0]);
        //IqrfEmbedLedr_Get_Response response = IqrfEmbedLedr_Get_Response.FromJson( responseStr[0] );
        //IqrfEmbedLedr_Pulse_Response response = IqrfEmbedLedr_Pulse_Response.FromJson( responseStr[0] );

        //IqrfEmbedLedg_Set_Response response = IqrfEmbedLedg_Set_Response.FromJson(responseStr[0]);
        //IqrfEmbedLedg_Get_Response response = IqrfEmbedLedg_Get_Response.FromJson( responseStr[0] );
        //IqrfEmbedLedg_Pulse_Response response = IqrfEmbedLedg_Pulse_Response.FromJson( responseStr[0] );

        /* THERMOMETER */

        //IqrfEmbedThermometer_Read_Response response = IqrfEmbedThermometer_Read_Response.FromJson(responseStr[0]);

        /* SPI */

        //IqrfEmbedSpi_WriteRead_Response response = IqrfEmbedSpi_WriteRead_Response.FromJson(responseStr[0]);

        /* UART */

        //IqrfEmbedUart_Open_Response response = IqrfEmbedUart_Open_Response.FromJson(responseStr[0]);
        //IqrfEmbedUart_WriteRead_Response response = IqrfEmbedUart_WriteRead_Response.FromJson(responseStr[0]);
        //IqrfEmbedUart_ClearWriteRead_Response response = IqrfEmbedUart_ClearWriteRead_Response.FromJson(responseStr[0]);
        //IqrfEmbedUart_Close_Response response = IqrfEmbedUart_Close_Response.FromJson(responseStr[0]);

        /* IO */

        //IqrfEmbedIo_Direction_Response response = IqrfEmbedIo_Direction_Response.FromJson(responseStr[0]);
        //IqrfEmbedIo_Set_Response response = IqrfEmbedIo_Set_Response.FromJson(responseStr[0]);
        //IqrfEmbedIo_Get_Response response = IqrfEmbedIo_Get_Response.FromJson(responseStr[0]);

        /* FRC */

        //IqrfEmbedFrc_Send_Response response = IqrfEmbedFrc_Send_Response.FromJson(responseStr[0]);
        //IqrfEmbedFrc_ExtraResult_Response response = IqrfEmbedFrc_ExtraResult_Response.FromJson(responseStr[0]);
        //IqrfEmbedFrc_SendSelective_Response response = IqrfEmbedFrc_SendSelective_Response.FromJson(responseStr[0]);
        //IqrfEmbedFrc_SetParams_Response response = IqrfEmbedFrc_SetParams_Response.FromJson(responseStr[0]);

        /* SENSOR */

        //IqrfSensor_Enumerate_Response response = IqrfSensor_Enumerate_Response.FromJson(responseStr[0]);
        /*
        IqrfSensor_ReadSensorsWithTypes_Response response = IqrfSensor_ReadSensorsWithTypes_Response.FromJson(responseStr[0]);
        foreach (iqrfSensor_ReadSensorsWithTypes_Response.Anonymous sensor in response.Data.Rsp.Result.Sensors) {}
        */
        //IqrfSensor_Frc_Response response = IqrfSensor_Frc_Response.FromJson(responseStr[0]);

        /* BINARYOUTPUT */

        //IqrfBinaryoutput_Enumerate_Response response = IqrfBinaryoutput_Enumerate_Response.FromJson(responseStr[0]);
        //IqrfBinaryoutput_SetOutput_Response response = IqrfBinaryoutput_SetOutput_Response.FromJson(responseStr[0]);

        /* LIGHT */

        //IqrfLight_Enumerate_Response response = IqrfLight_Enumerate_Response.FromJson(responseStr[0]);
        //IqrfLight_SetPower_Response response = IqrfLight_SetPower_Response.FromJson(responseStr[0]);
        //IqrfLight_IncrementPower_Response response = IqrfLight_IncrementPower_Response.FromJson(responseStr[0]);
        //IqrfLight_DecrementPower_Response response = IqrfLight_DecrementPower_Response.FromJson(responseStr[0]);

        /* IQMESH */

        //IqmeshNetwork_BondNodeLocal_Response response = IqmeshNetwork_BondNodeLocal_Response.FromJson(responseStr[0]);
        //IqmeshNetwork_SmartConnect_Response response = IqmeshNetwork_SmartConnect_Response.FromJson(responseStr[0]);
        //IqmeshNetwork_EnumerateDevice_Response response = IqmeshNetwork_EnumerateDevice_Response.FromJson(responseStr[0]);
        //IqmeshNetwork_ReadTrConf_Response response = IqmeshNetwork_ReadTrConf_Response.FromJson(responseStr[0]);
        //IqmeshNetwork_WriteTrConf_Response response = IqmeshNetwork_WriteTrConf_Response.FromJson(responseStr[0]);
        //IqmeshNetwork_OtaUpload_Response response = IqmeshNetwork_OtaUpload_Response.FromJson(responseStr[0]);

        /* MNG */

        //MngDaemon_Mode_Response response = MngDaemon_Mode_Response.FromJson(responseStr[0]);
        MngDaemon_Exit_Response response = MngDaemon_Exit_Response.FromJson(responseStr[0]);

        /* SCHEDULER */

        //MngScheduler_List_Response response = MngScheduler_List_Response.FromJson(responseStr[0]);
        //MngScheduler_AddTask_Response response = MngScheduler_AddTask_Response.FromJson(responseStr[0]);
        //MngScheduler_PeriodicTask_Response response = MngScheduler_PeriodicTask_Response.FromJson(responseStr[0]);
        //MngScheduler_RemoveTask_Response response = MngScheduler_RemoveTask_Response.FromJson(responseStr[0]);

        Console.WriteLine( response.JsonSerialize() );
      }
      catch ( Exception ex )
      {
        Console.WriteLine( ex.Message );
      }

      Console.WriteLine( "\r\nPress any key..." );
      Console.ReadKey();
    }
  }

  #region Extensions
  public static class Extensions
  {
    /// <summary>
    /// Extension of object type for easy serialization to JSON
    /// </summary>
    public static string JsonSerialize ( this object obj ) => JsonConvert.SerializeObject( obj, Formatting.Indented );

    /// <summary>
    /// Extension of string type for easy deserialization from JSON
    /// </summary>
    public static dynamic JsonDeserialize ( this string json ) => JsonConvert.DeserializeObject<ExpandoObject>( json, new ExpandoObjectConverter() );

    /// <summary>
    /// Extension of string type for easy List deserialization from JSON
    /// </summary>
    public static List<ExpandoObject> JsonDeserializeList ( this string json ) => JsonConvert.DeserializeObject<List<ExpandoObject>>( json, new ExpandoObjectConverter() );
  }
  #endregion
}
