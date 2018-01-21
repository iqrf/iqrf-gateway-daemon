# Software Requirements

#### **DEM-01** Component Design [SPEC_01](../product-spec/product-spec.md#1-software-specification), [#6](https://gitlab.iqrfsdk.org/gateway/iqrf-daemon/issues/6)
  * Bussines logic decomposed to Components.
  * Components bound via Interfaces
  * Components are black boxes for other Components
  * Components may be part of broader solution (e.g. other networks)

#### **DEM-02** Async API
API is designed in dedicated repository:
https://gitlab.iqrfsdk.org/gateway/iqrf-daemon-api/
Daemon has to support all features required in API

#### **DEM-03** Scheduler [#9](https://gitlab.iqrfsdk.org/gateway/iqrf-daemon/issues/9)
Scheduler has to be implemented
- support Cron syntax in seconds granularity
- support planning of periodic actions
- initial configuration from file
- online planning by API

#### **DEM-04** Base DPA Service [#10](https://gitlab.iqrfsdk.org/gateway/iqrf-daemon/issues/10)
- allows send/receive DPA request/responses from API
- allows send/receive DPA request/responses from Scheduler

#### **DEM-05** Json Serializer/Parser [#11](https://gitlab.iqrfsdk.org/gateway/iqrf-daemon/issues/11)
- delivered messages from API has to be parsed from JSON
- hierarchical parser to cope with different schema dedicated to message purpose
- JSON schema validation has to be supported

#### **DEM-06** Udp Messaging [#12](https://gitlab.iqrfsdk.org/gateway/iqrf-daemon/issues/12)
- Allows connection with IQRF IDE4
- Connection has support these modes:
 - operational (IDE4 just connected, no DPA messages received/send)
 - forwarding (all DPA messages forwrded to IDE4)
 - service (IDE4 has direct access to IQRF network without daemon interfering)
- Switching of modes is done via API (configuration messages) 
- Just one connection is possible

#### **DEM-07** MQTT Messaging [#13](https://gitlab.iqrfsdk.org/gateway/iqrf-daemon/issues/13)
- Messaging channel to MQTT broker
- Messages according API
- Possible connection to more brokers simultaneously

#### **DEM-08** MQ Messaging [#14](https://gitlab.iqrfsdk.org/gateway/iqrf-daemon/issues/14)
- Inter process communication
- Messages according API
- Possible more connections simultaneously

#### **DEM-09** Standard Devices [#15](https://gitlab.iqrfsdk.org/gateway/iqrf-daemon/issues/15)
- Support of IQRF Standard devices
- Process JS downloaded from IQRF Standard devices repository

#### **DEM-10** Configuration Service [#16](https://gitlab.iqrfsdk.org/gateway/iqrf-daemon/issues/16)
- allows Daemon configuration via API

#### **DEM-11** Statistic Service [#17](https://gitlab.iqrfsdk.org/gateway/iqrf-daemon/issues/17)
- allows sending of statistic information as described by API

#### **DEM-12** Network Management Service
- allows IQRF control via messages from API

#### **DEM-13** JavaScript engine V8 Integration
- required to process downloaded Java Scripts  

#### **DEM-14** Embedded periphery
- Embedded periphery support.  
- Process JS downloaded from IQRF Standard devices repository

#### **DEM-15** CDC Interface
- CDC interface to IQRF coordinator
- Configurable features (port number)

#### **DEM-16** SPI Interface
- SPI interface to IQRF coordinator
- Configurable ports (GPIO) to support different HW wiring

#### **DEM-17** More IQRF networks
- It is possible to support more coordinators
