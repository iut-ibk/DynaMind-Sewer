cmake_minimum_required(VERSION 2.6)
project(swmm)

option(SWMM_LIB "compile as library" OFF)

find_package(OpenMP)
if(OPENMP_FOUND)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
	message(STATUS "OpenMP is enabled")
else()
	add_definitions(-DOPENMP_DISABLED)
	message(STATUS "OpenMP is disabled. Consider using a compiler that supports OpenMP")
endif()

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
