cmake_minimum_required(VERSION 3.22)
include(ExternalProject)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# In order to use CMake for cross-compiling
set(CMAKE_CXX_COMPILER_FORCED "true") 

# Include toolchain file
include("gcc-arm-none-eabi.cmake")

# Set the project name
set(CMAKE_PROJECT_NAME STM32H745I-DUALCORE)

# Core project settings
project(${CMAKE_PROJECT_NAME})

# VID-18 supplies a standalone CM4 project. CM7 is optional only when its
# actual source project is supplied; this does not program MCU boot options.
set(BUILD_CONTEXT "CM4" CACHE STRING "Core build context: CM4 or CM7")
if(BUILD_CONTEXT MATCHES "CM7" AND NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/CM7/CMakeLists.txt")
    message(FATAL_ERROR "CM7 source project is absent; use BUILD_CONTEXT=CM4 for VID-18")
endif()

# Include external project file
include("mx-generated.cmake")

