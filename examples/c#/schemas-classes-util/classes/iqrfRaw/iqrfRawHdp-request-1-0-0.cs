//----------------------
// <auto-generated>
//     Generated using the NJsonSchema v9.10.56.0 (Newtonsoft.Json v9.0.0.0) (http://NJsonSchema.org)
// </auto-generated>
//----------------------

namespace MyNamespace
{
    #pragma warning disable // Disable all warnings

    [System.CodeDom.Compiler.GeneratedCode("NJsonSchema", "9.10.56.0 (Newtonsoft.Json v9.0.0.0)")]
    public partial class Anonymous : System.ComponentModel.INotifyPropertyChanged
    {
        private MType _mType;
        private Data _data = new Data();
    
        /// <summary>Message type for Raw Hdp communication.</summary>
        [Newtonsoft.Json.JsonProperty("mType", Required = Newtonsoft.Json.Required.Always)]
        [System.ComponentModel.DataAnnotations.Required(AllowEmptyStrings = true)]
        [Newtonsoft.Json.JsonConverter(typeof(Newtonsoft.Json.Converters.StringEnumConverter))]
        public MType MType
        {
            get { return _mType; }
            set 
            {
                if (_mType != value)
                {
                    _mType = value; 
                    RaisePropertyChanged();
                }
            }
        }
    
        [Newtonsoft.Json.JsonProperty("data", Required = Newtonsoft.Json.Required.Always)]
        [System.ComponentModel.DataAnnotations.Required]
        public Data Data
        {
            get { return _data; }
            set 
            {
                if (_data != value)
                {
                    _data = value; 
                    RaisePropertyChanged();
                }
            }
        }
    
        public string ToJson() 
        {
            return Newtonsoft.Json.JsonConvert.SerializeObject(this);
        }
        
        public static Anonymous FromJson(string data)
        {
            return Newtonsoft.Json.JsonConvert.DeserializeObject<Anonymous>(data);
        }
    
        public event System.ComponentModel.PropertyChangedEventHandler PropertyChanged;
        
        protected virtual void RaisePropertyChanged([System.Runtime.CompilerServices.CallerMemberName] string propertyName = null)
        {
            var handler = PropertyChanged;
            if (handler != null) 
                handler(this, new System.ComponentModel.PropertyChangedEventArgs(propertyName));
        }
    
    }
    
    [System.CodeDom.Compiler.GeneratedCode("NJsonSchema", "9.10.56.0 (Newtonsoft.Json v9.0.0.0)")]
    public enum MType
    {
        [System.Runtime.Serialization.EnumMember(Value = "iqrfRawHdp")]
    
        IqrfRawHdp = 0,
    
    
    }
    
    [System.CodeDom.Compiler.GeneratedCode("NJsonSchema", "9.10.56.0 (Newtonsoft.Json v9.0.0.0)")]
    public partial class Data : System.ComponentModel.INotifyPropertyChanged
    {
        private string _msgId;
        private int _timeout;
        private Req _req = new Req();
        private bool _returnVerbose;
    
        /// <summary>Message identification for binding request with response.</summary>
        [Newtonsoft.Json.JsonProperty("msgId", Required = Newtonsoft.Json.Required.Always)]
        [System.ComponentModel.DataAnnotations.Required(AllowEmptyStrings = true)]
        public string MsgId
        {
            get { return _msgId; }
            set 
            {
                if (_msgId != value)
                {
                    _msgId = value; 
                    RaisePropertyChanged();
                }
            }
        }
    
        /// <summary>Timeout to wait for IQRF DPA response.</summary>
        [Newtonsoft.Json.JsonProperty("timeout", Required = Newtonsoft.Json.Required.DisallowNull, NullValueHandling = Newtonsoft.Json.NullValueHandling.Ignore)]
        public int Timeout
        {
            get { return _timeout; }
            set 
            {
                if (_timeout != value)
                {
                    _timeout = value; 
                    RaisePropertyChanged();
                }
            }
        }
    
        [Newtonsoft.Json.JsonProperty("req", Required = Newtonsoft.Json.Required.Always)]
        [System.ComponentModel.DataAnnotations.Required]
        public Req Req
        {
            get { return _req; }
            set 
            {
                if (_req != value)
                {
                    _req = value; 
                    RaisePropertyChanged();
                }
            }
        }
    
