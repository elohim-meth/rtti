project(Rtti CXX)
cmake_minimum_required(VERSION 3.1.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra")

message(STATUS ${PROJECT_BINARY_DIR})

include_directories(../include)
add_subdirectory(./src)
