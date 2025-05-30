# Copyright 2015-2025 IQRF Tech s.r.o.
# Copyright 2019-2025 MICRORISC s.r.o.
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

# IQRF GW daemon image for the IQD-GW-01A board
# Edit config/* files and set accordingly for your target

FROM balenalib/orange-pi-zero-debian:latest

# copy custom config
WORKDIR /etc
COPY config-iqube/iqrf-gateway.json iqrf-gateway.json

RUN apt-get update \
 && apt-get install --no-install-recommends -y dirmngr gnupg2 openssl jq \
 && apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 9C076FCC7AB8F2E43C2AB0E73241B9B7B4BD8F8E \
 && echo "deb http://repos.iqrf.org/iqd-gw-01 bullseye stable" | tee /etc/apt/sources.list.d/iqrf.list \
 && apt-get update \
 && apt-get install --no-install-recommends -y iqrf-gateway-daemon \
 && apt-get clean \
 && rm -rf /var/lib/apt/lists/*

# copy custom config
WORKDIR /etc/iqrf-gateway-daemon
COPY config-iqube/iqrf-gateway-daemon/. .

# expose ports
EXPOSE 1338 1438 55000/udp 55300/udp

# run the daemon service
CMD ["iqrfgd2", "-c", "/etc/iqrf-gateway-daemon/config.json"]
