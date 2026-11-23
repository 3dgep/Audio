/*
 * Audio v2 - version, results, logging, devices and listeners.
 */
#include "audio_internal.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#if defined( _WIN32 )
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif

/* ------------------------------------------------------------------------------
 * Version
 * ------------------------------------------------------------------------------ */

uint32_t AUDIO_CALL audio_get_version( void )
{
    return AUDIO_VERSION;
}

/* ------------------------------------------------------------------------------
 * Results
 * ------------------------------------------------------------------------------ */

const char* AUDIO_CALL audio_result_to_string( audio_result result )
{
    switch ( result )
    {
    case AUDIO_SUCCESS:
        return "Success";
    case AUDIO_ERROR:
        return "Unknown error";
    case AUDIO_INVALID_ARGS:
        return "Invalid argument";
    case AUDIO_INVALID_OPERATION:
        return "Invalid operation";
    case AUDIO_OUT_OF_MEMORY:
        return "Out of memory";
    case AUDIO_FILE_NOT_FOUND:
        return "File not found";
    case AUDIO_INVALID_FILE:
        return "Invalid or unsupported file";
    case AUDIO_NO_BACKEND:
        return "No audio backend available";
    case AUDIO_DEVICE_ERROR:
        return "Audio device error";
    default:
        return "Unknown result";
    }
}

audio_result audio__result( ma_result result )
{
    switch ( result )
    {
    case MA_SUCCESS:
        return AUDIO_SUCCESS;
    case MA_INVALID_ARGS:
        return AUDIO_INVALID_ARGS;
    case MA_INVALID_OPERATION:
        return AUDIO_INVALID_OPERATION;
    case MA_OUT_OF_MEMORY:
        return AUDIO_OUT_OF_MEMORY;
    case MA_DOES_NOT_EXIST:
        return AUDIO_FILE_NOT_FOUND;
    case MA_INVALID_FILE:
    case MA_INVALID_DATA:
    case MA_FORMAT_NOT_SUPPORTED:
    case MA_NOT_IMPLEMENTED:
        return AUDIO_INVALID_FILE;
    case MA_NO_BACKEND:
        return AUDIO_NO_BACKEND;
    case MA_NO_DEVICE:
    case MA_DEVICE_NOT_INITIALIZED:
    case MA_FAILED_TO_INIT_BACKEND:
    case MA_FAILED_TO_OPEN_BACKEND_DEVICE:
    case MA_FAILED_TO_START_BACKEND_DEVICE:
        return AUDIO_DEVICE_ERROR;
    default:
        return AUDIO_ERROR;
    }
}

/* ------------------------------------------------------------------------------
 * Logging
 * ------------------------------------------------------------------------------ */

static void AUDIO_CALL audio__default_log( void* user_data, audio_log_level level, const char* message )
{
    (void)user_data;

    if ( level == AUDIO_LOG_LEVEL_ERROR )
        fprintf( stderr, "[Audio] Error: %s\n", message );
    else if ( level == AUDIO_LOG_LEVEL_WARNING )
        fprintf( stderr, "[Audio] Warning: %s\n", message );
}

static audio_log_proc g_log_proc      = audio__default_log;
static void*          g_log_user_data = NULL;

void AUDIO_CALL audio_set_log_callback( audio_log_proc callback, void* user_data )
{
    g_log_proc      = callback;
    g_log_user_data = user_data;
}

void audio__log( audio_log_level level, const char* format, ... )
{
    char    message[1024];
    va_list args;

    if ( !g_log_proc )
        return;

    va_start( args, format );
    vsnprintf( message, sizeof( message ), format, args );
    va_end( args );

    g_log_proc( g_log_user_data, level, message );
}

/* ------------------------------------------------------------------------------
 * UTF-8 virtual file system (Windows only)
 * ------------------------------------------------------------------------------ */

#if defined( _WIN32 )

/* Returns a malloc'd wide string, or NULL if path is not valid UTF-8. */
static wchar_t* audio__utf8_to_wide( const char* path )
{
    wchar_t* wide;
    int      length = MultiByteToWideChar( CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, NULL, 0 );

    if ( length <= 0 )
        return NULL;

    wide = (wchar_t*)malloc( (size_t)length * sizeof( wchar_t ) );
    if ( !wide )
        return NULL;

    if ( MultiByteToWideChar( CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, wide, length ) <= 0 )
    {
        free( wide );
        return NULL;
    }

    return wide;
}

static ma_vfs* audio__inner_vfs( ma_vfs* vfs )
{
    return &( (audio_utf8_vfs*)vfs )->inner;
}

