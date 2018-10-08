#!/bin/bash
# After install script for iqrf-gateway-daemon

NAME=iqrf-gateway-daemon
OLD=iqrfgd2

daemon_chmod_dirs() {
    chmod -R 666 /etc/${NAME}/
    chmod 777 /etc/${NAME}/
    chmod 777 /etc/${NAME}/cfgSchemas/
    chmod -R 666 /var/cache/${NAME}/scheduler/
    chmod 777 /var/cache/${NAME}/scheduler/

    # CLEAN
    if [ -d "/etc/${OLD}" ]; then
        rm -rf /etc/${OLD}
    fi

    if [ -d "/var/cache/${OLD}" ]; then
        rm -rf /var/cache/${OLD}
    fi

    if [ -d "/usr/lib/${OLD}" ]; then
        rm -rf /usr/lib/${OLD}
    fi

    if [ -d "/usr/share/${OLD}" ]; then
        rm -rf /usr/share/${OLD}
    fi

    if [ -f "/lib/systemd/system/${OLD}.service" ]; then
        rm -f /lib/systemd/system/${OLD}.service
    fi
}

daemon_enable-restart() {
    if [ -d /run/systemd/system ]; then
        systemctl --system daemon-reload
        deb-systemd-invoke enable ${NAME}.service
        deb-systemd-invoke restart ${NAME}.service
    fi

    # PAHO
    ldconfig
}

if [ "$1" = "configure" ]; then
    daemon_chmod_dirs
    daemon_enable-restart
fi
