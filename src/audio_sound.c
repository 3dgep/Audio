/*
 * Audio v2 - sounds.
 */
#include "audio_internal.h"

#include <stdlib.h>

#define AUDIO__KNOWN_SOUND_FLAGS \
    ( AUDIO_SOUND_FLAG_STREAM | AUDIO_SOUND_FLAG_NO_SPATIALIZATION | AUDIO_SOUND_FLAG_LOOPING )

static audio_vec3 audio__vec3( ma_vec3f v )
{
    audio_vec3 result;
    result.x = v.x;
    result.y = v.y;
    result.z = v.z;
    return result;
}

static const audio_vec3 audio__zero = { 0.0f, 0.0f, 0.0f };

static audio_result audio__sound_create( audio_device* device, audio_group* group, const char* file_path, audio_sound_flags flags, audio_sound** out_sound )
{
    audio_sound* sound;
    ma_uint32    ma_flags = 0;
    ma_result    result;

    if ( !out_sound )
        return AUDIO_INVALID_ARGS;

    *out_sound = NULL;

    if ( !file_path )
        return AUDIO_INVALID_ARGS;

    /* Reject flags from a newer version of the library instead of silently ignoring them. */
    if ( flags & ~AUDIO__KNOWN_SOUND_FLAGS )
    {
        audio__log( AUDIO_LOG_LEVEL_ERROR, "audio_sound_create: unknown flags 0x%X.", (unsigned)( flags & ~AUDIO__KNOWN_SOUND_FLAGS ) );
        return AUDIO_INVALID_ARGS;
    }

    ma_flags |= ( flags & AUDIO_SOUND_FLAG_STREAM ) ? MA_SOUND_FLAG_STREAM : MA_SOUND_FLAG_DECODE;
    if ( flags & AUDIO_SOUND_FLAG_NO_SPATIALIZATION )
        ma_flags |= MA_SOUND_FLAG_NO_SPATIALIZATION;
    if ( flags & AUDIO_SOUND_FLAG_LOOPING )
        ma_flags |= MA_SOUND_FLAG_LOOPING;

    sound = (audio_sound*)calloc( 1, sizeof( audio_sound ) );
    if ( !sound )
        return AUDIO_OUT_OF_MEMORY;

    /* The sound keeps these references until it is destroyed. */
    sound->device = audio__acquire_device( group ? group->device : device );
    if ( !sound->device )
    {
        free( sound );
        return AUDIO_DEVICE_ERROR;
    }

    result = ma_sound_init_from_file( &sound->device->engine, file_path, ma_flags, group ? &group->group : NULL, NULL, &sound->sound );
    if ( result != MA_SUCCESS )
    {
        audio__log( AUDIO_LOG_LEVEL_ERROR, "Failed to load sound '%s': %s", file_path, ma_result_description( result ) );
        audio__release_device( sound->device );
        free( sound );
        return audio__result( result );
    }

    audio__retain_group( group );
    sound->group = group;

    *out_sound = sound;
    return AUDIO_SUCCESS;
}

audio_result AUDIO_CALL audio_sound_create( audio_device* device, const char* file_path, audio_sound_flags flags, audio_sound** out_sound )
{
    return audio__sound_create( device, NULL, file_path, flags, out_sound );
}

audio_result AUDIO_CALL audio_sound_create_in_group( audio_group* group, const char* file_path, audio_sound_flags flags, audio_sound** out_sound )
{
    return audio__sound_create( NULL, group, file_path, flags, out_sound );
}

void AUDIO_CALL audio_sound_destroy( audio_sound* sound )
{
    if ( !sound )
        return;

    ma_sound_uninit( &sound->sound );
    audio__release_group( sound->group );
    audio__release_device( sound->device );
    free( sound );
}

/* ------------------------------------------------------------------------------
 * Playback
 * ------------------------------------------------------------------------------ */

audio_result AUDIO_CALL audio_sound_play( audio_sound* sound )
{
    if ( !sound )
        return AUDIO_INVALID_ARGS;

    return audio__result( audio__ma_sound_play( &sound->sound, &sound->fade_out ) );
}

