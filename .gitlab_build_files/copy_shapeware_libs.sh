#!/bin/bash
# CI Helper

set -e

LIB=$1
IQRFGD2=/usr/lib/iqrfgd2

if [ -z $1 ];
then
    exit 1
fi

echo "Shapeware folder ${LIB}${IQRFGD2} created."
mkdir -p ${LIB}${IQRFGD2}

cp shapeware-build/bin/libCppRestService.so ${LIB}${IQRFGD2}
cp shapeware-build/bin/libWebsocketService.so ${LIB}${IQRFGD2}

echo "Shapeware libs copied."
