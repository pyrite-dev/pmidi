cmake_minimum_required(VERSION 3.11)
project(pmidi)


file(GLOB LIBMIDI_SOURCES midi/*.c)
add_library(midi STATIC ${LIBMIDI_SOURCES})
target_include_directories(midi PUBLIC midi)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	target_compile_definitions(midi PRIVATE DEBUG)
endif()


find_package(SDL2 REQUIRED)

file(GLOB PMIDI_SOURCES src/*.c)
add_executable(pmidi ${PMIDI_SOURCES})
target_link_libraries(pmidi PRIVATE midi SDL2::SDL2)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
	target_link_libraries(pmidi PRIVATE m)
endif()
