# Author: j0lama

# COMPILER
CC = gcc

#FLAGS
CFLAGS = -c -Wall -ansi -g -std=c99 -D_DEBUG

#LIB FLAGS
LIBFLAGS = -lgc -rdynamic -lcrypto -lpthread

#LINK FLAGS
LINKFLAGS = -lsctp

TARGET = 

# PATHS
SOURCE_DIRECTORY = source/
INCLUDE_DIRECTORY = headers/
OBJECT_DIRECTORY = objects/
BUILD_DIRECTORY = build/
COMMON_SRC_DIRECTORY = source/common/
COMMON_HDR_DIRECTORY = headers/common/

CFILES = $(wildcard $(SOURCE_DIRECTORY)*.c)
COMMON_CFILES = $(wildcard $(COMMON_SRC_DIRECTORY)*.c)
OBJECTS = $(patsubst $(SOURCE_DIRECTORY)%.c, $(OBJECT_DIRECTORY)%.o, $(CFILES))
OBJECTS += $(patsubst $(COMMON_SRC_DIRECTORY)%.c, $(OBJECT_DIRECTORY)%.o, $(COMMON_CFILES))

ue: 
	SOURCE_DIRECTORY=source/ue/
	INCLUDE_DIRECTORY=headers/ue/
	TARGET=ue_emulator
ue: all

enb: 
	SOURCE_DIRECTORY=source/enb/
	INCLUDE_DIRECTORY=headers/enb/
	TARGET=enb_emulator
enb: all

all: $(OBJECT_DIRECTORY) $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "Building $@..."
	@$(CC) $(OBJECTS) $(LIBFLAGS) $(LINKFLAGS) -o $@

$(OBJECT_DIRECTORY):
	@mkdir $(OBJECT_DIRECTORY)
	@echo "Building objects directory..."

$(OBJECT_DIRECTORY)%.o: $(SOURCE_DIRECTORY)%.c $(COMMON_SRC_DIRECTORY)%.c
	@echo "Building $@..."
	@$(CC) -I $(COMMON_HDR_DIRECTORY) -I $(INCLUDE_DIRECTORY) $(CFLAGS) $(LIBFLAGS) $< -o $@

.PHONY: clean
clean:
	@echo "Removing objects files"
	@rm -rf $(OBJECT_DIRECTORY) $(TARGET)
