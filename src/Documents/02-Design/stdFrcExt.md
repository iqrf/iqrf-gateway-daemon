# Standard FRC JSON API extension

Standard Sensor and Dali implement dedicated FRC handling. This can be invoked by messages **iqrfSensor_Frc** and **iqrfStandard_Frc**. It has similar features. It differs from other API messages wrapping in fact two DPA transactions. The 1st one is covered by DPA FRC command, the 2nd one by DPA FRC extra result command. Returned data  is aggregation of information from more nodes. It is reasonable to annotate returned information by node address they are coming from and possibly associated metadata in the same way as standalone request to particular node, e.g. by **iqrfSensor_ReadSensorsWithTypes**

## General features
- **iqrf::JsonDpaApiIqrfStdExt** implements special handling for:
  - **iqrfSensor_Frc**
  - **iqrfDali_Frc**
- request message is handled by two DPA transaction in exclusive access to IQRF DPA interface to get consistent data 
- there may be just one DPA transaction if FRC extra result is not required. It is controlled by dedicate request item **"getExtraResult"**
- nadr and metadata annotation is controlled by **"getNadrWithMetaData"**. If it is set to false or not present the response in returned keeps backward compatible pattern

## JSON Schemes
Are slightly complicated as they cover variant responses:
- iqrfSensor_Frc-request-1-0-0.json
- iqrfSensor_Frc-response-1-0-0.json
- iqrfDali_Frc-request-1-0-0.json
- iqrfDali_Frc-response-1-0-0.json

## Configuration
To get associated metadata it is necessary to configure and enable **iqrf::JsonMngMetaDataApi** to be referenced (optionally) by **iqrf::JsonDpaApiIqrfStdExt**

## Message example
Message:
```json
{
    "mType": "iqrfSensor_Frc",
    "data": {
      "msgId": "testEmbedSensor",
      "req": {
        "nAdr": 0,
        "param": {
          "sensorType": 129,
          "sensorIndex": 0,
          "frcCommand": 144,
		  "selectedNodes": [3,4,5,6],
          "getExtraResult": true,
		  "getNadrWithMetaData": true
        }
      }
    }
}
```
Is responded by (note node 4 was switched off)
```json
{
  "mType": "iqrfSensor_Frc",
  "data": {
    "msgId": "testEmbedSensor",
    "rsp": {
      "nAdr": 0,
      "hwpId": 0,
      "rCode": 0,
      "dpaVal": 88,
      "result": {
        "sensors": [
          {
            "nadr": 3,
            "sensor": {
              "id": "BINARYDATA7",
              "type": 129,
              "name": "Binary data7",
              "shortName": "bin7",
              "value": 7,
              "unit": "?",
              "decimalPlaces": 0
            },
            "metaData": {}
          },
          {
            "nadr": 4,
            "sensor": null,
            "metaData": {}
          },
          {
            "nadr": 5,
            "sensor": {
              "id": "BINARYDATA7",
              "type": 129,
              "name": "Binary data7",
              "shortName": "bin7",
              "value": 7,
              "unit": "?",
              "decimalPlaces": 0
            },
            "metaData": {}
          },
          {
            "nadr": 6,
            "sensor": {
              "id": "BINARYDATA7",
              "type": 129,
              "name": "Binary data7",
              "shortName": "bin7",
              "value": 7,
              "unit": "?",
              "decimalPlaces": 0
            },
            "metaData": {}
          }
        ]
      }
    },
    "status": 0
  }
}
```

## Next steps
According requirement these enhancemets can be implemented:
- if required the response message can be split to many simpler response messages as it would be invoked by many  **iqrfSensor_ReadSensorsWithTypes**. It could be some traffic in larger networks