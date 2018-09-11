#!/bin/bash
# CI Helper

fpm \
-t deb \
-s dir \
-C iqrf-daemon-deploy \
--name "iqrf-gateway-daemon-dev" \
--version ${DAEMON_VERSION}${CI_PIPELINE_ID} \
--license "Apache License, Version 2.0" \
--vendor "IQRF Tech s.r.o." \
--category "devel" \
--architecture ${ARCH} \
--maintainer "Rostislav Spinar <rostislav.spinar@iqrf.com>" \
--url "https://github.com/iqrfsdk/iqrf-gateway-daemon" \
--description "IQRF Gateway Daemon with UDP/MQ/MQTT/WS communication channels." \
--deb-changelog debian/changelog \
--after-install debian/after-install.sh \
--after-remove debian/after-remove.sh \
--depends libc6 \
--depends libstdc++6 \
--depends openssl \
--depends libcpprest2.9 \
--depends libboost-filesystem1.62.0 \
--verbose \
.
