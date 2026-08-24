find_package(SDL2)
find_package(OpenGL)

if(SDL2_FOUND AND OPENGL_FOUND)
	file(GLOB PMIDIGL_SOURCES *.c)
	add_executable(pmidigl ${PMIDIGL_SOURCES})
	target_link_libraries(pmidigl PRIVATE turbosynth SDL2::SDL2 OpenGL::GL)
endif()
