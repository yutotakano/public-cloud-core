# Comment: Image base with ran_emulator installed

# Download ubuntu from the Docker Hub
FROM ubuntu:bionic

# Install dependencies
RUN apt-get update
RUN apt-get -y install git libsctp-dev build-essential libssl-dev python3 python3-pip net-tools iputils-ping nano

RUN pip3 install --upgrade pip
# Install Python dependencies
RUN pip3 install MarkupSafe==2.0.0
RUN pip3 install Flask
RUN pip3 install kubernetes
# Install engineio and socketio manually to avoid version errors
RUN pip3 install python-engineio==3.13.2
RUN pip3 install python-socketio==4.6.0
RUN pip3 install Flask-SocketIO==4.3.1

# Install GarbageCollector
RUN git clone https://github.com/j0lama/GarbageCollector.git
WORKDIR GarbageCollector/
RUN make
RUN make install

WORKDIR ../

# Install ran_emulator
COPY ran_emulator/ ran_emulator/
WORKDIR ran_emulator/
RUN make

ENTRYPOINT [ "python3" ]

CMD [ "ran_controller/ran_controller.py" ]
