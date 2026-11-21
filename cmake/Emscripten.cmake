# Set up the Emscripten SDK for a web (WebAssembly) build: AUDIO_BUILD_WEB=ON.
#
# This file is included before project(), because the toolchain file must be set
# before the compilers are found. It looks for an installed Emscripten SDK, and
# downloads one into out/emsdk if there is none, so a web build works without
# installing anything first. Desktop builds don't use this file.
#
# The Emscripten SDK is found, in this order:
#  1. CMAKE_TOOLCHAIN_FILE, if it is already set (for example by emcmake).
#  2. The EMSDK environment variable (set by emsdk_env).
#  3. The EMSCRIPTEN environment variable (the emscripten folder of an SDK).
#  4. emcc on the PATH (also Homebrew and Linux packages).
#  5. An emsdk folder in one of the usual places (~/emsdk, C:/emsdk, /opt/emsdk).
#  6. Otherwise, emsdk AUDIO_EMSDK_VERSION is downloaded and installed into
#     AUDIO_EMSDK_DIR (out/emsdk). This downloads about 1 GB, once.
#
# On Windows, Ninja is used from the PATH or else from Visual Studio.

if( NOT AUDIO_BUILD_WEB )
    return()
endif()

set( AUDIO_EMSDK_VERSION "7.0.0" CACHE STRING "Version of the Emscripten SDK to download if none is installed." )
set( AUDIO_EMSDK_DIR "${CMAKE_CURRENT_LIST_DIR}/../out/emsdk" CACHE PATH "Folder to download the Emscripten SDK to if none is installed." )

