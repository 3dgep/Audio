#pragma once

/// Audio::Device and the functions that use the default device
/// (Audio::playSound, Audio::setMasterVolume, Audio::getListener).

#include "Config.hpp"
#include "Listener.hpp"

#include <cstdint>
#include <filesystem>
#include <memory>

namespace Audio
{

/// An explicitly created audio device.
///
/// Most programs do not need this: sounds, waveforms and the free functions below use
/// the default device. Create a `Device` when you need control over its configuration.
///
/// `Device` is a shared handle: copies refer to the same device. Sounds and waveforms
/// keep their device alive, so a `Device` may be destroyed before the sounds created
/// with it.
class Device
{
public:
    struct Config
    {
        uint32_t listenerCount = 0;  ///< [1 .. 4]. 0 = the maximum (4).
        uint32_t sampleRate    = 0;  ///< 0 = the playback device's native sample rate.
        uint32_t channels      = 0;  ///< 0 = the playback device's native channel count.
    };

    /// An empty device. Use `create` to create one.
    Device() noexcept = default;

    /// Create a device. Check the result with `operator bool` or `create`'s return value.
    explicit Device( const Config& config )
    {
        create( config );
    }

    /// Create (or re-create) this device.
    Result create( const Config& config = {} )
    {
        reset();

        audio_device_config c = audio_device_config_init();
        c.listener_count      = config.listenerCount;
        c.sample_rate         = config.sampleRate;
        c.channels            = config.channels;

        audio_device*      device = nullptr;
        const audio_result result = audio_device_create( &c, &device );
        impl                      = detail::adopt( device, &audio_device_destroy );
        return detail::toResult( result );
    }

    /// Release this handle. The device is destroyed when the last copy of this handle
    /// and the last sound created with it are gone.
    void reset() noexcept
    {
        impl.reset();
    }

    explicit operator bool() const noexcept
    {
        return impl != nullptr;
    }

    /// Play a sound in a "fire and forget" way.
    Result playSound( const std::filesystem::path& filePath ) const
    {
        if ( !get() )
            return Result::InvalidOperation;
        return detail::toResult( audio_play_sound( get(), detail::toUtf8( filePath ).c_str() ) );
    }

    void setMasterVolume( float volume ) const noexcept
    {
        if ( get() )
            audio_device_set_master_volume( get(), volume );
    }

    float getMasterVolume() const noexcept
    {
        return get() ? audio_device_get_master_volume( get() ) : 0.0f;
    }

    uint32_t getSampleRate() const noexcept
    {
        return get() ? audio_device_get_sample_rate( get() ) : 0u;
    }

    uint32_t getChannels() const noexcept
    {
        return get() ? audio_device_get_channels( get() ) : 0u;
    }

    uint32_t getListenerCount() const noexcept
    {
        return get() ? audio_device_get_listener_count( get() ) : 0u;
    }

    /// Get a listener. Returns an invalid listener if the device is empty or the
    /// index is out of range.
    Listener getListener( uint32_t index = 0 ) const noexcept
    {
        return impl ? Listener { impl, index } : Listener {};
    }

    /// The underlying C handle.
    audio_device* get() const noexcept
    {
        return impl.get();
    }

private:
    std::shared_ptr<audio_device> impl;
};

// ----------------------------------------------------------------------------
// Default device
// ----------------------------------------------------------------------------

/// Play a sound in a "fire and forget" way on the default device.
/// Multiple calls may overlap. There is no way to stop the sound once started.
inline Result playSound( const std::filesystem::path& filePath )
{
    return detail::toResult( audio_play_sound( detail::useDevice( nullptr ), detail::toUtf8( filePath ).c_str() ) );
}

/// Set the master volume of the default device. 0 is silent, 1 is 100%, above 1 amplifies.
inline void setMasterVolume( float volume ) noexcept
{
    audio_device_set_master_volume( detail::useDevice( nullptr ), volume );
}

inline float getMasterVolume() noexcept
{
    return audio_device_get_master_volume( detail::useDevice( nullptr ) );
}

/// Get a listener of the default device [0 .. 3].
inline Listener getListener( uint32_t index = 0 ) noexcept
{
    detail::useDevice( nullptr );
    return Listener { nullptr, index };
}

}  // namespace Audio
