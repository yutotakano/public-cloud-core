# Build CPP
FROM ubuntu:latest

RUN apt-get update
RUN apt-get -y install build-essential git cmake

RUN mkdir ~/.ssh
RUN ssh-keyscan -t rsa github.com >> ~/.ssh/known_hosts

RUN mkdir /core-metrics-aggregator
WORKDIR /core-metrics-aggregator
COPY ./source /core-metrics-aggregator/source
COPY ./include /core-metrics-aggregator/include
COPY ./Makefile /core-metrics-aggregator/Makefile
RUN make

CMD ["./core-metrics-aggregator"]
