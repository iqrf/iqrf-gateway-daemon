# DB feature
This is description of the DB feature. The feature is under construction. This document may change to reflect ongoing work.
It is meant for developement purposes only to share information within team. It can be removed later, transformed to set of developement issues or evolve to official documentation later.

## Terms
### IqrfInfo
Is componet responsible for DB

### Device
Is type of HW resp. product in term of Iqrf Repo. It is identified by fingerprint.

### Device fingerprint
Is identification of device resp. product. The certified product has associated package in Iqrf Repo. fingerprint consist from:
  - HWPID
  - HWPID version
  - OS build
  - DPA version

### Driver
Is source code of JS driver identified by Standard ID an Version.

### Node
Is HW instance of Device identified by MID. It may but must not be bonded. Unbonded (orphaned) node just exists in DB and may be bonded later, This is intended for temporary unbonded devices not to miss matadata (not implemented yet).

### Sensor
Standard Sensor defines group of particular sensors types. The term "sensor" is meant as the type associated with a device. One device then can have multiple sensors.

## DB structure
For details see init SQL script: https://gitlab.iqrf.org/open-source/iqrf-gateway-daemon/blob/feature/DB/src/IqrfInfo/DB/init/IqrfInfo.db.sql

Tables:
- **Bonded** is Nadr, Mid, discovery relation
- **Device** holds fingerprint and package data if exists
- **Driver** is version and driver source code relation 
- **DeviceDriver** is junction table device - drivers
- **Node** is Mid - device relation, must not be bonded (orphaned mid)
- **Sensor** - enumerated sensors related to device
- **Binout** - enumerated binouts related to device

You can use some DB browse, e.g. https://sqlitebrowser.org/ tested on Win) or somethig else for Lin. Just browse to DB file:
- Win `...\shape\deploy\VS15_2017_x64\Debug\iqrf-gateway-daemon\runcfg\iqrfgd2-WinCdc\configuration\DB\IqrfInfo.db`
- Lin `/usr/share/lersengw/DataBase/IqrfInfo.db`

## Start Up phases
Next chapters described order steps of IqrfInfo start up

### Init DB
When DB file doesn't exist, it is created by init SQL script. Th DB is empty then.

### Privisory load drivers
Gets the coordinator fingerprint and loads all drivers into single JS engine with the highest version matching coordinator's os, dpa. This is the way used up to now in previous iqrfgd2 releases.
 
### Fast enum

For all discovered NADR:
- gets in the fastest way (FRC for dpa > 4.02 - not implemented yet) MID and device fingerprint.
- select from DB previously stored MID and fingerprint for bonded and discovered NADR
- compare actual and stored MID and fingerprint
- if comparison fails or NADR is not in DB then NADR is put to full enum list

### Full enum

For all NADR from full enum list:
- gets data via raw dpa *iqrf.embed.os.read*
- gets data via raw dpa *iqrf.embed.exprore.enumerate*
- tries to find Iqrf Repo package according fingerprint
- if package is not found (uncertified device) prepare virtual adhoc package 
  - gets Package 0 => plain DPA to get embed periferies drivers for the fingerprint 
  - gets via raw dpa *iqrf.embed.exprore.PeripheralInformation* for Standars Sensor, Binaryout (Light, Dali not implemented yet) to find standard versions from *Par1*
- select from DB device with fingerprint
- if device is not in DB then insert into Device
- insert into Driver if any of the drivers from package missing in DB
- MID insert into Node

### Load drivers

Now we have all data necessary for JS drivers to load:
- iterate trough all stored devices
- select from DB all device's drivers (already matching versions from prev. steps)
- load JS engine - there is one JS engine related to device
- associate NADRs handled by this engine. When request comes to NADR it is routed to correct JS engine.

### Deep enum

This is continuation of full enum, but after JS drivers load. Previous full enum was done only by raw DPA usage. Now the drivers are used. This phase take care only of Standards enumeration with usage of proper driver's versions
- Sensor
- Binaryouput
- Light (not implemented yet)
- Dali (not implemented yet)

During this phase:
- select devices without deep enum (DeepEnum flag in Device table)
- for all these devices:
  - select lowest NADR with discovered device instance
  - send enum  request (e.g. *iqrf.sensor.Enumerate*)
  - store result to DB

TODO:
- Is the principle correct - all devices has the same sensors?
- It wouldn't work if particular sensor of a device can be set in SW or HW (e.g. jumpers) way. In this case we need to deep eval not per device, but per node (all NADRs).
 
## Configuration
DB feature is on `feature/DB` branch. The component **IqrfInfo** has simple optional configuration parameter:
```json
"enumAtStartUp": {
            "type": "boolean",
            "description": "Flag to initiate network full enumaration just after startup",
            "default": "false"
        }
```
If set to false, the application **iqrfgd2** behaves as previous versions with respect to drivers load (provisory load drivers). If set to true, all phases of feature are processed.  

## Next possible development steps

- download only detected drivers from Iqrf Repo
- provide event system of finished enum phases
- move enum phases to standalone thread to do it asynchronously, not to block iqrfgd2 start-up 
- provide API to invoke enum phases explicitly
- integrate enum phases and DB info with Autonetwork, Discovery, Bond/Unbond, SmartConnect, OTA, etc ...
- implement appropriate API to get info from DB