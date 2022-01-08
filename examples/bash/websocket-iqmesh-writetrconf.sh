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

# Script for testing Websocket channel on Linux machine
#
# wget https://github.com/vi/websocat/releases/download/v1.1.0/websocat_1.1.0_amd64.deb
# wget https://github.com/vi/websocat/releases/download/v1.1.0/websocat_1.1.0_armhf.deb
# sudo dpkg -i websocat_1.1.0_*.deb
# sudo apt-get install jq

echo "sending request to write config of device and listen for response"

echo \
"{\"mType\":\"iqmeshNetwork_WriteTrConf\","  					\
"\"data\":{\"msgId\":\"test\",\"repeat\":1," 					\
"\"req\":{\"deviceAddr\":0,\"embPeripherals\":{\"frc\":true,\"ledr\":true}},"	\
"\"returnVerbose\":true}}" 		     					\
| websocat --no-close ws://localhost:1338 | jq '.'
