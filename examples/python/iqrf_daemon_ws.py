#!/usr/bin/env python

# Copyright 2015-2023 IQRF Tech s.r.o.
# Copyright 2019-2023 MICRORISC s.r.o.
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
