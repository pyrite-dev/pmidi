cmake_minimum_required(VERSION 3.11)
project(pmidi)

add_subdirectory(lib)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR AND NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
	add_subdirectory(src)
	add_subdirectory(glsrc)
endif()
