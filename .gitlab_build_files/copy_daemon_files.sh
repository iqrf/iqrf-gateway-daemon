#!/bin/bash
# CI Helper

set -e

IQRFGD2_DEPLOY=$1

IQRFGD2_CFG=/etc/iqrfgd2
IQRFGD2_BIN=/usr/bin
IQRFGD2_LIBS=/usr/lib/iqrfgd2
IQRFGD2_SHARE=/usr/share/iqrfgd2
IQRFGD2_CACHE=/var/cache/iqrfgd2

if [ -z $1 ];
then
    exit 1
fi

mkdir -p ${LIB}${IQRFGD2_CFG}
mkdir -p ${LIB}${IQRFGD2_BIN}
mkdir -p ${LIB}${IQRFGD2_LIBS}
mkdir -p ${LIB}${IQRFGD2_SHARE}
mkdir -p ${LIB}${IQRFGD2_CACHE}
echo "Daemon folders created."

# CFG
cp iqrf-daemon-build/bin/configuration/*.json ${LIB}${IQRFGD2_CFG}
cp -r iqrf-daemon-build/bin/configuration/jsonschema ${LIB}${IQRFGD2_CFG}

# BIN
cp iqrf-daemon-build/bin/start-IqrfDaemon ${LIB}${IQRFGD2_BIN}/iqrfgd2
cp iqrf-daemon-build/bin/*.so ${LIB}${IQRFGD2_LIBS}

# SHARE
cp -r iqrf-daemon-build/bin/configuration/JsonSchemas ${LIB}${IQRFGD2_SHARE}
cp -r iqrf-daemon-build/bin/configuration/JavaScript ${LIB}${IQRFGD2_SHARE}

# CACHE
cp -r iqrf-daemon-build/bin/configuration/Scheduler ${LIB}${IQRFGD2_CACHE}

# SHAPE
cp shape-libs/usr/lib/iqrfgd2/* ${LIB}${IQRFGD2_LIBS}

# SHAPEWARE
cp shapeware-libs/usr/lib/iqrfgd2/* ${LIB}${IQRFGD2_LIBS}

echo "Daemon files copied."
