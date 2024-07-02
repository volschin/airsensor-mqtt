FROM gcc:14.1@sha256:3ceef1fb3ef2833590659f3cc7c857cdf0cf2c821ee0f62fa9a294bb4895ce2c AS builder
ENV DEBIAN_FRONTEND noninteractive
ENV TERM xterm

# Install base environment
RUN apt-get update \
  && apt-get install -qqy --no-install-recommends apt-utils ca-certificates \
  apt-transport-https \
#  build-essential gcc make cmake cmake-gui cmake-curses-gui \
  libusb-dev libpaho-mqtt-dev

COPY airsensor.c /airsensor.c
# for ssl support -lpaho-mqtt3cs, without -lpaho-mqtt3c
RUN gcc -static -o airsensor airsensor.c -lusb -lpaho-mqtt3c -lpthread

FROM scratch
COPY --from=builder /airsensor /airsensor
ENTRYPOINT ["/airsensor", "-v"]
