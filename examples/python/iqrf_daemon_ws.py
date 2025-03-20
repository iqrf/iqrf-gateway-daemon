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

from websocket import create_connection
import json
import uuid
import time

# IQRF STD JSON request
def create_std_json_req(m_type, msg_id, addr):

    request = {}
    request['mType'] = m_type

    data = {}
    data['msgId'] = msg_id

    req = {}
    req['nAdr'] = addr

    param = {}
    param['sensorIndexes'] = -1
    req['param'] = param

    data['req'] = req
    data['returnVerbose'] = True

    request['data'] = data

    return json.dumps(request)

def main():

    # websocket connection
    ws = create_connection("ws://localhost:1338")

    while True:
        m_type = 'iqrfSensor_ReadSensorsWithTypes'

        msg_id = str(uuid.uuid4())

        json_sensor_read_req = create_std_json_req(m_type, msg_id, 1)
        print(json_sensor_read_req)

        ws.send(json_sensor_read_req)
        rsp = ws.recv()

        json_sensor_read_rsp = json.loads(rsp)
        print(json_sensor_read_rsp)

        status = json_sensor_read_rsp['data']['status']
        if status == 0:
            print("ok")

        time.sleep(5)

    ws.close()
    return status

if __name__ == "__main__":
    main()
