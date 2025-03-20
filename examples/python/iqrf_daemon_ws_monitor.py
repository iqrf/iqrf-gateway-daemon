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
