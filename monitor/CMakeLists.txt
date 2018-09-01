cmake_minimum_required(VERSION 3.10)
project(monitor C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "-O3") ## Optimize

add_subdirectory(../dronebridge_base/common db_common)

FIND_PACKAGE(PkgConfig REQUIRED)
PKG_CHECK_MODULES(GTK3 REQUIRED gtk+-3.0)

# Setup CMake to use GTK+, tell the compiler where to look for headers
# and to the linker where to look for libraries
INCLUDE_DIRECTORIES(${GTK3_INCLUDE_DIRS})
LINK_DIRECTORIES(${GTK3_LIBRARY_DIRS})

# Add other flags to the compiler
ADD_DEFINITIONS(${GTK3_CFLAGS_OTHER})
ADD_DEFINITIONS(${CMAKE_CXX_FLAGS})

add_executable(monitor main.c)

TARGET_LINK_LIBRARIES(monitor ${GTK3_LIBRARIES})
target_link_libraries(monitor db_common)
file(COPY glade DESTINATION ${CMAKE_CURRENT_BINARY_DIR})