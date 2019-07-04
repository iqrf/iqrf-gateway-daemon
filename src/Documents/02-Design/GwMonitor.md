# GwMonitor component design

The GwMonitor component gets periodically an information about key components of running **iqrfgd2** It forms Json message with the information and sends it via websocket.

## General features
It monitors:
- **num** counter of the messages. It counts from zero at start-up 
- **timestamp** time since epoch in seconds (Unix time) 
- **dpaQueueLen** length of pending DPA transaction queue (16 is maximum)
- **dpaChannelState** state of DPA channel (one of CDC, SPI or UART interface)
 - Ready,
 - NotReady,
 - ExclusiveAcess
- **msgQueueLen** length of pending API msg queue (32 is maximum)
- **operMode** is operational mode
  - operational
  - service
  - forwarding

## Communication channel
The messages are send out to all of connected clients to 'ws://127.0.0.1:1438'
More parallel client connections are possible (Webapp, watcher process, Pixla, ...)

## Json message
The messages are sent periodically to connected clients.
Message content:
``` json
{
  "mType":"gwMonitor",
  "data":{
    "num":24,
    "timestamp":1562256558,
    "dpaQueueLen":0,
    "dpaChannelState":"Ready",
    "msgQueueLen":0,
    "operMode":"operational"
  }
}
```


## Configuration
The item "reportPeriod" is period of getting and sending information in seconds.
If you want to change WS port it has to be specified in configuration of referenced instance of `shape::IWebsocketService` and related here via property `"target": {"WebsocketPort": <port>"`
      }. 

```json
{
  "component": "iqrf::GwMonitorService",
  "instance": "iqrf::GwMonitorService",
  "reportPeriod": 10,
  "RequiredInterfaces": [
    {
      "name": "shape::IWebsocketService",
      "target": {
        "WebsocketPort": 1438
      }
    }
  ]
}
```