# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 2.6

# Default target executed when no arguments are given to make.
default_target: all
.PHONY : default_target

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canoncical targets will work.
.SUFFIXES:

# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list

# Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/www/chat-project/new/cpp/Sserver

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/zion/chat-project/new/cpp/Sserver

#=============================================================================
# Targets provided globally by CMake.

# Special rule for the target edit_cache
edit_cache:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "Running interactive CMake command-line interface..."
	/usr/bin/cmake -i .
.PHONY : edit_cache

# Special rule for the target edit_cache
edit_cache/fast: edit_cache
.PHONY : edit_cache/fast

# Special rule for the target rebuild_cache
rebuild_cache:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "Running CMake to regenerate build system..."
	/usr/bin/cmake -H$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR)
.PHONY : rebuild_cache

# Special rule for the target rebuild_cache
rebuild_cache/fast: rebuild_cache
.PHONY : rebuild_cache/fast

# The main all target
all: cmake_check_build_system
	$(CMAKE_COMMAND) -E cmake_progress_start /home/zion/chat-project/new/cpp/Sserver/CMakeFiles /home/zion/chat-project/new/cpp/Sserver/CMakeFiles/progress.make
	$(MAKE) -f CMakeFiles/Makefile2 all
	$(CMAKE_COMMAND) -E cmake_progress_start /home/zion/chat-project/new/cpp/Sserver/CMakeFiles 0
.PHONY : all

# The main clean target
clean:
	$(MAKE) -f CMakeFiles/Makefile2 clean
.PHONY : clean

# The main clean target
clean/fast: clean
.PHONY : clean/fast

# Prepare targets for installation.
preinstall: all
	$(MAKE) -f CMakeFiles/Makefile2 preinstall
.PHONY : preinstall

# Prepare targets for installation.
preinstall/fast:
	$(MAKE) -f CMakeFiles/Makefile2 preinstall
.PHONY : preinstall/fast

# clear depends
depend:
	$(CMAKE_COMMAND) -H$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 1
.PHONY : depend

#=============================================================================
# Target rules for targets named sserv

# Build rule for target.
sserv: cmake_check_build_system
	$(MAKE) -f CMakeFiles/Makefile2 sserv
.PHONY : sserv

# fast build rule for target.
sserv/fast:
	$(MAKE) -f CMakeFiles/sserv.dir/build.make CMakeFiles/sserv.dir/build
.PHONY : sserv/fast

json_parser.o: json_parser.cpp.o
.PHONY : json_parser.o

# target to build an object file
json_parser.cpp.o:
	$(MAKE) -f CMakeFiles/sserv.dir/build.make CMakeFiles/sserv.dir/json_parser.cpp.o
.PHONY : json_parser.cpp.o

json_parser.i: json_parser.cpp.i
.PHONY : json_parser.i

# target to preprocess a source file
json_parser.cpp.i:
	$(MAKE) -f CMakeFiles/sserv.dir/build.make CMakeFiles/sserv.dir/json_parser.cpp.i
.PHONY : json_parser.cpp.i

json_parser.s: json_parser.cpp.s
.PHONY : json_parser.s

# target to generate assembly for a file
json_parser.cpp.s:
	$(MAKE) -f CMakeFiles/sserv.dir/build.make CMakeFiles/sserv.dir/json_parser.cpp.s
.PHONY : json_parser.cpp.s

new_betta.o: new_betta.cpp.o
.PHONY : new_betta.o

# target to build an object file
new_betta.cpp.o:
	$(MAKE) -f CMakeFiles/sserv.dir/build.make CMakeFiles/sserv.dir/new_betta.cpp.o
.PHONY : new_betta.cpp.o

new_betta.i: new_betta.cpp.i
.PHONY : new_betta.i

# target to preprocess a source file
new_betta.cpp.i:
	$(MAKE) -f CMakeFiles/sserv.dir/build.make CMakeFiles/sserv.dir/new_betta.cpp.i
.PHONY : new_betta.cpp.i

new_betta.s: new_betta.cpp.s
.PHONY : new_betta.s

# target to generate assembly for a file
new_betta.cpp.s:
	$(MAKE) -f CMakeFiles/sserv.dir/build.make CMakeFiles/sserv.dir/new_betta.cpp.s
.PHONY : new_betta.cpp.s

# Help Target
help:
	@echo "The following are some of the valid targets for this Makefile:"
	@echo "... all (the default if no target is provided)"
	@echo "... clean"
	@echo "... depend"
	@echo "... edit_cache"
	@echo "... rebuild_cache"
	@echo "... sserv"
	@echo "... json_parser.o"
	@echo "... json_parser.i"
	@echo "... json_parser.s"
	@echo "... new_betta.o"
	@echo "... new_betta.i"
	@echo "... new_betta.s"
.PHONY : help



#=============================================================================
# Special targets to cleanup operation of make.

# Special rule to run CMake to check the build system integrity.
# No rule that depends on this can have commands that come from listfiles
# because they might be regenerated.
cmake_check_build_system:
	$(CMAKE_COMMAND) -H$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 0
.PHONY : cmake_check_build_system

