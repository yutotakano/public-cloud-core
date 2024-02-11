CC := g++
CPPFLAGS := -Iinclude -Iexternal/quill_install/include -Iexternal/subprocess_install/include
CFLAGS := -Wall
LDFLAGS := -Lexternal/quill_install/lib -Lexternal/subprocess_install/lib
LDLIBS := -lpthread -lquill -lsubprocess

SRC_DIR := source
OBJ_DIR := objects

EXE := publicore

SRC_LIST := $(wildcard $(SRC_DIR)/*.cpp)
OBJ_LIST := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRC_LIST))

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

all: external $(EXE)

 # Prevents make from doing something with a file with the same name as the target
.PHONY: all clean clean_all

# Compiling the source files, $< is the first prerequisite, $@ is the target
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
		$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# Create the objects directory
$(OBJ_DIR):
		@$(call MKDIR, $@)

# Linking the object files, $^ is the list of all prerequisites
$(EXE): $(OBJ_LIST)
		$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

# If dependencies are needed, install them
external: external/quill_install external/subprocess_install

# If quill isn't present as an installed library, clone and install it
external/quill_install:
		@$(call MKDIR, external)
		@cd external && git clone https://github.com/odygrd/quill.git && cd quill && git checkout tags/v3.6.0
		@$(call MKDIR, external/quill/cmake_build)
# The install prefix is set to the external/quill_install dir -- this will
# install the library to external/quill_install/lib and the headers to
# external/quill_install/include.
# The -G "Unix Makefiles" flag is used to generate GNU Makefiles even on Windows,
# because the default generator on Windows is the Visual Studio generator.
		cd external/quill/cmake_build && cmake -DCMAKE_INSTALL_PREFIX=../../quill_install/ -G "Unix Makefiles" ..
		cd external/quill/cmake_build && make install

external/subprocess_install:
		@$(call MKDIR, external)
		@cd external && git clone https://github.com/benman64/subprocess.git
		@$(call MKDIR, external/subprocess/cmake_build)
# We remove the definition of __MINGW32__ from the CXX flags, because the
# subprocess library has a bug when compiling with MinGW where it tries to
# define a macro that happens to already be defined by the compiler in recent
# versions of MinGW.
# The -G "Unix Makefiles" flag is used to generate GNU Makefiles even on Windows,
# because the default generator on Windows is the Visual Studio generator.
		cd external/subprocess/cmake_build && cmake -DCMAKE_CXX_FLAGS=-U__MINGW32__ -G "Unix Makefiles" ..
		cd external/subprocess/cmake_build && make subprocess
# The subprocess library doesn't have an install target, so we'll just copy
# the headers and the library to the required directories
		@$(call MKDIR, external/subprocess_install/include)
		@$(call CP, external/subprocess/src/cpp, external/subprocess_install/include/subprocess)
		@$(call MKDIR, external/subprocess_install/lib)
		@$(call CP, external/subprocess/cmake_build/subprocess/libsubprocess.a, external/subprocess_install/lib)

clean:
		@$(call RMDIR, $(OBJ_DIR))
		@$(call RM, $(EXE))
		@$(call RM, $(EXE).exe)

clean_all: clean
		@$(call RMDIR, external)
