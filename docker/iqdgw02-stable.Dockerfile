# IQRF GW daemon image for the IQD-GW-01A board
# Edit config/* files and set accordingly for your target

FROM balenalib/orange-pi-zero-debian:latest

# copy custom config
WORKDIR /etc
COPY config/iqrf-gateway.json iqrf-gateway.json

RUN apt-get update \
 && apt-get install --no-install-recommends -y dirmngr gnupg2 \
 && apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 9C076FCC7AB8F2E43C2AB0E73241B9B7B4BD8F8E \
 && echo "deb http://repos.iqrf.org/debian buster stable" | tee /etc/apt/sources.list.d/iqrf.list \
 && apt-get update \
 && apt-get install --no-install-recommends -y iqrf-gateway-daemon \
 && apt-get clean \
 && rm -rf /var/lib/apt/lists/*

# copy custom config
WORKDIR /etc/iqrf-gateway-daemon
COPY config/iqrf-gateway-daemon/. .

# expose ports
EXPOSE 1338 1438 55000/udp 55300/udp

# run the daemon service
CMD ["iqrfgd2", "/etc/iqrf-gateway-daemon/config.json"]
