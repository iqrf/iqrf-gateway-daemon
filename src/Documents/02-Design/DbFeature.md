# DB feature
*This is description of the DB feature. The feature is under construction. This document may change to reflect ongoing work. It is meant for developement purposes only to share information within team. It can be removed later, transformed to set of developement issues or evolve to official documentation later.*

Motivation of this feature is to hold and collect complete information about IQRF network and correct selection of JS drivers versions from IQRF Repo.

This feature is time consuming if fully applied at start-up of newly established IQRF network but it is essential for IQRF network utilization in the most cases. The applications would get it in other means - static description of the network (not always possible or desirable) or dynamically via API calls. The dynamic way is exactly the ambition of this feature and implemented on behalf of user applications instead or repeated re-invention by users. 

## Terms
### IIqrfInfo
It is the interface to access DB information. It is done via methods returning appropriate objects. It doesn't expose direct interface (SQL) of DB 

### IqrfInfo
It is the component responsible for DB implementing **IIqrfInfo** interface

### JsonIqrfInfoApi
It is the component providing JSON API to get DB information
 
### Device
Is a type of HW resp. product in term of Iqrf Repo. It is identified by the fingerprint.

### Device fingerprint
It is identification of device. The certified product has associated package in Iqrf Repo. fingerprint consist from:
  - HWPID
  - HWPID version
  - OS build
  - DPA version

### Driver
It is the source code of JS driver identified by Standard ID an Version.

### Node
It is an instance of a device identified by MID. It may (must not) be bonded. Unbonded (orphaned) node just exists in DB and may be bonded later, This is intended for temporary unbonded devices not to miss associated matadata (not implemented yet).

### Sensor
Standard Sensor defines group of particular sensors types. The term "sensor" is meant as a sensor type associated with a device. One device then can have multiple sensors.

## DB structure
For details see initialization SQL script: https://gitlab.iqrf.org/open-source/iqrf-gateway-daemon/blob/feature/DB/src/IqrfInfo/DB/init/IqrfInfo.db.sql

Tables:
- **Bonded** is Nadr, Mid, discovery relation
- **Device** holds fingerprint and package data if exists
- **Driver** is version and driver source code relation 
- **DeviceDriver** is junction table device - drivers
- **Node** is Mid - device relation, must not be bonded (orphaned mid)
- **Sensor** - enumerated sensors related to device
- **Binout** - enumerated binouts related to device
- **Light** - enumerated light standard related to device
- **Dali** - enumerated dali standard related to device

You can use some DB browse, e.g. https://sqlitebrowser.org/ tested on Win) or something else for Lin. Just browse to DB file:
- Win `...\shape\deploy\VS15_2017_x64\Debug\iqrf-gateway-daemon\runcfg\iqrfgd2-WinCdc\configuration\DB\IqrfInfo.db`
- Lin `/usr/share/iqrf-gateway-daemon/DB/IqrfInfo.db`

When DB file doesn't exist, it is created by init SQL script.

## Privisory load drivers
At the startup the IqrfInfo gets the coordinator fingerprint and loads all drivers into single JS engine with the highest version matching coordinator's os, dpa. This is the way used in previous iqrfgd2 releases. It provides correct functionality but fails in case of usage of products with support of lower versions of drivers. This "blind" load means download of all JS standard drivers and all custom drivers running in separate JS engines. This is not an ideal situation (amount of downloaded data, memory consumption, longer start-up) as the probability of real network with all existing products is very low.

## Enumeration phases
Enumeration phases are done:
- at start up if configured
- periodically if configured
- when detected a response to DPA request:
  - CMD_COORDINATOR_CLEAR_ALL_BONDS
  - CMD_COORDINATOR_BOND_NODE
  - CMD_COORDINATOR_REMOVE_BOND
  - CMD_COORDINATOR_DISCOVERY
  - CMD_COORDINATOR_RESTORE
  - CMD_COORDINATOR_SMART_CONNECT
  - CMD_COORDINATOR_SET_MID

