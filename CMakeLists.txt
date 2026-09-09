cmake_minimum_required(VERSION 3.11)
project(pmidi)

add_subdirectory(lib)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR AND NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
	add_subdirectory(src)
	add_subdirectory(glsrc)

	include(GNUInstallDirs)
	install(
		TARGETS pmidi
		DESTINATION ${CMAKE_INSTALL_BINDIR}
	)

	file(GLOB PATCHES patches/florestan-*)
	install(
		DIRECTORY ${PATCHES}
		DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/pmidi
	)
	install(
		FILES patches/florestan.cfg
		RENAME pmidi.cfg
		DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/pmidi
	)
endif()
