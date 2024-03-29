CC := g++
CPPFLAGS := -Iinclude -Iexternal/quill_install/include -Iexternal/cpr_install/include -Iexternal/tiny-process-library_install/include
CFLAGS := -Wall -DCURL_STATICLIB -std=c++17
LDFLAGS := -Lexternal/quill_install/lib -Lexternal/cpr_install/lib -Lexternal/tiny-process-library_install/lib
LDLIBS := -lpthread -lquill -lcpr -lcurl -ltiny-process-library

ifeq ($(OS), Windows_NT)
		LDLIBS += -lws2_32 -lbcrypt -lcrypt32
else
		UNAME_S := $(shell uname -s)

		ifeq ($(UNAME_S), Darwin)
				LDLIBS += -framework CoreFoundation -framework SystemConfiguration -framework Security -lz -lssh2
		endif
endif

SRC_DIR := source
OBJ_DIR := objects

# Recursive wildcard (cross-platform alternative to using shell find)
rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

SRC_LIST := $(call rwildcard, $(SRC_DIR), *.cpp)
OBJ_LIST := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRC_LIST))

ifeq ($(OS), Windows_NT)
		EXE := publicore.exe
else
		EXE := publicore
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
external: external/quill_install external/cpr_install external/tiny-process-library_install

# If quill isn't present as an installed library, clone and install it
external/quill_install:
		@$(call MKDIR, external)
		@$(call IF_NOT_EXISTS, external/quill, cd external && git clone https://github.com/odygrd/quill.git && cd quill && git checkout tags/v3.6.0)
		@$(call MKDIR, external/quill/cmake_build)
# The install prefix is set to the external/quill_install dir -- this will
# install the library to external/quill_install/lib and the headers to
# external/quill_install/include.
# The -G "Unix Makefiles" flag is used to generate GNU Makefiles even on Windows,
# because the default generator on Windows is the Visual Studio generator.
		cd external/quill/cmake_build && cmake -DCMAKE_INSTALL_PREFIX=../../quill_install/ -G "Unix Makefiles" ..
		cd external/quill/cmake_build && make install

external/cpr_install:
		@$(call MKDIR, external)
		@$(call IF_NOT_EXISTS, external/cpr, cd external && git clone https://github.com/libcpr/cpr)
		@$(call MKDIR, external/cpr/cmake_build)
		cd external/cpr/cmake_build && cmake -DCMAKE_INSTALL_PREFIX=../../cpr_install/ -DBUILD_SHARED_LIBS=OFF -DCURL_ZLIB=OFF -G "Unix Makefiles" ..
		cd external/cpr/cmake_build && make install

external/tiny-process-library_install:
		@$(call MKDIR, external)
		@$(call IF_NOT_EXISTS, external/tiny-process-library, cd external && git clone http://gitlab.com/eidheim/tiny-process-library)
		@$(call MKDIR, external/tiny-process-library/cmake_build)
		cd external/tiny-process-library/cmake_build && cmake -DCMAKE_INSTALL_PREFIX=../../tiny-process-library_install/ -G "Unix Makefiles" ..
		cd external/tiny-process-library/cmake_build && make install

clean:
		@$(call RMDIR, $(OBJ_DIR))
		@$(call RM, $(EXE))
		@$(call RM, $(EXE).exe)

clean_all: clean
		@$(call RMDIR, external)
