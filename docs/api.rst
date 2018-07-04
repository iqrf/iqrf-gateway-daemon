Gateway-daemon API
==================

JSON messages for communication via MQ/WS/MQTT channels. Messages are compatible with OS v4.02D and 
DPA v3.02 at TR-7xD. 

IQRF Generic
------------

Raw
+++

- `Raw request v1-0-0`_ and `example`__ 
- `Raw response v1-0-0`_ and `example`__

.. _`Raw request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfRaw-request-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfRaw-request-1-0-0-example.json
.. _`Raw response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfRaw-response-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfRaw-response-1-0-0-example.json

RawHdp
++++++

- `RawHdp request v1-0-0`_ and `example`__
- `RawHdp response v1-0-0`_ and and `example`__

.. _`RawHdp request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfRawHdp-request-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfRawHdp-request-1-0-0-example.json
.. _`RawHdp response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfRawHdp-response-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfRawHdp-response-1-0-0-example.json

IQRF Standard
-------------

Embed Explore
+++++++++++++

- `Enumerate request v1-0-0`_
- `Enumerate response v1-0-0`_
- `PeripheralInformation request v1-0-0`_
- `PeripheralInformation response v1-0-0`_
- `MorePeripheralsInformation request v1-0-0`_
- `MorePeripheralsInformation response v1-0-0`_

.. _`Enumerate request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedExplore_Enumerate-request-1-0-0.json 
.. _`Enumerate response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedExplore_Enumerate-response-1-0-0.json 
.. _`PeripheralInformation request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedExplore_PeripheralInformation-request-1-0-0.json 
.. _`PeripheralInformation response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedExplore_PeripheralInformation-response-1-0-0.json 
.. _`MorePeripheralsInformation request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedExplore_MorePeripheralsInformation-request-1-0-0.json 
.. _`MorePeripheralsInformation response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedExplore_MorePeripheralsInformation-response-1-0-0.json

Embed Coordinator
+++++++++++++++++

- `AddrInfo request v1-0-0`_
- `AddrInfo response v1-0-0`_
- `DiscoveredDevices request v1-0-0`_
- `DiscoveredDevices response v1-0-0`_
- `BondedDevices request v1-0-0`_
- `BondedDevices response v1-0-0`_
- `ClearAllBonds request v1-0-0`_
- `ClearAllBonds response v1-0-0`_
- `BondNode request v1-0-0`_
- `BondNode response v1-0-0`_
- `RemoveBond request v1-0-0`_
- `RemoveBond response v1-0-0`_
- `Rebond request v1-0-0`_
- `Rebond response v1-0-0`_
- `Discovery request v1-0-0`_
- `Discovery response v1-0-0`_
- `SetDpaParams request v1-0-0`_
- `SetDpaParams response v1-0-0`_
- `SetHops request v1-0-0`_
- `SetHops response v1-0-0`_
- `DiscoveryData request v1-0-0`_
- `DiscoveryData response v1-0-0`_
- `Backup request v1-0-0`_
- `Backup response v1-0-0`_
- `Restore request v1-0-0`_
- `Restore response v1-0-0`_
- `AuthorizeBond request v1-0-0`_
- `AuthorizeBond response v1-0-0`_
- `ReadRemotelyBondedMid request v1-0-0`_
- `ReadRemotelyBondedMid response v1-0-0`_
- `ClearRemotelyBondedMid request v1-0-0`_
- `ClearRemotelyBondedMid response v1-0-0`_
- `EnableRemoteBonding request v1-0-0`_
- `EnableRemoteBonding response v1-0-0`_

.. _`AddrInfo request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_AddrInfo-request-1-0-0.json
.. _`AddrInfo response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_AddrInfo-response-1-0-0.json
.. _`DiscoveredDevices request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_DiscoveredDevices-request-1-0-0.json
.. _`DiscoveredDevices response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_DiscoveredDevices-response-1-0-0.json
.. _`BondedDevices request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_BondedDevices-request-1-0-0.json
.. _`BondedDevices response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_BondedDevices-response-1-0-0.json
.. _`ClearAllBonds request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_ClearAllBonds-request-1-0-0.json
.. _`ClearAllBonds response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_ClearAllBonds-response-1-0-0.json
.. _`BondNode request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_BondNode-request-1-0-0.json
.. _`BondNode response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_BondNode-response-1-0-0.json
.. _`RemoveBond request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_RemoveBond-request-1-0-0.json
.. _`RemoveBond response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_RemoveBond-response-1-0-0.json
.. _`Rebond request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_Rebond-request-1-0-0.json 
.. _`Rebond response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_Rebond-response-1-0-0.json
.. _`Discovery request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_Discovery-request-1-0-0.json
.. _`Discovery response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_Discovery-response-1-0-0.json
.. _`SetDpaParams request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_SetDpaParams-request-1-0-0.json
.. _`SetDpaParams response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_SetDpaParams-response-1-0-0.json
.. _`SetHops request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_SetHops-request-1-0-0.json
.. _`SetHops response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_SetHops-response-1-0-0.json
.. _`DiscoveryData request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_DiscoveryData-request-1-0-0.json
.. _`DiscoveryData response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_DiscoveryData-response-1-0-0.json
.. _`Backup request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_Backup-request-1-0-0.json
.. _`Backup response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_Backup-response-1-0-0.json
.. _`Restore request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_Restore-request-1-0-0.json
.. _`Restore response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_Restore-response-1-0-0.json
.. _`AuthorizeBond request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_AuthorizeBond-request-1-0-0.json
.. _`AuthorizeBond response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_AuthorizeBond-response-1-0-0.json
.. _`ReadRemotelyBondedMid request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_ReadRemotelyBondedMid-request-1-0-0.json
.. _`ReadRemotelyBondedMid response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_ReadRemotelyBondedMid-response-1-0-0.json
.. _`ClearRemotelyBondedMid request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_ClearRemotelyBondedMid-request-1-0-0.json
.. _`ClearRemotelyBondedMid response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_ClearRemotelyBondedMid-response-1-0-0.json
.. _`EnableRemoteBonding request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_EnableRemoteBonding-request-1-0-0.json
.. _`EnableRemoteBonding response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_EnableRemoteBonding-response-1-0-0.json

IQMESH Network
--------------



Daemon Scheduler
----------------



Daemon Management
-----------------



Daemon Configuration
--------------------
