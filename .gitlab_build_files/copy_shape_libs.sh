#!/bin/bash
# CI Helper

set -e

LIB=$1
IQRFGD2=/usr/lib/iqrf-gateway-daemon

if [ -z $1 ];
then
    exit 1
fi

mkdir -p ${LIB}${IQRFGD2}
echo "Shape folder ${LIB}${IQRFGD2} created."

cp shape-build/bin/libTraceFormatService.so ${LIB}${IQRFGD2}
cp shape-build/bin/libTraceFileService.so ${LIB}${IQRFGD2}
cp shape-build/lib/liblauncher.a ${LIB}${IQRFGD2}

echo "Shape libs copied."
