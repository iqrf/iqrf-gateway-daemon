***************************
How to configure the daemon
***************************

**CK-USB-04A or GW-USB-06 devices must be switched to USB CDC IQRF mode using IDE
menu:** Tools/USB Classes/Switch to CDC IQRF mode

Install IQRF Gateway Webapp
---------------------------

Follow the `installation guide`_ or you can modify configuration files manually
as shown in following chapter.

It is recommended to install `IQRF Gateway Webapp`_ since it also gives you powerful 
tool to setup and work with IQRF and Clouds (IBM Cloud, MS Azure, Amazon AWS IoT, 
Inteliments InteliGlue).

Main configuration parameters and its location
----------------------------------------------

`IQRF Gateway Webapp`_ gives you nice GUI to configure all these parameters an easy way.

Folder /etc/iqrf-gateway-daemon:

- config.json

  - **Tip: check/enable/disable daemon components**
  - Select either SPI, UART or CDC

- iqrf__IqrfSpi.json

  - **Tip: configure your IQRF interface - SPI**
  - Raspberry Pi (/dev/spidev0.0)
  - Orange Pi Zero (/dev/spidev1.0)
  - UP board (/dev/spidev2.0)
  - UP2 board (/dev/spidev1.0)
  - Aurora (/dev/spidev0.0)

- iqrf__IqrfUart.json

  - **Tip: configure your IQRF interface - UART**
  - Interface (/dev/ttyS0)
  - BaudRate (*Depends on TR configuration of your coordinator module*)

- iqrf__IqrfCdc.json

  - **Tip: check/configure your IQRF interface - CDC**
  - Interface (/dev/ttyACMx {x=0...y})

- iqrf__IqrfDpa.json

  - **Tip: check/configure your DPA params**
  - DPA Handler timeout (default time to wait for DPA cnf/rsp, 500ms)

- iqrf__MqttMessaging.json

  - **Tip: check/configure your MQTT broker**
  - Broker IP (127.0.0.1)
  - Broker port (1883)
  - Client topics (Iqrf/DpaRequest, Iqrf/DpaResponse)
  - Accept async msgs (true)

- iqrf__MqMessaging.json   

  - **Tip: check/configure your MQ names**
  - Remote MQ name (iqrf-daemon-100)
  - Local MQ name (iqrf-daemon-110)
  - Accept async msgs (true)

- iqrf__UdpMessaging.json

  - **Tip: check/configure your UDP ports**
  - Remote port (55000)
  - Local port (55300)

- iqrf__WebsocketMessaging.json

  - **Tip: check/configure your Websocket msgs**
  - Accept async msgs (true)

- shape__WebsocketService.json

  - **Tip: check/configure your Websocket port**
  - Websocket port (1338)

- iqrf__JsCache.json

  - **Tip: check/configure IQRF repository cache update period**
  - Cache update period (360m)

- shape__TraceFileService.json

  - **Tip: check/configure log file**
  - Path (/var/log/iqrf-gateway-daemon)
  - Filename (*Timestamp*-iqrf-gateway-daemon.log)
  - Size (1048576MB)
  - Timestamp (true)
  - Verbosity (INF)

Folder /var/cache/iqrf-gateway-daemon/scheduler:

- Tasks.json

  - **Tip: configure your regural DPA tasks**
  - See `Scheduler documentation`_

Restart the daemon
++++++++++++++++++

To load new configuration restart the daemon.

.. code-block:: bash

	sudo systemctl restart iqrf-gateway-daemon.service

.. _`installation guide`: https://docs.iqrf.org/iqrf-gateway-webapp/install.html
.. _`IQRF Gateway Webapp`: https://docs.iqrf.org/iqrf-gateway-webapp
.. _`Scheduler documentation`: https://docs.iqrf.org/iqrf-gateway-webapp/scheduler.html