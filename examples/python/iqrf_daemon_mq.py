#!/usr/bin/env python2.7

# #############################################################################
# Author: 2016 Josef Hajek <hajek@rehivetech.com>                             #
#         RehiveTech spol. s r.o., www.rehivetech.com                         #
#                                                                             #
# Author: 2017 Rostislav Spinar <rostislav.spinar@iqrf.com>                   #
#         IQRF Tech s.r.o.                                                    #
# #############################################################################

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
    Class handles dpa messages communication with iqrf daemon
    """

    rxq = None
    txq = None

    # how often check rx queue for data during reading (in miliseconds)
    RX_CHECK_PERIOD = 10

    # iqrf daemon config files
    IQRF_CFG_DIR = '/etc/iqrf-daemon/'
    IQRF_MQ_CONFIG = IQRF_CFG_DIR + 'MqMessaging.json'
    IQRF_CFG_IFACE = IQRF_CFG_DIR + 'IqrfInterface.json'
    IQRF_CFG_MQTT = IQRF_CFG_DIR + 'MqttMessaging.json'
    
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
            cfg = cfg['Instances']
            for i in cfg:  # iterate instances
                if i['Name'] == 'MqMessaging' and i['Enabled'] is True:
                    return (i['Properties']['LocalMqName'],
                            i['Properties']['RemoteMqName'])
            return (None, None)

    def clear_rx_queue(self):
        """
        Reads all the messages from RX queue.
        """
        while self.rxq.current_messages > 0:
            self.rxq.receive()

    def dpa_request(self, data, tout=TIMEOUT, ctype='dpa'):
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

        # when requesting bonded nodes, dpa ctype is not working
        # ctype and msgid must be removed from request
        request['ctype'] = ctype
        request['type'] = 'raw'
        request['msgid'] = msgid
        request['timeout'] = tout
        request['request'] = data

        self.clear_rx_queue()

        dpa_response = None

        try:
            self.txq.send(json.dumps(request))
        except Exception:
            dpa_response = {'status': 'ERROR_QUEUE_BUSY'}

        while True:
            while self.rxq.current_messages > 0:
                try:
                    rsp = self.rxq.receive()
                    rsp = json.loads(rsp[0])
                    if rsp['msgid'] == msgid:
                        dpa_response = rsp
                except Exception:
                    pass

            if dpa_response is not None:
                break

            time.sleep(self.RX_CHECK_PERIOD / 1000.0)
            tout = tout - self.RX_CHECK_PERIOD
            if tout < 0:
                dpa_response = {'status': 'ERROR_TIMEOUT'}
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

    def get_device_info(self, node_id):
        """
        Get peripheral information about the node.
        Params:
            node_id - id of the node
        """

        values = {}
        # dummy data
        values["module_id"] = 0
        values["os_version"] = "Unknown"
        values["tr_mcu_type"] = "Unknown"
        values["os_build"] = 0
        values["rssi"] = "-130 dBm"
        values["supply_voltage"] = "0 V"
        values["flags"] = 0
        values["online"] = False
        values["rssi_numerical"] = -131
        values["supply_voltage_numerical"] = 0

        bstr = self.cmd_byte_string(node_id, 0x02, 0x00)
        ret = self.dpa_request(bstr)
        if ret['status'] != 'STATUS_NO_ERROR':
            return values

        ret = self.dpabytes2bytes(ret['response'])[8:]

        # get module id
        values["module_id"] = ret[3] << 24 | ret[2] << 16 | ret[1] << 8 | ret[0]

        # get os version
        if ret[5] & 0x7 == 4:
            postfix = "D"
        else:
            postfix = "?"
        values["os_version"] = "%d.%02d%s" % (ret[4] >> 4, ret[4] & 0xf, postfix)

        # get TR and MCU type
        if values["module_id"] & 0x80000000 > 0:
            dctr = "DCTR"
        else:
            dctr = "TR"

        val = (ret[5] >> 4) & 0x7
        tr = "-?%d" % val
        if val == 0:
            tr = "-52D"
        if val == 1:
            tr = "58D-RJ"
        if val == 2:
            tr = "-72D"
        if val == 3:
            tr = "-53D"
        if val == 8:
            tr = "-54D"
        if val == 9:
            tr = "-55D"
        if val == 10:
            tr = "-56D"
        if val == 11:
            tr = "-76D"

        val = (ret[5] >> 3) & 0x1
        fcc = "FCC not certified"
        if val == 1:
            fcc = "FCC certified"

        val = ret[5] & 0x7
        mcu = "unknown MCU"
        if val == 4:
            mcu = "PIC16F1938"

        values['tr_mcu_type'] = "%s%s, %s, %s" % (dctr, tr, fcc, mcu)

        # get OS build
        values["os_build"] = ret[7] << 8 | ret[6]

        # get RSSI
        val = ret[8] - 130
        values["rssi_numerical"] = val
        values["rssi"] = "%d dBm" % (val)

        # get supply voltage
        val = 261.12 / (127.0 - ret[9])
        values["supply_voltage"] = "%f V" % (val)
        values["supply_voltage_numerical"] = val

        values["flags"] = ret[10]
        values["online"] = True

        return values

if __name__ == "__main__":
    mq = IQRF_MQ()
    
    print mq.pulse_ledr(1)
    print mq.get_device_info(1)
