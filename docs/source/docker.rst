******
Docker
******

Run IQRF Gateway in Docker environment in a few seconds. 

Repository
##########

.. code-block:: bash

  git clone https://gitlab.iqrf.org/open-source/iqrf-gateway-docker
  cd iqrf-gateway-docker

Start the gateway
+++++++++++++++++

.. code-block:: bash

  docker compose up -d

Stop the gateway
++++++++++++++++

.. code-block:: bash

  docker compose down

IQRF Gateway Webapp
###################

Point the browser to http://gw-ip:8080/ and explore.

IQRF Gateway Daemon
###################

Tools
+++++

.. code-block:: bash

  sudo apt-get install mosquitto-clients

.. code-block:: bash

  wget https://github.com/vi/websocat/releases/download/v1.1.0/websocat_1.1.0_amd64.deb
  wget https://github.com/vi/websocat/releases/download/v1.1.0/websocat_1.1.0_armhf.deb

  sudo dpkg -i websocat_1.1.0_*.deb
  sudo apt-get install -y jq
  rm -f websocat_1.1.0_*.deb

Examples
++++++++

.. code-block:: bash

  git clone https://gitlab.iqrf.org/open-source/iqrf-gateway-daemon
  cd iqrf-gateway-daemon/examples/bash
  ./mqtt-generic-blink.sh
  ./websocket-generic-blink.sh
