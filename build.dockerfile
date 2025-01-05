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

# BUILDS IQRF GW daemon from source code
# RUNS IQRF GW daemon with USB IQRF CDC coordinator

FROM multiarch/debian-debootstrap:amd64-buster

LABEL maintainer="Rostislav Spinar <rostislav.spinar@iqrf.com>"

RUN apt-get update \
  && apt-get install --no-install-recommends -y apt-transport-https ca-certificates dirmngr gnupg2 \
  && apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 9C076FCC7AB8F2E43C2AB0E73241B9B7B4BD8F8E \
  && echo "deb https://repos.iqrf.org/debian buster stable" | tee -a /etc/apt/sources.list \
  && apt-get update \
  && apt-get install --no-install-recommends -y build-essential git cmake libssl-dev zlib1g-dev \
    libcurl4-openssl-dev libpaho-mqtt-dev libboost-filesystem-dev libsqlite3-dev libzmq3-dev \
  && apt-get clean \
  && rm -rf /var/lib/apt/lists/*

RUN mkdir iqrf \
  && cd iqrf \
  && git clone --recurse-submodules https://github.com/logimic/shape.git \
  && cd shape \
  && ./buildMake.sh \
  && cd .. \
  && git clone --recurse-submodules https://github.com/logimic/shapeware.git \
  && cd shapeware \
  && ./buildMake.sh \
  && cd .. \
  && mkdir iqrftech \
  && cd iqrftech \
  && git clone --recurse-submodules https://gitlab.iqrf.org/open-source/iqrf-gateway-daemon.git \
  && cd iqrf-gateway-daemon \
  && ./buildMake.sh

WORKDIR $HOME/iqrf/shape/deploy/Unix_Makefiles/Debug/iqrf-gateway-daemon/runcfg/iqrfgd2-LinCdc

ENV PATH="$HOME/iqrf/shape/deploy/Unix_Makefiles/Debug/iqrf-gateway-daemon/bin:${PATH}"

EXPOSE 1338 55000/udp 55300/udp

CMD ["iqrfgd2", "-c", "configuration/config.json"]
