# Comment: Image base with ran_emulator installed

# Download ubuntu from the Docker Hub
FROM ubuntu:bionic

# Install dependencies
RUN apt-get update
RUN apt-get -y install git libsctp-dev build-essential libssl-dev python3 python3-pip

# Install Python dependencies
RUN pip3 install Flask
RUN pip3 install kubernetes

# Install GarbageCollector
RUN git clone https://github.com/j0lama/GarbageCollector.git
WORKDIR GarbageCollector/
RUN make
RUN make install

WORKDIR ../

# Install ran_emulator
# TODO
COPY ran_emulator/ ran_emulator/
WORKDIR ran_emulator/
RUN make

# Install tools to generate traffic
RUN apt-get -y install iperf net-tools iputils-ping d-itg
