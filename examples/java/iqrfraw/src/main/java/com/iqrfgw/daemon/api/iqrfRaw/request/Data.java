package com.iqrfgw.daemon.api.iqrfRaw.request;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.annotation.JsonPropertyDescription;
import com.fasterxml.jackson.annotation.JsonPropertyOrder;

@JsonInclude(JsonInclude.Include.NON_NULL)
@JsonPropertyOrder({
    "msgId",
    "timeout",
    "req",
    "returnVerbose"
})
public class Data {

    /**
     * Message identification for binding request with response.
     * (Required)
     * 
     */
    @JsonProperty("msgId")
    @JsonPropertyDescription("Message identification for binding request with response.")
    private String msgId;
    /**
     * Timeout to wait for IQRF DPA response
     * 
     */
    @JsonProperty("timeout")
    @JsonPropertyDescription("Timeout to wait for IQRF DPA response")
    private Integer timeout;
    /**
     * 
     * (Required)
     * 
     */
    @JsonProperty("req")
    private Req req;
    /**
     * Flag that includes additional parameters in the response.
     * 
     */
    @JsonProperty("returnVerbose")
    @JsonPropertyDescription("Flag that includes additional parameters in the response.")
    private Boolean returnVerbose;

    /**
     * Message identification for binding request with response.
     * (Required)
     * 
     */
    @JsonProperty("msgId")
    public String getMsgId() {
        return msgId;
    }

    /**
     * Message identification for binding request with response.
     * (Required)
     * 
     */
    @JsonProperty("msgId")
    public void setMsgId(String msgId) {
        this.msgId = msgId;
    }

    /**
     * Timeout to wait for IQRF DPA response
     * 
     */
    @JsonProperty("timeout")
    public Integer getTimeout() {
        return timeout;
    }

    /**
     * Timeout to wait for IQRF DPA response
     * 
     */
    @JsonProperty("timeout")
    public void setTimeout(Integer timeout) {
        this.timeout = timeout;
    }

    /**
     * 
     * (Required)
     * 
     */
    @JsonProperty("req")
    public Req getReq() {
        return req;
    }

    /**
     * 
     * (Required)
     * 
     */
    @JsonProperty("req")
    public void setReq(Req req) {
        this.req = req;
    }

    /**
     * Flag that includes additional parameters in the response.
     * 
     */
    @JsonProperty("returnVerbose")
    public Boolean getReturnVerbose() {
        return returnVerbose;
    }

    /**
     * Flag that includes additional parameters in the response.
     * 
     */
    @JsonProperty("returnVerbose")
    public void setReturnVerbose(Boolean returnVerbose) {
        this.returnVerbose = returnVerbose;
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append(Data.class.getName()).append('@').append(Integer.toHexString(System.identityHashCode(this))).append('[');
        sb.append("msgId");
        sb.append('=');
        sb.append(((this.msgId == null)?"<null>":this.msgId));
        sb.append(',');
        sb.append("timeout");
        sb.append('=');
        sb.append(((this.timeout == null)?"<null>":this.timeout));
        sb.append(',');
        sb.append("req");
        sb.append('=');
        sb.append(((this.req == null)?"<null>":this.req));
        sb.append(',');
        sb.append("returnVerbose");
        sb.append('=');
        sb.append(((this.returnVerbose == null)?"<null>":this.returnVerbose));
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
        result = ((result* 31)+((this.msgId == null)? 0 :this.msgId.hashCode()));
        result = ((result* 31)+((this.returnVerbose == null)? 0 :this.returnVerbose.hashCode()));
        result = ((result* 31)+((this.timeout == null)? 0 :this.timeout.hashCode()));
        result = ((result* 31)+((this.req == null)? 0 :this.req.hashCode()));
        return result;
    }

    @Override
    public boolean equals(Object other) {
        if (other == this) {
            return true;
        }
        if ((other instanceof Data) == false) {
            return false;
        }
        Data rhs = ((Data) other);
        return (((((this.msgId == rhs.msgId)||((this.msgId!= null)&&this.msgId.equals(rhs.msgId)))&&((this.returnVerbose == rhs.returnVerbose)||((this.returnVerbose!= null)&&this.returnVerbose.equals(rhs.returnVerbose))))&&((this.timeout == rhs.timeout)||((this.timeout!= null)&&this.timeout.equals(rhs.timeout))))&&((this.req == rhs.req)||((this.req!= null)&&this.req.equals(rhs.req))));
    }

}
