CC := g++
CPPFLAGS := -Iinclude -Iexternal_include
CFLAGS := -Wall
LDFLAGS := -Llib
LDLIBS := -lpthread

SRC_DIR := source
OBJ_DIR := objects

EXE := main

SRC_LIST := $(wildcard $(SRC_DIR)/*.cpp)
OBJ_LIST := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRC_LIST))

all: $(EXE)

 # Prevents make from doing something with a file with the same name as the target
.PHONY: all clean

# Compiling the source files, $< is the first prerequisite, $@ is the target
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
		$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
		mkdir -p $@

# Linking the object files, $^ is the list of all prerequisites
$(EXE): $(OBJ_LIST)
		$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

clean:
		@rm -rf $(OBJ_DIR)
