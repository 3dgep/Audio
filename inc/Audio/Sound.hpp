#pragma once

/// Audio::Sound: a sound effect or music loaded from a file.

#include "Config.hpp"
#include "Device.hpp"
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
/// between sounds loaded from the same file, so this is cheap. For overlapping one-shot
/// effects, use `Audio::playSound`.
///
/// All functions are safe to call on an empty sound (they do nothing), so a sound that
/// failed to load is silent rather than a crash.
class Sound
{
public:
    enum class Type
    {
        Sound,               ///< A short sound effect, fully decoded on load.
        Music,               ///< A longer sound like background music, streamed while playing.
        Background = Music,  ///< An alias for Music.
        Stream     = Music,  ///< An alias for Music.
    };

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

    /// Load a sound on the default device.
    explicit Sound( const std::filesystem::path& filePath, Type type = Type::Sound )
    {
        load( filePath, type );
    }

    /// Load a sound on a specific device.
    Sound( const Device& device, const std::filesystem::path& filePath, Type type = Type::Sound )
    {
        load( device, filePath, type );
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

    /// Start playing (or resume) the sound.
    Result play() noexcept
    {
        return detail::toResult( audio_sound_play( get() ) );
    }

    /// Stop the sound. Calling `play` again resumes from the same position.
    Result stop() noexcept
    {
        return detail::toResult( audio_sound_stop( get() ) );
    }

    /// The same as `stop`.
    Result pause() noexcept
    {
        return stop();
    }

    /// Play the sound again from the beginning.
    Result replay() noexcept
    {
        seek( 0u );
        return play();
    }

    /// Seek to a position (in milliseconds).
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

    float getDurationInSeconds() const noexcept
    {
        return audio_sound_get_duration_seconds( get() );
    }

    float getCursorInSeconds() const noexcept
    {
        return audio_sound_get_cursor_seconds( get() );
    }

    bool isPlaying() const noexcept
    {
        return audio_sound_is_playing( get() ) != AUDIO_FALSE;
    }

    /// `true` once a non-looping sound has finished. Looping sounds never end.
    bool isEnd() const noexcept
    {
        return audio_sound_is_at_end( get() ) != AUDIO_FALSE;
    }

    void setLooping( bool looping ) noexcept
    {
        audio_sound_set_looping( get(), looping ? AUDIO_TRUE : AUDIO_FALSE );
    }

    bool isLooping() const noexcept
    {
        return audio_sound_is_looping( get() ) != AUDIO_FALSE;
    }

    /// 0 is silent, 1 is 100%, above 1 amplifies.
    void setVolume( float volume ) noexcept
    {
        audio_sound_set_volume( get(), volume );
    }

    float getVolume() const noexcept
    {
        return audio_sound_get_volume( get() );
    }

    /// -1 is fully left, 0 is centered, 1 is fully right.
    void setPan( float pan ) noexcept
    {
        audio_sound_set_pan( get(), pan );
    }

    float getPan() const noexcept
    {
        return audio_sound_get_pan( get() );
    }

    /// Must be greater than 0. 1 is the original pitch; larger is higher.
    void setPitch( float pitch ) noexcept
    {
        audio_sound_set_pitch( get(), pitch );
    }

    float getPitch() const noexcept
    {
        return audio_sound_get_pitch( get() );
    }

    /// Fade from the current volume to `endVolume` over a number of milliseconds.
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

    template<class Rep, class Period>
    void setStopTime( const std::chrono::duration<Rep, Period>& delay ) noexcept
    {
        setStopTime( detail::toMilliseconds( delay ) );
    }

    // --- Spatialization (ignored for Type::Music) ---

    /// By default, sounds are spatialized relative to the closest listener.
    /// Use this to pin the sound to a specific listener instead.
    void setPinnedListener( const Listener& listener ) noexcept
    {
        audio_sound_set_pinned_listener( get(), listener ? listener.getIndex() : AUDIO_LISTENER_CLOSEST );
    }

    void setPosition( const Vector& position ) noexcept
    {
        audio_sound_set_position( get(), detail::toC( position ) );
    }

    Vector getPosition() const noexcept
    {
        return detail::fromC( audio_sound_get_position( get() ) );
    }

    void setDirection( const Vector& direction ) noexcept
    {
        audio_sound_set_direction( get(), detail::toC( direction ) );
    }

    Vector getDirection() const noexcept
    {
        return detail::fromC( audio_sound_get_direction( get() ) );
    }

    void setVelocity( const Vector& velocity ) noexcept
    {
        audio_sound_set_velocity( get(), detail::toC( velocity ) );
    }

    Vector getVelocity() const noexcept
    {
        return detail::fromC( audio_sound_get_velocity( get() ) );
    }

    /// Set the sound cone (angles in radians).
    void setCone( float innerAngle, float outerAngle, float outerGain ) noexcept
    {
        audio_sound_set_cone( get(), innerAngle, outerAngle, outerGain );
    }

    void getCone( float& innerAngle, float& outerAngle, float& outerGain ) const noexcept
    {
        audio_sound_get_cone( get(), &innerAngle, &outerAngle, &outerGain );
    }

    void setAttenuationModel( AttenuationModel model ) noexcept
    {
        audio_sound_set_attenuation_model( get(), static_cast<audio_attenuation_model>( model ) );
    }

    AttenuationModel getAttenuationModel() const noexcept
    {
        return static_cast<AttenuationModel>( audio_sound_get_attenuation_model( get() ) );
    }

    void setRollOff( float rollOff ) noexcept
    {
        audio_sound_set_rolloff( get(), rollOff );
    }

    float getRollOff() const noexcept
    {
        return audio_sound_get_rolloff( get() );
    }

    void setMinGain( float minGain ) noexcept
    {
        audio_sound_set_min_gain( get(), minGain );
    }

    float getMinGain() const noexcept
    {
        return audio_sound_get_min_gain( get() );
    }

    void setMaxGain( float maxGain ) noexcept
    {
        audio_sound_set_max_gain( get(), maxGain );
    }

    float getMaxGain() const noexcept
    {
        return audio_sound_get_max_gain( get() );
    }

    void setMinDistance( float minDistance ) noexcept
    {
        audio_sound_set_min_distance( get(), minDistance );
    }

    float getMinDistance() const noexcept
    {
        return audio_sound_get_min_distance( get() );
    }

    void setMaxDistance( float maxDistance ) noexcept
    {
        audio_sound_set_max_distance( get(), maxDistance );
    }

    float getMaxDistance() const noexcept
    {
        return audio_sound_get_max_distance( get() );
    }

    void setDopplerFactor( float dopplerFactor ) noexcept
    {
        audio_sound_set_doppler_factor( get(), dopplerFactor );
    }

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
    Result create( audio_device* device, const std::filesystem::path& filePath, Type type )
    {
        // Only release this handle: other copies keep the previous sound.
        reset();

        const audio_sound_flags flags  = type == Type::Music ? AUDIO_SOUND_FLAGS_MUSIC : AUDIO_SOUND_FLAG_NONE;
        audio_sound*            sound  = nullptr;
        const audio_result      result = audio_sound_create( device, detail::toUtf8( filePath ).c_str(), flags, &sound );
        impl                           = detail::adopt( sound, &audio_sound_destroy );
        return detail::toResult( result );
    }

    std::shared_ptr<audio_sound> impl;
};

}  // namespace Audio
