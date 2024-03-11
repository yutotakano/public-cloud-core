# Build CPP
FROM ubuntu:latest

RUN apt-get update
RUN apt-get -y install build-essential git cmake

RUN mkdir ~/.ssh
RUN ssh-keyscan -t rsa github.com >> ~/.ssh/known_hosts

RUN mkdir /core-metrics-bus
WORKDIR /core-metrics-bus
COPY ./source /core-metrics-bus/source
COPY ./include /core-metrics-bus/include
COPY ./Makefile /core-metrics-bus/Makefile
RUN make

CMD ["./core-metrics-bus"]
