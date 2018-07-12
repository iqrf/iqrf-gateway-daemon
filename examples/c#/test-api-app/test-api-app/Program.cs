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

  using iqrfEmbedEeeprom_Read_Request;
  using iqrfEmbedEeeprom_Read_Response;
  using iqrfEmbedEeeprom_Write_Request;
  using iqrfEmbedEeeprom_Write_Response;

  using iqrfEmbedOs_Read_Request;
  using iqrfEmbedOs_Read_Response;

  // IqmeshNetwork APIs
  using iqmeshNetwork_BondNodeLocal_Request;
  using iqmeshNetwork_BondNodeLocal_Response;
  using iqmeshNetwork_EnumerateDevice_Request;
  using iqmeshNetwork_EnumerateDevice_Response;
  using iqmeshNetwork_OtaUpload_Request;
  using iqmeshNetwork_OtaUpload_Response;
  using iqmeshNetwork_ReadTrConf_Request;
  using iqmeshNetwork_ReadTrConf_Response;
  using iqmeshNetwork_SmartConnect_Request;
  using iqmeshNetwork_SmartConnect_Response;
  using iqmeshNetwork_WriteTrConf_Request;
  using iqmeshNetwork_WriteTrConf_Response;

  // Scheduler APIs
  using mngSched_List_Request;
  using mngSched_List_Response;
  using mngSched_AddTask_Request;
  using mngSched_AddTask_Response;
  using mngSched_SchedulerMessagingTask;
  using mngSched_StartTask_Request;
  using mngSched_StartTask_Response;
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
    const string IqrfGwDeamonIP = "192.168.1.101";

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
        request.Data.Req.PData = new System.Collections.ObjectModel.ObservableCollection<int> {}; ;
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
        request.Data.Req.Param.ReqAddr = 6;
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
        //request.Data.Timeout = 11000;
        request.Data.Req.NAdr = 0;
        request.Data.Req.Param.ReqAddr = 6;
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
        request.Data.Req.Param.BondAddr = 6;
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedCoordinator_Restore_Request request = new IqrfEmbedCoordinator_Restore_Request();
        request.Data.MsgId = "testEmbedCoordinator";
        request.Data.Req.NAdr = 0;
        request.Data.Req.Param.NetData = new System.Collections.ObjectModel.ObservableCollection<int> { 237, 96, 20, 81, 130, 222, 124, 114, 222, 248, 144, 163, 238, 130, 117, 237, 144, 84, 167, 183, 129, 51, 202, 101, 112, 7, 123, 36, 3, 67, 8, 30, 130, 98, 206, 11, 16, 43, 35, 67, 163, 88, 214, 166, 140, 145, 17, 104, 168 };
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

        /* EEEPROM */

        /*
        IqrfEmbedEeeprom_Read_Request request = new IqrfEmbedEeeprom_Read_Request();
        request.Data.MsgId = "testEmbedEeeprom";
        request.Data.Timeout = 1000;
        request.Data.Req.NAdr = 1;
        request.Data.Req.Param.Address= 0;
        request.Data.Req.Param.Len = 10;
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfEmbedEeeprom_Write_Request request = new IqrfEmbedEeeprom_Write_Request();
        request.Data.MsgId = "testEmbedEeeprom";
        request.Data.Req.NAdr = 1;
        request.Data.Req.Param.Address = 0;
        request.Data.Req.Param.PData = new System.Collections.ObjectModel.ObservableCollection<int> { 0, 1, 2, 3, 4 };
        request.Data.ReturnVerbose = true;
        */

        /* OS */

        /*
        IqrfEmbedOs_Read_Request request = new IqrfEmbedOs_Read_Request();
        request.Data.MsgId = "testEmbedOs";
        request.Data.Req.NAdr = 0;
        request.Data.ReturnVerbose = true;
        */

        /* IQMESH */

        IqmeshNetwork_BondNodeLocal_Request request = new IqmeshNetwork_BondNodeLocal_Request();
        request.Data.MsgId = "testIqmeshBondNodeLocal";
        request.Data.Repeat = 1;
        request.Data.Req.DeviceAddr = 1;
        request.Data.ReturnVerbose = true;

        /*
        IqmeshNetwork_EnumerateDevice_Request request = new IqmeshNetwork_EnumerateDevice_Request();
        request.Data.MsgId = "testIqmeshEnumerate";
        request.Data.Repeat = 1;
        request.Data.Req.DeviceAddr = 0 ;
        request.Data.ReturnVerbose = true;
        */

        /*
        IqmeshNetwork_ReadTrConf_Request request = new IqmeshNetwork_ReadTrConf_Request();
        request.Data.MsgId = "testIqmeshRead";
        request.Data.Repeat = 1;
        request.Data.Req.DeviceAddr = new System.Collections.ObjectModel.ObservableCollection<int> { 0 };
        request.Data.ReturnVerbose = true;
        */

        /*
        IqmeshNetwork_WriteTrConf_Request request = new IqmeshNetwork_WriteTrConf_Request();
        request.Data.MsgId = "testIqmeshWrite";
        request.Data.Repeat = 1;
        request.Data.Req.DeviceAddr = new System.Collections.ObjectModel.ObservableCollection<int> { 0 };
        request.Data.Req.TxPower = 3;
        request.Data.ReturnVerbose = true;
        */

        /*
        IqmeshNetwork_OtaUpload_Request request = new IqmeshNetwork_OtaUpload_Request();
        request.Data.MsgId = "testIqmeshOta";
        request.Data.Repeat = 1;
        request.Data.Req.DeviceAddr = new System.Collections.ObjectModel.ObservableCollection<int> { 3 };
        request.Data.Req.FileName = "CustomDpaHandler-LED-Red-On-7xD-V302-171116.hex";
        request.Data.Req.LoadingAction = "WithCodeLoading";
        request.Data.Req.StartMemAddr = 0;
        request.Data.ReturnVerbose = true;
        */

        /* SCHEDULER */

        /*
        MngSched_List_Request request = new MngSched_List_Request();
        request.Data.MsgId = "testSchedList";
        request.Data.Req.ClientId = "SchedulerMessaging";
        request.Data.ReturnVerbose = true;
        */

        /*
        IqrfRaw_Request rawReq = new IqrfRaw_Request();
        rawReq.Data.MsgId = "testRaw";
        rawReq.Data.Req.RData = "00.00.06.03.FF.FF";
        rawReq.Data.Timeout = 1000;
        rawReq.Data.ReturnVerbose = true;

        MngSched_SchedulerMessagingTask task = new MngSched_SchedulerMessagingTask();
        task.Messaging = MngSched_SchedulerMessagingTaskMessaging.WebsocketMessaging;
        task.Message = rawReq;

        MngSched_AddTask_Request request = new MngSched_AddTask_Request();
        request.Data.MsgId = "testSchedAdd";
        request.Data.Req.ClientId = "SchedulerMessaging";
        request.Data.Req.Task = task;
        request.Data.Req.CronTime = new System.Collections.ObjectModel.ObservableCollection<string> { "*\/10", "*", "*", "*", "*", "*", "*" };
        request.Data.ReturnVerbose = true;
        */

        // Create response
        List<string> responseStr = new List<string>();

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

        /* EEEPROM */
        //IqrfEmbedEeeprom_Read_Response response = IqrfEmbedEeeprom_Read_Response.FromJson( responseStr[0] );
        //IqrfEmbedEeeprom_Write_Response response = IqrfEmbedEeeprom_Write_Response.FromJson( responseStr[0] );

        /* OS */
        //IqrfEmbedOs_Read_Response response = IqrfEmbedOs_Read_Response.FromJson(responseStr[0]);

        /* IQMESH */
        IqmeshNetwork_BondNodeLocal_Response response = IqmeshNetwork_BondNodeLocal_Response.FromJson(responseStr[0]);
        //IqmeshNetwork_EnumerateDevice_Response response = IqmeshNetwork_EnumerateDevice_Response.FromJson( responseStr[0] );
        //IqmeshNetwork_ReadTrConf_Response response = IqmeshNetwork_ReadTrConf_Response.FromJson(responseStr[0]);
        //IqmeshNetwork_WriteTrConf_Response response = IqmeshNetwork_WriteTrConf_Response.FromJson(responseStr[0]);
        //IqmeshNetwork_OtaUpload_Response response = IqmeshNetwork_OtaUpload_Response.FromJson(responseStr[0]);

        //foreach (string s in responseStr)
        //  Console.WriteLine(s);

        /* SCHEDULER */
        //MngSched_List_Response response = MngSched_List_Response.FromJson ( responseStr[0] );
        //MngSched_AddTask_Response response = MngSched_AddTask_Response.FromJson(responseStr[0]);

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
