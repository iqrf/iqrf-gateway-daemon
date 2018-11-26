#!/bin/bash
# CI Helper

set -e

DEPLOY=$1

# configuration
IQRFGD2_CFG=/etc/iqrf-gateway-daemon

# binary files
IQRFGD2_BIN=/usr/bin

# dynamic libraries
IQRFGD2_LIBS=/usr/lib/iqrf-gateway-daemon

# json api schemas, javascript wrapper
IQRFGD2_SHARE=/usr/share/iqrf-gateway-daemon

# iqrf repository, scheduler
IQRFGD2_CACHE=/var/cache/iqrf-gateway-daemon

# systemd service
IQRFGD2_SERVICE=/lib/systemd/system

if [ -z $1 ];
then
    exit 1
fi

mkdir -p ${DEPLOY}${IQRFGD2_CFG}
mkdir -p ${DEPLOY}${IQRFGD2_BIN}
mkdir -p ${DEPLOY}${IQRFGD2_LIBS}
mkdir -p ${DEPLOY}${IQRFGD2_SHARE}
mkdir -p ${DEPLOY}${IQRFGD2_CACHE}
mkdir -p ${DEPLOY}${IQRFGD2_SERVICE}
echo "Daemon folders created."

# CFG
cp iqrf-daemon-build/bin/configuration/*.json ${DEPLOY}${IQRFGD2_CFG}
cp iqrf-daemon-build/bin/configuration/deploy/release/*.json ${DEPLOY}${IQRFGD2_CFG}
rm -f ${DEPLOY}${IQRFGD2_CFG}/iqrf__NativeUploadService.json ${DEPLOY}${IQRFGD2_CFG}/iqrf__OtaUploadService.json

cp -r iqrf-daemon-build/bin/configuration/cfgSchemas ${DEPLOY}${IQRFGD2_CFG}
rm -f ${DEPLOY}${IQRFGD2_CFG}/cfgSchemas/schema__iqrf__NativeUploadService.json ${DEPLOY}${IQRFGD2_CFG}/cfgSchemas/schema__iqrf__OtaUploadService.json

# BIN
cp iqrf-daemon-build/bin/iqrfgd2 ${DEPLOY}${IQRFGD2_BIN}/iqrfgd2
cp iqrf-daemon-build/bin/*.so ${DEPLOY}${IQRFGD2_LIBS}
rm -f ${DEPLOY}${IQRFGD2_LIBS}/libNativeUploadService.so ${DEPLOY}${IQRFGD2_LIBS}/libOtaUploadService.so

# SHARE
cp -r iqrf-daemon-build/bin/configuration/apiSchemas ${DEPLOY}${IQRFGD2_SHARE}
# NOT YET UPLOAD
rm -f ${DEPLOY}${IQRFGD2_SHARE}/apiSchemas/iqmeshNetwork_OtaUpload*.json
rm -f ${DEPLOY}${IQRFGD2_SHARE}/apiSchemas/mngDaemon_Upload*.json

cp -r iqrf-daemon-build/bin/configuration/javaScript ${DEPLOY}${IQRFGD2_SHARE}

# CACHE
cp -r iqrf-daemon-build/bin/configuration/scheduler ${DEPLOY}${IQRFGD2_CACHE}
cp -r iqrf-daemon-build/bin/configuration/iqrfRepoCache ${DEPLOY}${IQRFGD2_CACHE}

# SERVICE
cp iqrf-daemon-build/bin/configuration/systemd/*.service ${DEPLOY}${IQRFGD2_SERVICE}

# SHAPE
cp shape-libs${IQRFGD2_LIBS}/* ${DEPLOY}${IQRFGD2_LIBS}

# SHAPEWARE
cp shapeware-libs${IQRFGD2_LIBS}/* ${DEPLOY}${IQRFGD2_LIBS}

# NO EXE
chmod -x ${DEPLOY}${IQRFGD2_LIBS}/*

echo "Daemon files copied."