audio_result AUDIO_CALL audio_sound_stop( audio_sound* sound )
{
    if ( !sound )
        return AUDIO_INVALID_ARGS;

    return audio__result( ma_sound_stop( &sound->sound ) );
}

audio_result AUDIO_CALL audio_sound_stop_with_fade( audio_sound* sound, uint64_t milliseconds )
{
    if ( !sound )
        return AUDIO_INVALID_ARGS;

    return audio__result( audio__ma_sound_stop_with_fade( &sound->sound, &sound->fade_out, milliseconds ) );
}

audio_result AUDIO_CALL audio_sound_set_group( audio_sound* sound, audio_group* group )
{
    if ( !sound )
        return AUDIO_INVALID_ARGS;

    return audio__move_to_group( sound->device, &sound->sound, &sound->group, group, "audio_sound_set_group" );
}

audio_group* AUDIO_CALL audio_sound_get_group( audio_sound* sound )
{
    return sound ? sound->group : NULL;
}

audio_result AUDIO_CALL audio_sound_play_one_shot( audio_sound* sound )
{
    audio_one_shot* shot;
    ma_sound*       copy;
    ma_sound*       source;
    ma_uint32       flags = 0;
    ma_result       result;
    float           inner, outer, gain;
    ma_uint32       listener;

    if ( !sound )
        return AUDIO_INVALID_ARGS;

    source = &sound->sound;

    /* Copies share the decoded audio data with the sound. This doesn't work for streamed
     * sounds, which read the file while they play. (Checked first, so the error is
     * reported even when nothing would play.) */
    if ( !source->pResourceManagerDataSource || ( source->pResourceManagerDataSource->flags & MA_RESOURCE_MANAGER_DATA_SOURCE_FLAG_STREAM ) )
    {
        audio__log( AUDIO_LOG_LEVEL_ERROR, "audio_sound_play_one_shot: streamed sounds (music) can't be played as a one-shot." );
        return AUDIO_INVALID_OPERATION;
    }

    /* A sound effect is only useful right now. While the device isn't pulling audio
     * (in a browser, before the user clicks or taps the page), a copy would wait at its
     * first frame and play late, together with every other waiting copy. The same goes
     * for a group that is paused or stopped. */
    if ( !audio__device_is_running( sound->device ) || !audio__group_is_playing( sound->group ) )
        return AUDIO_SUCCESS;

    /* Free the one-shots that have finished playing, and stop the oldest one if the
     * limit is reached, so the list stays short. */
    audio__free_finished_one_shots( sound->device );
    if ( sound->device->one_shot_count >= AUDIO__MAX_ONE_SHOTS )
        audio__free_oldest_one_shot( sound->device );

    shot = (audio_one_shot*)calloc( 1, sizeof( audio_one_shot ) );
    if ( !shot )
        return AUDIO_OUT_OF_MEMORY;

    if ( !ma_sound_is_spatialization_enabled( source ) )
        flags |= MA_SOUND_FLAG_NO_SPATIALIZATION;

    copy   = &shot->sound;
    result = ma_sound_init_copy( &sound->device->engine, source, flags, sound->group ? &sound->group->group : NULL, copy );
    if ( result != MA_SUCCESS )
    {
        audio__log( AUDIO_LOG_LEVEL_ERROR, "audio_sound_play_one_shot: %s", ma_result_description( result ) );
        free( shot );
        return result == MA_INVALID_OPERATION ? AUDIO_INVALID_OPERATION : audio__result( result );
    }

    /* Copy the sound's current settings (but not its looping or its cursor). */
    ma_sound_set_volume( copy, ma_sound_get_volume( source ) * ma_sound_get_current_fade_volume( source ) );
    ma_sound_set_pan( copy, ma_sound_get_pan( source ) );
    ma_sound_set_pitch( copy, ma_sound_get_pitch( source ) );
    ma_sound_set_position( copy, ma_sound_get_position( source ).x, ma_sound_get_position( source ).y, ma_sound_get_position( source ).z );
    ma_sound_set_direction( copy, ma_sound_get_direction( source ).x, ma_sound_get_direction( source ).y, ma_sound_get_direction( source ).z );
    ma_sound_set_velocity( copy, ma_sound_get_velocity( source ).x, ma_sound_get_velocity( source ).y, ma_sound_get_velocity( source ).z );
    ma_sound_get_cone( source, &inner, &outer, &gain );
    ma_sound_set_cone( copy, inner, outer, gain );
    ma_sound_set_attenuation_model( copy, ma_sound_get_attenuation_model( source ) );
    ma_sound_set_rolloff( copy, ma_sound_get_rolloff( source ) );
    ma_sound_set_min_gain( copy, ma_sound_get_min_gain( source ) );
    ma_sound_set_max_gain( copy, ma_sound_get_max_gain( source ) );
    ma_sound_set_min_distance( copy, ma_sound_get_min_distance( source ) );
    ma_sound_set_max_distance( copy, ma_sound_get_max_distance( source ) );
    ma_sound_set_doppler_factor( copy, ma_sound_get_doppler_factor( source ) );
    listener = ma_sound_get_pinned_listener_index( source );
    if ( listener != MA_LISTENER_INDEX_CLOSEST )
        ma_sound_set_pinned_listener_index( copy, listener );

    result = ma_sound_start( copy );
    if ( result != MA_SUCCESS )
    {
        ma_sound_uninit( copy );
        free( shot );
        return audio__result( result );
    }

    shot->group             = sound->group;
    shot->next               = sound->device->one_shots;
    sound->device->one_shots = shot;
    sound->device->one_shot_count++;
    return AUDIO_SUCCESS;
}

