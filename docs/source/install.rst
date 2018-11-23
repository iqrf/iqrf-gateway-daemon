*************************
How to install the daemon
*************************

Add IQRF Gateway repository
###########################

`https://repos.iqrf.org`_

-   iqrf-gateway-daemon_2.0.0_amd64.deb
-   iqrf-gateway-daemon_2.0.0_armhf.deb
-   iqrf-gateway-daemon_2.0.0_arm64.deb

For Debian, UbiLinux, Raspbian, Armbian
---------------------------------------

Stretch 9
+++++++++
.. code-block:: bash

	sudo apt-get install dirmngr apt-transport-https
	sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 9C076FCC7AB8F2E43C2AB0E73241B9B7B4BD8F8E
	echo "deb https://repos.iqrf.org/debian stretch stable" | sudo tee -a /etc/apt/sources.list
	sudo apt-get update

For Ubuntu
----------

Xenial 16.04
++++++++++++
.. code-block:: bash

	sudo apt-get install dirmngr apt-transport-https
	sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 9C076FCC7AB8F2E43C2AB0E73241B9B7B4BD8F8E
	echo "deb https://repos.iqrf.org/ubuntu/xenial xenial stable" | sudo tee -a /etc/apt/sources.list
	sudo apt-get update

Bionic 18.04
++++++++++++
.. code-block:: bash

	sudo apt-get install dirmngr apt-transport-https
	sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 9C076FCC7AB8F2E43C2AB0E73241B9B7B4BD8F8E
	echo "deb https://repos.iqrf.org/ubuntu/bionic bionic stable" | sudo tee -a /etc/apt/sources.list
	sudo apt-get update

Stop and disable the daemon v1
##############################

If there is IQRF Gateway Daemon v1 already running in the system.

.. code-block:: bash

	sudo systemctl stop iqrf-daemon
	sudo systemctl disable iqrf-daemon

Install the daemon
##################
.. code-block:: bash

	sudo apt-get install iqrf-gateway-daemon

or **update** if the daemon is already installed.

.. code-block:: bash

	sudo apt-get update
	sudo apt-get --only-upgrade install iqrf-gateway-daemon

Check the status of the daemon
##############################
.. code-block:: bash
	
	systemctl status iqrf-gateway-daemon.service

Direct link
###########

Packages and tarballs for download.

Stretch
-------

- https://dl.iqrf.org/iqrf-gateway-daemon/amd64/stretch/stable/
- https://dl.iqrf.org/iqrf-gateway-daemon/armhf/stretch/stable/
- https://dl.iqrf.org/iqrf-gateway-daemon/arm64/stretch/stable/

Bionic
------

- http://dl.iqrf.org/iqrf-gateway-daemon/amd64/bionic/stable/

Xenial
------

- http://dl.iqrf.org/iqrf-gateway-daemon/amd64/xenial/stable/

.. _`https://repos.iqrf.org`: https://repos.iqrf.org
