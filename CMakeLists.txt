cmake_minimum_required( VERSION 3.22.1 )

option( AUDIO_BUILD_EXAMPLES "Include the example projects." ON )
option( BUILD_SHARED_LIBS "Build Audio library as a shared library (DLL)." OFF )

# Use solution folders in Visual Studio.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project( Audio LANGUAGES C CXX )

if( CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    # Make sure DLL and EXE targets go to the same directory.
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
endif()

# Enable multi-processor builds in MSVC.
if(MSVC)
    add_compile_options(/MP)
endif(MSVC)

set( INC_FILES
    inc/Audio/Config.hpp
    inc/Audio/Device.hpp
    inc/Audio/Listener.hpp
    inc/Audio/Sound.hpp
    inc/Audio/Vector.hpp
	inc/Audio/Waveform.hpp
)

set( SRC_FILES
    src/Device.cpp
    src/Listener.cpp
    src/ListenerImpl.hpp
    src/ListenerImpl.cpp
    src/miniaudio.c
    src/miniaudio.h
    src/Sound.cpp
    src/SoundImpl.hpp
    src/SoundImpl.cpp
	src/Waveform.cpp
	src/WaveformImpl.hpp
	src/WaveformImpl.cpp
    src/stb_vorbis.c
)

set( ALL_FILES 
    ${SRC_FILES} 
    ${INC_FILES} 
)

add_library( Audio ${ALL_FILES} .clang-format )
add_library( Audio::Audio ALIAS Audio )

set_target_properties( Audio
    PROPERTIES
        CXX_STANDARD 17
        DEBUG_POSTFIX _d
)

target_include_directories( Audio
    PUBLIC inc
)

if(BUILD_SHARED_LIBS)
    target_compile_definitions( Audio
        PRIVATE Audio_EXPORTS
        INTERFACE Audio_IMPORTS
    )
endif(BUILD_SHARED_LIBS)

if( AUDIO_BUILD_EXAMPLES )
	add_subdirectory( example )
	# Set the startup project.
    set_directory_properties( PROPERTIES
        VS_STARTUP_PROJECT example
	)
endif( AUDIO_BUILD_EXAMPLES)
