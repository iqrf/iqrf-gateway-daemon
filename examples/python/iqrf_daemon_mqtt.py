# #############################################################################
# Author: 2017                                                                #  
#         Rostislav Spinar <rostislav.spinar@iqrf.com>                        #
#         Roman Ondracek <roman.ondracek@iqrf.com>                            #
#         IQRF Tech s.r.o.                                                    #
# #############################################################################

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
    request['ctype'] = 'dpa'
    request['type'] = 'raw'
    request['msgid'] = msg_id
    request['request'] = dpa_frame
    request['request_ts'] = ''
    request['confirmation'] = ''
    request['confirmation_ts'] = ''
    request['response'] = ''
    request['response_ts'] = ''

    return json.dumps(request)


def main():
    # IQRF
    # default hwpid
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

    # dpa frame
    dpa_frame = create_dpa_frame(0x0f, 0x06, 0x03, hwpid)

    while True:
        # json dpa
        msg_id = str(time.time())
        json_dpa = create_dpa_json(msg_id, dpa_frame)

        # publish
        (rc, mid) = client.publish(topic_pub, json_dpa, qos=1)

        # sleep
        time.sleep(10)


if __name__ == "__main__":
    main()
