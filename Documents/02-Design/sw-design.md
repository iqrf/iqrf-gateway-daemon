# API Design

## 1 MQTT Topics

IQRF Daemon suffix is:
```
iqrf/mcat
```
* _iqrf_ - constant identifier
* _mcat_ - IQRF message category (unique command identifier)

### Azure examples [(link..)](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-mqtt-support#using-the-mqtt-protocol-directly)

* gateway-to-cloud messages

```
devices/{device_id}/messages/events/iqrf/mcat
```

* cloud-to-gateway messages

```
devices/{device_id}/messages/devicebound/iqrf/mcat
```

### IBM Cloud examples [link..](https://console.bluemix.net/docs/services/IoT/gateways/mqtt.html#mqtt)

* gateway-to-cloud messages

```
iot-2/type/mygateway/id/gateway1/evt/status/fmt/json/iqrf/mcat
```

* cloud-to-gateway messages

```
iot-2/type/typeId/id/deviceId/cmd/commandId/fmt/formatString/iqrf/mcat
```

## 2 Messages

### 2.1 Messages parameters

* `"mcat": "string"` - Message category represents unique command identifier.
* `"msgid": "string"` - Message identification unique for a sender. If used in request and repeated in response. Then it can be used by the sender to match outgoing and incoming messages.
* `"timeout": "integer"` - Timeout value for DPA request. if zero, the timeout is infinite else it represents time in milliseconds. If omitted the default timer value is used.
* `"request": "string"` - Bytes to be sent as a DPA request IQRF network. Data is coded as a variable number of 2 character pairs separated by dot or space, e.g. _00.00.06.03.ff.ff_.
* `"response": "string"` - Bytes received as a DPA response from IQRF network. Data is coded as a variable number of 2 character pairs separated by dot or space, e.g. _00.00.06.03.ff.ff_.
* `"return_verbose": "boolean"` - TBD.
* `"return_status": "boolean"` - Status response at coordinator.
* `"request_ts": "string"` - Timestamp of request YYYY-MM-DDTHH:MM:SS.MS e.g. _2017-12-20T20:21:05.123_
* `"response_ts": "string"` - Timestamp of response YYYY-MM-DDTHH:MM:SS.MS e.g. _2017-12-20T20:21:05.123_
* `"nadr": "string"` - Node address as defined in "foursome".
* `"pnum": "string"` - Periphery number as defined in "foursome".
* `"pcmd": "string"` - Periphery command as defined in "foursome".
* `"hwpid": "string"` - Hardware profile identification as defined in "foursome". It can be optional, in this case the default value is ffff.

### 2.2 Raw DPA Request

* Message: sender-to-gateway
* Message category (mcat): ntw-com-raw
* Scheme: [ntw-com-raw-request.json](../../JsonSchemes/async-api-json-schemes/ntw-com-raw-request.json)
* Example: [msg-ntw-com-raw-request.json](../../JsonSchemes/async-api-json-schemes/examples/msg-ntw-com-raw-request.json)

````
{
	"mcat": "ntw-com-raw",
	"msg": {
		"msgid": "CCCC",
		"timeout": 1,
		"req": {
			"request": "EEEE"
		},
		"return_verbose": false
	}
}
````

### 2.3 Raw DPA Response

* Message: gateway-to-sender as a response to Raw DPA Request
* Message category (mcat): ntw-com-raw
* Scheme: [ntw-com-raw-response.json](../../JsonSchemes/async-api-json-schemes/ntw-com-raw-response.json)
* Example: [msg-ntw-com-raw-response.json](../../JsonSchemes/async-api-json-schemes/examples/msg-ntw-com-raw-response.json)

````
{
	"mcat": "ntw-com-raw",
	"msg": {
		"msgid": "CCCC",
		"timeout": 1,
		"resp": {
			"response": "EEEE"
		},
		"raw": {
			"request": "DD",
			"confirmation": "aaaaa",
			"response": "c",
			"request_ts": "aa",
			"confirmation_ts": "B",
			"response_ts": "CCC"
		},
		"status": 1,
		"status_str": "C"
	}
}
````

### 2.4 Raw HDP Request

* Message: sender-to-gateway
* Message category (mcat): ntw-com-raw-hdp
* Scheme: [ntw-com-raw-hdp-request.json](../../JsonSchemes/async-api-json-schemes/ntw-com-raw-hdp-request.json)
* Example: [msg-ntw-com-raw-hdp-request.json](../../JsonSchemes/async-api-json-schemes/examples/msg-ntw-com-raw-hdp-request.json)

````
{
	"mcat": "ntw-com-raw-hdp",
	"msg": {
		"msgid": "CCCC",
		"timeout": 1,
		"req": {
			"nadr": "EEEE",
			"pnum": "DD",
			"pcmd": "aaaaa",
			"hwpid": "c",
			"request": "aa"
		},
		"return_verbose": false
	}
}
````

### 2.5 Raw HDP Response

* Message: gateway-to-sender
* Message category (mcat): ntw-com-raw-hdp
* Scheme: [ntw-com-raw-hdp-response.json](../../JsonSchemes/async-api-json-schemes/ntw-com-raw-hdp-response.json)
* Example: [msg-ntw-com-raw-hdp-response.json](../../JsonSchemes/async-api-json-schemes/examples/msg-ntw-com-raw-hdp-response.json)

````
{
	"mcat": "ntw-com-raw",
	"msg": {
		"msgid": "CCCC",
		"timeout": 1,
		"resp": {
			"nadr": "EEEE",
			"pnum": "DD",
			"pcmd": "aaaaa",
			"hwpid": "c",
			"response": "aa"
		},
		"raw": {
			"request": "B",
			"confirmation": "CCC",
			"response": "C",
			"request_ts": "ddddd",
			"confirmation_ts": "B",
			"response_ts": "ee"
		},
		"status": 1,
		"status_str": "EEEEE"
	}
}
````

### 2.6 Embedded Periphery Thermometer Request

* Message: sender-to-gateway
* Message category (mcat): ntw-com-embper-therm
* Scheme: [ntw-com-embper-therm-request.json](../../JsonSchemes/async-api-json-schemes/ntw-com-embper-therm-request.json)
* Example: [msg-ntw-com-embper-therm-request.json](../../JsonSchemes/async-api-json-schemes/examples/msg-ntw-com-embper-thermometer-request.json)

````
{
	"mcat": "ntw-com-embper-therm",
	"msg": {
		"msgid": "CCCC",
		"timeout": 1,
		"cmd": {
			"command": "EEEE"
		},
		"return_verbose": false
	}
}
````

### 2.7  Embedded Periphery Thermometer Response

* Message: gateway-to-sender
* Message category (mcat): ntw-com-embper-therm
* Scheme: [ntw-com-embper-therm-response.json](../../JsonSchemes/async-api-json-schemes/ntw-com-embper-therm-response.json)
* Example: [msg-ntw-com-embper-therm-response.json](../../JsonSchemes/async-api-json-schemes/examples/msg-ntw-com-embper-thermometer-response.json)

````
{
	"mcat": "ntw-com-embper-therm",
	"msg": {
		"msgid": "CCCC",
		"timeout": 1,
		"resp": {
			"response": "EEEE"
		},
		"raw": {
			"request": "DD",
			"confirmation": "aaaaa",
			"response": "c",
			"request_ts": "aa",
			"confirmation_ts": "B",
			"response_ts": "CCC"
		},
		"status": 1,
		"status_str": "C"
	}
}
````
