#!/bin/bash

# Copyright 2015-2024 IQRF Tech s.r.o.
# Copyright 2019-2024 MICRORISC s.r.o.
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

#operational 	... MQ, MQTT and Websocket channels are enabled
#service 	... UDP channel is enabled and IQRF IDE can be used
#forwarding 	... trafic via MQ, MQTT and Websocket channels is forwarded to UDP channel

if [ ! -z $1 ]
then
# user selected mode
        if [ $1 == "operational" ]
        then
                cmd=$1
        elif [ $1 == "service" ]
        then
                cmd=$1
        elif [ $1 == "forwarding" ]
        then
                cmd=$1
        else
                cmd="operational"
        fi
else
# mode operational by default
        cmd="operational"

fi

echo "sending request to the deamon and listen for response"

echo \
"{\"mType\":\"mngDaemon_Mode\"," 	\
"\"data\":{\"msgId\":\"test\","  	\
"\"req\":{\"operMode\":\"${cmd}\"},"	\
"\"returnVerbose\":true}}" 		\
| websocat --no-close ws://localhost:1338 | jq '.'
