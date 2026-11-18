#pragma once

/// Audio::Waveform: a generated sine, square, triangle or sawtooth wave.

#include "Config.hpp"
#include "Device.hpp"

#include <cstdint>
#include <memory>

namespace Audio
{

/// A generated waveform (sine, square, triangle or sawtooth).
///
/// `Waveform` is a shared handle: copies refer to the same waveform, which is destroyed
/// when the last copy is gone. All functions are safe to call on an empty waveform.
class Waveform
{
public:
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

    /// Create (or replace) this waveform on the default device.
    Result create( Type type, float amplitude = 1.0f, float frequency = 440.0f )
    {
        return createOn( detail::useDevice( nullptr ), type, amplitude, frequency );
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

    explicit operator bool() const noexcept
    {
        return impl != nullptr;
    }

    Result start() noexcept
    {
        return detail::toResult( audio_waveform_start( get() ) );
    }

    Result stop() noexcept
    {
        return detail::toResult( audio_waveform_stop( get() ) );
    }

    bool isPlaying() const noexcept
    {
        return audio_waveform_is_playing( get() ) != AUDIO_FALSE;
    }

    void setType( Type type ) noexcept
    {
        audio_waveform_set_type( get(), static_cast<audio_waveform_type>( type ) );
    }

    Type getType() const noexcept
    {
        return static_cast<Type>( audio_waveform_get_type( get() ) );
    }

    void setAmplitude( float amplitude ) noexcept
    {
        audio_waveform_set_amplitude( get(), amplitude );
    }

    float getAmplitude() const noexcept
    {
        return audio_waveform_get_amplitude( get() );
    }

    /// Frequency in Hz.
    void setFrequency( float frequency ) noexcept
    {
        audio_waveform_set_frequency( get(), frequency );
    }

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
    Result createOn( audio_device* device, Type type, float amplitude, float frequency )
    {
        reset();

        audio_waveform*    waveform = nullptr;
        const audio_result result   = audio_waveform_create( device, static_cast<audio_waveform_type>( type ), amplitude, frequency, &waveform );
        impl                        = detail::adopt( waveform, &audio_waveform_destroy );
        return detail::toResult( result );
    }

    std::shared_ptr<audio_waveform> impl;
};

}  // namespace Audio
