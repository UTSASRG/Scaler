# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
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

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/st/Projects/Scaler

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/st/Projects/Scaler/build

# Include any dependencies generated for this target.
include unittests/libScalerHook/CMakeFiles/libScalerHook-TestMultiThreadFuncCall.dir/depend.make

# Include the progress variables for this target.
include unittests/libScalerHook/CMakeFiles/libScalerHook-TestMultiThreadFuncCall.dir/progress.make

# Include the compile flags for this target's objects.
include unittests/libScalerHook/CMakeFiles/libScalerHook-TestMultiThreadFuncCall.dir/flags.make

unittests/libScalerHook/CMakeFiles/libScalerHook-TestMultiThreadFuncCall.dir/TestMultiThreadFuncCall.cpp.o: unittests/libScalerHook/CMakeFiles/libScalerHook-TestMultiThreadFuncCall.dir/flags.make
unittests/libScalerHook/CMakeFiles/libScalerHook-TestMultiThreadFuncCall.dir/TestMultiThreadFuncCall.cpp.o: ../unittests/libScalerHook/TestMultiThreadFuncCall.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/st/Projects/Scaler/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object unittests/libScalerHook/CMakeFiles/libScalerHook-TestMultiThreadFuncCall.dir/TestMultiThreadFuncCall.cpp.o"
	cd /home/st/Projects/Scaler/build/unittests/libScalerHook && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/libScalerHook-TestMultiThreadFuncCall.dir/TestMultiThreadFuncCall.cpp.o -c /home/st/Projects/Scaler/unittests/libScalerHook/TestMultiThreadFuncCall.cpp

unittests/libScalerHook/CMakeFiles/libScalerHook-TestMultiThreadFuncCall.dir/TestMultiThreadFuncCall.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/libScalerHook-TestMultiThreadFuncCall.dir/TestMultiThreadFuncCall.cpp.i"
	cd /home/st/Projects/Scaler/build/unittests/libScalerHook && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/st/Projects/Scaler/unittests/libScalerHook/TestMultiThreadFuncCall.cpp > CMakeFiles/libScalerHook-TestMultiThreadFuncCall.dir/TestMultiThreadFuncCall.cpp.i

unittests/libScalerHook/CMakeFiles/libScalerHook-TestMultiThreadFuncCall.dir/TestMultiThreadFuncCall.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/libScalerHook-TestMultiThreadFuncCall.dir/TestMultiThreadFuncCall.cpp.s"
	cd /home/st/Projects/Scaler/build/unittests/libScalerHook && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/st/Projects/Scaler/unittests/libScalerHook/TestMultiThreadFuncCall.cpp -o CMakeFiles/libScalerHook-TestMultiThreadFuncCall.dir/TestMultiThreadFuncCall.cpp.s

# Object files for target libScalerHook-TestMultiThreadFuncCall
libScalerHook__TestMultiThreadFuncCall_OBJECTS = \
"CMakeFiles/libScalerHook-TestMultiThreadFuncCall.dir/TestMultiThreadFuncCall.cpp.o"

# External object files for target libScalerHook-TestMultiThreadFuncCall
libScalerHook__TestMultiThreadFuncCall_EXTERNAL_OBJECTS =

unittests/libScalerHook/libScalerHook-TestMultiThreadFuncCall: unittests/libScalerHook/CMakeFiles/libScalerHook-TestMultiThreadFuncCall.dir/TestMultiThreadFuncCall.cpp.o
unittests/libScalerHook/libScalerHook-TestMultiThreadFuncCall: unittests/libScalerHook/CMakeFiles/libScalerHook-TestMultiThreadFuncCall.dir/build.make
unittests/libScalerHook/libScalerHook-TestMultiThreadFuncCall: libScalerHook/libscalerhook.a
unittests/libScalerHook/libScalerHook-TestMultiThreadFuncCall: unittests/libScalerHook/liblibScalerHook-InstallTest.so
unittests/libScalerHook/libScalerHook-TestMultiThreadFuncCall: unittests/libScalerHook/libFuncCallTest.so
unittests/libScalerHook/libScalerHook-TestMultiThreadFuncCall: unittests/libScalerHook/CMakeFiles/libScalerHook-TestMultiThreadFuncCall.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/st/Projects/Scaler/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable libScalerHook-TestMultiThreadFuncCall"
	cd /home/st/Projects/Scaler/build/unittests/libScalerHook && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/libScalerHook-TestMultiThreadFuncCall.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
unittests/libScalerHook/CMakeFiles/libScalerHook-TestMultiThreadFuncCall.dir/build: unittests/libScalerHook/libScalerHook-TestMultiThreadFuncCall

.PHONY : unittests/libScalerHook/CMakeFiles/libScalerHook-TestMultiThreadFuncCall.dir/build

unittests/libScalerHook/CMakeFiles/libScalerHook-TestMultiThreadFuncCall.dir/clean:
	cd /home/st/Projects/Scaler/build/unittests/libScalerHook && $(CMAKE_COMMAND) -P CMakeFiles/libScalerHook-TestMultiThreadFuncCall.dir/cmake_clean.cmake
.PHONY : unittests/libScalerHook/CMakeFiles/libScalerHook-TestMultiThreadFuncCall.dir/clean

unittests/libScalerHook/CMakeFiles/libScalerHook-TestMultiThreadFuncCall.dir/depend:
	cd /home/st/Projects/Scaler/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/st/Projects/Scaler /home/st/Projects/Scaler/unittests/libScalerHook /home/st/Projects/Scaler/build /home/st/Projects/Scaler/build/unittests/libScalerHook /home/st/Projects/Scaler/build/unittests/libScalerHook/CMakeFiles/libScalerHook-TestMultiThreadFuncCall.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : unittests/libScalerHook/CMakeFiles/libScalerHook-TestMultiThreadFuncCall.dir/depend

