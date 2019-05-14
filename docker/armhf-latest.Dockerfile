# IQRF GW daemon image for the RPI board
# Edit config/* files and set accordingly for your target

FROM multiarch/debian-debootstrap:armhf-stretch

MAINTAINER Rostislav Spinar <rostislav.spinar@iqrf.com>
LABEL maintainer="rostislav.spinar@iqrf.com"

# add iqrf repo
RUN apt-get update \
 && apt-get install --no-install-recommends -y apt-transport-https dirmngr gnupg2 \
 && apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 9C076FCC7AB8F2E43C2AB0E73241B9B7B4BD8F8E \
 && echo "deb http://repos.iqrf.org/testing/debian stretch testing" | tee -a /etc/apt/sources.list

# install daemon
RUN apt-get update \
 && apt-get install --no-install-recommends -y iqrf-gateway-daemon \
 && apt-get clean \
 && rm -rf /var/lib/apt/lists/*

# copy custom config
WORKDIR /etc/iqrf-gateway-daemon
COPY config-rpi/iqrf-gateway-daemon/. .

# expose ports
EXPOSE 1338 55000/udp 55300/udp

# run the daemon service
CMD ["iqrfgd2", "/etc/iqrf-gateway-daemon/config.json"]
