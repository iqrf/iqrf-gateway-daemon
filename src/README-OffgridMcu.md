# OffGidMcu deploy spec

### Added api schemes:
../api/iqrfGwMcu_...

### Added Components:
./JsonOffGridCoreMcuApi
./OffGridCoreMcu

### Configuration:
Just copied from ./start-IqrfDaemon/configuration-LinDeploy to ./start-IqrfDaemon/configuration-LinDeploy-Offgrid
Important adapted files:
- `iqrf__IqrfDpa.json` explicitly refers /dev/ttyS0 to Iqrf [C] (must replace orig. json)
- `iqrf__JsonOffGridCoreMcuApi.json` configs added component 
- `iqrf__OffGridCoreMcu.json`  configs added component, explicitly refers /dev/ttyS1 to Mcu
- `iqrf__IqrfUart.json` /dev/ttyS0 
- `iqrf__IqrfUart1.json` /dev/ttyS1

