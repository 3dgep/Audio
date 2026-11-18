/*
 * Internal declarations shared by the Audio implementation files.
 * Nothing in this file is part of the public API.
 */
#ifndef AUDIO_INTERNAL_H
#define AUDIO_INTERNAL_H

#include <Audio/audio.h>

/* AUDIO_USE_SYSTEM_DEPS: use an installed miniaudio instead of the copy in src/. */
#if defined( AUDIO_USE_SYSTEM_DEPS )
    #include <miniaudio.h>
#else
    #include "miniaudio.h"
#endif

#if defined( _WIN32 )
/*
 * A virtual file system that interprets narrow (char) file paths as UTF-8.
 * miniaudio's default VFS passes narrow paths to the Windows ANSI file APIs, so
 * this wrapper converts them to wide strings first.
 */
typedef struct audio_utf8_vfs
{
    ma_vfs_callbacks cb; /* Must be the first member. */
    ma_default_vfs   inner;
} audio_utf8_vfs;
#endif

struct audio_device
{
    volatile ma_spinlock lock;      /* Protects ref_count. */
    uint32_t             ref_count; /* One for the creator, plus one per sound and waveform. */
    uint32_t             listener_count;
#if defined( _WIN32 )
    audio_utf8_vfs vfs; /* Must outlive the engine. */
#endif
    ma_engine engine;
};

struct audio_sound
{
    audio_device* device; /* Holds a reference. */
    ma_sound      sound;
};

struct audio_waveform
{
    audio_device*       device; /* Holds a reference. */
    ma_waveform         waveform;
    ma_data_source_node node;
    audio_waveform_type type;
    float               amplitude;
    float               frequency;
};

/* Logging. */
void audio__log( audio_log_level level, const char* format, ... );

/* Convert a miniaudio result to an audio result. */
audio_result audio__result( ma_result result );

/*
 * Get a device and take a reference to it.
 * When device is NULL, the default device is returned (and created on first use).
 * Returns NULL if the default device could not be created.
 * Every successful call must be matched by a call to audio__release_device.
 */
audio_device* audio__acquire_device( audio_device* device );

/* Release a reference. The device is destroyed when the last reference is released. */
void audio__release_device( audio_device* device );

/* Reset a sound to use the closest listener. Implemented in miniaudio.c. */
void audio__ma_sound_unpin_listener( ma_sound* sound );

#endif /* AUDIO_INTERNAL_H */
