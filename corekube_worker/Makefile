# Author: andrewferguson (adapted from Makefile by j0lama)

# COMPILER
CC = gcc

#FLAGS
CFLAGS = -c -Wall -g -DTHREAD_LOGS # -ansi -std=c99

#LIBRARY FLAGS
LIBFLAGS = $(LIB_BIN_DIRECTORY)*

#LINK FLAGS 
LINKFLAGS = -Wl,-rpath=$(LIB_BIN_DIRECTORY) -lsctp -lck

#INCLUDE FLAGS
INCLUDE_FLAGS = -I lib/core/include -I lib/ -I lib/s1ap/asn1c/ -I lib/nas -I lib/hss -I headers/

TARGETS = corekube_udp_worker corekube_sctp_worker udp_client

# PATHS
SOURCE_DIRECTORY = source/
LIB_BIN_DIRECTORY = bin/
OBJECT_DIRECTORY = objects/
BUILD_DIRECTORY = build/

CFILES = $(wildcard $(SOURCE_DIRECTORY)*.c)
OBJECTS = $(patsubst $(SOURCE_DIRECTORY)%.c, $(OBJECT_DIRECTORY)%.o, $(CFILES))

SCTP_WORKER_OBJECTS = $(filter-out %udp_listener.o %udp_client.o, $(OBJECTS))
UDP_WORKER_OBJECTS = $(filter-out %sctp_listener.o %udp_client.o, $(OBJECTS))
UDP_CLIENT_OBJECTS = $(filter %udp_client.o, $(OBJECTS))

all: $(OBJECT_DIRECTORY) $(TARGETS)

corekube_udp_worker: $(UDP_WORKER_OBJECTS)
	@echo "Building $@..."
	@$(CC) $(UDP_WORKER_OBJECTS) $(LIBFLAGS) $(LINKFLAGS) -o $@

corekube_sctp_worker: $(SCTP_WORKER_OBJECTS)
	@echo "Building $@..."
	@$(CC) $(SCTP_WORKER_OBJECTS) $(LIBFLAGS) $(LINKFLAGS) -o $@

udp_client: $(UDP_CLIENT_OBJECTS)
	@echo "Building $@..."
	@$(CC) $(UDP_CLIENT_OBJECTS) $(LIBFLAGS) $(LINKFLAGS) -o $@

$(OBJECT_DIRECTORY):
	@mkdir $(OBJECT_DIRECTORY)

$(OBJECT_DIRECTORY)%.o: $(SOURCE_DIRECTORY)%.c
	@echo "Building $@..."
	@$(CC) $(INCLUDE_FLAGS) $(CFLAGS) $< -o $@

.PHONY: clean
clean:
	@echo "Removing objects files"
	@rm -rf $(OBJECT_DIRECTORY) $(TARGETS)
