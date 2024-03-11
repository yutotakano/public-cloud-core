# Comment: Image base with corekube frontend installed
# Based upon ran-emulator/kubernetes/docker_slave/Dockerfile by github.com/j0lama

# Download ubuntu from the Docker Hub
FROM ubuntu:focal

# Install dependencies
RUN apt-get update
RUN apt-get -y install git libsctp-dev build-essential tcpdump

# Install dig to resolve DNS for worker (use DEBIAN_FRONTEND to avoid the
# interactive prompt that pops up despite the -y flag)
RUN DEBIAN_FRONTEND=noninteractive apt-get -y install dnsutils

# Extra dependencies for testing / debugging
RUN apt-get -y install python3 curl wget netcat screen libsctp1 lksctp-tools python3-pip nano

# Install the frontend
COPY ./ corekube_frontend/
WORKDIR corekube_frontend/
RUN make
