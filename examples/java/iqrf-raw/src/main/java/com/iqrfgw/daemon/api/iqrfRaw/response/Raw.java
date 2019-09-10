package com.iqrfgw.daemon.api.iqrfRaw.response;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.annotation.JsonPropertyDescription;
import com.fasterxml.jackson.annotation.JsonPropertyOrder;

@JsonInclude(JsonInclude.Include.NON_NULL)
@JsonPropertyOrder({
    "request",
    "requestTs",
    "confirmation",
    "confirmationTs",
    "response",
    "responseTs"
})
public class Raw {

    /**
     * Binary buffer with DPA request
     * (Required)
     * 
     */
    @JsonProperty("request")
    @JsonPropertyDescription("Binary buffer with DPA request")
    private String request;
    /**
     * YYYY-MM-DDTHH:MM:SS.MS
     * (Required)
     * 
     */
    @JsonProperty("requestTs")
    @JsonPropertyDescription("YYYY-MM-DDTHH:MM:SS.MS")
    private String requestTs;
    /**
     * Binary buffer with DPA confirmation
     * (Required)
     * 
     */
    @JsonProperty("confirmation")
    @JsonPropertyDescription("Binary buffer with DPA confirmation")
    private String confirmation;
    /**
     * YYYY-MM-DDTHH:MM:SS.MS
     * (Required)
     * 
     */
    @JsonProperty("confirmationTs")
    @JsonPropertyDescription("YYYY-MM-DDTHH:MM:SS.MS")
    private String confirmationTs;
    /**
     * Binary buffer with DPA response
     * (Required)
     * 
     */
    @JsonProperty("response")
    @JsonPropertyDescription("Binary buffer with DPA response")
    private String response;
    /**
     * YYYY-MM-DDTHH:MM:SS.MS
     * (Required)
     * 
     */
    @JsonProperty("responseTs")
    @JsonPropertyDescription("YYYY-MM-DDTHH:MM:SS.MS")
    private String responseTs;

    /**
     * Binary buffer with DPA request
     * (Required)
     * 
     */
    @JsonProperty("request")
    public String getRequest() {
        return request;
    }

    /**
     * Binary buffer with DPA request
     * (Required)
     * 
     */
    @JsonProperty("request")
    public void setRequest(String request) {
        this.request = request;
    }

    /**
     * YYYY-MM-DDTHH:MM:SS.MS
     * (Required)
     * 
     */
    @JsonProperty("requestTs")
    public String getRequestTs() {
        return requestTs;
    }

    /**
     * YYYY-MM-DDTHH:MM:SS.MS
     * (Required)
     * 
     */
    @JsonProperty("requestTs")
    public void setRequestTs(String requestTs) {
        this.requestTs = requestTs;
    }

    /**
     * Binary buffer with DPA confirmation
     * (Required)
     * 
     */
    @JsonProperty("confirmation")
    public String getConfirmation() {
        return confirmation;
    }

    /**
     * Binary buffer with DPA confirmation
     * (Required)
     * 
     */
    @JsonProperty("confirmation")
    public void setConfirmation(String confirmation) {
        this.confirmation = confirmation;
    }

    /**
     * YYYY-MM-DDTHH:MM:SS.MS
     * (Required)
     * 
     */
    @JsonProperty("confirmationTs")
    public String getConfirmationTs() {
        return confirmationTs;
    }

    /**
     * YYYY-MM-DDTHH:MM:SS.MS
     * (Required)
     * 
     */
    @JsonProperty("confirmationTs")
    public void setConfirmationTs(String confirmationTs) {
        this.confirmationTs = confirmationTs;
    }

    /**
     * Binary buffer with DPA response
     * (Required)
     * 
     */
    @JsonProperty("response")
    public String getResponse() {
        return response;
    }

    /**
     * Binary buffer with DPA response
     * (Required)
     * 
     */
    @JsonProperty("response")
    public void setResponse(String response) {
        this.response = response;
    }

    /**
     * YYYY-MM-DDTHH:MM:SS.MS
     * (Required)
     * 
     */
    @JsonProperty("responseTs")
    public String getResponseTs() {
        return responseTs;
    }

    /**
     * YYYY-MM-DDTHH:MM:SS.MS
     * (Required)
     * 
     */
    @JsonProperty("responseTs")
    public void setResponseTs(String responseTs) {
        this.responseTs = responseTs;
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append(Raw.class.getName()).append('@').append(Integer.toHexString(System.identityHashCode(this))).append('[');
        sb.append("request");
        sb.append('=');
        sb.append(((this.request == null)?"<null>":this.request));
        sb.append(',');
        sb.append("requestTs");
        sb.append('=');
        sb.append(((this.requestTs == null)?"<null>":this.requestTs));
        sb.append(',');
        sb.append("confirmation");
        sb.append('=');
        sb.append(((this.confirmation == null)?"<null>":this.confirmation));
        sb.append(',');
        sb.append("confirmationTs");
        sb.append('=');
        sb.append(((this.confirmationTs == null)?"<null>":this.confirmationTs));
        sb.append(',');
        sb.append("response");
        sb.append('=');
        sb.append(((this.response == null)?"<null>":this.response));
        sb.append(',');
        sb.append("responseTs");
        sb.append('=');
        sb.append(((this.responseTs == null)?"<null>":this.responseTs));
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
        result = ((result* 31)+((this.request == null)? 0 :this.request.hashCode()));
        result = ((result* 31)+((this.requestTs == null)? 0 :this.requestTs.hashCode()));
        result = ((result* 31)+((this.responseTs == null)? 0 :this.responseTs.hashCode()));
        result = ((result* 31)+((this.response == null)? 0 :this.response.hashCode()));
        result = ((result* 31)+((this.confirmationTs == null)? 0 :this.confirmationTs.hashCode()));
        result = ((result* 31)+((this.confirmation == null)? 0 :this.confirmation.hashCode()));
        return result;
    }

    @Override
    public boolean equals(Object other) {
        if (other == this) {
            return true;
        }
        if ((other instanceof Raw) == false) {
            return false;
        }
        Raw rhs = ((Raw) other);
        return (((((((this.request == rhs.request)||((this.request!= null)&&this.request.equals(rhs.request)))&&((this.requestTs == rhs.requestTs)||((this.requestTs!= null)&&this.requestTs.equals(rhs.requestTs))))&&((this.responseTs == rhs.responseTs)||((this.responseTs!= null)&&this.responseTs.equals(rhs.responseTs))))&&((this.response == rhs.response)||((this.response!= null)&&this.response.equals(rhs.response))))&&((this.confirmationTs == rhs.confirmationTs)||((this.confirmationTs!= null)&&this.confirmationTs.equals(rhs.confirmationTs))))&&((this.confirmation == rhs.confirmation)||((this.confirmation!= null)&&this.confirmation.equals(rhs.confirmation))));
    }

}