audio_result AUDIO_CALL audio_sound_seek( audio_sound* sound, uint64_t milliseconds )
{
    ma_uint32 sample_rate = 0;
    ma_result result;

    if ( !sound )
        return AUDIO_INVALID_ARGS;

    result = ma_sound_get_data_format( &sound->sound, NULL, NULL, &sample_rate, NULL, 0 );
    if ( result != MA_SUCCESS )
        return audio__result( result );

    return audio__result( ma_sound_seek_to_pcm_frame( &sound->sound, ( (ma_uint64)sample_rate * milliseconds ) / 1000 ) );
}

float AUDIO_CALL audio_sound_get_duration_seconds( audio_sound* sound )
{
    float length = 0.0f;

    if ( sound )
        ma_sound_get_length_in_seconds( &sound->sound, &length );

    return length;
}

float AUDIO_CALL audio_sound_get_cursor_seconds( audio_sound* sound )
{
    float cursor = 0.0f;

    if ( sound )
        ma_sound_get_cursor_in_seconds( &sound->sound, &cursor );

    return cursor;
}

audio_bool32 AUDIO_CALL audio_sound_is_playing( audio_sound* sound )
{
    return ( sound && ma_sound_is_playing( &sound->sound ) ) ? AUDIO_TRUE : AUDIO_FALSE;
}

audio_bool32 AUDIO_CALL audio_sound_is_at_end( audio_sound* sound )
{
    return ( sound && ma_sound_at_end( &sound->sound ) ) ? AUDIO_TRUE : AUDIO_FALSE;
}

void AUDIO_CALL audio_sound_set_looping( audio_sound* sound, audio_bool32 looping )
{
    if ( sound )
        ma_sound_set_looping( &sound->sound, looping ? MA_TRUE : MA_FALSE );
}

audio_bool32 AUDIO_CALL audio_sound_is_looping( audio_sound* sound )
{
    return ( sound && ma_sound_is_looping( &sound->sound ) ) ? AUDIO_TRUE : AUDIO_FALSE;
}

void AUDIO_CALL audio_sound_set_volume( audio_sound* sound, float volume )
{
    if ( sound )
        ma_sound_set_volume( &sound->sound, volume );
}

float AUDIO_CALL audio_sound_get_volume( audio_sound* sound )
{
    return sound ? ma_sound_get_volume( &sound->sound ) : 0.0f;
}

void AUDIO_CALL audio_sound_set_pan( audio_sound* sound, float pan )
{
    if ( sound )
        ma_sound_set_pan( &sound->sound, pan );
}

float AUDIO_CALL audio_sound_get_pan( audio_sound* sound )
{
    return sound ? ma_sound_get_pan( &sound->sound ) : 0.0f;
}

