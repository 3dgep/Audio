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

/*
 * A sound played with audio_sound_play_one_shot. The device owns it and frees it once
 * it has finished (see audio__free_finished_one_shots). It holds no references: it is
 * stopped and freed when its group or its device is destroyed.
 */
typedef struct audio_one_shot
{
    ma_sound               sound;
    audio_group*           group; /* NULL for the device's output. */
    struct audio_one_shot* next;
} audio_one_shot;

struct audio_device
{
    volatile ma_spinlock lock;      /* Protects ref_count. */
    uint32_t             ref_count; /* One for the creator, plus one per sound, waveform and group. */
    uint32_t             listener_count;
    audio_one_shot*      one_shots; /* Playing one-shots, newest first. */
#if defined( _WIN32 )
    audio_utf8_vfs vfs; /* Must outlive the engine. */
#endif
    ma_engine engine;
};

/*
 * The state of a fade-out started by audio_sound_stop_with_fade or
 * audio_group_stop_with_fade. Playing again restores the volume from before the fade.
 */
typedef struct audio_fade_out
{
    audio_bool32 active;
    float        volume; /* The fade volume before the fade-out. */
} audio_fade_out;

struct audio_group
{
    audio_device*  device;    /* Holds a reference. */
    audio_group*   parent;    /* Holds a reference. NULL for the device's output. */
    uint32_t       ref_count; /* One for the creator, plus one per sound, waveform and child group. */
    audio_fade_out fade_out;
    ma_sound_group group;
};

struct audio_sound
{
    audio_device*  device; /* Holds a reference. */
    audio_group*   group;  /* Holds a reference. NULL for the device's output. */
    audio_fade_out fade_out;
    ma_sound       sound;
};

struct audio_waveform
{
    audio_device*       device; /* Holds a reference. */
    audio_group*        group;  /* Holds a reference. NULL for the device's output. */
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

/* Take and release a reference to a group. The group is destroyed when the last
 * reference is released. audio__retain_group accepts NULL. */
void audio__retain_group( audio_group* group );
void audio__release_group( audio_group* group );

/* The node that sounds and waveforms in a group attach to: the group, or the device's
 * output when group is NULL. */
ma_node* audio__group_node( audio_device* device, audio_group* group );

/* Attach a sound's or waveform's node to a group (or the device's output when group
 * is NULL), and move the reference in *current to the new group. function is the name
 * of the public function, for error messages. */
audio_result audio__move_to_group( audio_device* device, ma_node* node, audio_group** current, audio_group* group, const char* function );

/* Play and stop with a fade-out, for sounds and groups (a group is an ma_sound).
 * Playing cancels a fade-out, and clears a stop time that has passed. */
ma_result audio__ma_sound_play( ma_sound* sound, audio_fade_out* fade_out );
ma_result audio__ma_sound_stop_with_fade( ma_sound* sound, audio_fade_out* fade_out, uint64_t milliseconds );

/* One-shots (audio_sound_play_one_shot). */
void audio__free_finished_one_shots( audio_device* device );
void audio__free_one_shots( audio_device* device, audio_group* group, audio_bool32 all );

/* Reset a sound to use the closest listener. Implemented in miniaudio.c. */
void audio__ma_sound_unpin_listener( ma_sound* sound );

#endif /* AUDIO_INTERNAL_H */
