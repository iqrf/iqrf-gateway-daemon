#!/bin/bash
# CI Helper

if [ -z $1 ];
then
    exit 1
fi

if [ -z $2 ];
then
    exit 1
fi

#stretch,xenial,bionic
DIST=$1

#amd64, armhf, arm64
ARCH=$2

#docker login
docker build -f dockerfile.${DIST}.${ARCH} -t iqrfsdk/iqrf-gateway-daemon-build:${DIST}-${ARCH} .
#docker push iqrfsdk/iqrf-gateway-daemon-build:${DIST}-${ARCH}
