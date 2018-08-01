#!/bin/bash
# CI Helper

fpm \
-t deb \
-s dir \
-C iqrf-daemon-deploy \
--name "iqrf-gateway-daemon" \
--version ${DAEMON_VERSION}${CI_PIPELINE_ID} \
--license "Apache License, Version 2.0" \
--vendor "IQRF Tech s.r.o." \
--category "testing" \
--architecture ${ARCH} \
--maintainer "Rostislav Spinar <rostislav.spinar@iqrf.com>" \
--url "https://github.com/iqrfsdk/iqrf-gateway-daemon" \
--description "IQRF Gateway Daemon v2 with the multiple communication channels - UDP/MQ/MQTT/WS." \
--depends openssl \
--depends zlib1g \
--depends libev4 \
--depends libuv1 \
--depends libcpprest2.9 \
--verbose \
.
