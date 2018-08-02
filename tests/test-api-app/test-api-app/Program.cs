namespace ConsoleApp3
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

  // If reference is missing, use NuGet to install "Newtonsoft.Json" package
  using Newtonsoft.Json;
  using Newtonsoft.Json.Converters;

  // Use NuGet to install "M2Mqtt" package
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

    const string IqrfGwDeamonIP = "192.168.0.108";

    /// <summary>
    /// Log transaction
    /// </summary>
    /// <param name="mType"></param>
    /// <param name="data"></param>
    private static void logTransaction ( string mType, string prefix, string data )
    {
      System.IO.File.AppendAllText( @"..\..\log\" + mType + ".log", prefix + DateTime.Now.ToString( "yyyy-MM-dd HH:mm:ss.FFF" ) + "\r\n" + data + "\r\n" );
    }

    /// <summary>
    /// Make example file
    /// </summary>
    /// <param name="mType"></param>
    /// <param name="direction"></param>
    /// <param name="data"></param>
    private static void logExample ( string mType, string direction, string data )
    {
      System.IO.File.WriteAllText( @"..\..\log\" + mType + "-" + direction + "-1-0-0-example.json", data );
    }

    /// <summary>
    /// WebSocket request to iqrf-daemon
    /// </summary>
    /// <param name="url"></param>
    /// <param name="request"></param>
    /// <returns></returns>
    private static List<string> WebSocketRequest ( string iqrfGwDaemonIP, dynamic request, int timeoutMs )
    {
      // Create WebSocket client
      WebSocket ws = new WebSocket( "ws://" + iqrfGwDaemonIP + ":1338", new string[] { "iqrf" } );
      try
      {
        int timeout = timeoutMs / 100;
        List<string> response = new List<string>();

        // Set MsgId
        string mtype = "";
        string strRequest = "";
        if ( request is string )
        {
          // Get mType
          int startIndex = request.IndexOf( "\"mType\": \"", 0 ) + "\"mType\": \"".Length;
          int length = request.IndexOf( "\"", startIndex ) - startIndex;
          mtype = request.Substring( startIndex, length );
          strRequest = request;
        }
        else
        {
          request.Data.MsgId = Guid.NewGuid().ToString();
          strRequest = ( (object)request ).JsonSerialize();
          mtype = request.MType.ToString();
          mtype = Char.ToLowerInvariant( mtype[0] ) + mtype.Substring( 1 );
        }
        // Log the request
        logTransaction( mtype, "==================================\r\nSending to WebSocket: ", strRequest );
        logExample( mtype, "request", strRequest );

        // Add event to receive MQTT message
        ws.OnMessage += ( sender, e ) =>
        {
          // Log response
          logTransaction( mtype, "Received from WebSocket: ", e.Data );
          logExample( mtype, "response", e.Data );
          response.Add( e.Data );
          // Preset timeout for next possible response to 0.5 sec (5 * 100 ms)
          timeout = 500 / 100;
        };

        // Connect to iqrf-daemon
        ws.Connect();
        // Send the request
        ws.Send( strRequest );
        // Wait for response 
        do
        {
          Thread.Sleep( 100 );
        } while ( --timeout != 0 );

        // Log end of the transction
        logTransaction( mtype, "", "==================================\r\n" );
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
    private static List<string> MQTTRequest ( string iqrfGwDaemon, dynamic request, int timeoutMs )
    {
      // Create MQTT client
      MqttClient mqttClient = new MqttClient( iqrfGwDaemon );
      // Connect to the MQTT broker
      mqttClient.Connect( Guid.NewGuid().ToString() );
      try
      {
        // Initial timeout
        int timeout = timeoutMs / 100;
        List<string> response = new List<string>();

        // Set MsgId
        string mtype = "";
        string strRequest = "";
        if ( request is string )
        {
          // Get mType
          int startIndex = request.IndexOf( "\"mType\": \"", 0 ) + "\"mType\": \"".Length;
          int length = request.IndexOf( "\"", startIndex) - startIndex;
          mtype = request.Substring( startIndex, length );
          strRequest = request;
        }
        else
        {
          request.Data.MsgId = Guid.NewGuid().ToString();          
          strRequest = ( (object)request ).JsonSerialize();
          mtype = request.MType.ToString();
          mtype = Char.ToLowerInvariant( mtype[0] ) + mtype.Substring( 1 );
        }

        // Log the request
        logTransaction( mtype, "==================================\r\nSending to MQTT: ", strRequest );
        logExample( mtype, "request", strRequest );

        // Add event to receive MQTT message
        mqttClient.MqttMsgPublishReceived += ( sender, e ) =>
        {
          // Log response
          string rsp = Encoding.UTF8.GetString( e.Message );
          logTransaction( mtype, "Received from MQTT: ", rsp );
          logExample( mtype, "response", rsp );
          response.Add( rsp );
          // Preset timeout for next possible response to 0.5 sec (5 * 100 ms)
          timeout = 500 / 100;
        };

        // Subscribe IQRF/DpaResponse at MQTT broker
        mqttClient.Subscribe( new string[] { "Iqrf/DpaResponse" }, new byte[] { MqttMsgBase.QOS_LEVEL_EXACTLY_ONCE } );
        mqttClient.Publish( "Iqrf/DpaRequest", Encoding.UTF8.GetBytes( strRequest ), MqttMsgBase.QOS_LEVEL_EXACTLY_ONCE, true );
        // Wait for response 
        do
        {
          Thread.Sleep( 100 );
        } while ( --timeout != 0 );

        // Log end of the transction
        logTransaction( mtype, "", "==================================\r\n" );
        return response;
      }
      finally
      {
        mqttClient.Disconnect();
      }
    }

    /// <summary>
    /// Generate cs class from JSON schema
    /// </summary>
    /// <param name="dirAPI"></param>
    /// <param name="dirClass"></param>
    private static void generateCSClassFromJSON ( string dirAPI, string dirClass )
    {
      Console.WriteLine( "Generating files.." );
      // Generate C# class from the json schema      
      try
      {
        DirectoryInfo d = new DirectoryInfo( dirAPI );
        FileInfo[] Files = d.GetFiles( "*.json*" ); //Getting Text files
        foreach ( FileInfo file in Files )
        {
          if ( !file.Name.Contains( "mngSched" ) )
          {
            var schemaJson = File.ReadAllText( file.FullName );
            var schema = JsonSchema4.FromJsonAsync( schemaJson ).Result;
            string fName = file.FullName;
            string nameSpace = file.Name.Substring( 0, file.Name.IndexOf( '-', 0 ) );
            string dir = nameSpace;
            if ( dir.Contains( '_' ) )
              dir = dir.Substring( 0, dir.IndexOf( '_', 0 ) );
            if ( file.Name.Contains( "request" ) )
            {
              nameSpace += "_Request";
            }
            if ( file.Name.Contains( "response" ) )
            {
              nameSpace += "_Response";
            }
            string className = char.ToUpper( nameSpace[0] ) + nameSpace.Substring( 1 );
            var generator = new CSharpGenerator( schema, new CSharpGeneratorSettings
            {
              ClassStyle = CSharpClassStyle.Poco,
              Namespace = nameSpace,
              SchemaType = SchemaType.Swagger2,
            } );
            if ( !Directory.Exists( dirClass + dir ) )
              Directory.CreateDirectory( dirClass + dir );
            Console.Write( "Generating " + file.Name.Replace( "json", "cs" ) + ".." );
            var generatedFile = generator.GenerateFile( className );
            string targetFile = dirClass + dir + '\\' + file.Name.Replace( "json", "cs" );
            File.WriteAllText( targetFile, generatedFile );
            Console.WriteLine( " OK" );
          }
        }
      }
      catch ( Exception ex )
      {
        Console.WriteLine( ex.Message );
      }
      Console.Write( "Completed.." );
    }

    /// <summary>
    /// Automated testing daemon API
    /// </summary>
    /// <param name="dirJsonRequest"></param>
    /// <param name="MQTT"></param>
    private static void testAPI ( string dirJsonRequest, Func<string, string, int, List<string>> sendRequest )
    {
      Console.WriteLine( "Starting test.." );
      // Test all requests contained in the directory dirJsonRequest    
      try
      {
        // Check dirJsonRequest
        if ( !Directory.Exists( dirJsonRequest ) )
        {
          throw new Exception( "Directory " + dirJsonRequest + " not found!" );
        }
        DirectoryInfo d = new DirectoryInfo( dirJsonRequest );
        FileInfo[] Files = d.GetFiles( "*.json*" ); //Getting Text files
        foreach ( FileInfo file in Files )
        {
          // Read the file
          string strRequest = File.ReadAllText( file.FullName );
          Console.WriteLine( file.Name );
          // Send the request to IQRF Daemon
          sendRequest( IqrfGwDeamonIP, strRequest, 50000 );
        }
      }
      catch ( Exception ex )
      {
        Console.WriteLine( ex.Message );
      }
      Console.Write( "Completed.." );
    }

    /// <summary>
    /// Main function
    /// </summary>
    private static void Main ()
    {
      try
      {
        // Generate c# classes from JSON schema
        //generateCSClassFromJSON( @"c:\Microrisc\Bastl\iqrfgd2_test_app\iqrfgd2_API_test\api\", @"c:\Microrisc\Bastl\iqrfgd2_test_app\iqrfgd2_API_test\" );

        // Automated test of predefined APIs
        testAPI( @"..\..\test\", MQTTRequest);
        // Zatim nepouzivam, az vyresime vice odpovedi na jeden request pres WebSocket
        //testAPI( @"..\..\test\", WebSocketRequest );

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
