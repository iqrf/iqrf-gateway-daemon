Scheduler
=========

Schedule any `JSON API`_ request as a single task by editing /var/cache/iqrf-gateway-daemon/scheduler/Tasks.json
and restart the daemon.

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

API
---

Scheduler can be also configured via `Scheduler API`_. **Configuration via API is not persistent yet**.

There are examples in `C# test app`_.

.. _`JSON API`: https://docs.iqrfsdk.org/iqrf-gateway-daemon/api.html
.. _`Cron format`: https://en.wikipedia.org/wiki/Cron
.. _`Scheduler API`: https://docs.iqrfsdk.org/iqrf-gateway-daemon/api.html#daemon-scheduler
.. _`C# test app`: https://github.com/iqrfsdk/iqrf-gateway-daemon/tree/master/examples/c#
