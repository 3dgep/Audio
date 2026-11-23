cmake_minimum_required( VERSION 3.22.1 )

set( AUDIO_VERSION_MAJOR 2 )
set( AUDIO_VERSION_MINOR 1 )
set( AUDIO_VERSION_PATCH 1 )
set( AUDIO_VERSION "${AUDIO_VERSION_MAJOR}.${AUDIO_VERSION_MINOR}.${AUDIO_VERSION_PATCH}" )

option( AUDIO_BUILD_EXAMPLES "Include the example projects." OFF )
option( AUDIO_INSTALL_CONFIG_SUBDIRS "Install libraries into per-configuration subdirectories." ON )
option( BUILD_SHARED_LIBS "Build Audio library as a shared library (DLL)." OFF )
option( AUDIO_USE_SYSTEM_DEPS "Use installed miniaudio and stb_vorbis instead of the copies in src/." OFF )
option( AUDIO_MSVC_ANY_CRT "MSVC static library: build with /MT /Zl so it links with any C runtime (/MD, /MDd, /MT, /MTd). Turn off to use CMAKE_MSVC_RUNTIME_LIBRARY." ON )

option( AUDIO_BUILD_WEB "Build for the web (WebAssembly) with Emscripten. Downloads the Emscripten SDK if it is not installed." OFF )

# The web build needs the Emscripten toolchain, which must be set before project().
include( cmake/Emscripten.cmake )

# Use solution folders in Visual Studio.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project( Audio VERSION ${AUDIO_VERSION} LANGUAGES C CXX )

if( CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    # Make sure DLL and EXE targets go to the same directory, with a subdirectory per
    # configuration (lib/Debug, lib/Release, ...), so Debug and Release builds don't
    # overwrite each other. Because the paths contain a generator expression,
    # multi-config generators (Visual Studio, Ninja Multi-Config) don't append another
    # configuration subdirectory, so all generators produce the same layout.
    # Web builds go to lib/web and bin/web, so they don't mix with desktop builds.
    if(EMSCRIPTEN)
        set(AUDIO_OUTPUT_SUBDIR web/)
    endif()
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib/${AUDIO_OUTPUT_SUBDIR}$<CONFIG>)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${AUDIO_OUTPUT_SUBDIR}$<CONFIG>)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${AUDIO_OUTPUT_SUBDIR}$<CONFIG>)
endif()

# Enable multi-processor builds in MSVC.
if(MSVC)
    add_compile_options(/MP)
endif(MSVC)

set( INC_FILES
    # The C API (the library's binary interface).
    include/Audio/audio.h
    # Header-only C++ wrapper. Audio.hpp includes all of the others.
    include/Audio/Audio.hpp
    include/Audio/Config.hpp
    include/Audio/Device.hpp
    include/Audio/Group.hpp
    include/Audio/Listener.hpp
    include/Audio/Sound.hpp
    include/Audio/Vector.hpp
    include/Audio/Waveform.hpp
)

# The library itself is written in C99, so that a prebuilt library does not depend
# on a particular C++ standard library, compiler version, or Debug/Release setting.
set( AUDIO_SRC_FILES
    src/audio_internal.h
    src/audio_device.c
    src/audio_group.c
    src/audio_sound.c
    src/audio_waveform.c
)

if( AUDIO_USE_SYSTEM_DEPS )
    # miniaudio and stb_vorbis are single-file libraries, installed as headers by
    # e.g. vcpkg's miniaudio and stb ports. Their implementations are compiled here.
    find_path( MINIAUDIO_INCLUDE_DIR miniaudio.h REQUIRED )
    find_path( STB_VORBIS_INCLUDE_DIR stb_vorbis.c REQUIRED )
    set( THIRD_PARTY_FILES
        src/miniaudio.c
        ${STB_VORBIS_INCLUDE_DIR}/stb_vorbis.c
    )
else()
    set( THIRD_PARTY_FILES
        src/miniaudio.c
        src/miniaudio.h
        src/stb_vorbis.c
    )
endif()

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
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if( AUDIO_USE_SYSTEM_DEPS )
    target_compile_definitions( Audio PRIVATE AUDIO_USE_SYSTEM_DEPS )
    target_include_directories( Audio PRIVATE ${MINIAUDIO_INCLUDE_DIR} ${STB_VORBIS_INCLUDE_DIR} )
endif()

# Warnings for our own code (not for the third-party files).
set_source_files_properties( ${AUDIO_SRC_FILES}
    PROPERTIES
        COMPILE_OPTIONS "$<$<C_COMPILER_ID:MSVC>:/W4>;$<$<NOT:$<C_COMPILER_ID:MSVC>>:-Wall>;$<$<NOT:$<C_COMPILER_ID:MSVC>>:-Wextra>"
)

if( BUILD_SHARED_LIBS )
    # CMake defines Audio_EXPORTS when building the shared library.
    target_compile_definitions( Audio INTERFACE Audio_IMPORTS )
elseif( MSVC AND AUDIO_MSVC_ANY_CRT )
    # Make the static library link with any C runtime the consumer uses (/MD, /MDd,
    # /MT or /MTd), in both Debug and Release builds:
    #  - /MT makes calls to CRT functions plain symbol references (e.g. `calloc`
    #    instead of `__imp_calloc`), which both the static and DLL runtimes provide.
    #  - /Zl omits the default runtime library name, so the consumer's runtime is used.
    set_target_properties( Audio PROPERTIES MSVC_RUNTIME_LIBRARY MultiThreaded )
    target_compile_options( Audio PRIVATE /Zl )
endif()

# Emscripten: no threads. miniaudio plays through Web Audio without them, and
# -pthread would require a web server that sends the COOP/COEP headers.
if( UNIX AND NOT EMSCRIPTEN )
    find_package( Threads REQUIRED )
    target_link_libraries( Audio PUBLIC Threads::Threads m ${CMAKE_DL_LIBS} )
endif()

if( AUDIO_BUILD_EXAMPLES )
	add_subdirectory( examples )
	# Set the startup project.
    set_directory_properties( PROPERTIES
        VS_STARTUP_PROJECT game
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
install(DIRECTORY include/Audio
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

# The target architecture, with the same names on all platforms (x64, x86, arm64).
# CMAKE_SYSTEM_PROCESSOR is the build machine's processor on Windows, so prefer the
# architecture the compiler targets.
if(CMAKE_C_COMPILER_ARCHITECTURE_ID)
    set(AUDIO_ARCH "${CMAKE_C_COMPILER_ARCHITECTURE_ID}")
else()
    set(AUDIO_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
endif()
string(TOLOWER "${AUDIO_ARCH}" AUDIO_ARCH)
if(AUDIO_ARCH MATCHES "^(x64|amd64|x86_64)$")
    set(AUDIO_ARCH "x64")
elseif(AUDIO_ARCH MATCHES "^(x86|i[3-6]86)$")
    set(AUDIO_ARCH "x86")
elseif(AUDIO_ARCH MATCHES "^(arm64|aarch64)$")
    set(AUDIO_ARCH "arm64")
endif()

string(TOLOWER "${CMAKE_SYSTEM_NAME}" AUDIO_SYSTEM_NAME)
set(CPACK_SYSTEM_NAME "${AUDIO_SYSTEM_NAME}-${AUDIO_ARCH}")

# The compiler is not part of the name: the library only exposes a C API, so it does
# not depend on the compiler's C++ ABI. (Release packages are built with the oldest
# supported Visual Studio, because MSVC can't link static libraries built by a newer
# toolset.)
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_SYSTEM_NAME}-${AUDIO_LIB_TYPE}")

include(CPack)
