#!/bin/bash
# CI Helper

set -e

LIB=$1

IQRFGD2=/usr/lib/iqrfgd2
LOCAL=/usr/local/lib

if [ -z $1 ];
then
    exit 1
fi

echo "Shapeware folder ${LIB}${IQRFGD2} created."

mkdir -p ${LIB}${IQRFGD2}
mkdir -p ${LIB}${LOCAL}

cp shapeware-build/bin/libCppRestService.so ${LIB}${IQRFGD2}
cp shapeware-build/bin/libWebsocketService.so ${LIB}${IQRFGD2}

cp -av shapeware-build/external/libwebsockets/lib/libwebsockets.so.12 ${LIB}${LOCAL}

echo "Shapeware libs copied."
