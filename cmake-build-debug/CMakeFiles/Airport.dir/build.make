# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.15

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
CMAKE_COMMAND = /snap/clion/98/bin/cmake/linux/bin/cmake

# The command to remove a file.
RM = /snap/clion/98/bin/cmake/linux/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/pedro/Documents/Universidade/2ºano/1ºSemestre/SO/Project/SO-Project

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/pedro/Documents/Universidade/2ºano/1ºSemestre/SO/Project/SO-Project/cmake-build-debug

# Include any dependencies generated for this target.
include CMakeFiles/Airport.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/Airport.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/Airport.dir/flags.make

CMakeFiles/Airport.dir/logging.c.o: CMakeFiles/Airport.dir/flags.make
CMakeFiles/Airport.dir/logging.c.o: ../logging.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pedro/Documents/Universidade/2ºano/1ºSemestre/SO/Project/SO-Project/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/Airport.dir/logging.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/Airport.dir/logging.c.o   -c /home/pedro/Documents/Universidade/2ºano/1ºSemestre/SO/Project/SO-Project/logging.c

CMakeFiles/Airport.dir/logging.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/Airport.dir/logging.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/pedro/Documents/Universidade/2ºano/1ºSemestre/SO/Project/SO-Project/logging.c > CMakeFiles/Airport.dir/logging.c.i

CMakeFiles/Airport.dir/logging.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/Airport.dir/logging.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/pedro/Documents/Universidade/2ºano/1ºSemestre/SO/Project/SO-Project/logging.c -o CMakeFiles/Airport.dir/logging.c.s

CMakeFiles/Airport.dir/ControlTower.c.o: CMakeFiles/Airport.dir/flags.make
CMakeFiles/Airport.dir/ControlTower.c.o: ../ControlTower.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pedro/Documents/Universidade/2ºano/1ºSemestre/SO/Project/SO-Project/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object CMakeFiles/Airport.dir/ControlTower.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/Airport.dir/ControlTower.c.o   -c /home/pedro/Documents/Universidade/2ºano/1ºSemestre/SO/Project/SO-Project/ControlTower.c

CMakeFiles/Airport.dir/ControlTower.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/Airport.dir/ControlTower.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/pedro/Documents/Universidade/2ºano/1ºSemestre/SO/Project/SO-Project/ControlTower.c > CMakeFiles/Airport.dir/ControlTower.c.i

CMakeFiles/Airport.dir/ControlTower.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/Airport.dir/ControlTower.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/pedro/Documents/Universidade/2ºano/1ºSemestre/SO/Project/SO-Project/ControlTower.c -o CMakeFiles/Airport.dir/ControlTower.c.s

CMakeFiles/Airport.dir/SimulationManager.c.o: CMakeFiles/Airport.dir/flags.make
CMakeFiles/Airport.dir/SimulationManager.c.o: ../SimulationManager.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pedro/Documents/Universidade/2ºano/1ºSemestre/SO/Project/SO-Project/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building C object CMakeFiles/Airport.dir/SimulationManager.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/Airport.dir/SimulationManager.c.o   -c /home/pedro/Documents/Universidade/2ºano/1ºSemestre/SO/Project/SO-Project/SimulationManager.c

CMakeFiles/Airport.dir/SimulationManager.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/Airport.dir/SimulationManager.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/pedro/Documents/Universidade/2ºano/1ºSemestre/SO/Project/SO-Project/SimulationManager.c > CMakeFiles/Airport.dir/SimulationManager.c.i

CMakeFiles/Airport.dir/SimulationManager.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/Airport.dir/SimulationManager.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/pedro/Documents/Universidade/2ºano/1ºSemestre/SO/Project/SO-Project/SimulationManager.c -o CMakeFiles/Airport.dir/SimulationManager.c.s

CMakeFiles/Airport.dir/SimulationUtils.c.o: CMakeFiles/Airport.dir/flags.make
CMakeFiles/Airport.dir/SimulationUtils.c.o: ../SimulationUtils.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pedro/Documents/Universidade/2ºano/1ºSemestre/SO/Project/SO-Project/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building C object CMakeFiles/Airport.dir/SimulationUtils.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/Airport.dir/SimulationUtils.c.o   -c /home/pedro/Documents/Universidade/2ºano/1ºSemestre/SO/Project/SO-Project/SimulationUtils.c

CMakeFiles/Airport.dir/SimulationUtils.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/Airport.dir/SimulationUtils.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/pedro/Documents/Universidade/2ºano/1ºSemestre/SO/Project/SO-Project/SimulationUtils.c > CMakeFiles/Airport.dir/SimulationUtils.c.i

CMakeFiles/Airport.dir/SimulationUtils.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/Airport.dir/SimulationUtils.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/pedro/Documents/Universidade/2ºano/1ºSemestre/SO/Project/SO-Project/SimulationUtils.c -o CMakeFiles/Airport.dir/SimulationUtils.c.s

# Object files for target Airport
Airport_OBJECTS = \
"CMakeFiles/Airport.dir/logging.c.o" \
"CMakeFiles/Airport.dir/ControlTower.c.o" \
"CMakeFiles/Airport.dir/SimulationManager.c.o" \
"CMakeFiles/Airport.dir/SimulationUtils.c.o"

# External object files for target Airport
Airport_EXTERNAL_OBJECTS =

Airport: CMakeFiles/Airport.dir/logging.c.o
Airport: CMakeFiles/Airport.dir/ControlTower.c.o
Airport: CMakeFiles/Airport.dir/SimulationManager.c.o
Airport: CMakeFiles/Airport.dir/SimulationUtils.c.o
Airport: CMakeFiles/Airport.dir/build.make
Airport: CMakeFiles/Airport.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/pedro/Documents/Universidade/2ºano/1ºSemestre/SO/Project/SO-Project/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Linking C executable Airport"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/Airport.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/Airport.dir/build: Airport

.PHONY : CMakeFiles/Airport.dir/build

CMakeFiles/Airport.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/Airport.dir/cmake_clean.cmake
.PHONY : CMakeFiles/Airport.dir/clean

CMakeFiles/Airport.dir/depend:
	cd /home/pedro/Documents/Universidade/2ºano/1ºSemestre/SO/Project/SO-Project/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/pedro/Documents/Universidade/2ºano/1ºSemestre/SO/Project/SO-Project /home/pedro/Documents/Universidade/2ºano/1ºSemestre/SO/Project/SO-Project /home/pedro/Documents/Universidade/2ºano/1ºSemestre/SO/Project/SO-Project/cmake-build-debug /home/pedro/Documents/Universidade/2ºano/1ºSemestre/SO/Project/SO-Project/cmake-build-debug /home/pedro/Documents/Universidade/2ºano/1ºSemestre/SO/Project/SO-Project/cmake-build-debug/CMakeFiles/Airport.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/Airport.dir/depend

