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

from websocket import create_connection, WebSocketTimeoutException
import json
import time

def main():

    # websocket connection
    ws = create_connection("ws://localhost:1438")
    ws.settimeout(1)

    while True:

        try:
            rsp = ws.recv()
            print(rsp)
            json_monitor_rsp = json.loads(rsp)
            print(json_monitor_rsp)
        except WebSocketTimeoutException as e:
            print("Monitor websocket '%s'" %e)
        except KeyboardInterrupt:
            print("Monitor stopped.")
            return

if __name__ == "__main__":
    main()
