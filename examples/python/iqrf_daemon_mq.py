#!/usr/bin/env python

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

# #############################################################################
#                                                                             #
# sudo apt-get install python-dev build-essential                             #
# sudo apt-get install python-pip                                             #
# sudo pip install posix_ipc                                                  #
#                                                                             #
# #############################################################################

import time
import json
import posix_ipc as ipc

class IQRF_MQ():
    """
    Class handles DPA messages communication with IQRF GW Daemon
    """

    rxq = None
    txq = None

    # how often check rx queue for data during reading (in miliseconds)
    RX_CHECK_PERIOD = 10

    # iqrf daemon config files
    IQRF_CFG_DIR = '/etc/iqrfgd2/'
    IQRF_MQ_CONFIG = IQRF_CFG_DIR + 'iqrf__MqMessaging.json'

    # default HWPID
    HWPID = 0xFFFF

    # default DPA timeout (in miliseconds)
    TIMEOUT = 1000

    def __init__(self):
        txq_name, rxq_name = self.load_config()
        self.rxq = ipc.MessageQueue('/' + rxq_name, ipc.O_CREAT)
        self.txq = ipc.MessageQueue('/' + txq_name, ipc.O_CREAT,
                                    max_messages=10, max_message_size=2048)
        self.txq.block = False
        self.rxq.block = False

    def load_config(self):
        """
        Return local and remote queue names loaded from iqrf config file.
        """
        with open(self.IQRF_MQ_CONFIG, 'r') as f:
            cfg = json.load(f)
            return (cfg['LocalMqName'], cfg['RemoteMqName'])

    def clear_rx_queue(self):
        """
        Reads all the messages from RX queue.
        """
        while self.rxq.current_messages > 0:
            self.rxq.receive()

    def dpa_request(self, data, tout=TIMEOUT, mtype='iqrfRaw'):
        """
        Send DPA request.
        Params
            - data - bytes string in format 01.05.ff.aa
            - tout - message timeout in miliseconds
        Return
            - DPA response dictionary or
            - None in case of timeout
        """
        msgid = str(time.time())  # message if for response identify

        request = {}
        request['mType'] = mtype

        rdata = {}
        rdata['msgId'] = msgid
        # can be left out, daemon handles right timing
        rdata['timeout'] = tout

        req = {}
        req['rData'] = data

        rdata['req'] = req
        rdata['returnVerbose'] = True

	    request['data'] = rdata

        self.clear_rx_queue()

        dpa_response = None

        try:
            self.txq.send(json.dumps(request))
        except Exception:
            dpa_response = {'statusStr': 'ERROR_QUEUE_BUSY'}

        while True:
            while self.rxq.current_messages > 0:
                try:
                    rsp = self.rxq.receive()
                    rsp = json.loads(rsp[0])
                    if rsp['data']['msgId'] == msgid:
                        dpa_response = rsp
                except Exception:
                    pass

            if dpa_response is not None:
                break

            time.sleep(self.RX_CHECK_PERIOD / 1000.0)
            tout = tout - self.RX_CHECK_PERIOD
            if tout < 0:
                dpa_response = {'statusStr': 'ERROR_TIMEOUT'}
                break
        return dpa_response

    def cmd_byte_string(self, node_id, pnum, pcmd, data=[]):
        """
        Generate DPA byte header string
        """
        hwpid = self.HWPID
        byte_str = '%02x.%02x.%02x.%02x.%02x.%02x' % (node_id & 0xFF,
                                                      node_id >> 8,
                                                      pnum, pcmd,
                                                      hwpid & 0xFF,
                                                      hwpid >> 8)
        for i in data:
            byte_str += '.%02x' % i
        return byte_str

    def dpabytes2bytes(self, dpastring):
        """
        Convert string of bytes separated by dots to list of integers
        """
        return [int(i, 16) for i in dpastring.split('.')]

    def pulse_ledr(self, node_id=0):
        """
        Pulse red led on given node_id.
        Return whole DPA response.
        """
        bstr = self.cmd_byte_string(node_id, 0x06, 0x03)
        return self.dpa_request(bstr)

    def pulse_ledg(self, node_id=0):
        """
        Pulse red led on given node_id.
        Return whole DPA response.
        """
        bstr = self.cmd_byte_string(node_id, 0x07, 0x03)
        return self.dpa_request(bstr)

if __name__ == "__main__":
    mq = IQRF_MQ()
    print mq.pulse_ledr(0)
    print mq.pulse_ledg(0)
