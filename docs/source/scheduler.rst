Scheduler
=========

Schedule any `JSON API`_ request as a single task by editing /var/cache/iqrf-gateway-daemon/scheduler/Tasks.json
and restart the daemon. Since v2.1.0 Tasks.json has been removed and replaced by **one json file for single task**. 

.. code-block:: bash
	
	sudo systemctl restart iqrf-gateway-daemon.service

Example
-------

Following example schedules two tasks. 

- first task is queued at 0 sec every 1 min
- second task is queued at 1 sec also every 1 min 
- time follows `Cron format`_ with addition for second resolution.
- use service **SchedulerMessaging**  
- choose messaging (**MqMessaging**, **WebsocketMessaging**, **MqttMessaging**) `JSON API`_ response will be sent 

v2.0.0
++++++

.. code-block:: json

  {
    "TasksJson": [
      {
        "time": "0 */1 * * * * *",
        "service": "SchedulerMessaging",
        "task": {
          "messaging": "WebsocketMessaging",
          "message": {
            "mType": "iqmeshNetwork_EnumerateDevice",
            "data": {
              "msgId": "a726ecb9-ee7c-433a-9aa4-3fb21cae2d4d",
              "repeat": 1,
              "req": {
              "deviceAddr": 1
              },
              "returnVerbose": true
            }
          }
        }
      },
      {
        "time": "1 */1 * * * * *",
        "service": "SchedulerMessaging",
        "task": {
          "messaging": "WebsocketMessaging",
          "message": {
            "mType": "iqmeshNetwork_EnumerateDevice",
            "data": {
              "msgId": "b726ecb9-ee7c-433a-9aa4-3fb21cae2d4d",
              "repeat": 1,
              "req": {
              "deviceAddr": 2
              },
              "returnVerbose": true
            }
          }
        }
      }
    ]
  }

v2.1.0
++++++

- Task 1, filename e.g. /var/cache/iqrf-gateway-daemon/scheduler/1.json

.. code-block:: json

  {
      "taskId": 1,
      "clientId": "SchedulerMessaging",
      "timeSpec": {
          "cronTime": "0 */1 * * * * *",
          "exactTime": false,
          "periodic": false,
          "period": 0,
          "startTime": ""
      },
      "task": {
          "messaging": "MqttMessaging",
          "message": {
              "mType": "iqrfEmbedLedr_Pulse",
              "data": {
                  "msgId": "testEmbedLedr",
                  "req": {
                      "nAdr": 1,
                      "param": {}
                  },
                  "returnVerbose": true
              }
          }
      }
  }

- Task 2, filename e.g. /var/cache/iqrf-gateway-daemon/scheduler/2.json

.. code-block:: json

  {
      "taskId": 2,
      "clientId": "SchedulerMessaging",
      "timeSpec": {
          "cronTime": "0 */2 * * * * *",
          "exactTime": false,
          "periodic": false,
          "period": 0,
          "startTime": ""
      },
      "task": {
          "messaging": "MqttMessaging",
          "message": {
              "mType": "iqrfEmbedLedr_Pulse",
              "data": {
                  "msgId": "testEmbedLedr",
                  "req": {
                      "nAdr": 2,
                      "param": {}
                  },
                  "returnVerbose": true
              }
          }
      }
  }

- cron time

  - exactTime = false
  - periodic = false
  - cronTime valid

- periodic time

  - exactTime = false
  - periodic = true
  - period > 0
  - if startTime valid > now => delayed start else now

- one shot time

  - exactTime = true
  - periodic = false
  - if startTime valid > now => delayed one shot time else ignored

- N/A

  - exactTime = true
  - periodic = true

CRON nicknames
--------------

It is possible to use CRON nicknames for time pattern.

- "@reboot": Run once after reboot.
- "@yearly": Run once a year, ie.  "0 0 0 0 1 1 *".
- "@annually": Run once a year, ie.  "0 0 0 0 1 1 *".
- "@monthly": Run once a month, ie. "0 0 0 0 1 * *".
- "@weekly": Run once a week, ie.  "0 0 0 * * * 0".
- "@daily": Run once a day, ie.   "0 0 0 * * * *".
- "@hourly": Run once an hour, ie. "0 0 * * * * *".
- "@minutely": Run once a minute, ie. "0 * * * * * *".

API
---

Scheduler can be also configured via `Scheduler API`_. **Configuration via API is persistent since v2.1.0**.

There are examples in `C# test app`_.

.. _`JSON API`: https://docs.iqrf.org/iqrf-gateway-daemon/api.html
.. _`Cron format`: https://en.wikipedia.org/wiki/Cron
.. _`Scheduler API`: https://docs.iqrf.org/iqrf-gateway-daemon/api.html#daemon-scheduler
.. _`C# test app`: https://gitlab.iqrf.org/open-source/iqrf-gateway-daemon/tree/master/examples/c#
