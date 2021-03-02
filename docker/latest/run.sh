#!/bin/bash

CA_DIR="/etc/iqrf-gateway-daemon/certs/core"
if [ ! -d ${CA_DIR} ]; then
  mkdir -p ${CA_DIR}
fi

if [ ! -f ${CA_DIR}/cert.pem ] || [ ! -f ${CA_DIR}/privkey.pem ]; then
  openssl ecparam -name secp384r1 -genkey -param_enc named_curve \
    -out ${CA_DIR}/privkey.pem
  openssl req -new -x509 -sha256 -nodes -days 3650 \
    -subj "/CN=IQRF Gateway/C=CZ/ST=Hradec Kralove Region/L=Jicin/O=IQRF Tech s.r.o." \
    -key ${CA_DIR}/privkey.pem -out ${CA_DIR}/cert.pem
  chmod 600 ${CA_DIR}/*.pem
fi

iqrfgd2 /etc/iqrf-gateway-daemon/config.json
