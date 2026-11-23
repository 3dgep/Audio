#pragma once

/// Audio::Sound: a sound effect or music loaded from a file.

#include "Config.hpp"
#include "Device.hpp"
#include "Group.hpp"
#include "Listener.hpp"
#include "Vector.hpp"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <memory>

namespace Audio
{

/// A sound loaded from a file (WAV, MP3, FLAC or OGG Vorbis).
///
/// `Sound` is a shared handle, so it can be passed around by value without thinking
/// about it: copies refer to the *same* sound (same playback position, volume, 3D
/// position, ...), and the sound is destroyed when the last copy is gone.
///
/// For an independent sound, load the same file again: decoded audio data is shared
/// between sounds loaded from the same file, so this is cheap. For sound effects that
/// overlap (footsteps, hits), use `playOneShot`.
///
/// All functions are safe to call on an empty sound (they do nothing), so a sound that
/// failed to load is silent rather than a crash.
class Sound
{
public:
    /// How the sound is loaded and played.
    enum class Type
    {
        Sound,               ///< A short sound effect, fully decoded on load. Can be placed in 3D.
        Music,               ///< A longer sound like background music, streamed while playing. Not spatialized.
        Background = Music,  ///< An alias for Music.
        Stream     = Music,  ///< An alias for Music.
    };

    /// How a spatialized sound gets quieter with distance from the listener.
    ///
    /// Note: `Off` is deliberately not named `None`, since X11's `X.h` defines
    /// `None` as a macro.
    enum class AttenuationModel : int32_t
    {
        Off         = AUDIO_ATTENUATION_MODEL_NONE,         ///< No distance attenuation.
        Inverse     = AUDIO_ATTENUATION_MODEL_INVERSE,      ///< Like OpenAL's AL_INVERSE_DISTANCE_CLAMPED. (default)
        Linear      = AUDIO_ATTENUATION_MODEL_LINEAR,       ///< Like OpenAL's AL_LINEAR_DISTANCE_CLAMPED.
        Exponential = AUDIO_ATTENUATION_MODEL_EXPONENTIAL,  ///< Like OpenAL's AL_EXPONENT_DISTANCE_CLAMPED.
    };

    /// An empty sound.
    Sound() noexcept = default;

    /// Load a sound on the default device. The path is relative to the working
    /// directory (or absolute). If loading fails, the error is printed and the sound is
    /// empty: check with `if ( sound )`, or use `load` to get the `Result`.
    explicit Sound( const std::filesystem::path& filePath, Type type = Type::Sound )
    {
        load( filePath, type );
    }

    /// Load a sound on a specific device.
    Sound( const Device& device, const std::filesystem::path& filePath, Type type = Type::Sound )
    {
        load( device, filePath, type );
    }

    /// Load a sound that plays through a group (for example "music" or "effects").
    Sound( const Group& group, const std::filesystem::path& filePath, Type type = Type::Sound )
    {
        load( group, filePath, type );
    }

    /// Load (or replace) this sound on the default device.
    Result load( const std::filesystem::path& filePath, Type type = Type::Sound )
    {
        return create( detail::useDevice( nullptr ), filePath, type );
    }

    /// Load (or replace) this sound on a specific device.
    Result load( const Device& device, const std::filesystem::path& filePath, Type type = Type::Sound )
    {
        if ( !device )
        {
            reset();
            return Result::InvalidArgs;
        }
        return create( device.get(), filePath, type );
    }

    /// Load (or replace) this sound, playing through a group.
    Result load( const Group& group, const std::filesystem::path& filePath, Type type = Type::Sound )
    {
        if ( !group )
        {
            reset();
            return Result::InvalidArgs;
        }
        return create( nullptr, filePath, type, group.get() );
    }

    /// Release this handle, leaving this object empty. The sound itself is destroyed
    /// when the last copy is released.
    void reset() noexcept
    {
        impl.reset();
    }

    /// `true` if a sound is loaded.
    explicit operator bool() const noexcept
    {
        return impl != nullptr;
    }

    /// Start playing (or resume) the sound. Does nothing if it is already playing.
    /// A sound that has played to its end starts again from the beginning.
    /// Also cancels a fade-out (`stop( fade )`).
    Result play() noexcept
    {
        return detail::toResult( audio_sound_play( get() ) );
    }

    /// Stop the sound. Calling `play` again resumes from the same position.
    Result stop() noexcept
    {
        return detail::toResult( audio_sound_stop( get() ) );
    }

