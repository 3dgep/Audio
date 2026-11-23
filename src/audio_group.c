/*
 * Audio v2 - groups, and the play and fade-out logic shared by sounds and groups.
 */
#include "audio_internal.h"

#include <stdlib.h>

/* ------------------------------------------------------------------------------
 * Play and stop with a fade-out (sounds and groups)
 * ------------------------------------------------------------------------------ */

ma_result audio__ma_sound_play( ma_sound* sound, audio_fade_out* fade_out )
{
    const ma_uint64 now = ma_engine_get_time_in_pcm_frames( ma_sound_get_engine( sound ) );

    if ( fade_out->active )
    {
        /* Cancel the fade-out (still fading, or already stopped): restore the volume
         * from before it, and remove its stop time. */
        ma_sound_reset_stop_time( sound );
        ma_sound_set_fade_in_pcm_frames( sound, fade_out->volume, fade_out->volume, 0 );
        fade_out->active = AUDIO_FALSE;
    }
    else if ( ma_node_get_state_time( sound, ma_node_state_stopped ) <= now )
    {
        /* A stop time that has passed (audio_sound_schedule_stop) would keep the sound
         * stopped, even after starting it again. */
        ma_sound_reset_stop_time( sound );
    }

    return ma_sound_start( sound );
}

ma_result audio__ma_sound_stop_with_fade( ma_sound* sound, audio_fade_out* fade_out, uint64_t milliseconds )
{
    if ( milliseconds == 0 || !ma_sound_is_playing( sound ) )
        return ma_sound_stop( sound );

    /* Remember the volume to restore when playing again. If a fade-out is already
     * running, keep the volume from before the first one. */
    if ( !fade_out->active )
    {
        fade_out->volume = ma_sound_get_current_fade_volume( sound );
        fade_out->active = AUDIO_TRUE;
    }

    return ma_sound_stop_with_fade_in_milliseconds( sound, milliseconds );
}

/* ------------------------------------------------------------------------------
 * One-shots
 * ------------------------------------------------------------------------------ */

static void audio__free_one_shot( audio_one_shot* shot )
{
    ma_sound_uninit( &shot->sound );
    free( shot );
}

void audio__free_finished_one_shots( audio_device* device )
{
    audio_one_shot** link = &device->one_shots;

    while ( *link )
    {
        audio_one_shot* shot = *link;
        if ( ma_sound_at_end( &shot->sound ) )
        {
            *link = shot->next;
            audio__free_one_shot( shot );
        }
        else
        {
            link = &shot->next;
        }
    }
}

void audio__free_one_shots( audio_device* device, audio_group* group, audio_bool32 all )
{
    audio_one_shot** link = &device->one_shots;

    while ( *link )
    {
        audio_one_shot* shot = *link;
        if ( all || shot->group == group )
        {
            *link = shot->next;
            audio__free_one_shot( shot );
        }
        else
        {
            link = &shot->next;
        }
    }
}

/* ------------------------------------------------------------------------------
 * Groups
 * ------------------------------------------------------------------------------ */

ma_node* audio__group_node( audio_device* device, audio_group* group )
{
    return group ? (ma_node*)&group->group : (ma_node*)ma_engine_get_endpoint( &device->engine );
}

audio_result AUDIO_CALL audio_group_create( audio_device* device, audio_group* parent, audio_group** out_group )
{
    audio_group* group;
    ma_result    result;

    if ( !out_group )
        return AUDIO_INVALID_ARGS;

    *out_group = NULL;

    if ( parent && device && parent->device != device )
    {
        audio__log( AUDIO_LOG_LEVEL_ERROR, "audio_group_create: the parent group belongs to a different device." );
        return AUDIO_INVALID_ARGS;
    }

    group = (audio_group*)calloc( 1, sizeof( audio_group ) );
    if ( !group )
        return AUDIO_OUT_OF_MEMORY;

    /* The group keeps this reference until it is destroyed. */
    group->device = audio__acquire_device( parent ? parent->device : device );
    if ( !group->device )
    {
        free( group );
        return AUDIO_DEVICE_ERROR;
    }

    /* Groups are started (playing) by default, and not spatialized. */
    result = ma_sound_group_init( &group->device->engine, 0, parent ? &parent->group : NULL, &group->group );
    if ( result != MA_SUCCESS )
    {
        audio__log( AUDIO_LOG_LEVEL_ERROR, "Failed to create a group: %s", ma_result_description( result ) );
        audio__release_device( group->device );
        free( group );
        return audio__result( result );
    }

    audio__retain_group( parent );
    group->parent    = parent;
    group->ref_count = 1;

    *out_group = group;
    return AUDIO_SUCCESS;
}

