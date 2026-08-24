cmake_minimum_required(VERSION 3.11)
project(pmidisynth)

file(GLOB LIBPMIDISYNTH_SOURCES src/*.c)
add_library(pmidisynth STATIC ${LIBPMIDISYNTH_SOURCES})
target_include_directories(pmidisynth PUBLIC include)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	target_compile_definitions(pmidisynth PRIVATE DEBUG)
endif()
