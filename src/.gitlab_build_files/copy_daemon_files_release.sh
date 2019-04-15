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
cp iqrf-daemon-source/start-IqrfDaemon/configuration/*.json ${DEPLOY}${IQRFGD2_CFG}
cp iqrf-daemon-source/start-IqrfDaemon/configuration-LinDeployRelease/*.json ${DEPLOY}${IQRFGD2_CFG}
rm ${DEPLOY}${IQRFGD2_CFG}/iqrf__OtaUploadService.json

cp -r iqrf-daemon-source/start-IqrfDaemon/cfgSchemas ${DEPLOY}${IQRFGD2_CFG}
rm ${DEPLOY}${IQRFGD2_CFG}/cfgSchemas/schema__iqrf__OtaUploadService.json

# BIN
cp iqrf-daemon-build/bin/iqrfgd2 ${DEPLOY}${IQRFGD2_BIN}/iqrfgd2
cp iqrf-daemon-build/bin/*.so ${DEPLOY}${IQRFGD2_LIBS}
rm ${DEPLOY}${IQRFGD2_LIBS}/libOtaUploadService.so

# SHARE
cp -r iqrf-daemon-source/libraries/iqrf-daemon-api/JsonSchemas ${DEPLOY}${IQRFGD2_SHARE}/apiSchemas
# NOT YET UPLOAD
rm ${DEPLOY}${IQRFGD2_SHARE}/apiSchemas/iqmeshNetwork_OtaUpload*.json

cp -r iqrf-daemon-source/start-IqrfDaemon/javaScript ${DEPLOY}${IQRFGD2_SHARE}

# CACHE
cp -r iqrf-daemon-source/start-IqrfDaemon/scheduler ${DEPLOY}${IQRFGD2_CACHE}
cp -r iqrf-daemon-source/start-IqrfDaemon/metaData ${DEPLOY}${IQRFGD2_CACHE}
cp -r iqrf-daemon-source/start-IqrfDaemon/iqrfRepoCache ${DEPLOY}${IQRFGD2_CACHE}

# SERVICE
cp iqrf-daemon-source/start-IqrfDaemon/systemd/*.service ${DEPLOY}${IQRFGD2_SERVICE}

# SHAPE
cp shape-libs${IQRFGD2_LIBS}/* ${DEPLOY}${IQRFGD2_LIBS}

# SHAPEWARE
cp shapeware-libs${IQRFGD2_LIBS}/* ${DEPLOY}${IQRFGD2_LIBS}

# NO EXE
chmod -x ${DEPLOY}${IQRFGD2_LIBS}/*

echo "Daemon files copied."