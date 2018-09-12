#!/bin/bash
# After install script for iqrf-gateway-daemon

NAME=iqrfgd2

daemon_chmod_dirs() {
    chmod -R 666 /etc/${NAME}/
    chmod 777 /etc/${NAME}/
    chmod 777 /etc/${NAME}/cfgSchemas/
    chmod -R 666 /var/cache/${NAME}/scheduler/
    chmod 777 /var/cache/${NAME}/scheduler/
}

daemon_enable-restart() {
    if [ -d /run/systemd/system ]; then
        systemctl --system daemon-reload
        deb-systemd-invoke enable ${NAME}.service
        deb-systemd-invoke restart ${NAME}.service
    fi
   
    # for paho libs
    ldconfig
}

if [ "$1" = "configure" ]; then
    daemon_chmod_dirs
    daemon_enable-restart
fi
