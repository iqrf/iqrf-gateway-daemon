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

  //IqrfRaw APIs 
  using iqrfRaw_Request;
  using iqrfRaw_Response;
  using iqrfRawHdp_Request;
  using iqrfRawHdp_Response;
  #endregion

  class Program
  {
    /// <summary>
    /// MQTT and WebSocket timeout [ms]
    /// </summary>
    const int Timeout = 20 * 1000;

    /// <summary>
    /// WebSocket request to iqrf-daemon
    /// </summary>
    /// <param name="url"></param>
    /// <param name="request"></param>
    /// <returns></returns>
    private static string WebSocketRequest ( string url, string request )
    {
      // Create WebSocket client
      WebSocket ws = new WebSocket( url, new string[] { "iqrf" } );
      try
      {
        string response = "";
        AutoResetEvent responseReceived = new AutoResetEvent( false );

        // Add event to receive WebSocket server reply
        ws.OnMessage += ( sender, e ) =>
        {
          response = e.Data;
          responseReceived.Set();
        };

        // Connect to iqrf-daemon
        ws.Connect();
        // Send the request
        ws.Send( request );
        // Wait for reply
        if ( !responseReceived.WaitOne( Timeout ) )
          throw new Exception( "Timeout expired waiting for WebSocket server response!" );
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
    private static string MQTTRequest ( string iqrfGwDaemon, string request )
    {
      // Create MQTT client
      MqttClient mqttClient = new MqttClient( iqrfGwDaemon );
      // Connect to the MQTT broker
      mqttClient.Connect( Guid.NewGuid().ToString() );
      try
      {
        string response = "";
        AutoResetEvent responseReceived = new AutoResetEvent( false );

        // Add event to receive MQTT message
        mqttClient.MqttMsgPublishReceived += ( sender, e ) =>
        {
          response = Encoding.UTF8.GetString( e.Message );
          responseReceived.Set();
        };
        // Subscribe IQRF/DpaResponse at MQTT broker
        mqttClient.Subscribe( new string[] { "Iqrf/DpaResponse" }, new byte[] { MqttMsgBase.QOS_LEVEL_EXACTLY_ONCE } );
        mqttClient.Publish( "Iqrf/DpaRequest", Encoding.UTF8.GetBytes( request ), MqttMsgBase.QOS_LEVEL_EXACTLY_ONCE, true );
        // Wait for response
        if ( !responseReceived.WaitOne( Timeout ) )
          throw new TimeoutException( "Timeout expired waiting for MQTT publish event" );
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
        IqrfRaw_Request request = new IqrfRaw_Request();
        request.Data.MsgId = "test";
        request.Data.Req.RData = "00.00.06.03.FF.FF";
        request.Data.Timeout = 1000;
        request.Data.ReturnVerbose = true;

        // Create response
        string responseStr = "";

        // Send the request to iqrf-daemon via MQTT
        //System.IO.File.WriteAllText( @"..\..\log\" + request.MType.ToString() + "_Request.json", request.JsonSerialize() );
        //responseStr = MQTTRequest( "192.168.0.108", request.ToJson() );
        //System.IO.File.WriteAllText( @"..\..\log\" + request.MType.ToString() + "_Response.json", responseStr);

        // Send the request to iqrf-daemon via WebSocket
        System.IO.File.WriteAllText( @"..\..\log\" + request.MType.ToString() + "_Request.json", request.JsonSerialize() );
        responseStr = WebSocketRequest( "ws://192.168.0.108:1338", request.JsonSerialize() );
        System.IO.File.WriteAllText( @"..\..\log\" + request.MType.ToString() + "_Response.json", responseStr );

        // Get response
        IqrfRaw_Response response = IqrfRaw_Response.FromJson( responseStr );    
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