All enumeration phases are done in a standalone thread at the background.

#### Check enum
The phase collects bonded MIDs from [C] EEEPROM and compares with stored DB MIDs. If there are differences or some nodes were not enumerated yet the next phases continue.

#### Full enum
The phase collects all necessary information about nodes:
- Hwpid, HwpidVer
- DpaVer
- OsBuild

This is done by:
- 4B FRC Memory read for DPA >= 4.00
  - Read Hwpid, HwpidVer
  - Read DpaVer
  - Read OsBuild
- or by polling DPA requests *iqrf.embed.os.Read* and *iqrf.embed.exprore.Enumerate* for DPA < 4.00
- if the only one node shall be enumerated the DPA requests are used as well rather then FRC
- if a bonded node doesn't response it is flagged as non enumerated (may be enumerated in next run)
- JS driver identification is done by an existing package from IqrfRepo
- if the device is not registered in IqrfRepo the drivers are evaluated according DPA requests *iqrf.embed.exprore.PeripheralInformation* and *iqrf.embed.exprore.MorePeripheralInformationn* 

#### Load drivers
This loads JS drivers from IqrfRepo to standalone JS engine per device.

#### Std enum
This phase take care only of Iqrf Standards enumeration with usage of matching JS drivers versions loaded in previous versions
- Sensor - used sensor types
- Binaryouput - number of outputs
- Light 
- Dali

## Configuration
The component **IqrfInfo** has optional configuration parameters:
```json
"enumAtStartUp": {
            "type": "boolean",
            "description": "Flag to initiate network full enumaration just after startup",
            "default": "false"
        }
```
If set to false, the application **iqrfgd2** doesn't start enumeration at all. It can be started explicitly by API command later. The only provisory load drivers takes care. If set to true, the enumeration is done at least once at start-up.

```json
        "enumPeriod": {
            "type": "integer",
            "description": "Period in minutes of regular check enumeration.",
            "default": "false"
        }
```
If set to value > 0 the enumeration is done periodically in the background.

```json
        "enumUniformDpaVer": {
            "type": "boolean",
            "description": "If set just Hwpid and HwpidVer is obtained from nodes. DpaVer and osBuild is set according coordinator",
            "default": "false"
        }
```  
If set getting of DpaVer and OsBuild is omitted and set according [C]. The process of enumeration is much faster then. The user shall assure that all nodes comply else there may be loaded incompatible JS driver (similar to provisory drivers)

## Info API
Component **JsonIqrfInfoApi** provides JSON API. See schemes at https://gitlab.iqrf.org/open-source/iqrf-gateway-daemon/tree/master/api

- infoDaemon_StartEnumeration - starts enumeration thread (as it would be started at stratup)
- infoDaemon_GetNodes - returns enumerated nodes
- infoDaemon_GetBinaryOutputs - returns nodes implementing Binaryoutput standard
- infoDaemon_GetSensors - returns nodes implementing Sensor standard (for breakdown use iqrfSensor_Enumerate - it is not supported by DB) 
- infoDaemon_GetDalis - returns nodes implementing Dali standard
- infoDaemon_GetLights - returns nodes implementing Light standard

TODO (not implemented yet)
- infoDaemon_StopEnumeration - stops enumeration thread (complement to StartEnumeration)
- infoDaemon_MakeEnumeration - make enumeration just now
- infoDaemon_RefreshEnumerate - delete DB and crate it from scratch
- infoDaemon_GetNodesNonEnum - returns bonded but non enumerated nodes (off, sleeping, out of range)
- infoDaemon_GetNodesNonBond - returns non bonded nodes (used to be bonded - DB keeps them as it may have associated metadata)
- infoDaemon_RemoveNonBond - remove non bonded nodes


## Next possible development steps

- download only detected drivers from Iqrf Repo (makes sense only if provisory load skipped)
- provide event system of finished enum phases and correct drivers load
- provide user an info what's going on - e.g. percentage
 