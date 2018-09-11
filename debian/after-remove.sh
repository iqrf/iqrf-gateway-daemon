#!/bin/bash
# After remove script for iqrf-gateway-daemon

NAME=iqrfgd2

daemon_stop-remove() {
    systemctl stop ${NAME}
    systemctl disable ${NAME}
    rm -f /lib/systemd/system/${NAME}.service
}

daemon_remove_config() {
    if [ -d /etc/${NAME}/ ] ; then
        rm -rf /etc/${NAME}/
    fi
}

daemon_remove_data() {
    if [ -d /usr/share/${NAME}/ ] ; then
        rm -rf /usr/share/${NAME}/
    fi

    if [ -d /var/cache/${NAME}/ ] ; then
        rm -rf /var/cache/${NAME}/
    fi
}

if [ "$1" = "remove" ] || [ "$1" = "purge" ]; then
    daemon_stop-remove
    daemon_remove_data
    daemon_remove_config
fi
