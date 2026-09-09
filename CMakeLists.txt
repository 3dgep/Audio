cmake_minimum_required( VERSION 3.22.1 )

set( AUDIO_VERSION_MAJOR 1 )
set( AUDIO_VERSION_MINOR 1 )
set( AUDIO_VERSION_PATCH 0 )
set( AUDIO_VERSION "${AUDIO_VERSION_MAJOR}.${AUDIO_VERSION_MINOR}.${AUDIO_VERSION_PATCH}" )

option( AUDIO_BUILD_EXAMPLES "Include the example projects." OFF )
option( AUDIO_INSTALL_CONFIG_SUBDIRS "Install libraries into per-configuration subdirectories." ON )
option( BUILD_SHARED_LIBS "Build Audio library as a shared library (DLL)." OFF )

# Use solution folders in Visual Studio.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project( Audio VERSION ${AUDIO_VERSION} LANGUAGES C CXX )

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
)

target_include_directories( Audio
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
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

# Install configuration
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Install the library
if(AUDIO_INSTALL_CONFIG_SUBDIRS)
    set(AUDIO_LIB_DEST ${CMAKE_INSTALL_LIBDIR}/$<CONFIG>)
    set(AUDIO_BIN_DEST ${CMAKE_INSTALL_BINDIR}/$<CONFIG>)
else()
    set(AUDIO_LIB_DEST ${CMAKE_INSTALL_LIBDIR})
    set(AUDIO_BIN_DEST ${CMAKE_INSTALL_BINDIR})
endif()

install(TARGETS Audio
    EXPORT AudioTargets
    ARCHIVE DESTINATION ${AUDIO_LIB_DEST}
    LIBRARY DESTINATION ${AUDIO_LIB_DEST}
    RUNTIME DESTINATION ${AUDIO_BIN_DEST}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install headers
install(DIRECTORY inc/Audio
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hpp"
)

# Install export targets
install(EXPORT AudioTargets
    FILE AudioTargets.cmake
    NAMESPACE Audio::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Audio
)

# Create version file
write_basic_package_version_file(
    AudioConfigVersion.cmake
    VERSION ${AUDIO_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Create config file
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/AudioConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/AudioConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Audio
)

# Install config files
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/AudioConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/AudioConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Audio
)

# Packaging configuration
set(CPACK_PACKAGE_NAME "Audio")
set(CPACK_PACKAGE_VERSION "${AUDIO_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Audio library")
set(CPACK_PACKAGE_VENDOR "3dgep")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Audio")

if(BUILD_SHARED_LIBS)
    set(AUDIO_LIB_TYPE "shared")
else()
    set(AUDIO_LIB_TYPE "static")
endif()

set(CPACK_SYSTEM_NAME "${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")

if(MSVC)
    set(AUDIO_COMPILER "msvc-v${MSVC_TOOLSET_VERSION}")
else()
    string(TOLOWER "${CMAKE_CXX_COMPILER_ID}" AUDIO_COMPILER_ID)
    set(AUDIO_COMPILER "${AUDIO_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}")
endif()

set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_SYSTEM_NAME}-${AUDIO_COMPILER}-${AUDIO_LIB_TYPE}")

include(CPack)
