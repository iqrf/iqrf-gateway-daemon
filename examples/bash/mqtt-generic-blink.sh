#!/bin/bash
# Script for testing MQTT channel on Linux machine
# mosquitto_pub/sub is using MQTT local broker to communicate to IQRF-Gateway daemon
# sudo apt-get install mosquitto-clients

echo "sending request to pulse red led on device"

mosquitto_pub -t "Iqrf/DpaRequest" -m "{\"mType\":\"iqrfRaw\",\"data\":{\"msgId\":\"test0\",\"req\":{\"rData\":\"00.00.06.03.ff.ff\"},\"returnVerbose\":true}}"
#mosquitto_pub -t "Iqrf/DpaRequest" -m "{\"mType\":\"iqrfRaw\",\"data\":{\"msgId\":\"test1\",\"req\":{\"rData\":\"01.00.06.03.ff.ff\"},\"returnVerbose\":true}}"
#mosquitto_pub -t "Iqrf/DpaRequest" -m "{\"mType\":\"iqrfRaw\",\"data\":{\"msgId\":\"test2\",\"req\":{\"rData\":\"02.00.06.03.ff.ff\"},\"returnVerbose\":true}}"

echo "listening for response"

mosquitto_sub -t "Iqrf/DpaResponse" | jq '.'
