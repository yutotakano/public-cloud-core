CC := g++
CPPFLAGS := -Iinclude -Iexternal/prometheus_install/include
CFLAGS := -Wall
LDFLAGS := -Lexternal/prometheus_install/lib
LDLIBS := -lpthread -lprometheus-cpp-pull -lprometheus-cpp-core

ifeq ($(OS), Windows_NT)
		LDLIBS += -lws2_32
endif

SRC_DIR := source
OBJ_DIR := objects

# Recursive wildcard (cross-platform alternative to using shell find)
rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

SRC_LIST := $(call rwildcard, $(SRC_DIR), *.cpp)
OBJ_LIST := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRC_LIST))

ifeq ($(OS), Windows_NT)
		EXE := core-metrics-bus.exe
else
		EXE := core-metrics-bus
endif

# Cross-platform command-line tools, use them like so: $(call RM, file)
ifeq ($(OS), Windows_NT)
		RM = del /Q $(1) > nul 2>&1 || (exit 0)
		RMDIR = rmdir /S /Q $(subst /,\,$(1)) > nul 2>&1 || (exit 0)
else
		RM = rm -f $(1)
		RMDIR = rm -rf $(1)
endif

ifeq ($(OS), Windows_NT)
		MKDIR = mkdir $(subst /,\,$(1)) 2> NUL || (exit 0)
else
		MKDIR = mkdir -p $(1)
endif

ifeq ($(OS), Windows_NT)
		CP = xcopy /E /I /Y /Q $(subst /,\,$(1)) $(subst /,\,$(2))
else
		CP = cp -r $(1) $(2)
endif

ifeq ($(OS), Windows_NT)
		IF_NOT_EXISTS = if not exist $(1) $(2)
else
		IF_NOT_EXISTS = test -e $(1) || $(2)
endif

all: external $(EXE)


 # Prevents make from doing something with a file with the same name as the target
.PHONY: all clean clean_all

# Compiling the source files, $< is the first prerequisite, $@ is the target.
# Create parent subdirectories first if necessary since the compile can't.
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
		@$(call MKDIR, $(dir $@))
		$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# Create the objects directory
$(OBJ_DIR):
		@$(call MKDIR, $@)

# Linking the object files, $^ is the list of all prerequisites
$(EXE): $(OBJ_LIST)
		$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

# If dependencies are needed, install them
external: external/prometheus_install

# If quill isn't present as an installed library, clone and install it
external/prometheus_install:
		@$(call MKDIR, external)
		@$(call IF_NOT_EXISTS, external/prometheus-cpp, cd external && git clone https://github.com/jupp0r/prometheus-cpp.git)
		@(cd external/prometheus-cpp && git submodule init && git submodule update)
		@$(call MKDIR, external/prometheus-cpp/cmake_build)
# The install prefix is set to the external/prometheus_install dir -- this will
# install the library to external/prometheus_install/lib and the headers to
# external/prometheus_install/include.
# The -G "Unix Makefiles" flag is used to generate GNU Makefiles even on Windows,
# because the default generator on Windows is the Visual Studio generator.
		cd external/prometheus-cpp/cmake_build && cmake -DBUILD_SHARED_LIBS=OFF -DENABLE_PUSH=OFF -DENABLE_COMPRESSION=OFF -DENABLE_TESTING=OFF -DCMAKE_INSTALL_PREFIX=../../prometheus_install/ -G "Unix Makefiles" ..
		cd external/prometheus-cpp/cmake_build && make install

clean:
		@$(call RMDIR, $(OBJ_DIR))
		@$(call RM, $(EXE))
		@$(call RM, $(EXE).exe)

clean_all: clean
		@$(call RMDIR, external)