static ma_result audio__vfs_open( ma_vfs* vfs, const char* path, ma_uint32 mode, ma_vfs_file* file )
{
    ma_result result;
    wchar_t*  wide = audio__utf8_to_wide( path );

    if ( !wide )
    {
        /* Not valid UTF-8, for example a string literal in the system code page
         * compiled without /utf-8. Fall back to the system code page. */
        return ma_vfs_open( audio__inner_vfs( vfs ), path, mode, file );
    }

    result = ma_vfs_open_w( audio__inner_vfs( vfs ), wide, mode, file );
    free( wide );
    return result;
}

static ma_result audio__vfs_open_w( ma_vfs* vfs, const wchar_t* path, ma_uint32 mode, ma_vfs_file* file )
{
    return ma_vfs_open_w( audio__inner_vfs( vfs ), path, mode, file );
}

static ma_result audio__vfs_close( ma_vfs* vfs, ma_vfs_file file )
{
    return ma_vfs_close( audio__inner_vfs( vfs ), file );
}

static ma_result audio__vfs_read( ma_vfs* vfs, ma_vfs_file file, void* dst, size_t size, size_t* bytes_read )
{
    return ma_vfs_read( audio__inner_vfs( vfs ), file, dst, size, bytes_read );
}

static ma_result audio__vfs_write( ma_vfs* vfs, ma_vfs_file file, const void* src, size_t size, size_t* bytes_written )
{
    return ma_vfs_write( audio__inner_vfs( vfs ), file, src, size, bytes_written );
}

static ma_result audio__vfs_seek( ma_vfs* vfs, ma_vfs_file file, ma_int64 offset, ma_seek_origin origin )
{
    return ma_vfs_seek( audio__inner_vfs( vfs ), file, offset, origin );
}

static ma_result audio__vfs_tell( ma_vfs* vfs, ma_vfs_file file, ma_int64* cursor )
{
    return ma_vfs_tell( audio__inner_vfs( vfs ), file, cursor );
}

static ma_result audio__vfs_info( ma_vfs* vfs, ma_vfs_file file, ma_file_info* info )
{
    return ma_vfs_info( audio__inner_vfs( vfs ), file, info );
}

static ma_result audio__vfs_init( audio_utf8_vfs* vfs )
{
    vfs->cb.onOpen  = audio__vfs_open;
    vfs->cb.onOpenW = audio__vfs_open_w;
    vfs->cb.onClose = audio__vfs_close;
    vfs->cb.onRead  = audio__vfs_read;
    vfs->cb.onWrite = audio__vfs_write;
    vfs->cb.onSeek  = audio__vfs_seek;
    vfs->cb.onTell  = audio__vfs_tell;
    vfs->cb.onInfo  = audio__vfs_info;

    return ma_default_vfs_init( &vfs->inner, NULL );
}

#endif /* _WIN32 */

/* ------------------------------------------------------------------------------
 * Device
 * ------------------------------------------------------------------------------ */

/* True if a versioned config struct is large enough to contain a field. */
#define AUDIO__HAS_FIELD( config, type, field ) \
    ( ( config )->struct_size >= offsetof( type, field ) + sizeof( ( (type*)0 )->field ) )

audio_result AUDIO_CALL audio_device_create( const audio_device_config* config, audio_device** out_device )
{
    audio_device_config defaults = audio_device_config_init();
    ma_engine_config    engine_config;
    audio_device*       device;
    ma_result           result;
    uint32_t            listener_count = 0;
    uint32_t            sample_rate    = 0;
    uint32_t            channels       = 0;

    if ( !out_device )
        return AUDIO_INVALID_ARGS;

    *out_device = NULL;

    if ( !config )
        config = &defaults;

    /* A struct_size of 0 means the config was not initialized with audio_device_config_init. */
    if ( config->struct_size == 0 )
    {
        audio__log( AUDIO_LOG_LEVEL_ERROR, "audio_device_create: the config was not initialized with audio_device_config_init." );
        return AUDIO_INVALID_ARGS;
    }

    if ( AUDIO__HAS_FIELD( config, audio_device_config, listener_count ) )
        listener_count = config->listener_count;
    if ( AUDIO__HAS_FIELD( config, audio_device_config, sample_rate ) )
        sample_rate = config->sample_rate;
    if ( AUDIO__HAS_FIELD( config, audio_device_config, channels ) )
        channels = config->channels;

    if ( listener_count == 0 )
        listener_count = AUDIO_MAX_LISTENERS;

    if ( listener_count > AUDIO_MAX_LISTENERS || listener_count > MA_ENGINE_MAX_LISTENERS )
    {
        audio__log( AUDIO_LOG_LEVEL_ERROR, "audio_device_create: listener_count must be in the range [1 .. %u].", AUDIO_MAX_LISTENERS );
        return AUDIO_INVALID_ARGS;
    }

    device = (audio_device*)calloc( 1, sizeof( audio_device ) );
    if ( !device )
        return AUDIO_OUT_OF_MEMORY;

    engine_config               = ma_engine_config_init();
    engine_config.listenerCount = listener_count;
    engine_config.sampleRate    = sample_rate;
    engine_config.channels      = channels;

#if defined( _WIN32 )
    result = audio__vfs_init( &device->vfs );
    if ( result != MA_SUCCESS )
    {
        free( device );
        return audio__result( result );
    }
    engine_config.pResourceManagerVFS = &device->vfs;
#endif

    result = ma_engine_init( &engine_config, &device->engine );
    if ( result != MA_SUCCESS )
    {
        audio__log( AUDIO_LOG_LEVEL_ERROR, "Failed to initialize the audio device: %s", ma_result_description( result ) );
        free( device );
        return audio__result( result );
    }

    device->ref_count      = 1;
    device->listener_count = listener_count;

    *out_device = device;
    return AUDIO_SUCCESS;
}