void AUDIO_CALL audio_sound_set_pitch( audio_sound* sound, float pitch )
{
    if ( sound && pitch > 0.0f )
        ma_sound_set_pitch( &sound->sound, pitch );
}

float AUDIO_CALL audio_sound_get_pitch( audio_sound* sound )
{
    return sound ? ma_sound_get_pitch( &sound->sound ) : 0.0f;
}

void AUDIO_CALL audio_sound_set_fade( audio_sound* sound, float begin_volume, float end_volume, uint64_t milliseconds )
{
    if ( sound )
    {
        /* miniaudio uses a begin volume of -1 to mean "the current volume". */
        ma_sound_set_fade_in_milliseconds( &sound->sound, begin_volume < 0.0f ? -1.0f : begin_volume, end_volume, milliseconds );
    }
}

void AUDIO_CALL audio_sound_schedule_start( audio_sound* sound, uint64_t delay_milliseconds )
{
    if ( sound )
    {
        /* miniaudio's start time is absolute, in engine time. */
        const ma_uint64 now = ma_engine_get_time_in_milliseconds( &sound->device->engine );
        ma_sound_set_start_time_in_milliseconds( &sound->sound, now + delay_milliseconds );
    }
}

void AUDIO_CALL audio_sound_schedule_stop( audio_sound* sound, uint64_t delay_milliseconds )
{
    if ( sound )
    {
        const ma_uint64 now = ma_engine_get_time_in_milliseconds( &sound->device->engine );
        ma_sound_set_stop_time_in_milliseconds( &sound->sound, now + delay_milliseconds );
    }
}

/* ------------------------------------------------------------------------------
 * Spatialization
 * ------------------------------------------------------------------------------ */

void AUDIO_CALL audio_sound_set_pinned_listener( audio_sound* sound, uint32_t listener_index )
{
    if ( !sound )
        return;

    if ( listener_index == AUDIO_LISTENER_CLOSEST )
        audio__ma_sound_unpin_listener( &sound->sound );
    else if ( listener_index < sound->device->listener_count )
        ma_sound_set_pinned_listener_index( &sound->sound, listener_index );
}

uint32_t AUDIO_CALL audio_sound_get_pinned_listener( audio_sound* sound )
{
    ma_uint32 index;

    if ( !sound )
        return AUDIO_LISTENER_CLOSEST;

    index = ma_sound_get_pinned_listener_index( &sound->sound );
    return index == MA_LISTENER_INDEX_CLOSEST ? AUDIO_LISTENER_CLOSEST : index;
}

void AUDIO_CALL audio_sound_set_position( audio_sound* sound, audio_vec3 position )
{
    if ( sound )
        ma_sound_set_position( &sound->sound, position.x, position.y, position.z );
}

audio_vec3 AUDIO_CALL audio_sound_get_position( audio_sound* sound )
{
    return sound ? audio__vec3( ma_sound_get_position( &sound->sound ) ) : audio__zero;
}

void AUDIO_CALL audio_sound_set_direction( audio_sound* sound, audio_vec3 direction )
{
    if ( sound )
        ma_sound_set_direction( &sound->sound, direction.x, direction.y, direction.z );
}

audio_vec3 AUDIO_CALL audio_sound_get_direction( audio_sound* sound )
{
    return sound ? audio__vec3( ma_sound_get_direction( &sound->sound ) ) : audio__zero;
}

void AUDIO_CALL audio_sound_set_velocity( audio_sound* sound, audio_vec3 velocity )
{
    if ( sound )
        ma_sound_set_velocity( &sound->sound, velocity.x, velocity.y, velocity.z );
}

audio_vec3 AUDIO_CALL audio_sound_get_velocity( audio_sound* sound )
{
    return sound ? audio__vec3( ma_sound_get_velocity( &sound->sound ) ) : audio__zero;
}

void AUDIO_CALL audio_sound_set_cone( audio_sound* sound, float inner_angle, float outer_angle, float outer_gain )
{
    if ( sound )
        ma_sound_set_cone( &sound->sound, inner_angle, outer_angle, outer_gain );
}

void AUDIO_CALL audio_sound_get_cone( audio_sound* sound, float* out_inner_angle, float* out_outer_angle, float* out_outer_gain )
{
    /* ma_sound_get_cone accepts NULL for the sound and for each output. */
    ma_sound_get_cone( sound ? &sound->sound : NULL, out_inner_angle, out_outer_angle, out_outer_gain );
}

