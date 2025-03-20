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

FROM iqrftech/debian-c-builder:debian-bullseye-amd64

COPY ./ /iqrf
WORKDIR /iqrf

RUN mkdir deploy \
    && git submodule init \
    && git submodule update \
    && git clone --depth=1 --recurse-submodules https://gitlab.iqrf.org/open-source/iqrf-gateway-daemon-utils/shape.git \
    && git clone --depth=1 --recurse-submodules https://gitlab.iqrf.org/open-source/iqrf-gateway-daemon-utils/shapeware.git\
    && cmake -Bshape-build -Hshape -DCMAKE_BUILD_TYPE=Debug -DSHAPE_DEPLOY:PATH=$PWD/deploy \
    && cmake --build shape-build --target install \
    && cmake -Bshapeware-build -Hshapeware -Dshape_DIR:PATH=`pwd`/shape-build -DCMAKE_BUILD_TYPE=Debug \
    && cmake --build shapeware-build --target install \
    && cmake -Biqrf-gateway-daemon-build -H. -Dshape_DIR:PATH=`pwd`/shape-build -Dshapeware_DIR:PATH=`pwd`/shapeware-build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=false \
    && cmake --build iqrf-gateway-daemon-build --target install

ENV PATH="/iqrf/deploy/Debug/iqrf-gateway-daemon/bin:${PATH}"

WORKDIR /iqrf/deploy/Debug/iqrf-gateway-daemon/runcfg/iqrfgd2-LinCdc

COPY ./docker/api_testing/configuration ./configuration/

EXPOSE 1338 55000/udp 55300/udp

CMD ["iqrfgd2", "-c", "configuration/config.json"]
