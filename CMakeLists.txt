cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

project(rtti-for-cpp)

# Add custom cmake modules
# list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# If the user did not customize the install prefix,
# set it to live under build so we don't inadverently pollute /usr/local
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set (CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Default install path" FORCE)
endif()

# Set default build type
if(NOT DEFINED CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default is Release")
    set(CMAKE_BUILD_TYPE "Release")
endif()
message(STATUS "rtti build type: ${CMAKE_BUILD_TYPE}")

# Build shared library by default
option(BUILD_SHARED_LIBS "Build shared library" ON)

include (GNUInstallDirs)

add_subdirectory(src)
add_subdirectory(test)

########################################################################################################################
## Tests
########################################################################################################################

