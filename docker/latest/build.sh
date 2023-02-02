#!/bin/bash

# Copyright 2015-2023 IQRF Tech s.r.o.
# Copyright 2019-2023 MICRORISC s.r.o.
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
