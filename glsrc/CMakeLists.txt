find_package(OpenGL)

include(FindPkgConfig)
pkg_search_module(SDL2 REQUIRED sdl2)

if(SDL2_FOUND AND OPENGL_FOUND)
	file(GLOB PMIDIGL_SOURCES *.c)
	add_executable(pmidigl ${PMIDIGL_SOURCES})
	target_include_directories(pmidigl PRIVATE ${SDL2_INCLUDE_DIRS})
	target_link_directories(pmidigl PRIVATE ${SDL2_LIBRARY_DIRS})
	target_link_libraries(pmidigl PRIVATE ${SDL2_LIBRARIES})
	target_link_libraries(pmidigl PRIVATE turbosynth OpenGL::GL)
endif()
