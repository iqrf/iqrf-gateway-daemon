*************************
How to install the daemon
*************************

Add IQRF Gateway repository
###########################

Stable
------

`https://repos.iqrf.org`_

-   iqrf-gateway-daemon_2.0.0_amd64.deb
-   iqrf-gateway-daemon_2.0.0_armhf.deb
-   iqrf-gateway-daemon_2.0.0_arm64.deb

Testing (Beta)
--------------

`https://repos.iqrf.org/testing`_

-   iqrf-gateway-daemon_2.1.0-*_amd64.deb
-   iqrf-gateway-daemon_2.1.0-*_armhf.deb
-   iqrf-gateway-daemon_2.1.0-*_arm64.deb

For Debian, UbiLinux, Raspbian, Armbian
---------------------------------------

Stretch 9
+++++++++
- Stable

.. code-block:: bash

	sudo apt-get install dirmngr apt-transport-https
	sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 9C076FCC7AB8F2E43C2AB0E73241B9B7B4BD8F8E
	echo "deb https://repos.iqrf.org/debian stretch stable" | sudo tee -a /etc/apt/sources.list
	sudo apt-get update

- Testing

.. code-block:: bash

	sudo apt-get install dirmngr apt-transport-https
	sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 9C076FCC7AB8F2E43C2AB0E73241B9B7B4BD8F8E
	echo "deb https://repos.iqrf.org/testing/debian stretch testing" | sudo tee -a /etc/apt/sources.list
	sudo apt-get update

For Ubuntu
----------

Xenial 16.04
++++++++++++
- Stable

.. code-block:: bash

	sudo apt-get install dirmngr apt-transport-https
	sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 9C076FCC7AB8F2E43C2AB0E73241B9B7B4BD8F8E
	echo "deb https://repos.iqrf.org/ubuntu/xenial xenial stable" | sudo tee -a /etc/apt/sources.list
	sudo apt-get update

- Testing

.. code-block:: bash

	sudo apt-get install dirmngr apt-transport-https
	sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 9C076FCC7AB8F2E43C2AB0E73241B9B7B4BD8F8E
	echo "deb https://repos.iqrf.org/testing/ubuntu/xenial xenial testing" | sudo tee -a /etc/apt/sources.list
	sudo apt-get update

Bionic 18.04
++++++++++++
- Stable

.. code-block:: bash

	sudo apt-get install dirmngr apt-transport-https
	sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 9C076FCC7AB8F2E43C2AB0E73241B9B7B4BD8F8E
	echo "deb https://repos.iqrf.org/ubuntu/bionic bionic stable" | sudo tee -a /etc/apt/sources.list
	sudo apt-get update

- Testing

.. code-block:: bash

	sudo apt-get install dirmngr apt-transport-https
	sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 9C076FCC7AB8F2E43C2AB0E73241B9B7B4BD8F8E
	echo "deb https://repos.iqrf.org/testing/ubuntu/bionic bionic testing" | sudo tee -a /etc/apt/sources.list
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

Update from beta release
------------------------

.. code-block:: bash

	sudo apt-get purge iqrf-gateway-daemon
	sudo apt-get install iqrf-gateway-daemon

Check the status of the daemon
##############################
.. code-block:: bash
	
	systemctl status iqrf-gateway-daemon.service

Direct links
############

Packages and tarballs for download.

- https://dl.iqrf.org/iqrf-gateway-daemon/stretch
- https://dl.iqrf.org/iqrf-gateway-daemon/bionic
- https://dl.iqrf.org/iqrf-gateway-daemon/xenial

.. _`https://repos.iqrf.org`: https://repos.iqrf.org
.. _`https://repos.iqrf.org/testing`: https://repos.iqrf.org/testing
