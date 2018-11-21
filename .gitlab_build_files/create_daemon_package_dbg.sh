#!/bin/bash
# CI Helper

fpm \
-t deb \
-s dir \
-C iqrf-daemon-deploy \
--name "iqrf-gateway-daemon-dbg" \
--version ${DAEMON_VERSION_NUM}"~"${CI_PIPELINE_ID} \
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
--deb-build-depends debhelper \
--deb-build-depends cmake \
--deb-build-depends libc6 \
--deb-build-depends libstdc++6 \
--deb-build-depends linux-libc-dev \
--deb-build-depends libcpaho-mqtt-dev \
--deb-build-depends libcurl4-openssl-dev \
--deb-build-depends libboost-filesystem-dev \
--deb-build-depends libssl-dev \
--deb-build-depends zlib1g-dev \
--verbose \
.
