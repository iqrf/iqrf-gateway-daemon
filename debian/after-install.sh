#!/bin/bash
# After install script for iqrf-gateway-daemon

NAME=iqrfgd2

daemon_chmod_dirs() {
    echo "Changing daemon dirs"
    chmod -R 666 /etc/${NAME}/
    chmod 777 /etc/${NAME}/
    chmod 777 /etc/${NAME}/cfgSchemas/
    chmod -R 666 /var/cache/${NAME}/scheduler/
    chmod 777 /var/cache/${NAME}/scheduler/
}

daemon_enable-restart() {
    which systemctl &>/dev/null
    if [[ $? -eq 0 ]]; then
        systemctl daemon-reload
        systemctl enable ${NAME}
        systemctl restart ${NAME}
    else
        echo "Init.d not supported."
    fi
    
    # for paho libs
    ldconfig
}

if [ "$1" = "configure" ]; then
    daemon_chmod_dirs
    daemon_enable-restart
fi
