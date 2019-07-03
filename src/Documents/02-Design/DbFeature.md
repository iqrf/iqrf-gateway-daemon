# DB feature
*This is description of the DB feature. The feature is under construction. This document may change to reflect ongoing work. It is meant for developement purposes only to share information within team. It can be removed later, transformed to set of developement issues or evolve to official documentation later.*

Motivation of this feature is to hold and collect complete information about IQRF network and correct selection of JS drivers versions from IQRF Repo.

This feature is time consuming if fully applied at start-up of newly established IQRF network but it is essential for IQRF network utilization in the most cases. The applications would get it in other means - static description of the network (not always possible or desirable) or dynamically via API calls. The dynamic way is exactly the ambition of this feature and implemented on behalf of user applications instead or repeated re-invention by users. 

## Terms
### IqrfInfo
It is the component responsible for DB

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

You can use some DB browse, e.g. https://sqlitebrowser.org/ tested on Win) or something else for Lin. Just browse to DB file:
- Win `...\shape\deploy\VS15_2017_x64\Debug\iqrf-gateway-daemon\runcfg\iqrfgd2-WinCdc\configuration\DB\IqrfInfo.db`
- Lin `/usr/share/iqrf-gateway-daemon/DB/IqrfInfo.db`

## Start Up phases
Next chapters described order steps of IqrfInfo start up. Note the phases are not strictly tied with start-up itself and may be launched/repeated anytime

### Init DB
When DB file doesn't exist, it is created by init SQL script.

### Privisory load drivers
Gets the coordinator fingerprint and loads all drivers into single JS engine with the highest version matching coordinator's os, dpa. This is the way used in previous iqrfgd2 releases. It provides correct functionality but fails in case of usage of products with support of lower versions of drivers. This "blind" load means download of all JS standard drivers and all custom drivers running in separate JS engines. This is not an ideal situation (amount of downloaded data, memory consumption, longer start-up) as the probability of real network with all existing products is very low.
 
### Fast enum
Fast enum collects MID and device fingerprint of all discovered nodes. It is key information to detect correct packages and thus correct driver versions. It shall be implemented in the fastest way. It sahll be possible to use FRC for DPA > 4.02. It verify DB information consistency (something may change outside iqrfgd2 - e.g. via IDE).

It performs for all discovered NADR:
- gets MID and device fingerprint.
- select from DB previously stored MID and fingerprint for bonded and discovered NADR
- compare actual and stored MID and fingerprint
- if comparison fails or NADR is not in DB then NADR is put to full enum list

### Full enum
It collects additional information about enumerated NADR.

For all NADR from full enum list:
- gets data via raw dpa *iqrf.embed.os.read*
- gets data via raw dpa *iqrf.embed.exprore.enumerate*
- tries to find Iqrf Repo package according fingerprint
- if a package is not found (not certified device) prepare virtual ad hoc package 
  - gets Package 0 => plain DPA to get embed periferies drivers for the fingerprint 
  - gets via raw dpa *iqrf.embed.exprore.PeripheralInformation* for Standars Sensor, Binaryout (Light, Dali not implemented yet) to find standard versions from *Par1*
- select device with fingerprint from DB
- if device is not in DB then insert into Device
- insert into Driver if any of the drivers from package are missing in DB
- MID insert into Node

### Load drivers

Now we have all data necessary for JS drivers to load:
- iterate trough all stored devices
- select from DB all device's drivers (already matching versions from prev. steps)
- load JS engine - there is one JS engine related to device
- associate NADRs handled by this engine. When request comes to NADR it is routed to correct JS engine.

### Std enum
This phase take care only of Standards enumeration with usage of proper JS drivers versions
- Sensor
- Binaryouput
- Light (not implemented yet)
- Dali (not implemented yet)

During this phase:
- select devices without std enum (StdEnum flag in Device table)
- for all these devices:
  - select lowest NADR with discovered device instance
  - send enum  request (e.g. *iqrf.sensor.Enumerate*)
  - store result to DB

TODO:
- Is the principle correct - all devices has the same sensors?
- It wouldn't work if a particular sensor of a device can be set in SW or HW (e.g. jumpers) way. In this case we need to std enum not per device, but per node.
 
## Configuration
DB feature is on `feature/DB` branch. The component **IqrfInfo** has simple optional configuration parameter:
```json
"enumAtStartUp": {
            "type": "boolean",
            "description": "Flag to initiate network full enumaration just after startup",
            "default": "false"
        }
```
If set to false, the application **iqrfgd2** behaves as previous versions with respect to drivers load (provisory load drivers). If set to true, all phases of feature are processed at start-up.  

## Next possible development steps

- download only detected drivers from Iqrf Repo
- provide event system of finished enum phases
- move enum phases to standalone thread to do it asynchronously, not to block iqrfgd2 start-up 
- provide API to invoke enum phases explicitly
- integrate enum phases and DB info with Autonetwork, Discovery, Bond/Unbond, SmartConnect, OTA, etc ...
- implement appropriate API to get info from DB
- provide user an info what's going on - e.g. percentage