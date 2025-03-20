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

FROM arm32v5/debian:bullseye

LABEL maintainer="roman.ondracek@iqrf.com"

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
 && apt-get install --no-install-recommends -y dirmngr gnupg2 curl openssl \
 && curl -sSLo /usr/share/keyrings/iqrf.gpg http://repos.iqrf.org/apt.gpg \
 && echo "deb [signed-by=/usr/share/keyrings/iqrf.gpg] http://repos.iqrf.org/debian bullseye stable" | tee /etc/apt/sources.list.d/iqrf.list \
 && apt-get update \
 && apt-get install --no-install-recommends -y iqrf-gateway-daemon \
 && apt-get clean \
 && rm -rf /var/lib/apt/lists/* /etc/iqrf-gateway-daemon/certs/core/*

EXPOSE 1338 1438 55000/udp 55300/udp

COPY run.sh run.sh
CMD ["bash", "run.sh"]
