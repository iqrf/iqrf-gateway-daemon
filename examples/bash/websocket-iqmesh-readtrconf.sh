#!/bin/bash
# Script for testing Websocket channel on Linux machine
#
# wget https://github.com/vi/websocat/releases/download/v1.0.0/websocat_1.0.0_amd64.deb
# sudo dpkg -i websocat_1.0.0_amd64.deb
# sudo apt-get install jq

echo "sending request to read config of device and listen for response"

echo \
"{\"mType\":\"iqmeshNetwork_ReadTrConf\","   \
"\"data\":{\"msgId\":\"test\",\"repeat\":1," \
"\"req\":{\"deviceAddr\":0},"		     \
"\"returnVerbose\":true}}" 		     \
| websocat --no-close ws://localhost:1338 | jq '.'
