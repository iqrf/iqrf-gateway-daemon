# Docker image for IQRF Gateway Daemon

# Build it

```bash
docker build -f amd64-latest.Dockerfile -t iqrftech/iqrf-gateway-daemon:latest-amd64 .
docker push iqrftech/iqrf-gateway-daemon:latest-amd64
```

# Run it

## Dedicated network for your containers

```bash
docker network create --subnet 10.1.0.0/16 --gateway 10.1.0.1 --ip-range=10.1.1.0/24
--driver=bridge --label=host1network bridge01
```

## MQTT broker

```bash
docker container run -d --name mqtt1broker -p 1883:1883 -p 9001:9001 --network=bridge01
--ip=10.1.1.1 --restart=always eclipse-mosquitto
```

## IQRF GW daemon

```bash
docker container run -d --name iqrf-gateway-daemon -p 1338:1338 -p 55000:55000/udp -p 55300:55300/udp --device /dev/spidev0.0:/dev/spidev0.0 --privileged --restart=always iqrftech/iqrf-gateway-daemon:latest-amd64
```
