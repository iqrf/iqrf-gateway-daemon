package com.iqrfgw.daemon.api.iqrfRaw.response;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.annotation.JsonPropertyDescription;
import com.fasterxml.jackson.annotation.JsonPropertyOrder;

@JsonInclude(JsonInclude.Include.NON_NULL)
@JsonPropertyOrder({
    "rData"
})
public class Rsp {

    /**
     * Binary buffer with DPA response
     * (Required)
     * 
     */
    @JsonProperty("rData")
    @JsonPropertyDescription("Binary buffer with DPA response")
    private String rData;

    /**
     * Binary buffer with DPA response
     * (Required)
     * 
     */
    @JsonProperty("rData")
    public String getrData() {
        return rData;
    }

    /**
     * Binary buffer with DPA response
     * (Required)
     * 
     */
    @JsonProperty("rData")
    public void setrData(String rData) {
        this.rData = rData;
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append(Rsp.class.getName()).append('@').append(Integer.toHexString(System.identityHashCode(this))).append('[');
        sb.append("rData");
        sb.append('=');
        sb.append(((this.rData == null)?"<null>":this.rData));
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
        result = ((result* 31)+((this.rData == null)? 0 :this.rData.hashCode()));
        return result;
    }

    @Override
    public boolean equals(Object other) {
        if (other == this) {
            return true;
        }
        if ((other instanceof Rsp) == false) {
            return false;
        }
        Rsp rhs = ((Rsp) other);
        return ((this.rData == rhs.rData)||((this.rData!= null)&&this.rData.equals(rhs.rData)));
    }

}
