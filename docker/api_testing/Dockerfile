FROM iqrftech/iqrf-gateway-daemon-builder:debian-buster-amd64

COPY ./ /iqrf
WORKDIR /iqrf

RUN mkdir deploy \
    && git clone --recurse-submodules https://github.com/logimic/shape.git \
    && cmake -Bshape-build -Hshape -DCMAKE_BUILD_TYPE=Debug -DSHAPE_DEPLOY:PATH=$PWD/deploy \
    && cmake --build shape-build --target install \
    && git clone --recurse-submodules https://github.com/logimic/shapeware.git \
    && cmake -Bshapeware-build -Hshapeware -Dshape_DIR:PATH=`pwd`/shape-build -DCMAKE_BUILD_TYPE=Debug \
    && cmake --build shapeware-build --target install \
    && cd src/start-IqrfDaemon/iqrfRepoCache/ && python3 update-cache.py && cd ../../../ \
    && cmake -Biqrf-gateway-daemon-build -H. -Dshape_DIR:PATH=`pwd`/shape-build -Dshapeware_DIR:PATH=`pwd`/shapeware-build -DCMAKE_BUILD_TYPE=Debug \
    && cmake --build iqrf-gateway-daemon-build --target install

ENV PATH="/deploy/Debug/iqrf-gateway-daemon/bin:${PATH}"

WORKDIR /deploy/Debug/iqrf-gateway-daemon/runcfg/iqrfgd2-LinCdc

COPY ./configuration ./configuration/

EXPOSE 1338 55000/udp 55300/udp

CMD ["iqrfgd2", "configuration/config.json"]
