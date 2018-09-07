#!/bin/bash
# Script for testing Websocket channel on Linux machine
#
# wget https://github.com/vi/websocat/releases/download/v1.0.0/websocat_1.0.0_amd64.deb
# sudo dpkg -i websocat_1.0.0_amd64.deb
# sudo apt-get install jq

echo "sending request to pulse red led on device and listen for response"

echo \
"{\"mType\":\"iqrfRaw\"," 			 \
"\"data\":{\"msgId\":\"test\",\"timeout\":1000," \
"\"req\":{\"rData\":\"00.00.06.03.ff.ff\"},"	 \
"\"returnVerbose\":true}}" 			 \
| websocat --no-close ws://localhost:1338 | jq '.'
