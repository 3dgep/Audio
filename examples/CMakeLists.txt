cmake_minimum_required( VERSION 4.0...4.2 )

# The examples load their audio files from the assets folder, so run them with
# that folder as the working directory (or pass -cwd path/to/examples/assets).
set( AUDIO_EXAMPLE_ASSETS ${CMAKE_CURRENT_SOURCE_DIR}/assets )

# These examples are desktop only: they run in a loop that never returns, which
# would freeze a web page.
if( EMSCRIPTEN )
    return()
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

foreach( example getting_started fade c_api )
    target_link_libraries( ${example} PRIVATE Audio )
    set_target_properties( ${example}
        PROPERTIES
            FOLDER examples
            DEBUGGER_WORKING_DIRECTORY ${AUDIO_EXAMPLE_ASSETS}
    )
endforeach()
