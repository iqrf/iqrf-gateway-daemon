#!/bin/bash

#latest
docker build -f amd64-latest.Dockerfile -t iqrftech/iqrf-gateway-daemon:latest-amd64 .
docker build -f armhf-latest.Dockerfile -t iqrftech/iqrf-gateway-daemon:latest-armhf .
docker build -f armel-latest.Dockerfile -t iqrftech/iqrf-gateway-daemon:latest-armel .
docker build -f iqdgw02-latest.Dockerfile -t iqrftech/iqrf-gateway-daemon:latest-iqdgw02 .

#stable(SPI pins set to -1, will work with v2.3)
#docker build -f amd64-stable.Dockerfile -t iqrftech/iqrf-gateway-daemon:stable-amd64 .
#docker build -f armhf-stable.Dockerfile -t iqrftech/iqrf-gateway-daemon:stable-armhf .
#docker build -f armel-stable.Dockerfile -t iqrftech/iqrf-gateway-daemon:stable-armel .
#docker build -f iqdgw02-stable.Dockerfile -t iqrftech/iqrf-gateway-daemon:stable-iqdgw02 .
