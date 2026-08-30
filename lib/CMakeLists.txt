cmake_minimum_required(VERSION 3.11)
project(turbosynth)

file(GLOB LIBTURBOSYNTH_SOURCES src/*.c)
add_library(turbosynth STATIC ${LIBTURBOSYNTH_SOURCES})
target_include_directories(turbosynth PUBLIC include)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	target_compile_definitions(turbosynth PRIVATE DEBUG)
endif()
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
	target_link_libraries(turbosynth PRIVATE m)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
	set(FS_SYMS "'_FileStream_New', '_FileStream_Destroy'")
	set(MIDI_SYMS "'_MidiStream_New', '_MidiStream_Advance', '_MidiStream_Destroy'")
	set(GUSPAT_SYMS "'_GUSPatSynth_New', '_GUSPatSynth_Load', '_GUSPatSynth_Unload', '_GUSPatSynth_Note', '_GUSPatSynth_NoteOffAll', '_GUSPatSynth_SetBank', '_GUSPatSynth_SetBankMSB', '_GUSPatSynth_SetBankLSB', '_GUSPatSynth_SetProgram', '_GUSPatSynth_SetDrum', '_GUSPatSynth_ChangePitchWheel', '_GUSPatSynth_SetVolume', '_GUSPatSynth_RenderShort', '_GUSPatSynth_RenderFloat', '_GUSPatSynth_Reset', '_GUSPatSynth_Destroy'")
	set(EASYMIDI_SYMS "'_EasyMidi_New', '_EasyMidi_New2', '_EasyMidi_MidiCallback', '_EasyMidi_Load', '_EasyMidi_Load2', '_EasyMidi_IsFinished', '_EasyMidi_RenderShort', '_EasyMidi_RenderFloat', '_EasyMidi_Reset', '_EasyMidi_Destroy'")
	add_executable(turbosynthwasm turbosynthwasm.c)
	target_link_options(turbosynthwasm PRIVATE
		"-sWASM=1"
		"-sSINGLE_FILE=1"
		"-sSINGLE_FILE_BINARY_ENCODE=0"
		"-sEXPORTED_FUNCTIONS=[\"_free\", \"_malloc\", ${FS_SYMS}, ${MIDI_SYMS}, ${GUSPAT_SYMS}, ${EASYMIDI_SYMS}]"
		"-sEXPORTED_RUNTIME_METHODS=[\"cwrap\", \"ccall\", \"HEAPF32\", \"addFunction\", \"FS\"]"
		"-sMODULARIZE=1"
		"-sALLOW_TABLE_GROWTH=1"
		"-sALLOW_MEMORY_GROWTH=1"
		"-sSTACK_SIZE=1048576"
		"-sFORCE_FILESYSTEM=1"
		"-sEXPORT_NAME=TurboSynthWASM"
	)
	target_link_libraries(turbosynthwasm PRIVATE turbosynth)
endif()
