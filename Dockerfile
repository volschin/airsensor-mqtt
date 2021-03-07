FROM debian:bullseye-slim AS builder
ENV DEBIAN_FRONTEND noninteractive
ENV TERM xterm

# Install base environment
RUN apt-get update \
  && apt-get install -qqy --no-install-recommends apt-utils \
  apt-transport-https \
  build-essential gcc make cmake cmake-gui cmake-curses-gui \
  libusb-dev libpaho-mqtt-dev \
  && apt-get autoremove -y && apt-get clean \
  && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

COPY airsensor.c /airsensor.c
# for ssl support -lpaho-mqtt3cs, without -lpaho-mqtt3c
RUN gcc -static -o airsensor airsensor.c -lusb -lpaho-mqtt3c -lpthread

FROM debian:bullseye-slim
ENV DEBIAN_FRONTEND noninteractive
ENV TERM xterm

# Install base environment
RUN apt-get update \
  && apt-get install -qqy --no-install-recommends apt-utils \
  apt-transport-https \
  netcat \
  && apt-get autoremove -y && apt-get clean \
  && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*
COPY --from=builder /airsensor /airsensor


#HEALTHCHECK --interval=30s --timeout=10s --start-period=10s --retries=3 CMD nc -z localhost 5111
#CMD bash 
ENTRYPOINT ["/airsensor", "-v"]