    /// Fade the sound out over a number of milliseconds, then stop it.
    /// `play` restores the volume from before the fade.
    Result stop( uint64_t fadeOutMilliseconds ) noexcept
    {
        return detail::toResult( audio_sound_stop_with_fade( get(), fadeOutMilliseconds ) );
    }

    /// Fade the sound out, then stop it, for example `music.stop( 2s )`.
    template<class Rep, class Period>
    Result stop( const std::chrono::duration<Rep, Period>& fadeOut ) noexcept
    {
        return stop( detail::toMilliseconds( fadeOut ) );
    }

    /// The same as `stop`.
    Result pause() noexcept
    {
        return stop();
    }

    /// Move the sound to a group, for example `song.setGroup( music )`. A sound is in
    /// one group at a time, so this takes it out of its current group. Use
    /// `Audio::Group::none()` to play it straight to the device's output again. A
    /// playing sound keeps playing. Returns `Result::InvalidArgs` if the sound is empty,
    /// or if the group belongs to a different device.
    Result setGroup( const Group& group ) noexcept
    {
        return detail::toResult( audio_sound_set_group( get(), group.get() ) );
    }

    /// The group the sound plays through. An empty group (`if ( !sound.getGroup() )`)
    /// means it plays straight to the device's output.
    Group getGroup() const
    {
        return Group::fromHandle( audio_sound_get_group( get() ) );
    }

    /// Play a copy of the sound that overlaps with it and with other copies. Use this
    /// for sound effects that can play several times at once, like footsteps or hits.
    ///
    /// The copy starts from the beginning, with this sound's current settings (volume,
    /// pan, pitch, 3D position, ...) and in the same group. To give a copy its own
    /// pitch, set the pitch first, then call `playOneShot`. Copies can't be stopped;
    /// they end on their own.
    ///
    /// Only works for `Type::Sound`: music is streamed and can't be copied
    /// (returns `Result::InvalidOperation`).
    Result playOneShot() noexcept
    {
        return detail::toResult( audio_sound_play_one_shot( get() ) );
    }

    /// Play the sound again from the beginning.
    Result replay() noexcept
    {
        seek( 0u );
        return play();
    }

    /// Seek to a position (in milliseconds from the start), while playing or stopped.
    Result seek( uint64_t milliseconds = 0u ) noexcept
    {
        return detail::toResult( audio_sound_seek( get(), milliseconds ) );
    }

    /// Seek to a position, for example `sound.seek( 1.5s )`.
    template<class Rep, class Period>
    Result seek( const std::chrono::duration<Rep, Period>& position ) noexcept
    {
        return seek( detail::toMilliseconds( position ) );
    }

    /// The length of the sound, in seconds.
    float getDurationInSeconds() const noexcept
    {
        return audio_sound_get_duration_seconds( get() );
    }

    /// The current playback position (the cursor), in seconds from the start.
    float getCursorInSeconds() const noexcept
    {
        return audio_sound_get_cursor_seconds( get() );
    }

    /// `true` while the sound is playing: after `play`, until it is stopped or reaches its end.
    bool isPlaying() const noexcept
    {
        return audio_sound_is_playing( get() ) != AUDIO_FALSE;
    }

    /// `true` once a non-looping sound has finished. Looping sounds never end.
    bool isEnd() const noexcept
    {
        return audio_sound_is_at_end( get() ) != AUDIO_FALSE;
    }

    /// When looping, the sound starts again from the beginning when it reaches its end,
    /// until it is stopped. Default: off.
    void setLooping( bool looping ) noexcept
    {
        audio_sound_set_looping( get(), looping ? AUDIO_TRUE : AUDIO_FALSE );
    }

    /// `true` if the sound loops.
    bool isLooping() const noexcept
    {
        return audio_sound_is_looping( get() ) != AUDIO_FALSE;
    }

    /// Set the volume. 0 is silent, 1 is 100%, above 1 amplifies. Default: 1.
    void setVolume( float volume ) noexcept
    {
        audio_sound_set_volume( get(), volume );
    }

    /// Get the volume (without a fade, see `setFade`).
    float getVolume() const noexcept
    {
        return audio_sound_get_volume( get() );
    }

    /// Set the pan. -1 is fully left, 0 is centered, 1 is fully right. Default: 0.
    void setPan( float pan ) noexcept
    {
        audio_sound_set_pan( get(), pan );
    }

    /// Get the pan.
    float getPan() const noexcept
    {
        return audio_sound_get_pan( get() );
    }

