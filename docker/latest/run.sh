#!/bin/bash

# IQRF Gateway Daemon
# Copyright (C) 2015-2025 IQRF Tech s.r.o., 2019-2025 MICRORISC s.r.o.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

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
