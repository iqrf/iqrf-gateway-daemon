DEBIAN STRETCH (UBUNTU XENIAL)
------------------------------

CMAKE & SSL & MQTT & CURL
=========================

sudo apt-get install dirmngr apt-transport-https \
&& sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 9C076FCC7AB8F2E43C2AB0E73241B9B7B4BD8F8E \
&& echo "deb https://repos.iqrf.org/debian stretch stable testing" | sudo tee -a /etc/apt/sources.list \
&& sudo apt-get update \
&& sudo apt-get install cmake libssl-dev zlib1g-dev libcurl4-openssl-dev libcpaho-mqtt-dev libboost-filesystem-dev libsqlite3-dev libzmqpp-dev

FOLDERS STRUCTURE
=================

iqrf-gw-daemon/shape
iqrf-gw-daemon/shapeware
iqrf-gw-daemon/iqrftech/iqrf-gateway-daemon

SHAPE loader
============

mkdir iqrf-gw-daemon \
&& cd iqrf-gw-daemon \
&& git clone https://github.com/logimic/shape.git \
&& cd shape \
&& git submodule init \
&& git submodule update \
&& ./buildMake.sh

SHAPEWARE modules
=================

cd .. \
&& git clone https://github.com/logimic/shapeware.git \
&& cd shapeware \
&& git submodule init \
&& git submodule update \
&& ./buildMake.sh

IQRF GW daemon
==============

cd .. \
&& mkdir iqrftech \
&& cd iqrftech \
&& git clone https://gitlab.iqrf.org/open-source/iqrf-gateway-daemon.git \
&& cd iqrf-gateway-daemon \
&& git submodule init \
&& git submodule update \
&& ./buildMake.sh

Stop the systemd service
========================

sudo systemctl stop iqrf-gateway-daemon

Configure the daemon
====================

cd shape/deploy/Unix_Makefiles/Debug/iqrf-gateway-daemon/runcfg/iqrfgd2-LinSpi/configuration
nano iqrf__IqrfSpi.json ... OPI /dev/spidev1.0 + mapovani pinů z clibspi pro OPI
                        ... UP /dev/spidev2.0 + mapovani pinu je default pro UP
...

Running the daemon
==================

cd shape/deploy/Unix_Makefiles/Debug/iqrf-gateway-daemon/runcfg/iqrfgd2-LinSpi
sudo ./StartUp.sh

Docker builder
==============

Build and run for Linux and USB IQRF CDC coordinator (e.g. server hw):

docker build -f build.dockerfile.stretch.amd64 -t iqrftech/iqrf-gateway-daemon:test-amd64 .

docker container run -d --name iqrf-gateway-daemon-tester -p 1338:1338 -p 55000:55000/udp -p 55300:55300/udp --device /dev/ttyACM0:/dev/ttyACM0 --privileged --restart=always iqrftech/iqrf-gateway-daemon:test-amd64