    /// Set the pitch. 1 is the original pitch; larger is higher (and faster), smaller is
    /// lower (and slower). Values of 0 or less are ignored. Default: 1.
    void setPitch( float pitch ) noexcept
    {
        audio_sound_set_pitch( get(), pitch );
    }

    /// Get the pitch.
    float getPitch() const noexcept
    {
        return audio_sound_get_pitch( get() );
    }

    /// Fade from the current volume to `endVolume` over a number of milliseconds.
    /// The fade is applied on top of the volume set with `setVolume`.
    void setFade( float endVolume, uint64_t milliseconds ) noexcept
    {
        audio_sound_set_fade( get(), -1.0f, endVolume, milliseconds );
    }

    /// Fade from `beginVolume` to `endVolume` over a number of milliseconds.
    void setFade( float beginVolume, float endVolume, uint64_t milliseconds ) noexcept
    {
        audio_sound_set_fade( get(), beginVolume, endVolume, milliseconds );
    }

    /// Fade from the current volume to `endVolume`, for example `sound.setFade( 0.0f, 2s )`.
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

    /// Start the sound after a delay (in milliseconds from now). `play` must still be called.
    void setStartTime( uint64_t delayMilliseconds ) noexcept
    {
        audio_sound_schedule_start( get(), delayMilliseconds );
    }

    /// Start the sound after a delay from now, for example `sound.setStartTime( 2s )`.
    /// `play` must still be called.
    template<class Rep, class Period>
    void setStartTime( const std::chrono::duration<Rep, Period>& delay ) noexcept
    {
        setStartTime( detail::toMilliseconds( delay ) );
    }

    /// Stop the sound after a delay (in milliseconds from now).
    void setStopTime( uint64_t delayMilliseconds ) noexcept
    {
        audio_sound_schedule_stop( get(), delayMilliseconds );
    }

    /// Stop the sound after a delay from now, for example `sound.setStopTime( 2s )`.
    template<class Rep, class Period>
    void setStopTime( const std::chrono::duration<Rep, Period>& delay ) noexcept
    {
        setStopTime( detail::toMilliseconds( delay ) );
    }

    // --- Spatialization (ignored for Type::Music) ---

    /// By default, sounds are spatialized relative to the closest listener.
    /// Use this to pin the sound to a specific listener instead. Pass an invalid
    /// listener (`Audio::Listener {}`) to use the closest listener again.
    void setPinnedListener( const Listener& listener ) noexcept
    {
        audio_sound_set_pinned_listener( get(), listener ? listener.getIndex() : AUDIO_LISTENER_CLOSEST );
    }

    /// Set the position of the sound in the world (the same space as the listeners).
    /// Default: (0, 0, 0).
    void setPosition( const Vector& position ) noexcept
    {
        audio_sound_set_position( get(), detail::toC( position ) );
    }

    /// Get the position of the sound in the world.
    Vector getPosition() const noexcept
    {
        return detail::fromC( audio_sound_get_position( get() ) );
    }

    /// Set the direction the sound is facing. Only matters when a sound cone is set
    /// (see `setCone`). Default: (0, 0, -1).
    void setDirection( const Vector& direction ) noexcept
    {
        audio_sound_set_direction( get(), detail::toC( direction ) );
    }

    /// Get the direction the sound is facing.
    Vector getDirection() const noexcept
    {
        return detail::fromC( audio_sound_get_direction( get() ) );
    }

    /// Set the velocity of the sound, in world units per second. Only used for the
    /// doppler effect (see `setDopplerFactor`). Default: (0, 0, 0).
    void setVelocity( const Vector& velocity ) noexcept
    {
        audio_sound_set_velocity( get(), detail::toC( velocity ) );
    }

    /// Get the velocity of the sound.
    Vector getVelocity() const noexcept
    {
        return detail::fromC( audio_sound_get_velocity( get() ) );
    }

    /// Set the sound cone, around the sound's direction (angles in radians). A listener
    /// inside the inner cone hears the sound at full volume, a listener outside the
    /// outer cone hears it attenuated by `outerGain`, and in between it is interpolated.
    /// Default: inner and outer angles of 2 pi (360 degrees, so no attenuation) and an
    /// `outerGain` of 0.
    void setCone( float innerAngle, float outerAngle, float outerGain ) noexcept
    {
        audio_sound_set_cone( get(), innerAngle, outerAngle, outerGain );
    }

    /// Get the sound cone (see `setCone`).
    void getCone( float& innerAngle, float& outerAngle, float& outerGain ) const noexcept
    {
        audio_sound_get_cone( get(), &innerAngle, &outerAngle, &outerGain );
    }

