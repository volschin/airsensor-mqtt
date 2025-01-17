FROM gcc:14.2@sha256:c8a47caa027efeb4254ecacc4a5b7f417625af0c4f48403c3a9b8dafb91da6c4 AS builder
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
