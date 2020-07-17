FROM iqrftech/iqrf-gateway-daemon-builder:debian-buster-amd64

COPY ./ /iqrf
WORKDIR /iqrf

RUN mkdir deploy \
    && git submodule init \
    && git submodule update \
    && git clone --recurse-submodules https://github.com/logimic/shape.git \
    && cmake -Bshape-build -Hshape -DCMAKE_BUILD_TYPE=Debug -DSHAPE_DEPLOY:PATH=$PWD/deploy \
    && cmake --build shape-build --target install \
    && git clone --recurse-submodules https://github.com/logimic/shapeware.git \
    && cmake -Bshapeware-build -Hshapeware -Dshape_DIR:PATH=`pwd`/shape-build -DCMAKE_BUILD_TYPE=Debug \
    && cmake --build shapeware-build --target install \
    && python3 src/start-IqrfDaemon/iqrfRepoCache/update-cache.py \
    && cmake -Biqrf-gateway-daemon-build -H. -Dshape_DIR:PATH=`pwd`/shape-build -Dshapeware_DIR:PATH=`pwd`/shapeware-build -DCMAKE_BUILD_TYPE=Debug \
    && cmake --build iqrf-gateway-daemon-build --target install

ENV PATH="/iqrf/deploy/Debug/iqrf-gateway-daemon/bin:${PATH}"

WORKDIR /iqrf/deploy/Debug/iqrf-gateway-daemon/runcfg/iqrfgd2-LinCdc

COPY ./docker/api_testing/configuration ./configuration/

EXPOSE 1338 55000/udp 55300/udp

CMD ["iqrfgd2", "configuration/config.json"]
