#!/bin/bash
# CI Helper

set -e

DEPLOY=$1

# configuration
IQRFGD2_CFG=/etc/iqrfgd2

# binary files
IQRFGD2_BIN=/usr/bin

# dynamic libraries
IQRFGD2_LIBS=/usr/lib/iqrfgd2
LOCAL_LIBS=/usr/local/lib

# json api schemas, javascript wrapper
IQRFGD2_SHARE=/usr/share/iqrfgd2

# iqrf repository, scheduler
IQRFGD2_CACHE=/var/cache/iqrfgd2

# systemd service
IQRFGD2_SERVICE=/lib/systemd/system

if [ -z $1 ];
then
    exit 1
fi

mkdir -p ${DEPLOY}${IQRFGD2_CFG}
mkdir -p ${DEPLOY}${IQRFGD2_BIN}
mkdir -p ${DEPLOY}${IQRFGD2_LIBS}
mkdir -p ${DEPLOY}${LOCAL_LIBS}
mkdir -p ${DEPLOY}${IQRFGD2_SHARE}
mkdir -p ${DEPLOY}${IQRFGD2_CACHE}
mkdir -p ${DEPLOY}${IQRFGD2_SERVICE}
echo "Daemon folders created."

# CFG
cp iqrf-daemon-build/bin/configuration/*.json ${DEPLOY}${IQRFGD2_CFG}
cp iqrf-daemon-build/bin/configuration/deploy/*.json ${DEPLOY}${IQRFGD2_CFG}
cp -r iqrf-daemon-build/bin/configuration/cfgSchemas ${DEPLOY}${IQRFGD2_CFG}

# BIN
cp iqrf-daemon-build/bin/iqrfgd2 ${DEPLOY}${IQRFGD2_BIN}/iqrfgd2
cp iqrf-daemon-build/bin/*.so ${DEPLOY}${IQRFGD2_LIBS}

# SHARE
cp -r iqrf-daemon-build/bin/configuration/apiSchemas ${DEPLOY}${IQRFGD2_SHARE}
cp -r iqrf-daemon-build/bin/configuration/javaScript ${DEPLOY}${IQRFGD2_SHARE}

# CACHE
cp -r iqrf-daemon-build/bin/configuration/scheduler ${DEPLOY}${IQRFGD2_CACHE}
cp -r iqrf-daemon-build/bin/configuration/iqrfRepoCache ${DEPLOY}${IQRFGD2_CACHE}

# SERVICE
cp iqrf-daemon-build/bin/configuration/systemd/*.service ${DEPLOY}${IQRFGD2_SERVICE}

# SHAPE 
cp shape-libs/usr/lib/iqrfgd2/* ${DEPLOY}${IQRFGD2_LIBS}

# SHAPEWARE
cp shapeware-libs/usr/lib/iqrfgd2/* ${DEPLOY}${IQRFGD2_LIBS}
cp shapeware-libs/usr/local/lib/* ${DEPLOY}${LOCAL_LIBS}

# PAHO
cp paho-build/src/*.so.1.2.0 ${DEPLOY}${LOCAL_LIBS}

echo "Daemon files copied."
