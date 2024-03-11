# Comment: Image base with corekube frontend installed
# Based upon ran_emulator/kubernetes/docker_slave/Dockerfile by github.com/j0lama

# Download ubuntu from the Docker Hub
FROM ubuntu:focal

# Install dependencies
RUN apt-get update
RUN apt-get -y install git libsctp-dev build-essential tcpdump

# Extra dependencies for testing / debugging
RUN apt-get -y install python3 curl wget netcat screen libsctp1 lksctp-tools python3-pip nano

# Install the frontend
COPY ./ corekube_frontend/
WORKDIR corekube_frontend/
RUN make
