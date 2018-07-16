//----------------------
// <auto-generated>
//     Generated using the NJsonSchema v9.10.56.0 (Newtonsoft.Json v9.0.0.0) (http://NJsonSchema.org)
// </auto-generated>
//----------------------

namespace iqrfEmbedOs_BatchRequest
{
    #pragma warning disable // Disable all warnings

    /// <summary>Single request for batch.</summary>
    [System.CodeDom.Compiler.GeneratedCode("NJsonSchema", "9.10.56.0 (Newtonsoft.Json v9.0.0.0)")]
    public partial class IqrfEmbedOs_BatchRequest 
    {
        /// <summary>Request cType.</summary>
        [Newtonsoft.Json.JsonProperty("ctype", Required = Newtonsoft.Json.Required.Always)]
        [System.ComponentModel.DataAnnotations.Required]
        [Newtonsoft.Json.JsonConverter(typeof(Newtonsoft.Json.Converters.StringEnumConverter))]
        public IqrfEmbedOs_BatchRequestCtype Ctype { get; set; }
    
        /// <summary>Request Type.</summary>
        [Newtonsoft.Json.JsonProperty("type", Required = Newtonsoft.Json.Required.Always)]
        [System.ComponentModel.DataAnnotations.Required]
        [Newtonsoft.Json.JsonConverter(typeof(Newtonsoft.Json.Converters.StringEnumConverter))]
        public IqrfEmbedOs_BatchRequestType Type { get; set; }
    
        /// <summary>Message Id.</summary>
        [Newtonsoft.Json.JsonProperty("msgid", Required = Newtonsoft.Json.Required.Default, NullValueHandling = Newtonsoft.Json.NullValueHandling.Ignore)]
        public string Msgid { get; set; }
    
        /// <summary>Network Address.</summary>
        [Newtonsoft.Json.JsonProperty("nadr", Required = Newtonsoft.Json.Required.Default, NullValueHandling = Newtonsoft.Json.NullValueHandling.Ignore)]
        public string Nadr { get; set; }
    
        /// <summary>Peripheral Number.</summary>
        [Newtonsoft.Json.JsonProperty("pnum", Required = Newtonsoft.Json.Required.Always)]
        [System.ComponentModel.DataAnnotations.Required]
        public string Pnum { get; set; }
    
        /// <summary>Peripheral Command.</summary>
        [Newtonsoft.Json.JsonProperty("pcmd", Required = Newtonsoft.Json.Required.Always)]
        [System.ComponentModel.DataAnnotations.Required]
        public string Pcmd { get; set; }
    
        /// <summary>Hardware Profile ID.</summary>
        [Newtonsoft.Json.JsonProperty("hwpid", Required = Newtonsoft.Json.Required.Default, NullValueHandling = Newtonsoft.Json.NullValueHandling.Ignore)]
        public string Hwpid { get; set; }
    
        /// <summary>Request Data.</summary>
        [Newtonsoft.Json.JsonProperty("rdata", Required = Newtonsoft.Json.Required.Default, NullValueHandling = Newtonsoft.Json.NullValueHandling.Ignore)]
        public string Rdata { get; set; }
    
        public string ToJson() 
        {
            return Newtonsoft.Json.JsonConvert.SerializeObject(this);
        }
        
        public static IqrfEmbedOs_BatchRequest FromJson(string data)
        {
            return Newtonsoft.Json.JsonConvert.DeserializeObject<IqrfEmbedOs_BatchRequest>(data);
        }
    
    }
    
    [System.CodeDom.Compiler.GeneratedCode("NJsonSchema", "9.10.56.0 (Newtonsoft.Json v9.0.0.0)")]
    public enum IqrfEmbedOs_BatchRequestCtype
    {
        [System.Runtime.Serialization.EnumMember(Value = "dpa")]
    
        Dpa = 0,
    
    
    }
    
    [System.CodeDom.Compiler.GeneratedCode("NJsonSchema", "9.10.56.0 (Newtonsoft.Json v9.0.0.0)")]
    public enum IqrfEmbedOs_BatchRequestType
    {
        [System.Runtime.Serialization.EnumMember(Value = "raw-hdp")]
    
        RawHdp = 0,
    
    
    }
}