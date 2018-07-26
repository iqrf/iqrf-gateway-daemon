# How to install the gateway-daemon

[http://repos.iqrfsdk.org/](http://repos.iqrfsdk.org/)

-   iqrf-gateway-daemon_2.0.0-x_amd64.deb
-   iqrf-gateway-daemon_2.0.0-x_armhf.deb
-   iqrf-gateway-daemon_2.0.0-x_arm64.deb

## Download public key to verify the packages from the repository

```Bash
sudo apt-get install dirmngr
sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 9C076FCC7AB8F2E43C2AB0E73241B9B7B4BD8F8E
```

## Add repository to the source list

-	For Debian (amd64, armhf, arm64)

```Bash
echo "deb http://repos.iqrfsdk.org/debian stretch testing" | sudo tee -a /etc/apt/sources.list
sudo apt-get update
```

-	For Ubuntu (amd64, armhf, arm64)

```Bash
echo "deb http://repos.iqrfsdk.org/ubuntu xenial testing" | sudo tee -a /etc/apt/sources.list
sudo apt-get update
```

## Stop and disable daemon v1

```Bash
sudo systemctl stop iqrf-daemon.service
sudo systemctl disable iqrf-daemon.service
```

## Install or update the daemon

```Bash
sudo apt-get install iqrf-gateway-daemon
```
or

```Bash
sudo apt-get update
sudo apt-get install --only-upgrade iqrf-gateway-daemon
```

## Check the daemon service status

```Bash
systemctl status iqrfgd2.service

● iqrfgd2.service - IQRF daemon iqrfgd2
   Loaded: loaded (/lib/systemd/system/iqrfgd2.service; enabled; vendor preset: enabled)
   Active: active (running) since Čt 2017-03-30 23:46:50 CEST; 2min 58s ago
 Main PID: 9540 (iqrfgd2)
   CGroup: /system.slice/iqrfgd2.service
           └─9540 /usr/bin/iqrfgd2 /etc/iqrfgd2/config.json
```

## Set your configuration 

CK-USB-04A or GW-USB-06 devices must be switched to USB CDC IQRF mode using IDE
menu: Tools/USB Classes/Switch to CDC IQRF mode

Configure/check mainly following components in folder /etc/iqrfgd2: 

- config.json           (tip: enable/disable daemon components)
- IqrfInterface.json    (tip: configure your IQRF interface - SPI/CDC)
- MqttMessaging.json    (tip: configure your MQTT brokers - you can enable second mqtt instance)
- Scheduler.json        (tip: configure your regural DPA tasks if any)
- ...

and restart the service:

```Bash
sudo systemctl restart iqrfgd2.service
```

## Content of iqrf-gateway-daemon package

```Bash
dpkg -L iqrf-gateway-daemon
```
