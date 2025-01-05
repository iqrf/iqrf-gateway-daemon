# Copyright 2015-2025 IQRF Tech s.r.o.
# Copyright 2019-2025 MICRORISC s.r.o.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

FROM amd64/debian:bookworm

LABEL maintainer="roman.ondracek@iqrf.com"

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
 && apt-get install --no-install-recommends -y dirmngr gnupg2 curl openssl \
 && curl -sSLo /usr/share/keyrings/iqrf.gpg http://repos.iqrf.org/apt.gpg \
 && echo "deb [signed-by=/usr/share/keyrings/iqrf.gpg] http://repos.iqrf.org/debian bookworm stable" | tee /etc/apt/sources.list.d/iqrf.list \
 && apt-get update \
 && apt-get install --no-install-recommends -y iqrf-gateway-daemon \
 && apt-get clean \
 && rm -rf /var/lib/apt/lists/* /etc/iqrf-gateway-daemon/certs/core/*

EXPOSE 1338 1438 55000/udp 55300/udp

COPY run.sh run.sh
CMD ["bash", "run.sh"]
