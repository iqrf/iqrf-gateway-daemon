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

# IQRF GW daemon image for the RPI board
# Edit config/* files and set accordingly for your target

FROM multiarch/debian-debootstrap:armhf-buster

LABEL maintainer="Rostislav Spinar <rostislav.spinar@iqrf.com>"

RUN apt-get update \
 && apt-get install --no-install-recommends -y dirmngr gnupg2 \
 && apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 9C076FCC7AB8F2E43C2AB0E73241B9B7B4BD8F8E \
 && echo "deb http://repos.iqrf.org/debian buster stable" | tee /etc/apt/sources.list.d/iqrf.list \
 && apt-get update \
 && apt-get install --no-install-recommends -y iqrf-gateway-daemon openssl \
 && apt-get clean \
 && rm -rf /var/lib/apt/lists/*

# copy custom config
WORKDIR /etc/iqrf-gateway-daemon
COPY config-rpi/iqrf-gateway-daemon/. .

# expose ports
EXPOSE 1338 1438 55000/udp 55300/udp

# run the daemon service
CMD ["iqrfgd2", "-c", "/etc/iqrf-gateway-daemon/config.json"]
