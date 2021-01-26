find_package(Threads REQUIRED)

# Enable ExternalProject CMake module
include(ExternalProject)

# Download and install GoogleTest
ExternalProject_Add(
    gtest
    URL https://github.com/google/googletest/archive/master.zip
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/gtest
    # Disable install step
    INSTALL_COMMAND ""
)

# Get GTest source and binary directories from CMake project
ExternalProject_Get_Property(gtest source_dir binary_dir)

# Create a libgtest target to be used as a dependency by test programs
add_library(libgtest IMPORTED STATIC GLOBAL)
add_dependencies(libgtest gtest)

# Set libgtest properties
set_target_properties(libgtest PROPERTIES
    "IMPORTED_LOCATION" "${binary_dir}/lib/libgtest.a"
    "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}"
)

# I couldn't make it work with INTERFACE_INCLUDE_DIRECTORIES
include_directories("${source_dir}/googletest/include")

# User Modification
# Compile the current directory *.cpp and compile main.cpp as executable file

message(STATUS "[__INFO__] CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
#include_directories(${CMAKE_SOURCE_DIR}/mushroom)

#file(GLOB SRCS ${CMAKE_SOURCE_DIR}/test/*.cpp)
add_executable(testcase ${CMAKE_SOURCE_DIR}/test/test.cpp ${CMAKE_SOURCE_DIR}/test/test_raft.cpp)

target_link_libraries(testcase libgtest ${CMAKE_SOURCE_DIR}/mushroom/build/libMushroom.a)

add_test(NAME testcase
         COMMAND testcase)