    /// Set how the sound gets quieter with distance. Default: `AttenuationModel::Inverse`.
    void setAttenuationModel( AttenuationModel model ) noexcept
    {
        audio_sound_set_attenuation_model( get(), static_cast<audio_attenuation_model>( model ) );
    }

    /// Get how the sound gets quieter with distance.
    AttenuationModel getAttenuationModel() const noexcept
    {
        return static_cast<AttenuationModel>( audio_sound_get_attenuation_model( get() ) );
    }

    /// Set how quickly the sound gets quieter with distance. Higher values fade faster.
    /// Default: 1.
    void setRollOff( float rollOff ) noexcept
    {
        audio_sound_set_rolloff( get(), rollOff );
    }

    /// Get how quickly the sound gets quieter with distance.
    float getRollOff() const noexcept
    {
        return audio_sound_get_rolloff( get() );
    }

    /// Set the lowest volume (gain) distance and cones can reduce the sound to, so it
    /// never gets quieter than this. Default: 0.
    void setMinGain( float minGain ) noexcept
    {
        audio_sound_set_min_gain( get(), minGain );
    }

    /// Get the lowest volume (gain) from spatialization.
    float getMinGain() const noexcept
    {
        return audio_sound_get_min_gain( get() );
    }

    /// Set the highest volume (gain) from spatialization. Default: 1.
    void setMaxGain( float maxGain ) noexcept
    {
        audio_sound_set_max_gain( get(), maxGain );
    }

    /// Get the highest volume (gain) from spatialization.
    float getMaxGain() const noexcept
    {
        return audio_sound_get_max_gain( get() );
    }

    /// Set the distance at which the sound starts to get quieter. Closer than this,
    /// it plays at full volume. Default: 1.
    void setMinDistance( float minDistance ) noexcept
    {
        audio_sound_set_min_distance( get(), minDistance );
    }

    /// Get the distance at which the sound starts to get quieter.
    float getMinDistance() const noexcept
    {
        return audio_sound_get_min_distance( get() );
    }

    /// Set the distance after which the sound doesn't get any quieter.
    /// Default: the largest float (no limit).
    void setMaxDistance( float maxDistance ) noexcept
    {
        audio_sound_set_max_distance( get(), maxDistance );
    }

    /// Get the distance after which the sound doesn't get any quieter.
    float getMaxDistance() const noexcept
    {
        return audio_sound_get_max_distance( get() );
    }

    /// Set the strength of the doppler effect: the change in pitch when a sound moves
    /// towards or away from the listener (see `setVelocity`). 0 disables it. Default: 1.
    void setDopplerFactor( float dopplerFactor ) noexcept
    {
        audio_sound_set_doppler_factor( get(), dopplerFactor );
    }

    /// Get the strength of the doppler effect.
    float getDopplerFactor() const noexcept
    {
        return audio_sound_get_doppler_factor( get() );
    }

    /// The underlying C handle.
    audio_sound* get() const noexcept
    {
        return impl.get();
    }

private:
    Result create( audio_device* device, const std::filesystem::path& filePath, Type type, audio_group* group = nullptr )
    {
        // Only release this handle: other copies keep the previous sound.
        reset();

        const audio_sound_flags flags = type == Type::Music ? AUDIO_SOUND_FLAGS_MUSIC : AUDIO_SOUND_FLAG_NONE;
        const std::string       path  = detail::toUtf8( filePath );
        audio_sound*            sound = nullptr;
        const audio_result      result = group ? audio_sound_create_in_group( group, path.c_str(), flags, &sound )
                                               : audio_sound_create( device, path.c_str(), flags, &sound );
        impl                           = detail::adopt( sound, &audio_sound_destroy );
        return detail::toResult( result );
    }

    std::shared_ptr<audio_sound> impl;
};

// Group::add and Group::remove for sounds (declared in Group.hpp).
inline Result Group::add( const Sound& sound ) noexcept
{
    if ( !get() || !sound )
        return Result::InvalidArgs;
    return detail::toResult( audio_sound_set_group( sound.get(), get() ) );
}

inline Result Group::remove( const Sound& sound ) noexcept
{
    if ( !get() || audio_sound_get_group( sound.get() ) != get() )
        return Result::Success;  // Not in this group: nothing to do.
    return detail::toResult( audio_sound_set_group( sound.get(), nullptr ) );
}

}  // namespace Audio
