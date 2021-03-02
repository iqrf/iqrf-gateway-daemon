#!/bin/bash

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
