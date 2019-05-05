# IQRF Gateway Daemon v2 builder

Docker build image for IQRF Gateway Daemon v2 CI

## Docker Hub link

https://hub.docker.com/r/iqrftech/iqrf-gateway-daemon-builder

## Manual build

```Bash
docker build -f dockerfile.stretch.amd64 -t iqrf-gateway-daemon-builder .
```

## Cross build

```Bash
docker build -f dockerfile.stretch.armhf -t iqrf-gateway-daemon-crossbuilder .
```

## Run cross image

```Bash
sudo apt-get install binfmt-support qemu-user-static
docker run --rm --privileged multiarch/qemu-user-static:register --reset
docker run -it --rm iqrf-gateway-daemon-crossbuilder:latest /bin/sh
uname -a
```
