# Win developement instalation

Install:
- install [vcpkg](https://github.com/Microsoft/vcpkg) to `c:\devel\vcpkg\`
- `vcpkg install paho-mqtt:x64-windows`
- `vcpkg install boost-filesystem:x64-windows`
- `vcpkg install boost-program-options:x64-windows`
- `vcpkg install curl:x64-windows`
- `vcpkg install sqlite3:x64-windows`
- [shape](https://github.com/logimic/shape) (basic components launcher, logging, ...) start `build64_2017.bat` (change MSVC2017 version to your MSVC if necessary)
- [shapeware](https://github.com/logimic/shapeware) (additional components websockets, ...) start `build64_2017.bat` (change MSVC2017 version to your MSVC if necessary)
- from this directory  start one of `build64_2017.bat` (change MSVC2017 version to your MSVC if necessary)

# Lin developement instalation

Install:
- install necessary packages TODO describe how
  - boost-filesystem
  - curl
  - paho
  - sqlite3
  - libsqlite3-dev
- [shape](https://github.com/logimic/shape) (basic components launcher, logging, ...) start `buildMake.sh`
- [shapeware](https://github.com/logimic/shapeware) (additional components websockets, ...) start `buildMake.sh`
- from this director  start one of `buildMake.sh` or `buildEclipse.sh`

# Deploy structure
Deploy directory can be specified in Shape build script `build*.bat` resp. `buildMake.sh` or is defaulted to `.../shape/deploy`. The directory is passed to dependent projects via cmake variables during configuration/generation phase.

The deploy directory is created by cmake install by build script and it can be re-run by INSTALL target in MSVC, resp. `make install`

The deploy directory contains ready to run installation and tests. It can be used by a packaging script. Note we intend to evaluate cmake packaging feature even for packaging if applicable. 

See more at https://github.com/logimic/shape/blob/master/Deployment.md

  
