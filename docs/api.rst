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

- `Enumerate request v1-0-0`_ and `example`__
- `Enumerate response v1-0-0`_ and `example`__
- `PeripheralInformation request v1-0-0`_ and `example`__
- `PeripheralInformation response v1-0-0`_ and `example`__
- `MorePeripheralsInformation request v1-0-0`_ and `example`__
- `MorePeripheralsInformation response v1-0-0`_ and `example`__

.. _`Enumerate request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedExplore_Enumerate-request-1-0-0.json 
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedExplore_Enumerate-request-1-0-0-example.json
.. _`Enumerate response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedExplore_Enumerate-response-1-0-0.json 
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedExplore_Enumerate-response-1-0-0-example.json
.. _`PeripheralInformation request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedExplore_PeripheralInformation-request-1-0-0.json 
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedExplore_PeripheralInformation-request-1-0-0-example.json
.. _`PeripheralInformation response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedExplore_PeripheralInformation-response-1-0-0.json 
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedExplore_PeripheralInformation-response-1-0-0-example.json
.. _`MorePeripheralsInformation request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedExplore_MorePeripheralsInformation-request-1-0-0.json 
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedExplore_MorePeripheralsInformation-request-1-0-0-example.json
.. _`MorePeripheralsInformation response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedExplore_MorePeripheralsInformation-response-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedExplore_MorePeripheralsInformation-response-1-0-0-example.json

Embed Coordinator
+++++++++++++++++

- `AddrInfo request v1-0-0`_ and `example`__
- `AddrInfo response v1-0-0`_ and `example`__
- `DiscoveredDevices request v1-0-0`_ and `example`__
- `DiscoveredDevices response v1-0-0`_ and `example`__
- `BondedDevices request v1-0-0`_ and `example`__
- `BondedDevices response v1-0-0`_ and `example`__
- `ClearAllBonds request v1-0-0`_ and `example`__
- `ClearAllBonds response v1-0-0`_ and `example`__
- `BondNode request v1-0-0`_ and `example`__
- `BondNode response v1-0-0`_ and `example`__
- `RemoveBond request v1-0-0`_ and `example`__
- `RemoveBond response v1-0-0`_ and `example`__
- `Rebond request v1-0-0`_ and `example`__
- `Rebond response v1-0-0`_ and `example`__
- `Discovery request v1-0-0`_ and `example`__
- `Discovery response v1-0-0`_ and `example`__
- `SetDpaParams request v1-0-0`_ and `example`__
- `SetDpaParams response v1-0-0`_ and `example`__
- `SetHops request v1-0-0`_ and `example`__
- `SetHops response v1-0-0`_ and `example`__
- `DiscoveryData request v1-0-0`_ and `example`__
- `DiscoveryData response v1-0-0`_ and `example`__
- `Backup request v1-0-0`_ and `example`__
- `Backup response v1-0-0`_ and `example`__
- `Restore request v1-0-0`_ and `example`__
- `Restore response v1-0-0`_ and `example`__
- `AuthorizeBond request v1-0-0`_ and `example`__
- `AuthorizeBond response v1-0-0`_ and `example`__
- `ReadRemotelyBondedMid request v1-0-0`_ and `example`__
- `ReadRemotelyBondedMid response v1-0-0`_ and `example`__
- `ClearRemotelyBondedMid request v1-0-0`_ and `example`__
- `ClearRemotelyBondedMid response v1-0-0`_ and `example`__
- `EnableRemoteBonding request v1-0-0`_ and `example`__
- `EnableRemoteBonding response v1-0-0`_ and `example`__

