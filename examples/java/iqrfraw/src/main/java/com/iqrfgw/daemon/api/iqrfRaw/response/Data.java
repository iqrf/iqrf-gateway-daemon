package com.iqrfgw.daemon.api.iqrfRaw.response;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.annotation.JsonPropertyDescription;
import com.fasterxml.jackson.annotation.JsonPropertyOrder;

@JsonInclude(JsonInclude.Include.NON_NULL)
@JsonPropertyOrder({
    "msgId",
    "timeout",
    "rsp",
    "raw",
    "insId",
    "status",
    "statusStr"
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
    @JsonProperty("rsp")
    private Rsp rsp;
    @JsonProperty("raw")
    private Raw raw;
    /**
     * IQRF GW daemon instance identification.
     * 
     */
    @JsonProperty("insId")
    @JsonPropertyDescription("IQRF GW daemon instance identification.")
    private String insId;
    /**
     * IQRF GW daemon API (general or mType) status.
     * (Required)
     * 
     */
    @JsonProperty("status")
    @JsonPropertyDescription("IQRF GW daemon API (general or mType) status.")
    private Integer status;
    /**
     * IQRF GW daemon API (general or mType) status in string form.
     * 
     */
    @JsonProperty("statusStr")
    @JsonPropertyDescription("IQRF GW daemon API (general or mType) status in string form.")
    private String statusStr;

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
    @JsonProperty("rsp")
    public Rsp getRsp() {
        return rsp;
    }

    /**
     * 
     * (Required)
     * 
     */
    @JsonProperty("rsp")
    public void setRsp(Rsp rsp) {
        this.rsp = rsp;
    }

    @JsonProperty("raw")
    public Raw getRaw() {
        return raw;
    }

    @JsonProperty("raw")
    public void setRaw(Raw raw) {
        this.raw = raw;
    }

    /**
     * IQRF GW daemon instance identification.
     * 
     */
    @JsonProperty("insId")
    public String getInsId() {
        return insId;
    }

    /**
     * IQRF GW daemon instance identification.
     * 
     */
    @JsonProperty("insId")
    public void setInsId(String insId) {
        this.insId = insId;
    }

    /**
     * IQRF GW daemon API (general or mType) status.
     * (Required)
     * 
     */
    @JsonProperty("status")
    public Integer getStatus() {
        return status;
    }

    /**
     * IQRF GW daemon API (general or mType) status.
     * (Required)
     * 
     */
    @JsonProperty("status")
    public void setStatus(Integer status) {
        this.status = status;
    }

    /**
     * IQRF GW daemon API (general or mType) status in string form.
     * 
     */
    @JsonProperty("statusStr")
    public String getStatusStr() {
        return statusStr;
    }

    /**
     * IQRF GW daemon API (general or mType) status in string form.
     * 
     */
    @JsonProperty("statusStr")
    public void setStatusStr(String statusStr) {
        this.statusStr = statusStr;
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
        sb.append("rsp");
        sb.append('=');
        sb.append(((this.rsp == null)?"<null>":this.rsp));
        sb.append(',');
        sb.append("raw");
        sb.append('=');
        sb.append(((this.raw == null)?"<null>":this.raw));
        sb.append(',');
        sb.append("insId");
        sb.append('=');
        sb.append(((this.insId == null)?"<null>":this.insId));
        sb.append(',');
        sb.append("status");
        sb.append('=');
        sb.append(((this.status == null)?"<null>":this.status));
        sb.append(',');
        sb.append("statusStr");
        sb.append('=');
        sb.append(((this.statusStr == null)?"<null>":this.statusStr));
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
        result = ((result* 31)+((this.insId == null)? 0 :this.insId.hashCode()));
        result = ((result* 31)+((this.statusStr == null)? 0 :this.statusStr.hashCode()));
        result = ((result* 31)+((this.msgId == null)? 0 :this.msgId.hashCode()));
        result = ((result* 31)+((this.raw == null)? 0 :this.raw.hashCode()));
        result = ((result* 31)+((this.timeout == null)? 0 :this.timeout.hashCode()));
        result = ((result* 31)+((this.rsp == null)? 0 :this.rsp.hashCode()));
        result = ((result* 31)+((this.status == null)? 0 :this.status.hashCode()));
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
        return ((((((((this.insId == rhs.insId)||((this.insId!= null)&&this.insId.equals(rhs.insId)))&&((this.statusStr == rhs.statusStr)||((this.statusStr!= null)&&this.statusStr.equals(rhs.statusStr))))&&((this.msgId == rhs.msgId)||((this.msgId!= null)&&this.msgId.equals(rhs.msgId))))&&((this.raw == rhs.raw)||((this.raw!= null)&&this.raw.equals(rhs.raw))))&&((this.timeout == rhs.timeout)||((this.timeout!= null)&&this.timeout.equals(rhs.timeout))))&&((this.rsp == rhs.rsp)||((this.rsp!= null)&&this.rsp.equals(rhs.rsp))))&&((this.status == rhs.status)||((this.status!= null)&&this.status.equals(rhs.status))));
    }

}

