#pragma once

/// Audio::Waveform: a generated sine, square, triangle or sawtooth wave.

#include "Config.hpp"
#include "Device.hpp"
#include "Group.hpp"

#include <cstdint>
#include <memory>

namespace Audio
{

/// A generated waveform (sine, square, triangle or sawtooth): a tone that is generated
/// while it plays, instead of loaded from a file. Good for retro sound effects.
///
/// The amplitude is the volume (0 is silent, 1 is full volume) and the frequency is the
/// pitch in Hz (for example 440 for the note A4). Waveforms are not spatialized.
///
/// `Waveform` is a shared handle: copies refer to the same waveform, which is destroyed
/// when the last copy is gone. All functions are safe to call on an empty waveform.
class Waveform
{
public:
    /// The shape of the wave, which changes how the tone sounds.
    enum class Type : int32_t
    {
        Sine     = AUDIO_WAVEFORM_TYPE_SINE,      ///< See: https://en.wikipedia.org/wiki/Sine_wave
        Square   = AUDIO_WAVEFORM_TYPE_SQUARE,    ///< See: https://en.wikipedia.org/wiki/Square_wave
        Triangle = AUDIO_WAVEFORM_TYPE_TRIANGLE,  ///< See: https://en.wikipedia.org/wiki/Triangle_wave
        Sawtooth = AUDIO_WAVEFORM_TYPE_SAWTOOTH,  ///< See: https://en.wikipedia.org/wiki/Sawtooth_wave
    };

    /// An empty waveform.
    Waveform() noexcept = default;

    /// Create a waveform on the default device. The waveform starts stopped.
    explicit Waveform( Type type, float amplitude = 1.0f, float frequency = 440.0f )
    {
        create( type, amplitude, frequency );
    }

    /// Create a waveform on a specific device. The waveform starts stopped.
    Waveform( const Device& device, Type type, float amplitude = 1.0f, float frequency = 440.0f )
    {
        create( device, type, amplitude, frequency );
    }

    /// Create a waveform that plays through a group. The waveform starts stopped.
    Waveform( const Group& group, Type type, float amplitude = 1.0f, float frequency = 440.0f )
    {
        create( group, type, amplitude, frequency );
    }

    /// Create (or replace) this waveform on the default device.
    Result create( Type type, float amplitude = 1.0f, float frequency = 440.0f )
    {
        return createOn( detail::useDevice( nullptr ), type, amplitude, frequency );
    }

    /// Create (or replace) this waveform, playing through a group.
    Result create( const Group& group, Type type, float amplitude = 1.0f, float frequency = 440.0f )
    {
        if ( !group )
        {
            reset();
            return Result::InvalidArgs;
        }
        return createOn( nullptr, type, amplitude, frequency, group.get() );
    }

    /// Create (or replace) this waveform on a specific device.
    Result create( const Device& device, Type type, float amplitude = 1.0f, float frequency = 440.0f )
    {
        if ( !device )
        {
            reset();
            return Result::InvalidArgs;
        }
        return createOn( device.get(), type, amplitude, frequency );
    }

    /// Release this handle, leaving this object empty. The waveform itself is
    /// destroyed when the last copy is released.
    void reset() noexcept
    {
        impl.reset();
    }

    /// `true` if a waveform was created.
    explicit operator bool() const noexcept
    {
        return impl != nullptr;
    }

    /// Start playing the waveform. Does nothing if it is already playing.
    Result start() noexcept
    {
        return detail::toResult( audio_waveform_start( get() ) );
    }

    /// Stop playing the waveform.
    Result stop() noexcept
    {
        return detail::toResult( audio_waveform_stop( get() ) );
    }

    /// `true` after `start`, until `stop`.
    bool isPlaying() const noexcept
    {
        return audio_waveform_is_playing( get() ) != AUDIO_FALSE;
    }

    /// Change the shape of the wave, also while it plays.
    void setType( Type type ) noexcept
    {
        audio_waveform_set_type( get(), static_cast<audio_waveform_type>( type ) );
    }

    /// Get the shape of the wave.
    Type getType() const noexcept
    {
        return static_cast<Type>( audio_waveform_get_type( get() ) );
    }

    /// Move the waveform to a group (see `Sound::setGroup`). Use
    /// `Audio::Group::none()` to play it straight to the device's output again.
    Result setGroup( const Group& group ) noexcept
    {
        return detail::toResult( audio_waveform_set_group( get(), group.get() ) );
    }

    /// The group the waveform plays through. An empty group means it plays straight
    /// to the device's output.
    Group getGroup() const
    {
        return Group::fromHandle( audio_waveform_get_group( get() ) );
    }

    /// Set the amplitude (volume). 0 is silent, 1 is full volume.
    void setAmplitude( float amplitude ) noexcept
    {
        audio_waveform_set_amplitude( get(), amplitude );
    }

    /// Get the amplitude (volume).
    float getAmplitude() const noexcept
    {
        return audio_waveform_get_amplitude( get() );
    }

    /// Set the frequency (pitch) in Hz, also while it plays. Doubling the frequency
    /// makes the tone one octave higher.
    void setFrequency( float frequency ) noexcept
    {
        audio_waveform_set_frequency( get(), frequency );
    }

    /// Get the frequency (pitch) in Hz.
    float getFrequency() const noexcept
    {
        return audio_waveform_get_frequency( get() );
    }

    /// The underlying C handle.
    audio_waveform* get() const noexcept
    {
        return impl.get();
    }

private:
    Result createOn( audio_device* device, Type type, float amplitude, float frequency, audio_group* group = nullptr )
    {
        reset();

        const auto         waveType = static_cast<audio_waveform_type>( type );
        audio_waveform*    waveform = nullptr;
        const audio_result result   = group ? audio_waveform_create_in_group( group, waveType, amplitude, frequency, &waveform )
                                            : audio_waveform_create( device, waveType, amplitude, frequency, &waveform );
        impl                        = detail::adopt( waveform, &audio_waveform_destroy );
        return detail::toResult( result );
    }

    std::shared_ptr<audio_waveform> impl;
};

// Group::add and Group::remove for waveforms (declared in Group.hpp).
inline Result Group::add( const Waveform& waveform ) noexcept
{
    if ( !get() || !waveform )
        return Result::InvalidArgs;
    return detail::toResult( audio_waveform_set_group( waveform.get(), get() ) );
}

inline Result Group::remove( const Waveform& waveform ) noexcept
{
    if ( !get() || audio_waveform_get_group( waveform.get() ) != get() )
        return Result::Success;  // Not in this group: nothing to do.
    return detail::toResult( audio_waveform_set_group( waveform.get(), nullptr ) );
}

}  // namespace Audio
