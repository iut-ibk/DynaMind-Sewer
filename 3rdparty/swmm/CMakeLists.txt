cmake_minimum_required(VERSION 2.6)
project(swmm)

option(SWMM_LIB "compile as library" OFF)

find_package(OpenMP REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")

file(GLOB SRCS *.c *.h)

if (SWMM_LIB)
    message(STATUS "Compiling as library")
    add_definitions(-DDLL)
    add_library(swmm STATIC ${SRCS})
else (SWMM_LIB)
    message(STATUS "Compiling as standalone CLI application")
    add_definitions(-DCLE)
    add_executable(swmm ${SRCS})
endif(SWMM_LIB)

target_link_libraries(swmm -lm)
