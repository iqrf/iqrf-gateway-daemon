#!/bin/bash
# Script for testing Websocket channel on Linux machine
#
# wget https://github.com/vi/websocat/releases/download/v1.0.0/websocat_1.0.0_amd64.deb
# sudo dpkg -i websocat_1.0.0_amd64.deb
# sudo apt-get install jq

#how long to wait for the discovery result in ms
if [ ! -z $1 ]
then
# user timeout
	tout=$1
else
# infinite timeout
	tout=0

fi

echo "sending request to the device and listen for response"

echo \
"{\"mType\":\"iqrfEmbedCoordinator_Discovery\","  	\
"\"data\":{\"msgId\":\"test\",\"timeout\":${tout},"  	\
"\"req\":{\"nAdr\":0,"	  			  	\
"\"param\":{\"txPower\":3,\"maxAddr\":0}},"	  	\
"\"returnVerbose\":true}}" 			  	\
| websocat --no-close ws://localhost:1338 | jq '.'
