FROM gcc:14.1@sha256:cddde55e71e1d5fda7cc08129ee215954b2b887bc873f0d24477945e718f0b07 AS builder
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
