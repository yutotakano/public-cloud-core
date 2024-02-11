CC := g++
CPPFLAGS := -Iinclude -Iexternal/quill_install/include
CFLAGS := -Wall
LDFLAGS := -Lexternal/quill_install/lib
LDLIBS := -lpthread -lquill

SRC_DIR := source
OBJ_DIR := objects

EXE := main

SRC_LIST := $(wildcard $(SRC_DIR)/*.cpp)
OBJ_LIST := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRC_LIST))

all: external $(EXE)

 # Prevents make from doing something with a file with the same name as the target
.PHONY: all clean clean_all

# Compiling the source files, $< is the first prerequisite, $@ is the target
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
		$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# Create the objects directory
$(OBJ_DIR):
		@mkdir -p $@

# Linking the object files, $^ is the list of all prerequisites
$(EXE): $(OBJ_LIST)
		$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

# If dependencies are needed, install them
external: external/quill_install

# If quill isn't present as an installed library, clone and install it
external/quill_install:
		@test -d external || mkdir -p external
		cd external && git clone https://github.com/odygrd/quill.git
		@mkdir -p external/quill/cmake_build
		# The install prefix is set to the external/quill_install dir -- this will
		# install the library to external/quill_install/lib and the headers to
		# external/quill_install/include
		cd external/quill/cmake_build && cmake -DCMAKE_INSTALL_PREFIX=../../quill_install/ ..
		cd external/quill/cmake_build && make install

clean:
		@rm -rf $(OBJ_DIR)

clean_all: clean
		@rm -rf external
		@rm -f $(EXE)
