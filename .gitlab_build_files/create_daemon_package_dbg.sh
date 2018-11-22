#!/bin/bash
# CI Helper

DIST=$1

if [ -z $1 ];
then
    exit 1
fi

if [ ${DIST} == "stretch" ];
then
    fpm \
    -t deb \
    -s dir \
    -C iqrf-daemon-deploy \
    --name "iqrf-gateway-daemon-dbg" \
    --version ${DAEMON_VERSION:1} \
    --license "Apache License, Version 2.0" \
    --vendor "IQRF Tech s.r.o." \
    --category "debug" \
    --architecture ${ARCH} \
    --maintainer "Rostislav Spinar <rostislav.spinar@iqrf.com>" \
    --url "https://gitlab.iqrf.org/open-source/iqrf-gateway-daemon" \
    --description "IQRF Gateway Daemon with UDP/MQ/MQTT/WS communication channels." \
    --deb-changelog debian/changelog \
    --after-install debian/postinst \
    --after-remove debian/postrm \
    --depends init-system-helpers \
    --depends libc6 \
    --depends libstdc++6 \
    --depends libgcc1 \
    --depends libcpaho-mqtt1.3 \
    --depends libcurl3 \
    --depends libboost-filesystem1.62.0 \
    --depends libboost-system1.62.0 \
    --depends zlib1g \
    --verbose \
    .
elif [ ${DIST} == "xenial" ];
then
    fpm \
    -t deb \
    -s dir \
    -C iqrf-daemon-deploy \
    --name "iqrf-gateway-daemon-dbg" \
    --version ${DAEMON_VERSION:1} \
    --license "Apache License, Version 2.0" \
    --vendor "IQRF Tech s.r.o." \
    --category "debug" \
    --architecture ${ARCH} \
    --maintainer "Rostislav Spinar <rostislav.spinar@iqrf.com>" \
    --url "https://gitlab.iqrf.org/open-source/iqrf-gateway-daemon" \
    --description "IQRF Gateway Daemon with UDP/MQ/MQTT/WS communication channels." \
    --deb-changelog debian/changelog \
    --after-install debian/postinst \
    --after-remove debian/postrm \
    --depends init-system-helpers \
    --depends libc6 \
    --depends libstdc++6 \
    --depends libgcc1 \
    --depends libcpaho-mqtt1.3 \
    --depends libcurl3 \
    --depends libboost-filesystem1.58.0 \
    --depends libboost-system1.58.0 \
    --depends zlib1g \
    --verbose \
    .
elif [ ${DIST} == "bionic" ];
then
    fpm \
    -t deb \
    -s dir \
    -C iqrf-daemon-deploy \
    --name "iqrf-gateway-daemon-dbg" \
    --version ${DAEMON_VERSION:1} \
    --license "Apache License, Version 2.0" \
    --vendor "IQRF Tech s.r.o." \
    --category "debug" \
    --architecture ${ARCH} \
    --maintainer "Rostislav Spinar <rostislav.spinar@iqrf.com>" \
    --url "https://gitlab.iqrf.org/open-source/iqrf-gateway-daemon" \
    --description "IQRF Gateway Daemon with UDP/MQ/MQTT/WS communication channels." \
    --deb-changelog debian/changelog \
    --after-install debian/postinst \
    --after-remove debian/postrm \
    --depends libc6 \
    --depends libstdc++6 \
    --depends libgcc1 \
    --depends libcpaho-mqtt1.3 \
    --depends libcurl4 \
    --depends libboost-filesystem1.65.0 \
    --depends libboost-system1.65.0 \
    --verbose \
    .
fi
