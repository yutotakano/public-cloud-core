# Comment: Image base with corekube DB installed
# Based upon ran-emulator/kubernetes/docker_slave/Dockerfile by github.com/j0lama

# Download ubuntu from the Docker Hub
FROM ubuntu:focal

# Fix the interactive tzdata prompt
ENV TZ=Europe/London
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# Install dependencies
RUN apt-get update
RUN apt-get -y install build-essential libconfig-dev

# Extra dependencies for testing / debugging
RUN apt-get -y install python3 curl wget netcat screen python3-pip nano

# Install the frontend
COPY ./ corekube-db/
WORKDIR corekube-db/
RUN make
