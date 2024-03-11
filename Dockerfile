# Comment: Image base with corekube 5G worker installed
# Based upon corekube-worker/Dockerfile

# Download ubuntu from the Docker Hub
FROM ubuntu:bionic as build

# Setup Timezone file
ENV TZ=Europe/London
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# Install dependencies
RUN apt-get update
RUN apt-get -y install git libsctp-dev build-essential

# Build libck
RUN git clone https://github.com/j0lama/libck.git
WORKDIR libck/
RUN make
RUN make install # installs headers for compilation

WORKDIR ../

# Build the Open5GS libraries:
RUN git clone https://github.com/andrewferguson/open5gs-corekube.git
RUN apt-get -y install python3-pip python3-setuptools python3-wheel ninja-build build-essential flex bison git libsctp-dev libgnutls28-dev libgcrypt-dev libssl-dev libidn11-dev libmongoc-dev libbson-dev libyaml-dev libnghttp2-dev libmicrohttpd-dev libcurl4-gnutls-dev libnghttp2-dev libtins-dev meson
WORKDIR open5gs-corekube
RUN meson build --prefix=`pwd`/install && \
    ninja -C build

WORKDIR ../

# Copy the corekube_worker
COPY ./ corekube_worker/

# Copy the libraries to the worker directory
RUN mkdir corekube_worker/bin
RUN LIBS=`find open5gs-corekube/build/lib/ -type f -name "libogs*.so.2.3.0"` && \
    for lib in $LIBS; do mv $lib corekube_worker/bin/$(basename "$lib" .3.0) ; done
RUN cp libck/libck.so corekube_worker/bin/

# Build corekube_worker
WORKDIR corekube_worker/
RUN make



FROM busybox:glibc

COPY --from=build /corekube_worker/corekube_udp_worker /
COPY --from=build /corekube_worker/bin /bin/