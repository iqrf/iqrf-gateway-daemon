#!/bin/bash
# CI Helper

if [ -z $1 ];
then
    exit 1
fi

#amd64, armhf
ARCH=$1

#docker login
docker build -f Dockerfile.${ARCH} -t iqrfsdk/iqrf-gateway-daemon-build:latest-${ARCH} .
#docker push iqrfsdk/iqrf-gateway-daemon-build:latest-${ARCH}
