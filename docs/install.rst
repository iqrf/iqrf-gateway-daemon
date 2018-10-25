**********************************
How to install IQRF Gateway Daemon
**********************************

Add IQRF Gateway repository
###########################

`https://repos.iqrfsdk.org/testing`_

-   iqrf-gateway-daemon_2.0.0-x_amd64.deb
-   iqrf-gateway-daemon_2.0.0-x_armhf.deb
-   iqrf-gateway-daemon_2.0.0-x_arm64.deb

For Debian and UbiLinux
-----------------------
.. code-block:: bash

	sudo apt-get install dirmngr apt-transport-https
	sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 9C076FCC7AB8F2E43C2AB0E73241B9B7B4BD8F8E
	echo "deb https://repos.iqrfsdk.org/testing/debian stretch testing" | sudo tee -a /etc/apt/sources.list
	sudo apt-get update

For Ubuntu
----------
Currently Ubuntu is not supported.

Stop and disable IQRF Gateway Daemon v1
#######################################

If there is IQRF Gateway Daemon v1 already running in the system.

.. code-block:: bash

	sudo systemctl stop iqrf-daemon
	sudo systemctl disable iqrf-daemon

Install IQRF Gateway Daemon
###########################
.. code-block:: bash

	sudo apt-get install iqrf-gateway-daemon

or **update** if the daemon is already installed.

.. code-block:: bash

	sudo apt-get update
	sudo apt-get --only-upgrade install iqrf-gateway-daemon

Check the status of IQRF Gateway Daemon
#######################################
.. code-block:: bash
	
	systemctl status iqrf-gateway-daemon.service

.. _`https://repos.iqrfsdk.org/testing`: https://repos.iqrfsdk.org/testing
