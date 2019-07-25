#!/bin/bash
# CI Helper

DIST=$1
VER=$2

if [ -z $1 ];
then
    exit 1
fi

if [ -z $2 ];
then
    exit 1
fi

if [ ${DIST} == "buster" ];
then
    if [ ${VER} == "release"];
    then
        fpm \
        -t deb \
        -s dir \
        -C iqrf-daemon-deploy \
        --name "iqrf-gateway-daemon-dbg" \
        --version "${DAEMON_VERSION:1}~debian10" \
        --license "Apache License, Version 2.0" \
        --vendor "IQRF Tech s.r.o." \
        --category "stable" \
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
        --depends libpaho-mqtt1.3 \
        --depends libcurl4 \
        --depends libboost-filesystem1.67.0 \
        --depends libboost-system1.67.0 \
        --depends zlib1g \
        --depends libsqlite3-0 \
        --verbose \
        .
    else
        fpm \
        -t deb \
        -s dir \
        -C iqrf-daemon-deploy \
        --name "iqrf-gateway-daemon-dbg" \
        --version "${DAEMON_VERSION:1}+debian10" \
        --license "Apache License, Version 2.0" \
        --vendor "IQRF Tech s.r.o." \
        --category "testing" \
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
        --depends libpaho-mqtt1.3 \
        --depends libcurl4 \
        --depends libboost-filesystem1.67.0 \
        --depends libboost-system1.67.0 \
        --depends zlib1g \
        --depends libsqlite3-0 \
        --verbose \
        .
    fi
elif [ ${DIST} == "stretch" ];
then
    if [ ${VER} == "release"];
    then
        fpm \
        -t deb \
        -s dir \
        -C iqrf-daemon-deploy \
        --name "iqrf-gateway-daemon-dbg" \
        --version "${DAEMON_VERSION:1}~debian9" \
        --license "Apache License, Version 2.0" \
        --vendor "IQRF Tech s.r.o." \
        --category "stable" \
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
        --depends libpaho-mqtt1.3 \
        --depends libcurl3 \
        --depends libboost-filesystem1.62.0 \
        --depends libboost-system1.62.0 \
        --depends zlib1g \
        --depends libsqlite3-0 \
        --verbose \
        .
    else
        fpm \
        -t deb \
        -s dir \
        -C iqrf-daemon-deploy \
        --name "iqrf-gateway-daemon-dbg" \
        --version "${DAEMON_VERSION:1}+debian9" \
        --license "Apache License, Version 2.0" \
        --vendor "IQRF Tech s.r.o." \
        --category "testing" \
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
        --depends libpaho-mqtt1.3 \
        --depends libcurl3 \
        --depends libboost-filesystem1.62.0 \
        --depends libboost-system1.62.0 \
        --depends zlib1g \
        --depends libsqlite3-0 \
        --verbose \
        .
    fi
elif [ ${DIST} == "xenial" ];
then
    if [ ${VER} == "release"];
    then
        fpm \
        -t deb \
        -s dir \
        -C iqrf-daemon-deploy \
        --name "iqrf-gateway-daemon-dbg" \
        --version "${DAEMON_VERSION:1}~ubuntu16.04" \
        --license "Apache License, Version 2.0" \
        --vendor "IQRF Tech s.r.o." \
        --category "stable" \
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
        --depends libpaho-mqtt1.3 \
        --depends libcurl3 \
        --depends libboost-filesystem1.58.0 \
        --depends libboost-system1.58.0 \
        --depends zlib1g \
        --depends libsqlite3-0 \
        --verbose \
        .
    else
        fpm \
        -t deb \
        -s dir \
        -C iqrf-daemon-deploy \
        --name "iqrf-gateway-daemon-dbg" \
        --version "${DAEMON_VERSION:1}+ubuntu16.04" \
        --license "Apache License, Version 2.0" \
        --vendor "IQRF Tech s.r.o." \
        --category "testing" \
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
        --depends libpaho-mqtt1.3 \
        --depends libcurl3 \
        --depends libboost-filesystem1.58.0 \
        --depends libboost-system1.58.0 \
        --depends zlib1g \
        --depends libsqlite3-0 \
        --verbose \
        .
    fi    
elif [ ${DIST} == "bionic" ];
then
    if [ ${VER} == "release"];
    then
        fpm \
        -t deb \
        -s dir \
        -C iqrf-daemon-deploy \
        --name "iqrf-gateway-daemon-dbg" \
        --version "${DAEMON_VERSION:1}~ubuntu18.04" \
        --license "Apache License, Version 2.0" \
        --vendor "IQRF Tech s.r.o." \
        --category "stable" \
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
        --depends libpaho-mqtt1.3 \
        --depends libcurl4 \
        --depends libboost-filesystem1.65.0 \
        --depends libboost-system1.65.0 \
        --depends libsqlite3-0 \
        --verbose \
        .
    else
        fpm \
        -t deb \
        -s dir \
        -C iqrf-daemon-deploy \
        --name "iqrf-gateway-daemon-dbg" \
        --version "${DAEMON_VERSION:1}+ubuntu18.04" \
        --license "Apache License, Version 2.0" \
        --vendor "IQRF Tech s.r.o." \
        --category "testing" \
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
        --depends libpaho-mqtt1.3 \
        --depends libcurl4 \
        --depends libboost-filesystem1.65.0 \
        --depends libboost-system1.65.0 \
        --depends libsqlite3-0 \
        --verbose \
        .
    fi
fi
