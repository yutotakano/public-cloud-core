# Author: j0lama

LIBNAME = libck

# COMPILER
CC = gcc

#FLAGS
CFLAGS = -c -fPIC -g -Os -Wall -Wextra -pedantic

TARGET = $(LIBNAME).so

# PATHS
SOURCE_DIRECTORY = source/
INCLUDE_DIRECTORY = include/
OBJECT_DIRECTORY = objects/
BUILD_DIRECTORY = build/

CFILES = $(wildcard $(SOURCE_DIRECTORY)*.c)
OBJECTS = $(patsubst $(SOURCE_DIRECTORY)%.c, $(OBJECT_DIRECTORY)%.o, $(CFILES))


ifeq ($(PREFIX),)
    PREFIX := /usr/local/
endif

all: $(OBJECT_DIRECTORY) $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "Building static library..."
	gcc -shared $^ -o $@

$(OBJECT_DIRECTORY):
	@mkdir $(OBJECT_DIRECTORY)

$(OBJECT_DIRECTORY)%.o: $(SOURCE_DIRECTORY)%.c
	@echo "Building $@..."
	@$(CC) -I $(INCLUDE_DIRECTORY) $(CFLAGS) $< -o $@

.PHONY: install
install: $(TARGET)
	install -d /usr/lib/x86_64-linux-gnu/
	install -m 644 $(TARGET) /usr/lib/x86_64-linux-gnu/
	install -d $(PREFIX)/include/
	install -m 644 $(INCLUDE_DIRECTORY)$(LIBNAME).h $(PREFIX)/include/
	@echo "Done."

.PHONY: uninstall
uninstall:
	@rm -f /usr/lib/x86_64-linux-gnu/$(TARGET)
	@rm -f $(PREFIX)/include/$(LIBNAME).h
	@echo "Done."

.PHONY: clean
clean:
	@echo "Removing library objects files"
	@rm -rf $(OBJECT_DIRECTORY) $(TARGET)