void audio__retain_group( audio_group* group )
{
    if ( group )
        ++group->ref_count;
}

void audio__release_group( audio_group* group )
{
    if ( !group || --group->ref_count > 0 )
        return;

    /* One-shots don't keep their group alive: stop them. */
    audio__free_one_shots( group->device, group, AUDIO_FALSE );

    ma_sound_group_uninit( &group->group );
    audio__release_group( group->parent );
    audio__release_device( group->device );
    free( group );
}

void AUDIO_CALL audio_group_destroy( audio_group* group )
{
    audio__release_group( group );
}

void AUDIO_CALL audio_group_retain( audio_group* group )
{
    audio__retain_group( group );
}

audio_result audio__move_to_group( audio_device* device, ma_node* node, audio_group** current, audio_group* group, const char* function )
{
    ma_result result;

    if ( group && group->device != device )
    {
        audio__log( AUDIO_LOG_LEVEL_ERROR, "%s: the group belongs to a different device.", function );
        return AUDIO_INVALID_ARGS;
    }

    if ( group == *current )
        return AUDIO_SUCCESS;

    /* Attaching an output bus detaches it from where it was first. This is safe while
     * the node plays. */
    result = ma_node_attach_output_bus( node, 0, audio__group_node( device, group ), 0 );
    if ( result != MA_SUCCESS )
        return audio__result( result );

    /* Take the new reference before releasing the old one: the old group may be freed. */
    audio__retain_group( group );
    audio__release_group( *current );
    *current = group;
    return AUDIO_SUCCESS;
}

audio_result AUDIO_CALL audio_group_play( audio_group* group )
{
    if ( !group )
        return AUDIO_INVALID_ARGS;

    return audio__result( audio__ma_sound_play( &group->group, &group->fade_out ) );
}

audio_result AUDIO_CALL audio_group_stop( audio_group* group )
{
    if ( !group )
        return AUDIO_INVALID_ARGS;

    return audio__result( ma_sound_group_stop( &group->group ) );
}

audio_result AUDIO_CALL audio_group_stop_with_fade( audio_group* group, uint64_t milliseconds )
{
    if ( !group )
        return AUDIO_INVALID_ARGS;

    return audio__result( audio__ma_sound_stop_with_fade( &group->group, &group->fade_out, milliseconds ) );
}

audio_bool32 AUDIO_CALL audio_group_is_playing( audio_group* group )
{
    return ( group && ma_sound_group_is_playing( &group->group ) ) ? AUDIO_TRUE : AUDIO_FALSE;
}

void AUDIO_CALL audio_group_set_volume( audio_group* group, float volume )
{
    if ( group )
        ma_sound_group_set_volume( &group->group, volume );
}

float AUDIO_CALL audio_group_get_volume( audio_group* group )
{
    return group ? ma_sound_group_get_volume( &group->group ) : 0.0f;
}

void AUDIO_CALL audio_group_set_pan( audio_group* group, float pan )
{
    if ( group )
        ma_sound_group_set_pan( &group->group, pan );
}

float AUDIO_CALL audio_group_get_pan( audio_group* group )
{
    return group ? ma_sound_group_get_pan( &group->group ) : 0.0f;
}

void AUDIO_CALL audio_group_set_pitch( audio_group* group, float pitch )
{
    if ( group && pitch > 0.0f )
        ma_sound_group_set_pitch( &group->group, pitch );
}

float AUDIO_CALL audio_group_get_pitch( audio_group* group )
{
    return group ? ma_sound_group_get_pitch( &group->group ) : 0.0f;
}

void AUDIO_CALL audio_group_set_fade( audio_group* group, float begin_volume, float end_volume, uint64_t milliseconds )
{
    if ( group )
        ma_sound_group_set_fade_in_milliseconds( &group->group, begin_volume < 0.0f ? -1.0f : begin_volume, end_volume, milliseconds );
}
