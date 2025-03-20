#!/bin/bash

# IQRF Gateway Daemon
# Copyright (C) 2015-2025 IQRF Tech s.r.o., 2019-2025 MICRORISC s.r.o.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

set -ex

TAG="latest"
TAGS=""
ARCHS=("amd64" "armel" "armhf" "arm64" "i386")
REPO="iqrftech/iqrf-gateway-daemon"

for ARCH in "${ARCHS[@]}"
do
   docker build --no-cache -f "${ARCH}.Dockerfile" -t "${REPO}:${TAG}-${ARCH}" .
   docker push "${REPO}:${TAG}-${ARCH}"
   TAGS="${TAGS} ${REPO}:${TAG}-${ARCH}"
done

export DOCKER_CLI_EXPERIMENTAL="enabled"
docker manifest create "${REPO}:${TAG}" ${TAGS}
docker manifest annotate "${REPO}:${TAG}" "${REPO}:${TAG}-armel" --os=linux --arch=arm --variant=v6
docker manifest push --purge "$REPO":"$TAG"
