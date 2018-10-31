**********************************
How to install IQRF Gateway Daemon
**********************************

Add IQRF Gateway repository
###########################

`https://repos.iqrf.org/testing`_

-   iqrf-gateway-daemon_2.0.0*_amd64.deb
-   iqrf-gateway-daemon_2.0.0*_armhf.deb
-   iqrf-gateway-daemon_2.0.0*_arm64.deb

For Debian, UbiLinux, Raspbian, Armbian
---------------------------------------
.. code-block:: bash

	sudo apt-get install dirmngr apt-transport-https
	sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 9C076FCC7AB8F2E43C2AB0E73241B9B7B4BD8F8E
	echo "deb https://repos.iqrf.org/testing/debian stretch testing" | sudo tee -a /etc/apt/sources.list
	sudo apt-get update

For Ubuntu
----------
Currently Ubuntu is not supported.

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
	sudo systemctl daemon-reload
	sudo systemctl restart iqrf-gateway-daemon

or **update** if the daemon is already installed.

.. code-block:: bash

	sudo apt-get update
	sudo apt-get --only-upgrade install iqrf-gateway-daemon
	sudo systemctl daemon-reload
	sudo systemctl restart iqrf-gateway-daemon

Check the status of the daemon
##############################
.. code-block:: bash
	
	systemctl status iqrf-gateway-daemon.service

.. _`https://repos.iqrf.org/testing`: https://repos.iqrf.org/testing
