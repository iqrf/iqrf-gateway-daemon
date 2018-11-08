# IQRF Gateway Daemon v2 builder

Docker build image for IQRF Gateway Daemon v2 CI

## Docker Hub link

https://hub.docker.com/r/iqrfsdk/iqrf-gateway-daemon-build

## Manual build

```Bash
docker build -f Dockerfile.Stretch.amd64 -t iqrf-gateway-daemon-build .
```

## Cross build

```Bash
docker build -f Dockerfile.Stretch.armhf -t iqrf-gateway-daemon-crossbuild .
```

## Run cross image

```Bash
sudo apt-get install binfmt-support qemu-user-static
docker run --rm --privileged multiarch/qemu-user-static:register --reset
docker run -it --rm iqrf-gateway-daemon-crossbuild:latest /bin/sh
uname -a
```