.. _`AddrInfo request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_AddrInfo-request-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedCoordinator_AddrInfo-request-1-0-0-example.json
.. _`AddrInfo response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_AddrInfo-response-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedCoordinator_AddrInfo-response-1-0-0-example.json
.. _`DiscoveredDevices request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_DiscoveredDevices-request-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedCoordinator_DiscoveredDevices-request-1-0-0-example.json
.. _`DiscoveredDevices response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_DiscoveredDevices-response-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedCoordinator_DiscoveredDevices-response-1-0-0-example.json
.. _`BondedDevices request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_BondedDevices-request-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedCoordinator_BondedDevices-request-1-0-0-example.json
.. _`BondedDevices response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_BondedDevices-response-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedCoordinator_BondedDevices-response-1-0-0-example.json
.. _`ClearAllBonds request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_ClearAllBonds-request-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedCoordinator_ClearAllBonds-request-1-0-0-example.json
.. _`ClearAllBonds response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_ClearAllBonds-response-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedCoordinator_ClearAllBonds-response-1-0-0-example.json
.. _`BondNode request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_BondNode-request-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedCoordinator_BondNode-request-1-0-0-example.json
.. _`BondNode response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_BondNode-response-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedCoordinator_BondNode-response-1-0-0-example.json
.. _`RemoveBond request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_RemoveBond-request-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedCoordinator_RemoveBond-request-1-0-0-example.json
.. _`RemoveBond response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_RemoveBond-response-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedCoordinator_RemoveBond-response-1-0-0-example.json
.. _`Rebond request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_Rebond-request-1-0-0.json 
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedCoordinator_Rebond-request-1-0-0-example.json
.. _`Rebond response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_Rebond-response-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedCoordinator_Rebond-response-1-0-0-example.json
.. _`Discovery request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_Discovery-request-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedCoordinator_Discovery-request-1-0-0-example.json
.. _`Discovery response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_Discovery-response-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedCoordinator_Discovery-response-1-0-0-example.json
.. _`SetDpaParams request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_SetDpaParams-request-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedCoordinator_SetDpaParams-request-1-0-0-example.json
.. _`SetDpaParams response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_SetDpaParams-response-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedCoordinator_SetDpaParams-response-1-0-0-example.json
.. _`SetHops request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_SetHops-request-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedCoordinator_SetHops-request-1-0-0-example.json
.. _`SetHops response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_SetHops-response-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedCoordinator_SetHops-response-1-0-0-example.json
.. _`DiscoveryData request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_DiscoveryData-request-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedCoordinator_DiscoveryData-request-1-0-0-example.json
.. _`DiscoveryData response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_DiscoveryData-response-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedCoordinator_DiscoveryData-response-1-0-0-example.json
.. _`Backup request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_Backup-request-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedCoordinator_Backup-request-1-0-0-example.json
.. _`Backup response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_Backup-response-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedCoordinator_Backup-response-1-0-0-example.json
.. _`Restore request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_Restore-request-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedCoordinator_Restore-request-1-0-0-example.json
.. _`Restore response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_Restore-response-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedCoordinator_Restore-response-1-0-0-example.json
.. _`AuthorizeBond request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_AuthorizeBond-request-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedCoordinator_AuthorizeBond-request-1-0-0-example.json
.. _`AuthorizeBond response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_AuthorizeBond-response-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedCoordinator_AuthorizeBond-response-1-0-0-example.json
.. _`ReadRemotelyBondedMid request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_ReadRemotelyBondedMid-request-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedCoordinator_ReadRemotelyBondedMid-request-1-0-0-example.json
.. _`ReadRemotelyBondedMid response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_ReadRemotelyBondedMid-response-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedCoordinator_ReadRemotelyBondedMid-response-1-0-0-example.json
.. _`ClearRemotelyBondedMid request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_ClearRemotelyBondedMid-request-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedCoordinator_ClearRemotelyBondedMid-request-1-0-0-example.json
.. _`ClearRemotelyBondedMid response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_ClearRemotelyBondedMid-response-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedCoordinator_ClearRemotelyBondedMid-response-1-0-0-example.json
.. _`EnableRemoteBonding request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_EnableRemoteBonding-request-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedCoordinator_EnableRemoteBonding-request-1-0-0-example.json
.. _`EnableRemoteBonding response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedCoordinator_EnableRemoteBonding-response-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedCoordinator_EnableRemoteBonding-response-1-0-0-example.json

Embed Node
++++++++++

- `Read request v1-0-0`_ and `example`__
- `Read response v1-0-0`_ and `example`__
- `RemoveBond request v1-0-0`_ and `example`__
- `RemoveBond response v1-0-0`_ and `example`__
- `Backup request v1-0-0`_ and `example`__
- `Backup response v1-0-0`_ and `example`__
- `Restore request v1-0-0`_ and `example`__
- `Restore response v1-0-0`_ and `example`__
- `ReadRemotelyBondedMid request v1-0-0`_ and `example`__
- `ReadRemotelyBondedMid response v1-0-0`_ and `example`__
- `ClearRemotelyBondedMid request v1-0-0`_ and `example`__
- `ClearRemotelyBondedMid response v1-0-0`_ and `example`__
- `EnableRemoteBonding request v1-0-0`_ and `example`__
- `EnableRemoteBonding response v1-0-0`_ and `example`__

.. _`Read request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedNode_Read-request-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedNode_Read-request-1-0-0-example.json
.. _`Read response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedNode_Read-response-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedNode_Read-response-1-0-0-example.json
.. _`RemoveBond request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedNode_RemoveBond-request-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedNode_RemoveBond-request-1-0-0-example.json
.. _`RemoveBond response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedNode_RemoveBond-response-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedNode_RemoveBond-response-1-0-0-example.json
.. _`Backup request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedNode_Backup-request-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedNode_Backup-request-1-0-0-example.json
.. _`Backup response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedNode_Backup-response-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedNode_Backup-response-1-0-0-example.json
.. _`Restore request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedNode_Restore-request-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedNode_Restore-request-1-0-0-example.json
.. _`Restore response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedNode_Restore-response-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedNode_Restore-response-1-0-0-example.json
.. _`ReadRemotelyBondedMid request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedNode_ReadRemotelyBondedMid-request-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedNode_ReadRemotelyBondedMid-request-1-0-0-example.json
.. _`ReadRemotelyBondedMid response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedNode_ReadRemotelyBondedMid-response-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedNode_ReadRemotelyBondedMid-response-1-0-0-example.json
.. _`ClearRemotelyBondedMid request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedNode_ClearRemotelyBondedMid-request-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedNode_ClearRemotelyBondedMid-request-1-0-0-example.json
.. _`ClearRemotelyBondedMid response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedNode_ClearRemotelyBondedMid-response-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedNode_ClearRemotelyBondedMid-response-1-0-0-example.json
.. _`EnableRemoteBonding request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedNode_EnableRemoteBonding-request-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedNode_EnableRemoteBonding-request-1-0-0-example.json
.. _`EnableRemoteBonding response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedNode_EnableRemoteBonding-response-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedNode_EnableRemoteBonding-response-1-0-0-example.json

