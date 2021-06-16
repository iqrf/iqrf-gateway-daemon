#!/usr/bin/env python

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

# #############################################################################
#                                                                             #
# sudo apt-get install python-dev python-pip build-essential                  #
# sudo pip install paho-mqtt                                                  #
#                                                                             #
# #############################################################################

import argparse
import time
import json

import paho.mqtt.client as paho

ARGS = argparse.ArgumentParser(description="IQRF daemon MQTT python example.")
ARGS.add_argument("-d", "--debug", action="store_true", dest="debug", help="Debug")
ARGS.set_defaults(debug=False)
ARGS.add_argument("-H", "--host", action="store", dest="mqtt_host", default="localhost", type=str, help="MQTT host")
ARGS.add_argument("-p", "--port", action="store", dest="mqtt_port", default=1883, type=int, help="MQTT port")
ARGS.add_argument("-tp", "--topic_pub", action="store", dest="topic_pub", default="Iqrf/DpaRequest", type=str,
                  help="MQTT topic for publishing")
ARGS.add_argument("-ts", "--topic_sub", action="store", dest="topic_sub", default="Iqrf/DpaResponse", type=str,
                  help="MQTT topis for subscribing")
ARGS.add_argument("-u", "--user", action="store", dest="mqtt_user", default=None, type=str, help="MQTT user")
ARGS.add_argument("-P", "--password", action="store", dest="mqtt_pass", default=None, type=str, help="MQTT password")


def on_connect(client, userdata, flags, rc):
    print('CONNACK received with code %d.' % (rc))


def on_publish(client, userdata, mid):
    print('mid: ' + str(mid))


def on_subscribe(client, userdata, mid, granted_qos):
    print('Subscribed: ' + str(mid) + ' ' + str(granted_qos))


def on_message(client, userdata, msg):
    print(msg.topic + ' ' + str(msg.qos) + ' ' + str(msg.payload))


def on_log(mqttc, userdata, level, string):
    print(string)


def create_dpa_frame(node_id, pnum, pcmd, hwpid, data=[]):
    byte_str = '%02x.%02x.%02x.%02x.%02x.%02x' % (node_id & 0xFF, node_id >> 8, pnum, pcmd, hwpid & 0xFF, hwpid >> 8)
    for i in data:
        byte_str += '.%02x' % i

    return byte_str


def create_dpa_json(msg_id, dpa_frame):
    request = {}
    request['mType'] = 'iqrfRaw'

    rdata = {}
    rdata['msgId'] = msg_id
    #rdata['timeout'] = timeout

    req = {}
    req['rData'] = dpa_frame

    rdata['req'] = req
    rdata['returnVerbose'] = True

    request['data'] = rdata

    return json.dumps(request)


def main():
    # IQRF
    # default HWPID
    hwpid = 0xffff
    # default DPA timeout (in miliseconds)
    timeout = 1000
    args = ARGS.parse_args()
    # MQTT
    host = args.mqtt_host
    port = args.mqtt_port
    keepalive = 60
    client_id = str(time.time())
    password = args.mqtt_pass
    username = args.mqtt_user
    debug = args.debug
    topic_pub = args.topic_pub
    topic_sub = args.topic_sub

    if debug:
        print("MQTT host: " + host + ":" + str(port))
        if username is not None and password is not None:
            print("username: " + username + "; password: " + password)
        print("Topics (pub/sub): " + topic_pub + "/" + topic_sub)

    # client
    client = paho.Client(client_id=client_id, clean_session=True, userdata=None, protocol=paho.MQTTv31)
    if username is not None and password is not None:
        print("username: " + username + "; password: " + password)
        client.username_pw_set(username, password)
    # client.tls_set("/path/to/ca.crt")

    # client callbacks
    client.on_connect = on_connect
    client.on_publish = on_publish
    client.on_subscribe = on_subscribe
    client.on_message = on_message

    # debug
    if debug:
        client.on_log = on_log

    # connect
    client.connect(host=host, port=port, keepalive=keepalive, bind_address='')

    # subscribe
    client.subscribe(topic_sub)

    # blocking, good for sub only
    # client.loop_forever()

    # not blocking, background thread, returns
    client.loop_start()
    # client.loop_stop()

    # DPA frame
    ledr_pulse = create_dpa_frame(0x00, 0x06, 0x03, hwpid)
    ledg_pulse = create_dpa_frame(0x00, 0x07, 0x03, hwpid)

    while True:
        # JSON DPA
        msg_id = str(time.time())
        json_ledr_pulse = create_dpa_json(msg_id, ledr_pulse)
	    msg_id = str(time.time())
	    json_ledg_pulse = create_dpa_json(msg_id, ledg_pulse)

        # publish
        (rc, mid) = client.publish(topic_pub, json_ledr_pulse, qos=1)

        # sleep
        time.sleep(2)

        # publish
        (rc, mid) = client.publish(topic_pub, json_ledg_pulse, qos=1)

        # sleep
        time.sleep(2)

if __name__ == "__main__":
    main()
