cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

# Used as subproject
set(IS_SUBPROJECT OFF)
if(DEFINED PROJECT_NAME)
  set(IS_SUBPROJECT ON)
endif()

project(rtti-for-cpp VERSION 1.0.0 LANGUAGES CXX)

# Require out-of-source build
file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOC_PATH)
if(EXISTS "${LOC_PATH}")
    message(FATAL_ERROR "Out of source build required! Please make a build subdirectory.")
endif()

# Add custom cmake modules
# list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
option(BUILD_EXAMPLES "Build examples and tutorials" ON)

if(NOT IS_SUBPROJECT)
    # If the user did not customize the install prefix,
    # set it to live under build so we don't inadverently pollute /usr/local
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set (CMAKE_INSTALL_PREFIX "${PROJECT_BINARY_DIR}/install" CACHE PATH "Default install path" FORCE)
    endif()

    # Set default build type
    if(NOT DEFINED CMAKE_BUILD_TYPE)
        message(STATUS "No build type selected, default is Release")
        set(CMAKE_BUILD_TYPE "Release")
    endif()
    message(STATUS "rtti build type: ${CMAKE_BUILD_TYPE}")
endif()

# Build shared library by default
option(BUILD_SHARED_LIBS "Build shared library" ON)

include(GNUInstallDirs)
include(CMakePrintHelpers)

add_subdirectory(src)

########################################################################################################################
## Tests
########################################################################################################################

if(NOT IS_SUBPROJECT)
    if (BUILD_EXAMPLES)
        add_subdirectory(examples)
    endif()

    include(CTest)
    if (BUILD_TESTING)
        add_subdirectory(tests)
    endif()
endif()

