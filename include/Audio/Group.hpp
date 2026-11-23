#pragma once

/// Audio::Group: a channel that sounds and waveforms play through.

#include "Config.hpp"
#include "Device.hpp"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <memory>

namespace Audio
{

class Sound;
class Waveform;

/// A channel that sounds and waveforms play through, like a bus on a mixing desk.
///
/// Its volume, pan, pitch and fade apply to everything in it, and stopping a group
/// pauses everything in it. A typical game has a group for the music and one for the
/// sound effects, so the player can set their volumes separately:
/// \code
///     Audio::Group music;
///     Audio::Group effects;
///
///     Audio::Sound song { music, "music.ogg", Audio::Sound::Type::Music };
///     Audio::Sound coin { effects, "coin.wav" };
///
///     music.setVolume( 0.5f );
/// \endcode
///
/// Put a sound in a group when you load it (`Audio::Sound { music, "song.ogg" }`), or
/// later with `music.add( song )` or `song.setGroup( music )`. A sound is in exactly
/// one group at a time (or in none: then it plays straight to the device's output).
///
/// `Group` is a shared handle: copies refer to the same group. Sounds, waveforms and
/// subgroups keep their group alive, so a `Group` may be destroyed before the sounds
/// in it. All functions are safe to call on an empty group.
class Group
{
public:
    /// Create a group on the default device. Its output goes to the device's output.
    Group()
    {
        create();
    }

    /// An empty group: no group. For example, `sound.setGroup( Audio::Group::none() )`
    /// plays the sound straight to the device's output again.
    static Group none() noexcept
    {
        return Group { nullptr };
    }

    /// Create a group on a specific device.
    explicit Group( const Device& device )
    {
        create( device );
    }

    /// Create (or re-create) this group on the default device.
    Result create()
    {
        return createOn( detail::useDevice( nullptr ), nullptr );
    }

    /// Create (or re-create) this group on a specific device.
    Result create( const Device& device )
    {
        if ( !device )
        {
            reset();
            return Result::InvalidArgs;
        }
        return createOn( device.get(), nullptr );
    }

    /// Create a group inside this group (for example "UI" inside "effects"). Its
    /// output goes to this group, so this group's volume also applies to it.
    /// Returns an empty group if this group is empty.
    Group createSubgroup() const
    {
        Group subgroup { nullptr };
        if ( get() )
            subgroup.createOn( nullptr, get() );
        return subgroup;
    }

    /// Release this handle, leaving this object empty. The group itself is destroyed
    /// when the last copy and the last sound in it are gone.
    void reset() noexcept
    {
        impl.reset();
    }

    /// `true` if a group was created (`false` for `Group::none()`).
    explicit operator bool() const noexcept
    {
        return impl != nullptr;
    }

    /// `true` if both refer to the same group, for example `sound.getGroup() == music`.
    bool operator==( const Group& other ) const noexcept
    {
        return get() == other.get();
    }

    bool operator!=( const Group& other ) const noexcept
    {
        return get() != other.get();
    }

    /// Move a sound into this group: the same as `sound.setGroup( group )`. A sound is
    /// in one group at a time, so this takes it out of its current group. A playing
    /// sound keeps playing. Returns `Result::InvalidArgs` if the group or the sound is
    /// empty, or if they belong to different devices.
    Result add( const Sound& sound ) noexcept;

    /// Move a waveform into this group (see `add( sound )`).
    Result add( const Waveform& waveform ) noexcept;

    /// Take a sound out of this group, so it plays straight to the device's output.
    /// Does nothing if the sound isn't in this group.
    Result remove( const Sound& sound ) noexcept;

    /// Take a waveform out of this group (see `remove( sound )`).
    Result remove( const Waveform& waveform ) noexcept;

    /// Resume the group after `stop`, so the sounds in it can be heard again.
    /// Also cancels a fade-out (`stop( fade )`). Groups are playing when created.
    Result play() noexcept
    {
        return detail::toResult( audio_group_play( get() ) );
    }

    /// Pause everything in the group. The sounds keep their playback positions, and
    /// continue when the group is played again.
    Result stop() noexcept
    {
        return detail::toResult( audio_group_stop( get() ) );
    }

    /// The same as `stop`.
    Result pause() noexcept
    {
        return stop();
    }

