#!/bin/bash

# Copyright 2015-2021 IQRF Tech s.r.o.
# Copyright 2019-2021 MICRORISC s.r.o.
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
