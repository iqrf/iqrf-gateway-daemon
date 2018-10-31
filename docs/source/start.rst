Getting started
===============

* Install the `IQRF Gateway Daemon`_
* Install the `IQRF Gateway Webapp`_
* `Configure`_ your IQRF interface for coordinator TR module

  * Select one of from SPI, UART or CDC based on your HW

* `Learn`_ about the daemon API
* `Scheduler`_ helps with regular tasks 
* Choose `MQ`_/`WS`_/`MQTT`_ channel for communication with the daemon

  * Check/Set configuration for your channel using `IQRF Gateway Webapp`_

.. _`IQRF Gateway Daemon`: https://docs.iqrf.org/iqrf-gateway-daemon/install.html
.. _`IQRF Gateway Webapp`: https://docs.iqrf.org/iqrf-gateway-webapp/install.html
.. _`Configure`: https://docs.iqrf.org/iqrf-gateway-daemon/configure.html
.. _`MQ`: https://en.wikipedia.org/wiki/Message_queue
.. _`WS`: https://en.wikipedia.org/wiki/WebSocket
.. _`MQTT`: https://cs.wikipedia.org/wiki/MQTT
.. _`Learn`: https://docs.iqrf.org/iqrf-gateway-daemon/api.html
.. _`Scheduler`: https://docs.iqrf.org/iqrf-gateway-daemon/scheduler.html

Next steps
----------

* Use http://IQRF-Gateway-Webapp-IP/iqrfnet/network/ to *Bond via button*/*Smart Connect via QR code* new devices into the IQRF network
* Use http://IQRF-Gateway-Webapp-IP/cloud/{aws/azure/bluemix/inteli-glue/} manager to connect GW to the favourite cloud 
* Configure any JSON API task in the `Scheduler`_ or send `JSON API`_ requests from your application directly
* If you use IQRF standard devices such as Sensor, Binary output or Light in your network, check `JSON API for Standard`_    
* Parse `JSON API`_ responses coming from the network

.. _`JSON API`: https://docs.iqrf.org/iqrf-gateway-daemon/api.html
.. _`JSON API for Standard`: https://docs.iqrf.org/iqrf-gateway-daemon/api.html#iqrf-standard

Examples
--------

Give a go with the API examples in your favourite programming language

- `Bash`_
- `C#`_
- `Java`_
- `Python`_
- `JavaScript`_
- `NodeRed`_

.. _`Bash`: https://gitlab.iqrf.org/open-source/iqrf-gateway-daemon/tree/master/examples/bash
.. _`C#`: https://gitlab.iqrf.org/open-source/iqrf-gateway-daemon/tree/master/examples/c#
.. _`Java`: https://gitlab.iqrf.org/open-source/iqrf-gateway-daemon/tree/master/examples/java
.. _`Python`: https://gitlab.iqrf.org/open-source/iqrf-gateway-daemon/tree/master/examples/Python
.. _`JavaScript`: https://gitlab.iqrf.org/open-source/iqrf-gateway-daemon/tree/master/examples/nodejs
.. _`NodeRed`: https://gitlab.iqrf.org/open-source/iqrf-gateway-daemon/tree/master/examples/node-red
