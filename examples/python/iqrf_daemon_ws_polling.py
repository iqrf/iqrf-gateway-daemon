#!/usr/bin/env python

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

# #############################################################################
#                                                                             #
# sudo apt-get install python3-dev python3-pip build-essential                #
# sudo pip3 install websocket-client                                          #
#                                                                             #
# #############################################################################

import argparse
import time
import json
import uuid

from websocket import create_connection
from datetime import datetime

#ARGS = argparse.ArgumentParser(description="IQRF GW daemon WS python example.")
#ARGS.add_argument("-d", "--debug", action="store_true", dest="debug", help="Debug")
#ARGS.set_defaults(debug=False)
#ARGS.add_argument("-h", "--host", action="store", dest="ws_host", default="localhost", type=str, help="WS host")
#ARGS.add_argument("-p", "--port", action="store", dest="ws_port", default=1338, type=int, help="WS port")

NODES=80

def create_json_req(m_type, msg_id, addr):
    request = {}
    request['mType'] = m_type

    data = {}
    data['msgId'] = msg_id

    req = {}
    req['nAdr'] = addr

    param = {}
    req['param'] = param

    data['req'] = req
    data['returnVerbose'] = True

    request['data'] = data

    return json.dumps(request)

def main():
    # args parsing
    #args = ARGS.parse_args()

    # connection
    #host = args.ws_host
    #port = args.ws_port
    #debug = args.debug
    debug = False

    # websocket connection
    #ws = create_connection("ws://" + host + ":" + port)
    ws = create_connection("ws://localhost:1338")

    # variables
    rsp_err = 0
    rsp_ok = 0
    cnt = 0

    m_type = 'iqrfEmbedOs_Read'
    nadr = 0

    start_loop = round(datetime.utcnow().timestamp() * 1000)

    while True:
        msg_id = str(uuid.uuid4())

        json_read_req = create_json_req(m_type, msg_id, nadr)
        if debug:
            print(json_read_req)

        start = round(datetime.utcnow().timestamp() * 1000)
        ws.send(json_read_req)
        rsp = ws.recv()
        end = round(datetime.utcnow().timestamp() * 1000)

        json_read_rsp = json.loads(rsp)
        if debug:
            print(json_read_rsp)

        # ok/error cnt
        status = json_read_rsp['data']['status']
        if status == 0:
            rsp_ok += 1
        else:
            print(json_read_rsp)
            rsp_err += 1

        # address handling
        if nadr < NODES:
            nadr += 1
        else:
            ws.close()
            break

        cnt += 1

        print('===')
        print('Number of sending message: %d' %(cnt))
        print('Time to receive response %d ms' %(end - start))
        print('Number of responses received correctly: %d' %(rsp_ok))
        print('Number of responses failed: %d' %(rsp_err))
        print('Time from loop start %d ms' %(end - start_loop))

        time.sleep(.001)

if __name__ == "__main__":
    main()
