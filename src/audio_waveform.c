/*
 * Audio v2 - waveforms.
 *
 * A waveform is a miniaudio waveform data source, wrapped in a data source node that
 * is attached to a group, or to the engine's endpoint.
 */
#include "audio_internal.h"

#include <stdlib.h>

static audio_bool32 audio__to_ma_waveform_type( audio_waveform_type type, ma_waveform_type* out_type )
{
    switch ( type )
    {
    case AUDIO_WAVEFORM_TYPE_SINE:
        *out_type = ma_waveform_type_sine;
        return AUDIO_TRUE;
    case AUDIO_WAVEFORM_TYPE_SQUARE:
        *out_type = ma_waveform_type_square;
        return AUDIO_TRUE;
    case AUDIO_WAVEFORM_TYPE_TRIANGLE:
        *out_type = ma_waveform_type_triangle;
        return AUDIO_TRUE;
    case AUDIO_WAVEFORM_TYPE_SAWTOOTH:
        *out_type = ma_waveform_type_sawtooth;
        return AUDIO_TRUE;
    default:
        return AUDIO_FALSE;
    }
}

static audio_result audio__waveform_create( audio_device* device, audio_group* group, audio_waveform_type type, float amplitude, float frequency, audio_waveform** out_waveform )
{
    audio_waveform*            waveform;
    ma_waveform_type           ma_type;
    ma_waveform_config         waveform_config;
    ma_data_source_node_config node_config;
    ma_engine*                 engine;
    ma_result                  result;

    if ( !out_waveform )
        return AUDIO_INVALID_ARGS;

    *out_waveform = NULL;

    if ( !audio__to_ma_waveform_type( type, &ma_type ) )
        return AUDIO_INVALID_ARGS;

    waveform = (audio_waveform*)calloc( 1, sizeof( audio_waveform ) );
    if ( !waveform )
        return AUDIO_OUT_OF_MEMORY;

    /* The waveform keeps this reference until it is destroyed. */
    waveform->device = audio__acquire_device( group ? group->device : device );
    if ( !waveform->device )
    {
        free( waveform );
        return AUDIO_DEVICE_ERROR;
    }

    engine              = &waveform->device->engine;
    waveform->type      = type;
    waveform->amplitude = amplitude;
    waveform->frequency = frequency;

    /* The node graph always processes 32-bit float samples. */
    waveform_config = ma_waveform_config_init( ma_format_f32, ma_engine_get_channels( engine ), ma_engine_get_sample_rate( engine ), ma_type, (double)amplitude, (double)frequency );

    result = ma_waveform_init( &waveform_config, &waveform->waveform );
    if ( result != MA_SUCCESS )
        goto fail_waveform;

    node_config = ma_data_source_node_config_init( &waveform->waveform );
    result      = ma_data_source_node_init( ma_engine_get_node_graph( engine ), &node_config, NULL, &waveform->node );
    if ( result != MA_SUCCESS )
        goto fail_node;

    /* Start stopped, so the waveform is silent until audio_waveform_start is called. */
    result = ma_node_set_state( &waveform->node, ma_node_state_stopped );
    if ( result == MA_SUCCESS )
        result = ma_node_attach_output_bus( &waveform->node, 0, audio__group_node( waveform->device, group ), 0 );
    if ( result != MA_SUCCESS )
        goto fail_attach;

    audio__retain_group( group );
    waveform->group = group;

    *out_waveform = waveform;
    return AUDIO_SUCCESS;

fail_attach:
    ma_data_source_node_uninit( &waveform->node, NULL );
fail_node:
    ma_waveform_uninit( &waveform->waveform );
fail_waveform:
    audio__log( AUDIO_LOG_LEVEL_ERROR, "Failed to create waveform: %s", ma_result_description( result ) );
    audio__release_device( waveform->device );
    free( waveform );
    return audio__result( result );
}

audio_result AUDIO_CALL audio_waveform_create( audio_device* device, audio_waveform_type type, float amplitude, float frequency, audio_waveform** out_waveform )
{
    return audio__waveform_create( device, NULL, type, amplitude, frequency, out_waveform );
}

audio_result AUDIO_CALL audio_waveform_create_in_group( audio_group* group, audio_waveform_type type, float amplitude, float frequency, audio_waveform** out_waveform )
{
    return audio__waveform_create( NULL, group, type, amplitude, frequency, out_waveform );
}

audio_result AUDIO_CALL audio_waveform_set_group( audio_waveform* waveform, audio_group* group )
{
    if ( !waveform )
        return AUDIO_INVALID_ARGS;

    return audio__move_to_group( waveform->device, &waveform->node, &waveform->group, group, "audio_waveform_set_group" );
}

audio_group* AUDIO_CALL audio_waveform_get_group( audio_waveform* waveform )
{
    return waveform ? waveform->group : NULL;
}

void AUDIO_CALL audio_waveform_destroy( audio_waveform* waveform )
{
    if ( !waveform )
        return;

    /* Uninitializing the node detaches it from the graph. */
    ma_data_source_node_uninit( &waveform->node, NULL );
    ma_waveform_uninit( &waveform->waveform );
    audio__release_group( waveform->group );
    audio__release_device( waveform->device );
    free( waveform );
}

audio_result AUDIO_CALL audio_waveform_start( audio_waveform* waveform )
{
    if ( !waveform )
        return AUDIO_INVALID_ARGS;

    return audio__result( ma_node_set_state( &waveform->node, ma_node_state_started ) );
}

audio_result AUDIO_CALL audio_waveform_stop( audio_waveform* waveform )
{
    if ( !waveform )
        return AUDIO_INVALID_ARGS;

    return audio__result( ma_node_set_state( &waveform->node, ma_node_state_stopped ) );
}

audio_bool32 AUDIO_CALL audio_waveform_is_playing( audio_waveform* waveform )
{
    return ( waveform && ma_node_get_state( &waveform->node ) == ma_node_state_started ) ? AUDIO_TRUE : AUDIO_FALSE;
}

void AUDIO_CALL audio_waveform_set_type( audio_waveform* waveform, audio_waveform_type type )
{
    ma_waveform_type ma_type;

    if ( waveform && audio__to_ma_waveform_type( type, &ma_type ) )
    {
        waveform->type = type;
        ma_waveform_set_type( &waveform->waveform, ma_type );
    }
}

audio_waveform_type AUDIO_CALL audio_waveform_get_type( audio_waveform* waveform )
{
    return waveform ? waveform->type : AUDIO_WAVEFORM_TYPE_SINE;
}

void AUDIO_CALL audio_waveform_set_amplitude( audio_waveform* waveform, float amplitude )
{
    if ( waveform )
    {
        waveform->amplitude = amplitude;
        ma_waveform_set_amplitude( &waveform->waveform, (double)amplitude );
    }
}

float AUDIO_CALL audio_waveform_get_amplitude( audio_waveform* waveform )
{
    return waveform ? waveform->amplitude : 0.0f;
}

void AUDIO_CALL audio_waveform_set_frequency( audio_waveform* waveform, float frequency )
{
    if ( waveform )
    {
        waveform->frequency = frequency;
        ma_waveform_set_frequency( &waveform->waveform, (double)frequency );
    }
}

float AUDIO_CALL audio_waveform_get_frequency( audio_waveform* waveform )
{
    return waveform ? waveform->frequency : 0.0f;
}
