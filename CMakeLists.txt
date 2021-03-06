cmake_minimum_required(VERSION 3.1)
message( STATUS "cmake: ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}" )

project (raft-cpp)
set (CMAKE_BUILD_TYPE Debug)

# c++ 11/14/17
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_EXTENSIONS OFF)

set(ENABLE_TESTING ON)

set (EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

message(STATUS "[__INFO__] CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")
message(STATUS "[__INFO__] CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
message(STATUS "[__INFO__] PROJECT_BINARY_DIR: ${PROJECT_BINARY_DIR}")

# pthread
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package (Threads)

if (ENABLE_TESTING)
    enable_testing()
    add_subdirectory("test")
endif()