        /// <summary>Flag that includes additional parameters in the response.</summary>
        [Newtonsoft.Json.JsonProperty("returnVerbose", Required = Newtonsoft.Json.Required.DisallowNull, NullValueHandling = Newtonsoft.Json.NullValueHandling.Ignore)]
        public bool ReturnVerbose
        {
            get { return _returnVerbose; }
            set 
            {
                if (_returnVerbose != value)
                {
                    _returnVerbose = value; 
                    RaisePropertyChanged();
                }
            }
        }
    
        public string ToJson() 
        {
            return Newtonsoft.Json.JsonConvert.SerializeObject(this);
        }
        
        public static Data FromJson(string data)
        {
            return Newtonsoft.Json.JsonConvert.DeserializeObject<Data>(data);
        }
    
        public event System.ComponentModel.PropertyChangedEventHandler PropertyChanged;
        
        protected virtual void RaisePropertyChanged([System.Runtime.CompilerServices.CallerMemberName] string propertyName = null)
        {
            var handler = PropertyChanged;
            if (handler != null) 
                handler(this, new System.ComponentModel.PropertyChangedEventArgs(propertyName));
        }
    
    }
    
    [System.CodeDom.Compiler.GeneratedCode("NJsonSchema", "9.10.56.0 (Newtonsoft.Json v9.0.0.0)")]
    public partial class Req : System.ComponentModel.INotifyPropertyChanged
    {
        private int _nAdr;
        private int _pNum;
        private int _pCmd;
        private int _hwpId;
        private System.Collections.ObjectModel.ObservableCollection<int> _pData = new System.Collections.ObjectModel.ObservableCollection<int>();
    
        /// <summary>Network device address.</summary>
        [Newtonsoft.Json.JsonProperty("nAdr", Required = Newtonsoft.Json.Required.Always)]
        public int NAdr
        {
            get { return _nAdr; }
            set 
            {
                if (_nAdr != value)
                {
                    _nAdr = value; 
                    RaisePropertyChanged();
                }
            }
        }
    
        /// <summary>Peripheral integer.</summary>
        [Newtonsoft.Json.JsonProperty("pNum", Required = Newtonsoft.Json.Required.Always)]
        public int PNum
        {
            get { return _pNum; }
            set 
            {
                if (_pNum != value)
                {
                    _pNum = value; 
                    RaisePropertyChanged();
                }
            }
        }
    
        /// <summary>Peripheral command.</summary>
        [Newtonsoft.Json.JsonProperty("pCmd", Required = Newtonsoft.Json.Required.Always)]
        public int PCmd
        {
            get { return _pCmd; }
            set 
            {
                if (_pCmd != value)
                {
                    _pCmd = value; 
                    RaisePropertyChanged();
                }
            }
        }
    
        /// <summary>Hardware profile identification.</summary>
        [Newtonsoft.Json.JsonProperty("hwpId", Required = Newtonsoft.Json.Required.DisallowNull, NullValueHandling = Newtonsoft.Json.NullValueHandling.Ignore)]
        public int HwpId
        {
            get { return _hwpId; }
            set 
            {
                if (_hwpId != value)
                {
                    _hwpId = value; 
                    RaisePropertyChanged();
                }
            }
        }
    
        /// <summary>Binary buffer with DPA request data.</summary>
        [Newtonsoft.Json.JsonProperty("pData", Required = Newtonsoft.Json.Required.DisallowNull, NullValueHandling = Newtonsoft.Json.NullValueHandling.Ignore)]
        public System.Collections.ObjectModel.ObservableCollection<int> PData
        {
            get { return _pData; }
            set 
            {
                if (_pData != value)
                {
                    _pData = value; 
                    RaisePropertyChanged();
                }
            }
        }
    
        public string ToJson() 
        {
            return Newtonsoft.Json.JsonConvert.SerializeObject(this);
        }
        
        public static Req FromJson(string data)
        {
            return Newtonsoft.Json.JsonConvert.DeserializeObject<Req>(data);
        }
    
        public event System.ComponentModel.PropertyChangedEventHandler PropertyChanged;
        
        protected virtual void RaisePropertyChanged([System.Runtime.CompilerServices.CallerMemberName] string propertyName = null)
        {
            var handler = PropertyChanged;
            if (handler != null) 
                handler(this, new System.ComponentModel.PropertyChangedEventArgs(propertyName));
        }
    
    }
}