static void audio__retain_device( audio_device* device )
{
    ma_spinlock_lock( &device->lock );
    ++device->ref_count;
    ma_spinlock_unlock( &device->lock );
}

void audio__release_device( audio_device* device )
{
    uint32_t ref_count;

    if ( !device )
        return;

    ma_spinlock_lock( &device->lock );
    ref_count = --device->ref_count;
    ma_spinlock_unlock( &device->lock );

    if ( ref_count == 0 )
    {
        /* One-shots don't keep their device alive: stop them. */
        audio__free_one_shots( device, NULL, AUDIO_TRUE );
        ma_engine_uninit( &device->engine );
        free( device );
    }
}

void AUDIO_CALL audio_device_destroy( audio_device* device )
{
    audio__release_device( device );
}

/* ------------------------------------------------------------------------------
 * Default device
 * ------------------------------------------------------------------------------ */

static volatile ma_spinlock g_default_lock   = 0;
static audio_device*        g_default_device = NULL;

/* Set when creating the default device failed, so we don't retry (and log) on every
 * call. Cleared by audio_shutdown. */
static audio_bool32 g_default_failed = AUDIO_FALSE;

audio_device* audio__acquire_device( audio_device* device )
{
    if ( device )
    {
        audio__retain_device( device );
        return device;
    }

    ma_spinlock_lock( &g_default_lock );

    if ( !g_default_device && !g_default_failed )
    {
        if ( audio_device_create( NULL, &g_default_device ) != AUDIO_SUCCESS )
            g_default_failed = AUDIO_TRUE;
    }

    device = g_default_device;
    if ( device )
        audio__retain_device( device );

    ma_spinlock_unlock( &g_default_lock );

    return device;
}

void AUDIO_CALL audio_shutdown( void )
{
    audio_device* device;

    ma_spinlock_lock( &g_default_lock );
    device           = g_default_device;
    g_default_device = NULL;
    g_default_failed = AUDIO_FALSE;
    ma_spinlock_unlock( &g_default_lock );

    /* Sounds and waveforms still using the device keep it alive. */
    audio__release_device( device );
}

/* ------------------------------------------------------------------------------
 * Device properties
 * ------------------------------------------------------------------------------ */

void AUDIO_CALL audio_device_set_master_volume( audio_device* device, float volume )
{
    device = audio__acquire_device( device );
    if ( !device )
        return;

    ma_engine_set_volume( &device->engine, volume );
    audio__release_device( device );
}

float AUDIO_CALL audio_device_get_master_volume( audio_device* device )
{
    float volume;

    device = audio__acquire_device( device );
    if ( !device )
        return 0.0f;

    volume = ma_engine_get_volume( &device->engine );
    audio__release_device( device );
    return volume;
}

uint32_t AUDIO_CALL audio_device_get_sample_rate( audio_device* device )
{
    uint32_t sample_rate;

    device = audio__acquire_device( device );
    if ( !device )
        return 0;

    sample_rate = ma_engine_get_sample_rate( &device->engine );
    audio__release_device( device );
    return sample_rate;
}

uint32_t AUDIO_CALL audio_device_get_channels( audio_device* device )
{
    uint32_t channels;

    device = audio__acquire_device( device );
    if ( !device )
        return 0;

    channels = ma_engine_get_channels( &device->engine );
    audio__release_device( device );
    return channels;
}

uint32_t AUDIO_CALL audio_device_get_listener_count( audio_device* device )
{
    uint32_t listener_count;

    device = audio__acquire_device( device );
    if ( !device )
        return 0;

    listener_count = device->listener_count;
    audio__release_device( device );
    return listener_count;
}

