# Comment: Image base with ran-emulator installed

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

# Install ran-emulator
# TODO
COPY ran-emulator/ ran-emulator/
WORKDIR ran-emulator/
RUN make

# Install tools to generate traffic
RUN apt-get -y install iperf iperf3 net-tools iputils-ping d-itg