    /// Fade the group out over a number of milliseconds, then stop it.
    /// `play` restores the volume from before the fade.
    Result stop( uint64_t fadeOutMilliseconds ) noexcept
    {
        return detail::toResult( audio_group_stop_with_fade( get(), fadeOutMilliseconds ) );
    }

    /// Fade the group out, then stop it, for example `music.stop( 2s )`.
    template<class Rep, class Period>
    Result stop( const std::chrono::duration<Rep, Period>& fadeOut ) noexcept
    {
        return stop( detail::toMilliseconds( fadeOut ) );
    }

    /// `true` unless the group was stopped.
    bool isPlaying() const noexcept
    {
        return audio_group_is_playing( get() ) != AUDIO_FALSE;
    }

    /// Set the volume of everything in the group, on top of each sound's own volume.
    /// 0 is silent, 1 is 100%, above 1 amplifies. Default: 1.
    void setVolume( float volume ) noexcept
    {
        audio_group_set_volume( get(), volume );
    }

    /// Get the volume of the group (without a fade, see `setFade`).
    float getVolume() const noexcept
    {
        return audio_group_get_volume( get() );
    }

    /// Set the pan. -1 is fully left, 0 is centered, 1 is fully right. Default: 0.
    void setPan( float pan ) noexcept
    {
        audio_group_set_pan( get(), pan );
    }

    /// Get the pan.
    float getPan() const noexcept
    {
        return audio_group_get_pan( get() );
    }

    /// Set the pitch of everything in the group. 1 is the original pitch. Values of 0
    /// or less are ignored. Default: 1.
    void setPitch( float pitch ) noexcept
    {
        audio_group_set_pitch( get(), pitch );
    }

    /// Get the pitch.
    float getPitch() const noexcept
    {
        return audio_group_get_pitch( get() );
    }

    /// Fade from the current volume to `endVolume` over a number of milliseconds.
    void setFade( float endVolume, uint64_t milliseconds ) noexcept
    {
        audio_group_set_fade( get(), -1.0f, endVolume, milliseconds );
    }

    /// Fade from `beginVolume` to `endVolume` over a number of milliseconds.
    void setFade( float beginVolume, float endVolume, uint64_t milliseconds ) noexcept
    {
        audio_group_set_fade( get(), beginVolume, endVolume, milliseconds );
    }

    /// Fade from the current volume to `endVolume`, for example `music.setFade( 0.0f, 2s )`.
    template<class Rep, class Period>
    void setFade( float endVolume, const std::chrono::duration<Rep, Period>& duration ) noexcept
    {
        setFade( endVolume, detail::toMilliseconds( duration ) );
    }

    /// Fade from `beginVolume` to `endVolume` over a duration.
    template<class Rep, class Period>
    void setFade( float beginVolume, float endVolume, const std::chrono::duration<Rep, Period>& duration ) noexcept
    {
        setFade( beginVolume, endVolume, detail::toMilliseconds( duration ) );
    }

    /// Play a sound in a "fire and forget" way through this group (see
    /// `Audio::playSound`). With an empty group, it plays on the default device.
    Result playSound( const std::filesystem::path& filePath ) const
    {
        if ( !get() )
            detail::useDevice( nullptr );  // A null group plays on the default device.
        return detail::toResult( audio_play_sound_in_group( get(), detail::toUtf8( filePath ).c_str() ) );
    }

    /// The underlying C handle.
    audio_group* get() const noexcept
    {
        return impl.get();
    }

private:
    friend class Sound;
    friend class Waveform;

    // An empty group.
    explicit Group( std::nullptr_t ) noexcept {}

    // A handle to an existing group, which takes a new reference (for getGroup). If
    // allocating the handle throws, shared_ptr releases the reference again.
    static Group fromHandle( audio_group* group )
    {
        Group result { nullptr };
        if ( group )
        {
            audio_group_retain( group );
            result.impl = detail::adopt( group, &audio_group_destroy );
        }
        return result;
    }

    Result createOn( audio_device* device, audio_group* parent )
    {
        reset();

        audio_group*       group  = nullptr;
        const audio_result result = audio_group_create( device, parent, &group );
        impl                      = detail::adopt( group, &audio_group_destroy );
        return detail::toResult( result );
    }

    std::shared_ptr<audio_group> impl;
};

/// Play a sound in a "fire and forget" way through a group.
inline Result playSound( const std::filesystem::path& filePath, const Group& group )
{
    return group.playSound( filePath );
}

}  // namespace Audio
