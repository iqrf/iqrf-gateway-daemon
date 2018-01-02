# API Specification

```
Instructions:
- Please feel free to work out both "API requirements" and "API Specification".
```

## 1 API requirements

1. MQTT messages.
    * Communication:
      * Request: --> Server (gateway)
      * Response: <-- Server (gateway)
      * Subscribed topic: Server (gateway) --> **???**  **_Do we want to let gateway send data out without any request? E.g. sensor triggers such event._**
    * Purpose:
      * Communication with nodes.
        * DPA format.
        * General sensor format. **???**  **_Do we need other formats than DPA?_**
      * Service commands.
      * Configuration commands.
      * Get statistics data.
2. Client - server data transfer.
    * Communication:
      * Request: --> Server (gateway)
      * Response: <-- Server (gateway)
    * Purpose:
      * Download data, JS and binaries.
      * Set data.
3. Repository access and downloads of drivers.
    * Communication:
      * Request: Client (gateway) --> Server (repo)
      * Response: Client (gateway) <-- Server (repo)
    * Purpose:
      * Download data, JS and binaries.      
4. Server - client data transfer. **_Not required in this version_**
    * Communication:
      * Request: Server (gateway) --> Clients
      * Response: Server (gateway) <-- Clients
    * Purpose:
      * Status update.

## 2 API Specification

#### [API_01] Logical API independent of transfer protocol.
* Transfer DPA, Standard Devices messages.
* Configuration.
* Scheduler messages.
* States messages.
* Service messages (updates).

#### [API_02] MQTT TCP/IP channel

[MQTT details..](https://en.wikipedia.org/wiki/MQTT)

**Covers:**
* 1.1 MQTT messages.

**Available API specifications:**
1. http://www.jsonrpc.org/
    * Example: [PROPOSAL_v0.md](https://gitlab.iqrfsdk.org/gateway/iqrf-daemon-api/blob/master/discussion/json-rpc/PROPOSAL_v0.md)
2. http://json-schema.org/
    * Example:[advantech-bb.com](http://advantech-bb.com/wp-content/uploads/2016/03/MQTT_Topics_and_JSON_Data_Format_R3_User_Manual_0316.pdf)
3. https://www.asyncapi.com/

**Specification:**

TBD....

#### [API_03] REST-API HTTP channel

[REST details..](https://en.wikipedia.org/wiki/Representational_state_transfer)

**Covers:**
* 1.2 Client - server data transfer.
* 1.3 Repository access and downloads of drivers.
* 1.4 Server - client data transfer.
  - This requirement can be done via REST-API with known limitations (client triggers periodical check).

**Available API specifications:**
1. https://www.openapis.org/
2. http://jsonapi.org/
3. http://json-schema.org/
    - it has C/C++ implementation

**Proposal:**
1. [OpenAPI 3.0.1](https://github.com/OAI/OpenAPI-Specification/blob/master/versions/3.0.1.md)
    * Broad support of this standard...

#### [API_04] WebSockets TCP channel

[WebSocket details..](https://en.wikipedia.org/wiki/WebSocket)

**Covers:**
* 1.4 Server - client data transfer.

**Available API specifications:**
1. https://www.openapis.org/
2. http://jsonapi.org/
3. http://json-schema.org/

**Proposal:**
The power of WebSockets is the ability to send data from server (gateway) to client (computer) without any client action.  So far we do not see such requirements, all configuration requirements can be done via REST API see chapter 2.2.

#### [API_05] IQRF Standard Devices Repository support.
