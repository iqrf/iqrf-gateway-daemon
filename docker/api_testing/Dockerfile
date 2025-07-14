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
    && cmake -Biqrf-gateway-daemon-build -H. -Dshape_DIR:PATH=`pwd`/shape-build -Dshapeware_DIR:PATH=`pwd`/shapeware-build -DCMAKE_BUILD_TYPE=Debug \
    && cmake --build iqrf-gateway-daemon-build --target install

ENV PATH="/iqrf/deploy/Debug/iqrf-gateway-daemon/bin:${PATH}"

WORKDIR /iqrf/deploy/Debug/iqrf-gateway-daemon/runcfg/iqrfgd2-LinCdc

COPY ./docker/api_testing/configuration ./configuration/

COPY ./docker/api_testing/healthcheck /iqrf/deploy/Debug/iqrf-gateway-daemon/bin
RUN chmod +x /iqrf/deploy/Debug/iqrf-gateway-daemon/bin/healthcheck
RUN pip install websocket-client
HEALTHCHECK --start-period=10s CMD healthcheck

EXPOSE 1338 1438 55000/udp 55300/udp

CMD ["iqrfgd2", "-c", "configuration/config.json"]
