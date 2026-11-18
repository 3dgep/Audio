/*
 * Audio v2 - C API
 *
 * A minimalistic audio library for games, built on miniaudio.
 *
 * This header is the stable, binary (ABI) interface of the library. It is written
 * in C99 so that a compiled Audio library (static or shared) can be used by any
 * compiler, any C++ standard library, and in both Debug and Release builds.
 *
 * C++ users should include <Audio/Audio.hpp> instead, which is a thin header-only
 * wrapper around this API.
 *
 * ABI rules (for maintainers):
 *   - Only fixed-size types cross the library boundary. No `bool`, no C `enum`.
 *   - Never change the signature or meaning of a published function. Add a new
 *     function instead.
 *   - Structs that may grow in the future start with a `struct_size` member that
 *     is filled in by an AUDIO_INLINE `*_init` function in this header.
 *   - The library owns all memory it allocates. Objects are destroyed with their
 *     matching `*_destroy` function.
 *
 * Conventions:
 *   - All strings (including file paths) are UTF-8.
 *   - Functions taking an `audio_device*` accept NULL, which means "the default
 *     device". The default device is created on first use, so simple programs
 *     never need to create a device themselves.
 *   - Operations that can fail return an `audio_result`.
 *   - Property setters return nothing and do nothing when passed a NULL object.
 *   - Property getters return 0 (or the documented default) when passed a NULL object.
 *   - Times are in milliseconds, angles are in radians.
 *
 * Threading:
 *   Creating the default device is thread-safe. Unless documented otherwise, other
 *   functions should be called from a single thread (usually the game thread), and
 *   an object must not be destroyed while another thread is using it.
 */
#ifndef AUDIO_H
#define AUDIO_H

#include <stddef.h>
#include <stdint.h>

/* ------------------------------------------------------------------------------
 * Platform and export macros
 * ------------------------------------------------------------------------------ */

#if defined( _WIN32 ) || defined( __CYGWIN__ )
    #define AUDIO_CALL   __cdecl
    #define AUDIO_IMPORT __declspec( dllimport )
    #define AUDIO_EXPORT __declspec( dllexport )
#elif defined( __GNUC__ ) && __GNUC__ >= 4
    #define AUDIO_CALL
    #define AUDIO_IMPORT __attribute__( ( visibility( "default" ) ) )
    #define AUDIO_EXPORT __attribute__( ( visibility( "default" ) ) )
#else
    #define AUDIO_CALL
    #define AUDIO_IMPORT
    #define AUDIO_EXPORT
#endif

/* Audio_EXPORTS is defined by CMake when building the shared library.
 * Audio_IMPORTS is defined for consumers of the shared library. */
#if defined( Audio_EXPORTS )
    #define AUDIO_API AUDIO_EXPORT
#elif defined( Audio_IMPORTS )
    #define AUDIO_API AUDIO_IMPORT
#else
    #define AUDIO_API
#endif

#if defined( __cplusplus )
    #define AUDIO_INLINE inline
#elif defined( _MSC_VER )
    #define AUDIO_INLINE static __inline
#else
    #define AUDIO_INLINE static inline
#endif