Embed OS
++++++++

- `Read request v1-0-0`_ and `example`__
- `Read response v1-0-0`_ and `example`__
- `Batch request v1-0-0`_ and `example`__
- `Batch response v1-0-0`_ and `example`__
- `SelectiveBatch request v1-0-0`_ and `example`__
- `SelectiveBatch response v1-0-0`_ and `example`__
- `Reset request v1-0-0`_ and `example`__
- `Reset response v1-0-0`_ and `example`__
- `Restart request v1-0-0`_ and `example`__
- `Restart response v1-0-0`_ and `example`__
- `RunRfpgm request v1-0-0`_ and `example`__
- `RunRfpgm response v1-0-0`_ and `example`__
- `SetSecurity request v1-0-0`_ and `example`__
- `SetSecurity response v1-0-0`_ and `example`__
- `Sleep request v1-0-0`_ and `example`__
- `Sleep response v1-0-0`_ and `example`__
- `LoadCode request v1-0-0`_ and `example`__
- `LoadCode response v1-0-0`_ and `example`__
- `ReadCfg request v1-0-0`_ and `example`__
- `ReadCfg response v1-0-0`_ and `example`__
- `WriteCfg request v1-0-0`_ and `example`__
- `WriteCfg response v1-0-0`_ and `example`__
- `WriteCfgByte request v1-0-0`_ and `example`__
- `WriteCfgByte response v1-0-0`_ and `example`__

.. _`Read request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedOs_Read-request-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedOs_Read-request-1-0-0-example.json
.. _`Read response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedOs_Read-response-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedOs_Read-response-1-0-0-example.json
.. _`Batch request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedOs_Batch-request-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedOs_Batch-request-1-0-0-example.json
.. _`Batch response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedOs_Batch-response-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedOs_Batch-response-1-0-0-example.json
.. _`SelectiveBatch request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedOs_SelectiveBatch-request-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedOs_SelectiveBatch-request-1-0-0-example.json
.. _`SelectiveBatch response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedOs_SelectiveBatch-response-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedOs_SelectiveBatch-response-1-0-0-example.json
.. _`Reset request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedOs_Reset-request-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedOs_Reset-request-1-0-0-example.json
.. _`Reset response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedOs_Reset-response-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedOs_Reset-response-1-0-0-example.json
.. _`Restart request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedOs_Restart-request-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedOs_Restart-request-1-0-0-example.json
.. _`Restart response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedOs_Restart-response-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedOs_Restart-response-1-0-0-example.json
.. _`RunRfpgm request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedOs_RunRfpgm-request-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedOs_RunRfpgm-request-1-0-0-example.json
.. _`RunRfpgm response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedOs_RunRfpgm-response-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedOs_RunRfpgm-response-1-0-0-example.json
.. _`SetSecurity request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedOs_SetSecurity-request-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedOs_SetSecurity-request-1-0-0-example.json
.. _`SetSecurity response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedOs_SetSecurity-response-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedOs_SetSecurity-response-1-0-0-example.json
.. _`Sleep request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedOs_Sleep-request-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedOs_Sleep-request-1-0-0-example.json
.. _`Sleep response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedOs_Sleep-response-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedOs_Sleep-response-1-0-0-example.json
.. _`LoadCode request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedOs_LoadCode-request-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedOs_LoadCode-request-1-0-0-example.json
.. _`LoadCode response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedOs_LoadCode-response-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedOs_LoadCode-response-1-0-0-example.json
.. _`ReadCfg request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedOs_ReadCfg-request-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedOs_ReadCfg-request-1-0-0-example.json
.. _`ReadCfg response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedOs_ReadCfg-response-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedOs_ReadCfg-response-1-0-0-example.json
.. _`WriteCfg request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedOs_WriteCfg-request-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedOs_WriteCfg-request-1-0-0-example.json
.. _`WriteCfg response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedOs_WriteCfg-response-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedOs_WriteCfg-response-1-0-0-example.json
.. _`WriteCfgByte request v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedOs_WriteCfgByte-request-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedOs_WriteCfgByte-request-1-0-0-example.json
.. _`WriteCfgByte response v1-0-0`: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/#iqrf/iqrfEmbedOs_WriteCfgByte-response-1-0-0.json
.. __: https://apidocs.iqrfsdk.org/iqrf-gateway-daemon/json/iqrf/examples/iqrfEmbedOs_WriteCfgByte-response-1-0-0-example.json

IQMESH Network
--------------



Daemon Scheduler
----------------



Daemon Management
-----------------



Daemon Configuration
--------------------
