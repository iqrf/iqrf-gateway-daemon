# Copyright 2019 IQRF Tech s.r.o.
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

FROM iqrftech/package-builders:debian-buster-arm64

MAINTAINER Roman Ondráček <roman.ondracek@iqrf.com>
LABEL maintainer="roman.ondracek@iqrf.com"

ENV DEBIAN_FRONTEND noninteractive

RUN apt-get update \
  && apt-get install --no-install-recommends -y apt-transport-https dirmngr gnupg2 \
  && apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 9C076FCC7AB8F2E43C2AB0E73241B9B7B4BD8F8E \
  && echo "deb http://repos.iqrf.org/debian buster stable" | tee -a /etc/apt/sources.list \
  && apt-get update \
  && apt-get install --no-install-recommends -y googletest libcurl4-openssl-dev libgtest-dev \
     libpaho-mqtt-dev libsqlite3-dev mlocate ruby ruby-dev zlib1g-dev \
  && apt-get clean \
  && rm -rf /var/lib/apt/lists/*

RUN gem install --no-rdoc --no-ri fpm
