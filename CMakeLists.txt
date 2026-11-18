cmake_minimum_required( VERSION 3.22.1 )

set( AUDIO_VERSION_MAJOR 2 )
set( AUDIO_VERSION_MINOR 0 )
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
    # The C API (the library's binary interface).
    inc/Audio/audio.h
    # Header-only C++ wrapper. Audio.hpp includes all of the others.
    inc/Audio/Audio.hpp
    inc/Audio/Config.hpp
    inc/Audio/Device.hpp
    inc/Audio/Listener.hpp
    inc/Audio/Sound.hpp
    inc/Audio/Vector.hpp
    inc/Audio/Waveform.hpp
)

# The library itself is written in C99, so that a prebuilt library does not depend
# on a particular C++ standard library, compiler version, or Debug/Release setting.
set( AUDIO_SRC_FILES
    src/audio_internal.h
    src/audio_device.c
    src/audio_sound.c
    src/audio_waveform.c
)

set( THIRD_PARTY_FILES
    src/miniaudio.c
    src/miniaudio.h
    src/stb_vorbis.c
)

add_library( Audio ${INC_FILES} ${AUDIO_SRC_FILES} ${THIRD_PARTY_FILES} .clang-format )
add_library( Audio::Audio ALIAS Audio )

set_target_properties( Audio
    PROPERTIES
        C_STANDARD 99
        C_STANDARD_REQUIRED ON
        # Only export the AUDIO_API functions from shared libraries.
        C_VISIBILITY_PRESET hidden
)

# The C++ wrapper (Audio.hpp) requires C++17 for std::filesystem.
target_compile_features( Audio INTERFACE cxx_std_17 )

target_include_directories( Audio
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Warnings for our own code (not for the third-party files).
set_source_files_properties( ${AUDIO_SRC_FILES}
    PROPERTIES
        COMPILE_OPTIONS "$<$<C_COMPILER_ID:MSVC>:/W4>;$<$<NOT:$<C_COMPILER_ID:MSVC>>:-Wall>;$<$<NOT:$<C_COMPILER_ID:MSVC>>:-Wextra>"
)

if( BUILD_SHARED_LIBS )
    # CMake defines Audio_EXPORTS when building the shared library.
    target_compile_definitions( Audio INTERFACE Audio_IMPORTS )
elseif( MSVC )
    # Make the static library link with any C runtime the consumer uses (/MD, /MDd,
    # /MT or /MTd), in both Debug and Release builds:
    #  - /MT makes calls to CRT functions plain symbol references (e.g. `calloc`
    #    instead of `__imp_calloc`), which both the static and DLL runtimes provide.
    #  - /Zl omits the default runtime library name, so the consumer's runtime is used.
    set_target_properties( Audio PROPERTIES MSVC_RUNTIME_LIBRARY MultiThreaded )
    target_compile_options( Audio PRIVATE /Zl )
endif()

if( UNIX )
    find_package( Threads REQUIRED )
    target_link_libraries( Audio PUBLIC Threads::Threads m ${CMAKE_DL_LIBS} )
endif()

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
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
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
