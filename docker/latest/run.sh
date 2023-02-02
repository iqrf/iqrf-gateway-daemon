#!/bin/bash

# Copyright 2015-2023 IQRF Tech s.r.o.
# Copyright 2019-2023 MICRORISC s.r.o.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

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

iqrfgd2 -c /etc/iqrf-gateway-daemon/config.json