audio_result AUDIO_CALL audio_play_sound( audio_device* device, const char* file_path )
{
    ma_result result;

    if ( !file_path )
        return AUDIO_INVALID_ARGS;

    device = audio__acquire_device( device );
    if ( !device )
        return AUDIO_DEVICE_ERROR;

    result = ma_engine_play_sound( &device->engine, file_path, NULL );
    if ( result != MA_SUCCESS )
        audio__log( AUDIO_LOG_LEVEL_ERROR, "Failed to play sound '%s': %s", file_path, ma_result_description( result ) );

    audio__release_device( device );
    return audio__result( result );
}

audio_result AUDIO_CALL audio_play_sound_in_group( audio_group* group, const char* file_path )
{
    ma_result result;

    if ( !group )
        return audio_play_sound( NULL, file_path );

    if ( !file_path )
        return AUDIO_INVALID_ARGS;

    /* miniaudio's fire-and-forget sounds are detached (and silenced) if the group is
     * destroyed while they play, so they don't need to keep the group alive. */
    result = ma_engine_play_sound( &group->device->engine, file_path, &group->group );
    if ( result != MA_SUCCESS )
        audio__log( AUDIO_LOG_LEVEL_ERROR, "Failed to play sound '%s': %s", file_path, ma_result_description( result ) );

    return audio__result( result );
}

/* ------------------------------------------------------------------------------
 * Listener
 * ------------------------------------------------------------------------------ */

static const audio_vec3 audio__zero = { 0.0f, 0.0f, 0.0f };

static audio_vec3 audio__vec3( ma_vec3f v )
{
    audio_vec3 result;
    result.x = v.x;
    result.y = v.y;
    result.z = v.z;
    return result;
}

/* Acquire the device if index is a valid listener index. Returns NULL otherwise. */
static audio_device* audio__acquire_listener( audio_device* device, uint32_t index )
{
    device = audio__acquire_device( device );
    if ( device && index >= device->listener_count )
    {
        audio__release_device( device );
        return NULL;
    }
    return device;
}

void AUDIO_CALL audio_listener_set_position( audio_device* device, uint32_t index, audio_vec3 position )
{
    device = audio__acquire_listener( device, index );
    if ( !device )
        return;

    ma_engine_listener_set_position( &device->engine, index, position.x, position.y, position.z );
    audio__release_device( device );
}

audio_vec3 AUDIO_CALL audio_listener_get_position( audio_device* device, uint32_t index )
{
    audio_vec3 position;

    device = audio__acquire_listener( device, index );
    if ( !device )
        return audio__zero;

    position = audio__vec3( ma_engine_listener_get_position( &device->engine, index ) );
    audio__release_device( device );
    return position;
}

void AUDIO_CALL audio_listener_set_direction( audio_device* device, uint32_t index, audio_vec3 direction )
{
    device = audio__acquire_listener( device, index );
    if ( !device )
        return;

    ma_engine_listener_set_direction( &device->engine, index, direction.x, direction.y, direction.z );
    audio__release_device( device );
}

audio_vec3 AUDIO_CALL audio_listener_get_direction( audio_device* device, uint32_t index )
{
    audio_vec3 direction;

    device = audio__acquire_listener( device, index );
    if ( !device )
        return audio__zero;

    direction = audio__vec3( ma_engine_listener_get_direction( &device->engine, index ) );
    audio__release_device( device );
    return direction;
}

void AUDIO_CALL audio_listener_set_world_up( audio_device* device, uint32_t index, audio_vec3 up )
{
    device = audio__acquire_listener( device, index );
    if ( !device )
        return;

    ma_engine_listener_set_world_up( &device->engine, index, up.x, up.y, up.z );
    audio__release_device( device );
}

audio_vec3 AUDIO_CALL audio_listener_get_world_up( audio_device* device, uint32_t index )
{
    audio_vec3 up;

    device = audio__acquire_listener( device, index );
    if ( !device )
        return audio__zero;

    up = audio__vec3( ma_engine_listener_get_world_up( &device->engine, index ) );
    audio__release_device( device );
    return up;
}

void AUDIO_CALL audio_listener_set_cone( audio_device* device, uint32_t index, float inner_angle, float outer_angle, float outer_gain )
{
    device = audio__acquire_listener( device, index );
    if ( !device )
        return;

    ma_engine_listener_set_cone( &device->engine, index, inner_angle, outer_angle, outer_gain );
    audio__release_device( device );
}

void AUDIO_CALL audio_listener_get_cone( audio_device* device, uint32_t index, float* out_inner_angle, float* out_outer_angle, float* out_outer_gain )
{
    float inner = 0.0f, outer = 0.0f, gain = 0.0f;

    device = audio__acquire_listener( device, index );
    if ( device )
    {
        ma_engine_listener_get_cone( &device->engine, index, &inner, &outer, &gain );
        audio__release_device( device );
    }

    if ( out_inner_angle )
        *out_inner_angle = inner;
    if ( out_outer_angle )
        *out_outer_angle = outer;
    if ( out_outer_gain )
        *out_outer_gain = gain;
}