void AUDIO_CALL audio_sound_set_attenuation_model( audio_sound* sound, audio_attenuation_model model )
{
    if ( !sound )
        return;

    switch ( model )
    {
    case AUDIO_ATTENUATION_MODEL_NONE:
        ma_sound_set_attenuation_model( &sound->sound, ma_attenuation_model_none );
        break;
    case AUDIO_ATTENUATION_MODEL_INVERSE:
        ma_sound_set_attenuation_model( &sound->sound, ma_attenuation_model_inverse );
        break;
    case AUDIO_ATTENUATION_MODEL_LINEAR:
        ma_sound_set_attenuation_model( &sound->sound, ma_attenuation_model_linear );
        break;
    case AUDIO_ATTENUATION_MODEL_EXPONENTIAL:
        ma_sound_set_attenuation_model( &sound->sound, ma_attenuation_model_exponential );
        break;
    default:
        break; /* Unknown model: ignore. */
    }
}

audio_attenuation_model AUDIO_CALL audio_sound_get_attenuation_model( audio_sound* sound )
{
    if ( !sound )
        return AUDIO_ATTENUATION_MODEL_NONE;

    switch ( ma_sound_get_attenuation_model( &sound->sound ) )
    {
    case ma_attenuation_model_inverse:
        return AUDIO_ATTENUATION_MODEL_INVERSE;
    case ma_attenuation_model_linear:
        return AUDIO_ATTENUATION_MODEL_LINEAR;
    case ma_attenuation_model_exponential:
        return AUDIO_ATTENUATION_MODEL_EXPONENTIAL;
    case ma_attenuation_model_none:
    default:
        return AUDIO_ATTENUATION_MODEL_NONE;
    }
}

void AUDIO_CALL audio_sound_set_rolloff( audio_sound* sound, float rolloff )
{
    if ( sound )
        ma_sound_set_rolloff( &sound->sound, rolloff );
}

float AUDIO_CALL audio_sound_get_rolloff( audio_sound* sound )
{
    return sound ? ma_sound_get_rolloff( &sound->sound ) : 0.0f;
}

void AUDIO_CALL audio_sound_set_min_gain( audio_sound* sound, float min_gain )
{
    if ( sound )
        ma_sound_set_min_gain( &sound->sound, min_gain );
}

float AUDIO_CALL audio_sound_get_min_gain( audio_sound* sound )
{
    return sound ? ma_sound_get_min_gain( &sound->sound ) : 0.0f;
}

void AUDIO_CALL audio_sound_set_max_gain( audio_sound* sound, float max_gain )
{
    if ( sound )
        ma_sound_set_max_gain( &sound->sound, max_gain );
}

float AUDIO_CALL audio_sound_get_max_gain( audio_sound* sound )
{
    return sound ? ma_sound_get_max_gain( &sound->sound ) : 0.0f;
}

void AUDIO_CALL audio_sound_set_min_distance( audio_sound* sound, float min_distance )
{
    if ( sound )
        ma_sound_set_min_distance( &sound->sound, min_distance );
}

float AUDIO_CALL audio_sound_get_min_distance( audio_sound* sound )
{
    return sound ? ma_sound_get_min_distance( &sound->sound ) : 0.0f;
}

void AUDIO_CALL audio_sound_set_max_distance( audio_sound* sound, float max_distance )
{
    if ( sound )
        ma_sound_set_max_distance( &sound->sound, max_distance );
}

float AUDIO_CALL audio_sound_get_max_distance( audio_sound* sound )
{
    return sound ? ma_sound_get_max_distance( &sound->sound ) : 0.0f;
}

void AUDIO_CALL audio_sound_set_doppler_factor( audio_sound* sound, float doppler_factor )
{
    if ( sound )
        ma_sound_set_doppler_factor( &sound->sound, doppler_factor );
}

float AUDIO_CALL audio_sound_get_doppler_factor( audio_sound* sound )
{
    return sound ? ma_sound_get_doppler_factor( &sound->sound ) : 0.0f;
}
