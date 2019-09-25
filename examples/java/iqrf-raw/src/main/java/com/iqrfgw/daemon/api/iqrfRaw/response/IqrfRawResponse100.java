package com.iqrfgw.daemon.api.iqrfRaw.response;

import java.util.HashMap;
import java.util.Map;
import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.annotation.JsonPropertyDescription;
import com.fasterxml.jackson.annotation.JsonPropertyOrder;
import com.fasterxml.jackson.annotation.JsonValue;

@JsonInclude(JsonInclude.Include.NON_NULL)
@JsonPropertyOrder({
    "mType",
    "data"
})
public class IqrfRawResponse100 {

    /**
     * Message type for Raw communication.
     * (Required)
     * 
     */
    @JsonProperty("mType")
    @JsonPropertyDescription("Message type for Raw communication.")
    private IqrfRawResponse100 .MType mType;
    /**
     * 
     * (Required)
     * 
     */
    @JsonProperty("data")
    private Data data;

    /**
     * Message type for Raw communication.
     * (Required)
     * 
     */
    @JsonProperty("mType")
    public IqrfRawResponse100 .MType getmType() {
        return mType;
    }

    /**
     * Message type for Raw communication.
     * (Required)
     * 
     */
    @JsonProperty("mType")
    public void setmType(IqrfRawResponse100 .MType mType) {
        this.mType = mType;
    }

    /**
     * 
     * (Required)
     * 
     */
    @JsonProperty("data")
    public Data getData() {
        return data;
    }

    /**
     * 
     * (Required)
     * 
     */
    @JsonProperty("data")
    public void setData(Data data) {
        this.data = data;
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append(IqrfRawResponse100 .class.getName()).append('@').append(Integer.toHexString(System.identityHashCode(this))).append('[');
        sb.append("mType");
        sb.append('=');
        sb.append(((this.mType == null)?"<null>":this.mType));
        sb.append(',');
        sb.append("data");
        sb.append('=');
        sb.append(((this.data == null)?"<null>":this.data));
        sb.append(',');
        if (sb.charAt((sb.length()- 1)) == ',') {
            sb.setCharAt((sb.length()- 1), ']');
        } else {
            sb.append(']');
        }
        return sb.toString();
    }

    @Override
    public int hashCode() {
        int result = 1;
        result = ((result* 31)+((this.data == null)? 0 :this.data.hashCode()));
        result = ((result* 31)+((this.mType == null)? 0 :this.mType.hashCode()));
        return result;
    }

    @Override
    public boolean equals(Object other) {
        if (other == this) {
            return true;
        }
        if ((other instanceof IqrfRawResponse100) == false) {
            return false;
        }
        IqrfRawResponse100 rhs = ((IqrfRawResponse100) other);
        return (((this.data == rhs.data)||((this.data!= null)&&this.data.equals(rhs.data)))&&((this.mType == rhs.mType)||((this.mType!= null)&&this.mType.equals(rhs.mType))));
    }

    public enum MType {

        IQRF_RAW("iqrfRaw");
        private final String value;
        private final static Map<String, IqrfRawResponse100 .MType> CONSTANTS = new HashMap<String, IqrfRawResponse100 .MType>();

        static {
            for (IqrfRawResponse100 .MType c: values()) {
                CONSTANTS.put(c.value, c);
            }
        }

        private MType(String value) {
            this.value = value;
        }

        @Override
        public String toString() {
            return this.value;
        }

        @JsonValue
        public String value() {
            return this.value;
        }

        @JsonCreator
        public static IqrfRawResponse100 .MType fromValue(String value) {
            IqrfRawResponse100 .MType constant = CONSTANTS.get(value);
            if (constant == null) {
                throw new IllegalArgumentException(value);
            } else {
                return constant;
            }
        }

    }

}
