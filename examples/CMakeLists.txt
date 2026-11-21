cmake_minimum_required( VERSION 4.0...4.2 )

# The examples load their audio files from the assets folder, so run them with
# that folder as the working directory (or pass -cwd path/to/examples/assets).
set( AUDIO_EXAMPLE_ASSETS ${CMAKE_CURRENT_SOURCE_DIR}/assets )

# game: a small game that runs on the desktop and in a web browser. The game
# (main.cpp) is the same everywhere; only the platform layer is different.
set( GAME_SOURCES
    game/main.cpp
    game/font.h
    game/platform.h
)

if( EMSCRIPTEN )
    add_executable( game ${GAME_SOURCES} game/platform_web.cpp game/shell.html )
    target_compile_features( game PRIVATE cxx_std_20 )
    target_link_libraries( game PRIVATE Audio )

    # Build game.html (with game.js, game.wasm and game.data), and pack the
    # audio files into game.data, because a web page can't load files from disk.
    set( GAME_ASSETS chiptune_theme.ogg paddle.wav wall.wav brick.wav lose.wav )
    set( GAME_SHELL ${CMAKE_CURRENT_SOURCE_DIR}/game/shell.html )
    set_target_properties( game PROPERTIES SUFFIX ".html" FOLDER examples )
    target_link_options( game PRIVATE
        "SHELL:--shell-file ${GAME_SHELL}"
        -sALLOW_MEMORY_GROWTH=1
    )
    foreach( asset ${GAME_ASSETS} )
        target_link_options( game PRIVATE "SHELL:--preload-file ${AUDIO_EXAMPLE_ASSETS}/${asset}@/${asset}" )
        set_property( TARGET game APPEND PROPERTY LINK_DEPENDS ${AUDIO_EXAMPLE_ASSETS}/${asset} )
    endforeach()
    set_property( TARGET game APPEND PROPERTY LINK_DEPENDS ${GAME_SHELL} )

    # The other examples are desktop only: they run in a loop that never returns,
    # which would freeze a web page.
    return()
endif()

# On the desktop, the window is made with fenster (game/fenster), which uses
# Win32 on Windows, Cocoa on macOS and X11 on Linux.
set( GAME_PLATFORM_LIBS )
set( BUILD_GAME ON )
if( WIN32 )
    set( GAME_PLATFORM_LIBS user32 gdi32 winmm )
elseif( APPLE )
    set( GAME_PLATFORM_LIBS "-framework Cocoa" )
else()
    find_package( X11 )
    if( X11_FOUND )
        set( GAME_PLATFORM_LIBS X11::X11 )
    else()
        message( STATUS "X11 was not found: the game example is not built (install libx11-dev)." )
        set( BUILD_GAME OFF )
    endif()
endif()

if( BUILD_GAME )
    add_executable( game ${GAME_SOURCES}
        game/platform_desktop.cpp
        game/fenster.c
        game/fenster/fenster.h
    )
    target_compile_features( game PRIVATE cxx_std_20 )
    target_link_libraries( game PRIVATE ${GAME_PLATFORM_LIBS} )
    # fenster passes char strings to the Win32 API, so don't use the UNICODE API.
    target_compile_definitions( game PRIVATE $<$<BOOL:${WIN32}>:_MBCS> )
endif()

# getting_started: narration, music and a waveform sound effect.
add_executable( getting_started
    getting_started/main.cpp
    getting_started/MarioCoin.hpp
    getting_started/MarioCoin.cpp
)
target_compile_features( getting_started PRIVATE cxx_std_20 )

# fade: fading sounds in and out.
add_executable( fade fade/main.cpp )
target_compile_features( fade PRIVATE cxx_std_20 )

# c_api: using the library from C.
add_executable( c_api c_api/main.c )
set_target_properties( c_api PROPERTIES C_STANDARD 99 )

set( EXAMPLES getting_started fade c_api )
if( BUILD_GAME )
    list( APPEND EXAMPLES game )
endif()

foreach( example ${EXAMPLES} )
    target_link_libraries( ${example} PRIVATE Audio )
    set_target_properties( ${example}
        PROPERTIES
            FOLDER examples
            DEBUGGER_WORKING_DIRECTORY ${AUDIO_EXAMPLE_ASSETS}
    )
endforeach()
