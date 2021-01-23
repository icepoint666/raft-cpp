cmake_minimum_required(VERSION 3.1)
message( STATUS "cmake: ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}" )

project (Mushroom)
set (CMAKE_BUILD_TYPE Debug)

# c++ 11/14/17
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_EXTENSIONS OFF)

message(STATUS "[__INFO__] CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")
message(STATUS "[__INFO__] CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
message(STATUS "[__INFO__] PROJECT_BINARY_DIR: ${PROJECT_BINARY_DIR}")

set (EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# pthread
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package (Threads)

file (GLOB_RECURSE HEADERS include/*.hpp)
file (GLOB_RECURSE NETWORK_SOURCES
        network/*.hpp
        network/*.cpp)
file (GLOB_RECURSE RPC_SOURCES
        rpc/*.hpp
        rpc/*.cpp)

add_library(${PROJECT_NAME}
    ${RPC_SOURCES}
    ${NETWORK_SOURCES}
    ${HEADERS})
