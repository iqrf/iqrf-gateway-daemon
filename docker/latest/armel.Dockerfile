FROM arm32v5/debian:bullseye

LABEL maintainer="roman.ondracek@iqrf.com"

ENV DEBIAN_FRONTEND noninteractive

RUN apt-get update \
 && apt-get install --no-install-recommends -y dirmngr gnupg2 \
 && apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 9C076FCC7AB8F2E43C2AB0E73241B9B7B4BD8F8E \
 && echo "deb http://repos.iqrf.org/debian bullseye stable" | tee /etc/apt/sources.list.d/iqrf.list \
 && apt-get update \
 && apt-get install --no-install-recommends -y iqrf-gateway-daemon openssl \
 && apt-get clean \
 && rm -rf /var/lib/apt/lists/* /etc/iqrf-gateway-daemon/certs/core/*

EXPOSE 1338 1438 55000/udp 55300/udp

COPY run.sh run.sh
CMD ["bash", "run.sh"]
