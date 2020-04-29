# Comment: Image base with ran_emulator installed

# Download ubuntu from the Docker Hub
FROM ubuntu:bionic

RUN apt update
RUN apt -y install git libsctp-dev

RUN git clone https://github.com/j0lama/GarbageCollector.git
WORKDIR GarbageCollector/