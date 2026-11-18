/*
 * Demonstrates the C API.
 *
 * Plays background music, then a short rising sine sweep on top of it, then fades
 * the music out. Run it with the examples/assets folder as the working directory:
 *
 *     c_api -cwd path/to/examples/assets
 */
#include <Audio/audio.h>

#include <stdio.h>
#include <string.h>

#if defined( _WIN32 )
    #define WIN32_LEAN_AND_MEAN
    #include <direct.h>
    #include <windows.h>
    #define chdir _chdir
static void sleep_ms( unsigned ms )
{
    Sleep( ms );
}
#else
    #include <time.h>
    #include <unistd.h>
static void sleep_ms( unsigned ms )
{
    struct timespec ts;
    ts.tv_sec  = ms / 1000;
    ts.tv_nsec = ( long )( ms % 1000 ) * 1000000L;
    nanosleep( &ts, NULL );
}
#endif

int main( int argc, char* argv[] )
{
    audio_sound*    music    = NULL;
    audio_waveform* waveform = NULL;
    audio_result    result;
    int             i;

    for ( i = 1; i + 1 < argc; ++i )
    {
        if ( strcmp( argv[i], "-cwd" ) == 0 )
            chdir( argv[++i] );
    }

    printf( "Audio library version %u.%u.%u\n",
            AUDIO_VERSION_GET_MAJOR( audio_get_version() ),
            AUDIO_VERSION_GET_MINOR( audio_get_version() ),
            AUDIO_VERSION_GET_PATCH( audio_get_version() ) );

    /* NULL means "use the default device". No device setup is needed. */
    result = audio_sound_create( NULL, "Rondo_Alla_Turka.ogg", AUDIO_SOUND_FLAGS_MUSIC | AUDIO_SOUND_FLAG_LOOPING, &music );
    if ( result != AUDIO_SUCCESS )
        printf( "Could not load the music: %s\n", audio_result_to_string( result ) );

    audio_sound_set_volume( music, 0.3f );
    audio_sound_play( music );

    printf( "Playing music...\n" );
    sleep_ms( 2000 );

    /* Functions are safe to call with NULL objects, so this works even if loading failed. */
    audio_waveform_create( NULL, AUDIO_WAVEFORM_TYPE_SINE, 0.1f, 220.0f, &waveform );
    audio_waveform_start( waveform );

    printf( "Sine sweep from 220 Hz to 880 Hz...\n" );
    for ( i = 0; i <= 100; ++i )
    {
        audio_waveform_set_frequency( waveform, 220.0f + 6.6f * (float)i );
        sleep_ms( 20 );
    }

    audio_waveform_stop( waveform );
    audio_waveform_destroy( waveform );

    printf( "Fading out...\n" );
    audio_sound_set_fade( music, -1.0f, 0.0f, 1500 );
    sleep_ms( 1600 );

    audio_sound_destroy( music );

    /* Optional when the program exits normally, but good practice. */
    audio_shutdown();

    printf( "Done.\n" );
    return 0;
}
