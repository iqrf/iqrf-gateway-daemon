#!/bin/bash

# IQRF Gateway Daemon
# Copyright (C) 2015-2025 IQRF Tech s.r.o., 2019-2025 MICRORISC s.r.o.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# Script for testing MQTT channel on Linux machine
# mosquitto_pub/sub is using MQTT local broker to communicate to IQRF-Gateway daemon
# sudo apt-get install mosquitto-clients

echo "sending request to pulse red led on device"

mosquitto_pub -t "Iqrf/DpaRequest" -m "{\"mType\":\"iqrfRaw\",\"data\":{\"msgId\":\"test0\",\"req\":{\"rData\":\"00.00.06.03.ff.ff\"},\"returnVerbose\":true}}"
#mosquitto_pub -t "Iqrf/DpaRequest" -m "{\"mType\":\"iqrfRaw\",\"data\":{\"msgId\":\"test1\",\"req\":{\"rData\":\"01.00.06.03.ff.ff\"},\"returnVerbose\":true}}"
#mosquitto_pub -t "Iqrf/DpaRequest" -m "{\"mType\":\"iqrfRaw\",\"data\":{\"msgId\":\"test2\",\"req\":{\"rData\":\"02.00.06.03.ff.ff\"},\"returnVerbose\":true}}"

echo "listening for response"

mosquitto_sub -t "Iqrf/DpaResponse" | jq '.'
