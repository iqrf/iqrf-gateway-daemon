#!/bin/bash
# Script for testing Websocket channel on Linux machine
#
# wget https://github.com/vi/websocat/releases/download/v1.1.0/websocat_1.1.0_amd64.deb
# wget https://github.com/vi/websocat/releases/download/v1.1.0/websocat_1.1.0_armhf.deb
# sudo dpkg -i websocat_1.1.0_*.deb
# sudo apt-get install jq

echo "sending request to enumerate device and listen for response"

echo \
"{\"mType\":\"iqmeshNetwork_EnumerateDevice\","   \
"\"data\":{\"msgId\":\"test\",\"repeat\":1," 	  \
"\"req\":{\"deviceAddr\":0},"                	  \
"\"returnVerbose\":true}}"                   	  \
| websocat --no-close ws://localhost:1338 | jq '.'
