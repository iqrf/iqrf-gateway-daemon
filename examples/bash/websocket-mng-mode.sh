#!/bin/bash
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
