#!/bin/bash

# Copyright 2015-2025 IQRF Tech s.r.o.
# Copyright 2019-2025 MICRORISC s.r.o.
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

# Script for testing MQTT channel on Linux machine
# mosquitto_pub/sub is using MQTT local broker to communicate to IQRF-Gateway daemon
# sudo apt-get install mosquitto-clients

echo "sending request to pulse red led on device"

mosquitto_pub -t "Iqrf/DpaRequest" -m "{\"mType\":\"iqrfRaw\",\"data\":{\"msgId\":\"test0\",\"req\":{\"rData\":\"00.00.06.03.ff.ff\"},\"returnVerbose\":true}}"
#mosquitto_pub -t "Iqrf/DpaRequest" -m "{\"mType\":\"iqrfRaw\",\"data\":{\"msgId\":\"test1\",\"req\":{\"rData\":\"01.00.06.03.ff.ff\"},\"returnVerbose\":true}}"
#mosquitto_pub -t "Iqrf/DpaRequest" -m "{\"mType\":\"iqrfRaw\",\"data\":{\"msgId\":\"test2\",\"req\":{\"rData\":\"02.00.06.03.ff.ff\"},\"returnVerbose\":true}}"

echo "listening for response"

mosquitto_sub -t "Iqrf/DpaResponse" | jq '.'
