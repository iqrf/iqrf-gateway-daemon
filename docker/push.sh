#!/bin/bash

#latest
docker push iqrftech/iqrf-gateway-daemon:latest-amd64
docker push iqrftech/iqrf-gateway-daemon:latest-armhf
docker push iqrftech/iqrf-gateway-daemon:latest-armel
docker push iqrftech/iqrf-gateway-daemon:latest-iqdgw02

#stable (SPI pins set to -1, will work with v2.3)
#docker push iqrftech/iqrf-gateway-daemon:stable-amd64
#docker push iqrftech/iqrf-gateway-daemon:stable-armhf
#docker push iqrftech/iqrf-gateway-daemon:stable-armel
#docker push iqrftech/iqrf-gateway-daemon:stable-iqdgw02