#if defined( __cplusplus )
extern "C" {
#endif

/* ------------------------------------------------------------------------------
 * Version
 * ------------------------------------------------------------------------------ */

#define AUDIO_VERSION_MAJOR 2
#define AUDIO_VERSION_MINOR 0
#define AUDIO_VERSION_PATCH 0

#define AUDIO_MAKE_VERSION( major, minor, patch ) \
    ( ( ( uint32_t )( major ) << 22 ) | ( ( uint32_t )( minor ) << 12 ) | ( uint32_t )( patch ) )
#define AUDIO_VERSION_GET_MAJOR( version ) ( ( uint32_t )( version ) >> 22 )
#define AUDIO_VERSION_GET_MINOR( version ) ( ( ( uint32_t )( version ) >> 12 ) & 0x3FFu )
#define AUDIO_VERSION_GET_PATCH( version ) ( ( uint32_t )( version ) & 0xFFFu )

/* The version of the headers you compiled against. */
#define AUDIO_VERSION AUDIO_MAKE_VERSION( AUDIO_VERSION_MAJOR, AUDIO_VERSION_MINOR, AUDIO_VERSION_PATCH )

/* The version of the library you linked against (may differ from AUDIO_VERSION
 * when using a shared library). */
AUDIO_API uint32_t AUDIO_CALL audio_get_version( void );

/* ------------------------------------------------------------------------------
 * Basic types
 * ------------------------------------------------------------------------------ */

typedef uint32_t audio_bool32;
#define AUDIO_TRUE  1u
#define AUDIO_FALSE 0u

typedef struct audio_vec3
{
    float x;
    float y;
    float z;
} audio_vec3;

/* Compile-time layout checks (C99 compatible). */
typedef char audio_static_assert_vec3_size[( sizeof( audio_vec3 ) == 3 * sizeof( float ) ) ? 1 : -1];
typedef char audio_static_assert_float_size[( sizeof( float ) == 4 ) ? 1 : -1];

/* Opaque handles. */
typedef struct audio_device   audio_device;
typedef struct audio_sound    audio_sound;
typedef struct audio_waveform audio_waveform;

/* ------------------------------------------------------------------------------
 * Results
 * ------------------------------------------------------------------------------ */

typedef int32_t audio_result;
#define AUDIO_SUCCESS            0
#define AUDIO_ERROR              -1 /* An unknown error occurred. */
#define AUDIO_INVALID_ARGS       -2 /* An argument was invalid (for example, a NULL out pointer). */
#define AUDIO_INVALID_OPERATION  -3 /* The operation is not valid for this object or state. */
#define AUDIO_OUT_OF_MEMORY      -4
#define AUDIO_FILE_NOT_FOUND     -5
#define AUDIO_INVALID_FILE       -6 /* The file is corrupt or its format is not supported. */
#define AUDIO_NO_BACKEND         -7 /* No audio backend is available on this system. */
#define AUDIO_DEVICE_ERROR       -8 /* The playback device could not be initialized or started. */

/* Returns a human-readable description of a result code. Never returns NULL. */
AUDIO_API const char* AUDIO_CALL audio_result_to_string( audio_result result );

/* ------------------------------------------------------------------------------
 * Logging
 * ------------------------------------------------------------------------------ */

typedef int32_t audio_log_level;
#define AUDIO_LOG_LEVEL_ERROR   1
#define AUDIO_LOG_LEVEL_WARNING 2
#define AUDIO_LOG_LEVEL_INFO    3
#define AUDIO_LOG_LEVEL_DEBUG   4

typedef void( AUDIO_CALL* audio_log_proc )( void* user_data, audio_log_level level, const char* message );

/*
 * Set the function that receives log messages.
 * By default, errors and warnings are written to stderr.
 * Pass NULL to disable logging entirely.
 */
AUDIO_API void AUDIO_CALL audio_set_log_callback( audio_log_proc callback, void* user_data );

/* ------------------------------------------------------------------------------
 * Device
 * ------------------------------------------------------------------------------ */

#define AUDIO_MAX_LISTENERS 4u

typedef struct audio_device_config
{
    uint32_t struct_size;    /* Set by audio_device_config_init. Do not change. */
    uint32_t listener_count; /* Number of listeners [1 .. AUDIO_MAX_LISTENERS]. 0 = AUDIO_MAX_LISTENERS. */
    uint32_t sample_rate;    /* 0 = the playback device's native sample rate. */
    uint32_t channels;       /* 0 = the playback device's native channel count. */
} audio_device_config;

/* Initialize a device config with default values. */
AUDIO_INLINE audio_device_config audio_device_config_init( void )
{
    audio_device_config config;
    config.struct_size    = ( uint32_t )sizeof( audio_device_config );
    config.listener_count = 0;
    config.sample_rate    = 0;
    config.channels       = 0;
    return config;
}

/*
 * Create a device explicitly. Most programs do not need this and can pass NULL
 * to use the default device instead.
 *
 * config may be NULL to use the defaults.
 */
AUDIO_API audio_result AUDIO_CALL audio_device_create( const audio_device_config* config, audio_device** out_device );

/*
 * Release a device created with audio_device_create. Passing NULL does nothing
 * (it does NOT destroy the default device; use audio_shutdown for that).
 *
 * Sounds and waveforms keep their device alive, so it is safe to destroy a device
 * while its sounds still exist. The device is freed when the last one is destroyed.
 */
AUDIO_API void AUDIO_CALL audio_device_destroy( audio_device* device );

/*
 * Release the default device. Like audio_device_destroy, the device is freed once
 * all sounds and waveforms using it have been destroyed.
 *
 * Calling this is optional when the process exits normally. It is required before
 * unloading the shared library with FreeLibrary/dlclose. If the default device is
 * used again after this call, a new default device is created.
 *
 * The C++ wrapper calls this automatically.
 */
AUDIO_API void AUDIO_CALL audio_shutdown( void );

/* Master volume. 0 is silent, 1 is 100%, values above 1 amplify. */
AUDIO_API void AUDIO_CALL  audio_device_set_master_volume( audio_device* device, float volume );
AUDIO_API float AUDIO_CALL audio_device_get_master_volume( audio_device* device );

AUDIO_API uint32_t AUDIO_CALL audio_device_get_sample_rate( audio_device* device );
AUDIO_API uint32_t AUDIO_CALL audio_device_get_channels( audio_device* device );
AUDIO_API uint32_t AUDIO_CALL audio_device_get_listener_count( audio_device* device );

/*
 * Play a sound effect in a "fire and forget" way. Multiple calls may overlap.
 * There is no way to control or stop the sound once it has started.
 */
AUDIO_API audio_result AUDIO_CALL audio_play_sound( audio_device* device, const char* file_path );

/* ------------------------------------------------------------------------------
 * Listener
 *
 * Listeners are identified by an index [0 .. audio_device_get_listener_count).
 * Calls with an out-of-range index are ignored.
 * ------------------------------------------------------------------------------ */

AUDIO_API void AUDIO_CALL       audio_listener_set_position( audio_device* device, uint32_t index, audio_vec3 position );
AUDIO_API audio_vec3 AUDIO_CALL audio_listener_get_position( audio_device* device, uint32_t index );

/* The forward direction of the listener. */
AUDIO_API void AUDIO_CALL       audio_listener_set_direction( audio_device* device, uint32_t index, audio_vec3 direction );
AUDIO_API audio_vec3 AUDIO_CALL audio_listener_get_direction( audio_device* device, uint32_t index );

/* The world up direction for the listener. Default: (0, 1, 0). */
AUDIO_API void AUDIO_CALL       audio_listener_set_world_up( audio_device* device, uint32_t index, audio_vec3 up );
AUDIO_API audio_vec3 AUDIO_CALL audio_listener_get_world_up( audio_device* device, uint32_t index );

/*
 * The listener cone. Sounds inside the inner cone are not attenuated, sounds outside
 * the outer cone are attenuated by outer_gain, and sounds in between are interpolated.
 */
AUDIO_API void AUDIO_CALL audio_listener_set_cone( audio_device* device, uint32_t index, float inner_angle, float outer_angle, float outer_gain );
AUDIO_API void AUDIO_CALL audio_listener_get_cone( audio_device* device, uint32_t index, float* out_inner_angle, float* out_outer_angle, float* out_outer_gain );

/* ------------------------------------------------------------------------------
 * Sound
 * ------------------------------------------------------------------------------ */

typedef uint32_t audio_sound_flags;
#define AUDIO_SOUND_FLAG_NONE              0x0u /* Decode the whole file on load. Best for short sound effects. */
#define AUDIO_SOUND_FLAG_STREAM            0x1u /* Stream the file while playing. Best for long sounds. */
#define AUDIO_SOUND_FLAG_NO_SPATIALIZATION 0x2u /* Ignore 3D position, direction and listeners. */
#define AUDIO_SOUND_FLAG_LOOPING           0x4u /* Start with looping enabled. */

/* Recommended flags for background music. */
#define AUDIO_SOUND_FLAGS_MUSIC ( AUDIO_SOUND_FLAG_STREAM | AUDIO_SOUND_FLAG_NO_SPATIALIZATION )

typedef int32_t audio_attenuation_model;
#define AUDIO_ATTENUATION_MODEL_NONE        0 /* No distance attenuation. */
#define AUDIO_ATTENUATION_MODEL_INVERSE     1 /* Like OpenAL's AL_INVERSE_DISTANCE_CLAMPED. (default) */
#define AUDIO_ATTENUATION_MODEL_LINEAR      2 /* Like OpenAL's AL_LINEAR_DISTANCE_CLAMPED. */
#define AUDIO_ATTENUATION_MODEL_EXPONENTIAL 3 /* Like OpenAL's AL_EXPONENT_DISTANCE_CLAMPED. */

/* Pin a sound to the closest listener (the default). */
#define AUDIO_LISTENER_CLOSEST 0xFFFFFFFFu

/*
 * Load a sound from a file. Supported formats: WAV, MP3, FLAC and OGG Vorbis.
 * device may be NULL to use the default device.
 * On failure, *out_sound is set to NULL.
 */
AUDIO_API audio_result AUDIO_CALL audio_sound_create( audio_device* device, const char* file_path, audio_sound_flags flags, audio_sound** out_sound );

/* Destroy a sound. Passing NULL does nothing. */
AUDIO_API void AUDIO_CALL audio_sound_destroy( audio_sound* sound );

/* Start playing (or resume) the sound. */
AUDIO_API audio_result AUDIO_CALL audio_sound_play( audio_sound* sound );

/* Stop the sound. The cursor is kept, so audio_sound_play resumes from the same position. */
AUDIO_API audio_result AUDIO_CALL audio_sound_stop( audio_sound* sound );

/* Move the cursor to a position (in milliseconds) from the start of the sound. */
AUDIO_API audio_result AUDIO_CALL audio_sound_seek( audio_sound* sound, uint64_t milliseconds );

AUDIO_API float AUDIO_CALL audio_sound_get_duration_seconds( audio_sound* sound );
AUDIO_API float AUDIO_CALL audio_sound_get_cursor_seconds( audio_sound* sound );

AUDIO_API audio_bool32 AUDIO_CALL audio_sound_is_playing( audio_sound* sound );

/* Returns AUDIO_TRUE once a non-looping sound has finished. Looping sounds never end. */
AUDIO_API audio_bool32 AUDIO_CALL audio_sound_is_at_end( audio_sound* sound );

AUDIO_API void AUDIO_CALL         audio_sound_set_looping( audio_sound* sound, audio_bool32 looping );
AUDIO_API audio_bool32 AUDIO_CALL audio_sound_is_looping( audio_sound* sound );

/* Volume. 0 is silent, 1 is 100%, values above 1 amplify. Default: 1. */
AUDIO_API void AUDIO_CALL  audio_sound_set_volume( audio_sound* sound, float volume );
AUDIO_API float AUDIO_CALL audio_sound_get_volume( audio_sound* sound );

/* Pan. -1 is fully left, 0 is centered, 1 is fully right. Default: 0. */
AUDIO_API void AUDIO_CALL  audio_sound_set_pan( audio_sound* sound, float pan );
AUDIO_API float AUDIO_CALL audio_sound_get_pan( audio_sound* sound );

/* Pitch. Must be greater than 0. 1 is the original pitch; larger is higher. Default: 1. */
AUDIO_API void AUDIO_CALL  audio_sound_set_pitch( audio_sound* sound, float pitch );
AUDIO_API float AUDIO_CALL audio_sound_get_pitch( audio_sound* sound );

/*
 * Fade the volume from begin_volume to end_volume over a number of milliseconds.
 * Pass a negative begin_volume to start from the current volume.
 */
AUDIO_API void AUDIO_CALL audio_sound_set_fade( audio_sound* sound, float begin_volume, float end_volume, uint64_t milliseconds );

/*
 * Schedule the sound to start or stop after a delay (in milliseconds from now).
 * The sound must still be started with audio_sound_play.
 */
AUDIO_API void AUDIO_CALL audio_sound_schedule_start( audio_sound* sound, uint64_t delay_milliseconds );
AUDIO_API void AUDIO_CALL audio_sound_schedule_stop( audio_sound* sound, uint64_t delay_milliseconds );

/* --- Spatialization (ignored for sounds created with AUDIO_SOUND_FLAG_NO_SPATIALIZATION) --- */

/* Pin the sound to a listener index, or AUDIO_LISTENER_CLOSEST (the default). */
AUDIO_API void AUDIO_CALL     audio_sound_set_pinned_listener( audio_sound* sound, uint32_t listener_index );
AUDIO_API uint32_t AUDIO_CALL audio_sound_get_pinned_listener( audio_sound* sound );

AUDIO_API void AUDIO_CALL       audio_sound_set_position( audio_sound* sound, audio_vec3 position );
AUDIO_API audio_vec3 AUDIO_CALL audio_sound_get_position( audio_sound* sound );

AUDIO_API void AUDIO_CALL       audio_sound_set_direction( audio_sound* sound, audio_vec3 direction );
AUDIO_API audio_vec3 AUDIO_CALL audio_sound_get_direction( audio_sound* sound );

AUDIO_API void AUDIO_CALL       audio_sound_set_velocity( audio_sound* sound, audio_vec3 velocity );
AUDIO_API audio_vec3 AUDIO_CALL audio_sound_get_velocity( audio_sound* sound );

/* The sound cone. The listener is attenuated by outer_gain when outside the outer cone. */
AUDIO_API void AUDIO_CALL audio_sound_set_cone( audio_sound* sound, float inner_angle, float outer_angle, float outer_gain );
AUDIO_API void AUDIO_CALL audio_sound_get_cone( audio_sound* sound, float* out_inner_angle, float* out_outer_angle, float* out_outer_gain );

AUDIO_API void AUDIO_CALL                    audio_sound_set_attenuation_model( audio_sound* sound, audio_attenuation_model model );
AUDIO_API audio_attenuation_model AUDIO_CALL audio_sound_get_attenuation_model( audio_sound* sound );

/* How quickly the sound attenuates with distance. Default: 1. */
AUDIO_API void AUDIO_CALL  audio_sound_set_rolloff( audio_sound* sound, float rolloff );
AUDIO_API float AUDIO_CALL audio_sound_get_rolloff( audio_sound* sound );

/* Limits for the gain applied by spatialization. Default: [0 .. 1]. */
AUDIO_API void AUDIO_CALL  audio_sound_set_min_gain( audio_sound* sound, float min_gain );
AUDIO_API float AUDIO_CALL audio_sound_get_min_gain( audio_sound* sound );
AUDIO_API void AUDIO_CALL  audio_sound_set_max_gain( audio_sound* sound, float max_gain );
AUDIO_API float AUDIO_CALL audio_sound_get_max_gain( audio_sound* sound );

/* Distance range for attenuation. Default: [1 .. FLT_MAX]. */
AUDIO_API void AUDIO_CALL  audio_sound_set_min_distance( audio_sound* sound, float min_distance );
AUDIO_API float AUDIO_CALL audio_sound_get_min_distance( audio_sound* sound );
AUDIO_API void AUDIO_CALL  audio_sound_set_max_distance( audio_sound* sound, float max_distance );
AUDIO_API float AUDIO_CALL audio_sound_get_max_distance( audio_sound* sound );

/* Strength of the doppler effect. 0 disables it. Default: 1. */
AUDIO_API void AUDIO_CALL  audio_sound_set_doppler_factor( audio_sound* sound, float doppler_factor );
AUDIO_API float AUDIO_CALL audio_sound_get_doppler_factor( audio_sound* sound );

/* ------------------------------------------------------------------------------
 * Waveform
 * ------------------------------------------------------------------------------ */

typedef int32_t audio_waveform_type;
#define AUDIO_WAVEFORM_TYPE_SINE     0
#define AUDIO_WAVEFORM_TYPE_SQUARE   1
#define AUDIO_WAVEFORM_TYPE_TRIANGLE 2
#define AUDIO_WAVEFORM_TYPE_SAWTOOTH 3

/*
 * Create a waveform generator. The waveform is created stopped; call
 * audio_waveform_start to hear it. device may be NULL to use the default device.
 * On failure, *out_waveform is set to NULL.
 */
AUDIO_API audio_result AUDIO_CALL audio_waveform_create( audio_device* device, audio_waveform_type type, float amplitude, float frequency, audio_waveform** out_waveform );

/* Destroy a waveform. Passing NULL does nothing. */
AUDIO_API void AUDIO_CALL audio_waveform_destroy( audio_waveform* waveform );

AUDIO_API audio_result AUDIO_CALL audio_waveform_start( audio_waveform* waveform );
AUDIO_API audio_result AUDIO_CALL audio_waveform_stop( audio_waveform* waveform );
AUDIO_API audio_bool32 AUDIO_CALL audio_waveform_is_playing( audio_waveform* waveform );

AUDIO_API void AUDIO_CALL                audio_waveform_set_type( audio_waveform* waveform, audio_waveform_type type );
AUDIO_API audio_waveform_type AUDIO_CALL audio_waveform_get_type( audio_waveform* waveform );

AUDIO_API void AUDIO_CALL  audio_waveform_set_amplitude( audio_waveform* waveform, float amplitude );
AUDIO_API float AUDIO_CALL audio_waveform_get_amplitude( audio_waveform* waveform );

/* Frequency in Hz. */
AUDIO_API void AUDIO_CALL  audio_waveform_set_frequency( audio_waveform* waveform, float frequency );
AUDIO_API float AUDIO_CALL audio_waveform_get_frequency( audio_waveform* waveform );

#if defined( __cplusplus )
}  /* extern "C" */
#endif

#endif /* AUDIO_H */
