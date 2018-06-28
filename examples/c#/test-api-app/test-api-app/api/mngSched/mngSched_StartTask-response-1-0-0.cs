//----------------------
// <auto-generated>
//     Generated using the NJsonSchema v9.10.56.0 (Newtonsoft.Json v9.0.0.0) (http://NJsonSchema.org)
// </auto-generated>
//----------------------

namespace mngSched_StartTask_Response
{
    #pragma warning disable // Disable all warnings

    [System.CodeDom.Compiler.GeneratedCode("NJsonSchema", "9.10.56.0 (Newtonsoft.Json v9.0.0.0)")]
    public partial class MngSched_StartTask_Response 
    {
        /// <summary>Scheduler management - Start Task response.</summary>
        [Newtonsoft.Json.JsonProperty("mType", Required = Newtonsoft.Json.Required.Always)]
        [System.ComponentModel.DataAnnotations.Required]
        [Newtonsoft.Json.JsonConverter(typeof(Newtonsoft.Json.Converters.StringEnumConverter))]
        public MngSched_StartTask_ResponseMType MType { get; set; }
    
        [Newtonsoft.Json.JsonProperty("data", Required = Newtonsoft.Json.Required.Always)]
        [System.ComponentModel.DataAnnotations.Required]
        public Data Data { get; set; } = new Data();
    
        public string ToJson() 
        {
            return Newtonsoft.Json.JsonConvert.SerializeObject(this);
        }
        
        public static MngSched_StartTask_Response FromJson(string data)
        {
            return Newtonsoft.Json.JsonConvert.DeserializeObject<MngSched_StartTask_Response>(data);
        }
    
    }
    
    [System.CodeDom.Compiler.GeneratedCode("NJsonSchema", "9.10.56.0 (Newtonsoft.Json v9.0.0.0)")]
    public enum MngSched_StartTask_ResponseMType
    {
        [System.Runtime.Serialization.EnumMember(Value = "mngSched_StartTask")]
    
        MngSched_StartTask = 0,
    
    
    }
    
    [System.CodeDom.Compiler.GeneratedCode("NJsonSchema", "9.10.56.0 (Newtonsoft.Json v9.0.0.0)")]
    public partial class Data 
    {
        /// <summary>Message identification for binding request with response.</summary>
        [Newtonsoft.Json.JsonProperty("msgId", Required = Newtonsoft.Json.Required.Always)]
        [System.ComponentModel.DataAnnotations.Required]
        public string MsgId { get; set; }
    
        [Newtonsoft.Json.JsonProperty("rsp", Required = Newtonsoft.Json.Required.Always)]
        [System.ComponentModel.DataAnnotations.Required]
        public Rsp Rsp { get; set; } = new Rsp();
    
        /// <summary>IQRF GW daemon instance identification.</summary>
        [Newtonsoft.Json.JsonProperty("insId", Required = Newtonsoft.Json.Required.Default, NullValueHandling = Newtonsoft.Json.NullValueHandling.Ignore)]
        public string InsId { get; set; }
    
        /// <summary>Status of operation.</summary>
        [Newtonsoft.Json.JsonProperty("status", Required = Newtonsoft.Json.Required.Always)]
        public int Status { get; set; }
    
        /// <summary>IQRF GW daemon API (general or mType) status in string form.</summary>
        [Newtonsoft.Json.JsonProperty("statusStr", Required = Newtonsoft.Json.Required.Default, NullValueHandling = Newtonsoft.Json.NullValueHandling.Ignore)]
        public string StatusStr { get; set; }
    
        public string ToJson() 
        {
            return Newtonsoft.Json.JsonConvert.SerializeObject(this);
        }
        
        public static Data FromJson(string data)
        {
            return Newtonsoft.Json.JsonConvert.DeserializeObject<Data>(data);
        }
    
    }
    
    [System.CodeDom.Compiler.GeneratedCode("NJsonSchema", "9.10.56.0 (Newtonsoft.Json v9.0.0.0)")]
    public partial class Rsp 
    {
        /// <summary>Client identifier.</summary>
        [Newtonsoft.Json.JsonProperty("clientId", Required = Newtonsoft.Json.Required.Always)]
        [System.ComponentModel.DataAnnotations.Required]
        public string ClientId { get; set; }
    
        /// <summary>Task identifier.</summary>
        [Newtonsoft.Json.JsonProperty("taskId", Required = Newtonsoft.Json.Required.Always)]
        public int TaskId { get; set; }
    
        /// <summary>Task.</summary>
        [Newtonsoft.Json.JsonProperty("task", Required = Newtonsoft.Json.Required.Always)]
        [System.ComponentModel.DataAnnotations.Required]
        public string Task { get; set; }
    
        public string ToJson() 
        {
            return Newtonsoft.Json.JsonConvert.SerializeObject(this);
        }
        
        public static Rsp FromJson(string data)
        {
            return Newtonsoft.Json.JsonConvert.DeserializeObject<Rsp>(data);
        }
    
    }
}