# Emscripten builds with Ninja or Make. Visual Studio comes with Ninja, but doesn't
# add it to the PATH outside the Developer Command Prompt. Find it first: the
# download below uses the same generator.
# Look again if the cached Ninja was removed (for example, when Visual Studio was updated).
if( CMAKE_HOST_WIN32 AND CMAKE_GENERATOR MATCHES "Ninja" AND NOT EXISTS "${CMAKE_MAKE_PROGRAM}" )
    find_program( _audio_ninja ninja NO_CACHE )
    if( NOT _audio_ninja )
        set( _audio_vswhere "$ENV{ProgramFiles\(x86\)}/Microsoft Visual Studio/Installer/vswhere.exe" )
        if( EXISTS "${_audio_vswhere}" )
            execute_process(
                COMMAND "${_audio_vswhere}" -latest -prerelease -products * -property installationPath
                OUTPUT_VARIABLE _audio_vs_path
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            set( _audio_ninja "${_audio_vs_path}/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe" )
            if( EXISTS "${_audio_ninja}" )
                file( TO_CMAKE_PATH "${_audio_ninja}" _audio_ninja )
                set( CMAKE_MAKE_PROGRAM "${_audio_ninja}" CACHE FILEPATH "Ninja" FORCE )
            endif()
        endif()
    endif()
endif()

set( _audio_toolchain_subpath "cmake/Modules/Platform/Emscripten.cmake" )

# Find an installed Emscripten SDK.
if( NOT CMAKE_TOOLCHAIN_FILE )
    # Folders that contain emcc (upstream/emscripten in an emsdk folder).
    set( _audio_emscripten_dirs )
    if( DEFINED ENV{EMSDK} )
        list( APPEND _audio_emscripten_dirs "$ENV{EMSDK}/upstream/emscripten" )
    endif()
    if( DEFINED ENV{EMSCRIPTEN} )
        list( APPEND _audio_emscripten_dirs "$ENV{EMSCRIPTEN}" )
    endif()

    find_program( _audio_emcc NAMES emcc emcc.bat NO_CACHE )
    if( _audio_emcc )
        # Follow symlinks, for example /usr/bin/emcc -> /usr/share/emscripten/emcc.
        file( REAL_PATH "${_audio_emcc}" _audio_emcc )
        get_filename_component( _audio_emcc_dir "${_audio_emcc}" DIRECTORY )
        list( APPEND _audio_emscripten_dirs "${_audio_emcc_dir}" "${_audio_emcc_dir}/../libexec" )
    endif()

    if( DEFINED ENV{HOME} )
        list( APPEND _audio_emscripten_dirs "$ENV{HOME}/emsdk/upstream/emscripten" )
    endif()
    if( DEFINED ENV{USERPROFILE} )
        list( APPEND _audio_emscripten_dirs "$ENV{USERPROFILE}/emsdk/upstream/emscripten" )
    endif()
    list( APPEND _audio_emscripten_dirs
        "C:/emsdk/upstream/emscripten"
        "/opt/emsdk/upstream/emscripten"
        "/usr/lib/emscripten"
        "/usr/share/emscripten"
    )

    foreach( _audio_dir IN LISTS _audio_emscripten_dirs )
        if( EXISTS "${_audio_dir}/${_audio_toolchain_subpath}" )
            file( REAL_PATH "${_audio_dir}/${_audio_toolchain_subpath}" _audio_toolchain )
            set( CMAKE_TOOLCHAIN_FILE "${_audio_toolchain}" CACHE FILEPATH "Toolchain file" FORCE )
            message( STATUS "Audio: Using the Emscripten SDK in ${_audio_dir}" )
            break()
        endif()
    endforeach()
endif()

# None found: download and install the Emscripten SDK.
if( NOT CMAKE_TOOLCHAIN_FILE )
    get_filename_component( AUDIO_EMSDK_DIR "${AUDIO_EMSDK_DIR}" ABSOLUTE )
    set( _audio_emscripten_dir "${AUDIO_EMSDK_DIR}/upstream/emscripten" )

    # The version that is installed in AUDIO_EMSDK_DIR, if any.
    set( _audio_installed_version "" )
    if( EXISTS "${_audio_emscripten_dir}/emscripten-version.txt" )
        file( READ "${_audio_emscripten_dir}/emscripten-version.txt" _audio_installed_version )
        string( REGEX REPLACE "[\" \r\n]" "" _audio_installed_version "${_audio_installed_version}" )
    endif()

    if( NOT _audio_installed_version STREQUAL AUDIO_EMSDK_VERSION )
        message( STATUS "Audio: No Emscripten SDK found. Downloading emsdk ${AUDIO_EMSDK_VERSION} to ${AUDIO_EMSDK_DIR} (about 1 GB, this only happens once)." )

        # The emsdk tool, a few hundred kB. It downloads the SDK itself.
        include( FetchContent )
        FetchContent_Declare( audio_emsdk
            URL "https://github.com/emscripten-core/emsdk/archive/refs/tags/${AUDIO_EMSDK_VERSION}.tar.gz"
            SOURCE_DIR "${AUDIO_EMSDK_DIR}"
            # emsdk is a tool, not a CMake project: download it, but don't add it.
            SOURCE_SUBDIR "no-cmake-project"
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        FetchContent_MakeAvailable( audio_emsdk )

        if( CMAKE_HOST_WIN32 )
            set( _audio_emsdk "${AUDIO_EMSDK_DIR}/emsdk.bat" )
        else()
            set( _audio_emsdk "${AUDIO_EMSDK_DIR}/emsdk" )
        endif()

        foreach( _audio_command install activate )
            execute_process(
                COMMAND "${_audio_emsdk}" ${_audio_command} ${AUDIO_EMSDK_VERSION}
                WORKING_DIRECTORY "${AUDIO_EMSDK_DIR}"
                RESULT_VARIABLE _audio_result
            )
            if( NOT _audio_result EQUAL 0 )
                message( FATAL_ERROR "Audio: 'emsdk ${_audio_command} ${AUDIO_EMSDK_VERSION}' failed. emsdk needs Python 3 on the PATH." )
            endif()
        endforeach()
    endif()

    set( CMAKE_TOOLCHAIN_FILE "${_audio_emscripten_dir}/${_audio_toolchain_subpath}" CACHE FILEPATH "Toolchain file" FORCE )
    message( STATUS "Audio: Using the Emscripten SDK in ${AUDIO_EMSDK_DIR}" )
endif()
