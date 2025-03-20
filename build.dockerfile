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
