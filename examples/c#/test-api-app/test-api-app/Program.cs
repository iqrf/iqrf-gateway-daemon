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

  // IqMeshNetwork APIs
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

  // iqrfEmbedCoordinator APIs
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
  using iqrfEmbedCoordinator_Rebond_Request;
  using iqrfEmbedCoordinator_Rebond_Response;
  using iqrfEmbedCoordinator_RemoveBond_Request;
  using iqrfEmbedCoordinator_RemoveBond_Response;
  using iqrfEmbedCoordinator_Restore_Request;
  using iqrfEmbedCoordinator_Restore_Response;
  using iqrfEmbedCoordinator_SetDpaParams_Request;
  using iqrfEmbedCoordinator_SetDpaParams_Response;
  using iqrfEmbedCoordinator_SetHops_Request;
  using iqrfEmbedCoordinator_SetHops_Response;
  #endregion

  class Program
  {
    /// <summary>
    /// Enter your IQRF GW Daemon (running MQTT broker) address/name
    /// </summary>
    const string IqrfGwDaemon = "192.168.1.105";
    //const string IqrfGwDaemon = "10.11.12.54";

    /// <summary>
    /// MQTT timeout [ms]
    /// </summary>
    const int MqttTimeout = 20 * 1000;

    /// <summary>
    /// MQTT client object
    /// </summary>
    static MqttClient mqttClient;

    /// <summary>
    /// Received object from the MQTT broker
    /// </summary>
    static dynamic mqttReceivedPublishObject;

    /// <summary>
    /// Message id of the MQTT response we wait for
    /// </summary>
    static string mqttMsgid;

    /// <summary>
    /// Event to signal data received from MQTT broker
    /// </summary>
    static readonly AutoResetEvent mqttPublishEvent = new AutoResetEvent( false );

    static void Main ()
    {
      try
      {
        string responseStr = "";
        string mType = "";

        // Create MQTT client
        mqttClient = new MqttClient( IqrfGwDaemon );

        // Connect to the MQTT broker
        mqttClient.Connect( Guid.NewGuid().ToString() );

        // Add event to receive MQTT message
        mqttClient.MqttMsgPublishReceived += ( sender, e ) =>
        {
          responseStr = Encoding.UTF8.GetString( e.Message );      
          System.IO.File.WriteAllText( @"..\..\Log\" + mType + "_Response.json", responseStr);
          mqttReceivedPublishObject = Encoding.UTF8.GetString( e.Message ).JsonDeserialize();
          
          // Signal that a new object was received only when message IDs match
          if ( mqttReceivedPublishObject.data.msgId == mqttMsgid )
            mqttPublishEvent.Set();
        };

        //IqrfRaw_Request request = new IqrfRaw_Request( "test", 1000, "00.00.06.03.FF.FF", true );
        //IqrfRawHdp_Request request = new IqrfRawHdp_Request( "test", new iqrfRawHdp_Request.Req( 0, 6, 3, 65535, new System.Collections.ObjectModel.ObservableCollection<int> { } ), 1000, true );

        //IqmeshNetwork_BondNodeLocal_Request request = new IqmeshNetwork_BondNodeLocal_Request( "test", 5, 1, true );
        //IqmeshNetwork_EnumerateDevice_Request request = new IqmeshNetwork_EnumerateDevice_Request( "test", 0, 1, true );
        //IqmeshNetwork_OtaUpload_Request request = new IqmeshNetwork_OtaUpload_Request( "test", new iqmeshNetwork_OtaUpload_Request.Req( new System.Collections.ObjectModel.ObservableCollection<int> { 0 }, "test.hex", 0, "WithoutCodeLoading" ), 1, true );
        IqmeshNetwork_ReadTrConf_Request request = new IqmeshNetwork_ReadTrConf_Request( "test", new System.Collections.ObjectModel.ObservableCollection<int> { 0 }, 1, true );
        //IqmeshNetwork_SmartConnect_Request request = new IqmeshNetwork_SmartConnect_Request( "test", new iqmeshNetwork_SmartConnect_Request.Req( 1, "0123456789abcdef", 1, new System.Collections.ObjectModel.ObservableCollection<int> { 11, 22, 33, 44 } ), 1000, true );
        //IqmeshNetwork_WriteTrConf_Request request = new IqmeshNetwork_WriteTrConf_Request( "test", new iqmeshNetwork_WriteTrConf_Request.Req(), 1000, true );

        //IqrfEmbedCoordinator_AddrInfo_Request request = new IqrfEmbedCoordinator_AddrInfo_Request( "test", 0, 65535, 2000, true );
        //IqrfEmbedCoordinator_AuthorizeBond_Request request = new IqrfEmbedCoordinator_AuthorizeBond_Request( "test", 0, 65535, 2, 0x11223344, 1000, true );
        //IqrfEmbedCoordinator_Backup_Request request = new IqrfEmbedCoordinator_Backup_Request( "test", 0, 65535, 0, 1000, true );
        //IqrfEmbedCoordinator_BondedDevices_Request request = new IqrfEmbedCoordinator_BondedDevices_Request( "test", 0, 65535, 1000, true );
        //IqrfEmbedCoordinator_BondNode_Request request = new IqrfEmbedCoordinator_BondNode_Request( "test", 0, 65535, 3, 7, 20000, true );
        //IqrfEmbedCoordinator_ClearAllBonds_Request request = new IqrfEmbedCoordinator_ClearAllBonds_Request( "test", 0, 65535, 1000, true );
        //IqrfEmbedCoordinator_ClearRemotelyBondedMid_Request request = new IqrfEmbedCoordinator_ClearRemotelyBondedMid_Request( "test", 0, 65535, 1000, true );
        //IqrfEmbedCoordinator_DiscoveredDevices_Request request = new IqrfEmbedCoordinator_DiscoveredDevices_Request( "test", 0, 65535, 1000, true );
        //IqrfEmbedCoordinator_Discovery_Request request = new IqrfEmbedCoordinator_Discovery_Request( "test", 0, 65535, 5, 7, 5000, true );
        //IqrfEmbedCoordinator_DiscoveryData_Request request = new IqrfEmbedCoordinator_DiscoveryData_Request( "test", 0, 65535, 0x5000, 1000, true );
        //IqrfEmbedCoordinator_EnableRemoteBonding_Request request = new IqrfEmbedCoordinator_EnableRemoteBonding_Request( "test", 0, 65535, 7, true, new System.Collections.ObjectModel.ObservableCollection<int> { 11, 22 }, 1000, true );
        //IqrfEmbedCoordinator_ReadRemotelyBondedMid_Request request = new IqrfEmbedCoordinator_ReadRemotelyBondedMid_Request( "test", 0, 65535, 1000, true );
        //IqrfEmbedCoordinator_Rebond_Request request = new IqrfEmbedCoordinator_Rebond_Request( "test", 0, 65535, 3, 1000, true );
        //IqrfEmbedCoordinator_RemoveBond_Request request = new IqrfEmbedCoordinator_RemoveBond_Request( "test", 0, 65535, 0x01, 1000, true );
        //IqrfEmbedCoordinator_Restore_Request request = new IqrfEmbedCoordinator_Restore_Request( "test", 0, 65535, new System.Collections.ObjectModel.ObservableCollection<int> { 0, 1, 2 }, 1000, true );
        //IqrfEmbedCoordinator_SetDpaParams_Request request = new IqrfEmbedCoordinator_SetDpaParams_Request( "test", 0, 65535, 11, 1000, true );
        //IqrfEmbedCoordinator_SetHops_Request request = new IqrfEmbedCoordinator_SetHops_Request( "test", 0, 65535, 0, 0, 1000, true );

        // Subscribe IQRF/DpaResponse at MQTT broker        
        mqttClient.Subscribe( new string[] { "Iqrf/DpaResponse" }, new byte[] { MqttMsgBase.QOS_LEVEL_EXACTLY_ONCE } );

        // Generate MsgId
        request.Data.MsgId = mqttMsgid = Guid.NewGuid().ToString();
        
        // Save mType 
        mType = request.MType.ToString();
        
        // Write request to file
        string req = ( (object)request ).JsonSerialize();
        System.IO.File.WriteAllText( @"..\..\log\" + request.MType.ToString() + "_Request.json", request.JsonSerialize() );
        
        // Send request 
        dynamic dResponse = GetMqttResponse( request );
        
        //IqrfRaw_Response response = IqrfRaw_Response.FromJson( responseStr );
        //IqrfRawHdp_Response response = IqrfRawHdp_Response.FromJson( responseStr );

        //IqmeshNetwork_BondNodeLocal_Response response = IqmeshNetwork_BondNodeLocal_Response.FromJson( responseStr );
        //IqmeshNetwork_EnumerateDevice_Response response = IqmeshNetwork_EnumerateDevice_Response.FromJson( responseStr );
        //IqmeshNetwork_OtaUpload_Response response = IqmeshNetwork_OtaUpload_Response.FromJson( responseStr );
        IqmeshNetwork_ReadTrConf_Response response = IqmeshNetwork_ReadTrConf_Response.FromJson( responseStr );
        //IqmeshNetwork_SmartConnect_Response response = IqmeshNetwork_SmartConnect_Response.FromJson( responseStr );
        //IqmeshNetwork_WriteTrConf_Response response = IqmeshNetwork_WriteTrConf_Response.FromJson( responseStr );

        //IqrfEmbedCoordinator_AddrInfo_Response response = IqrfEmbedCoordinator_AddrInfo_Response.FromJson( responseStr );
        //IqrfEmbedCoordinator_AuthorizeBond_Response response = IqrfEmbedCoordinator_AuthorizeBond_Response.FromJson( responseStr );
        //IqrfEmbedCoordinator_Backup_Response response = IqrfEmbedCoordinator_Backup_Response.FromJson( responseStr );
        //IqrfEmbedCoordinator_BondedDevices_Response response = IqrfEmbedCoordinator_BondedDevices_Response.FromJson( responseStr );
        //IqrfEmbedCoordinator_BondNode_Response response = IqrfEmbedCoordinator_BondNode_Response.FromJson( responseStr );
        //IqrfEmbedCoordinator_ClearAllBonds_Response response = IqrfEmbedCoordinator_ClearAllBonds_Response.FromJson( responseStr );
        //IqrfEmbedCoordinator_ClearRemotelyBondedMid_Response response = IqrfEmbedCoordinator_ClearRemotelyBondedMid_Response.FromJson( responseStr );
        //IqrfEmbedCoordinator_DiscoveredDevices_Response response = IqrfEmbedCoordinator_DiscoveredDevices_Response.FromJson( responseStr );
        //IqrfEmbedCoordinator_Discovery_Response response = IqrfEmbedCoordinator_Discovery_Response.FromJson( responseStr );
        //IqrfEmbedCoordinator_DiscoveryData_Response response = IqrfEmbedCoordinator_DiscoveryData_Response.FromJson( responseStr );
        //IqrfEmbedCoordinator_EnableRemoteBonding_Response response = IqrfEmbedCoordinator_EnableRemoteBonding_Response.FromJson( responseStr );
        //IqrfEmbedCoordinator_ReadRemotelyBondedMid_Response response = IqrfEmbedCoordinator_ReadRemotelyBondedMid_Response.FromJson( responseStr );
        //IqrfEmbedCoordinator_Rebond_Response response = IqrfEmbedCoordinator_Rebond_Response.FromJson( responseStr );
        //IqrfEmbedCoordinator_RemoveBond_Response response = IqrfEmbedCoordinator_RemoveBond_Response.FromJson( responseStr );
        //IqrfEmbedCoordinator_Restore_Response response = IqrfEmbedCoordinator_Restore_Response.FromJson( responseStr );
        //IqrfEmbedCoordinator_SetDpaParams_Response response = IqrfEmbedCoordinator_SetDpaParams_Response.FromJson( responseStr );
        //IqrfEmbedCoordinator_SetHops_Response response = IqrfEmbedCoordinator_SetHops_Response.FromJson( responseStr );        
        Console.WriteLine( responseStr );
      }
      finally
      {
        mqttClient.Disconnect();
      }

      Console.ReadKey();
    }

    #region Helper methods
    static dynamic GetMqttResponse ( dynamic request )
    {
      mqttClient.Publish( "Iqrf/DpaRequest", Encoding.UTF8.GetBytes( ( (object)request ).JsonSerialize() ), MqttMsgBase.QOS_LEVEL_EXACTLY_ONCE, true );
      if ( !mqttPublishEvent.WaitOne( MqttTimeout ) )
        throw new TimeoutException( "Timeout expired waiting for MQTT publish event" );
      return mqttReceivedPublishObject;
    }
    #endregion
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
