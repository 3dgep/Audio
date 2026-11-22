#pragma once

/// Audio::Listener: the "ears" in the 3D world.

#include "Config.hpp"
#include "Vector.hpp"

#include <cstdint>
#include <memory>
#include <utility>

namespace Audio
{

/// A listener (the "ears" in the 3D world), usually the player or the camera.
///
/// What you hear from a sound depends on where it is relative to the listener. Get a
/// listener with `Audio::getListener()` (or `Device::getListener` for your own device).
///
/// A small value type that refers to a listener on a device. Copies refer to the same
/// listener. A listener keeps its device alive. All functions are safe to call on an
/// invalid listener: setters do nothing, and getters return zeros.
class Listener
{
public:
    /// An invalid listener.
    Listener() noexcept = default;

    /// Set the position of the listener in the world. Default: (0, 0, 0).
    void setPosition( const Vector& position ) noexcept
    {
        audio_listener_set_position( device.get(), index, detail::toC( position ) );
    }

    /// Get the position of the listener in the world.
    Vector getPosition() const noexcept
    {
        return detail::fromC( audio_listener_get_position( device.get(), index ) );
    }

    /// Set the forward direction of the listener (the way it is facing). Default: (0, 0, -1).
    void setDirection( const Vector& direction ) noexcept
    {
        audio_listener_set_direction( device.get(), index, detail::toC( direction ) );
    }

    /// Get the forward direction of the listener.
    Vector getDirection() const noexcept
    {
        return detail::fromC( audio_listener_get_direction( device.get(), index ) );
    }

    /// Set the up direction of the listener. Default: (0, 1, 0).
    void setUp( const Vector& up ) noexcept
    {
        audio_listener_set_world_up( device.get(), index, detail::toC( up ) );
    }

    /// Get the up direction of the listener.
    Vector getUp() const noexcept
    {
        return detail::fromC( audio_listener_get_world_up( device.get(), index ) );
    }

    /// Set the listener cone, around its forward direction (angles in radians).
    /// Sounds inside the inner cone are not attenuated, sounds outside the outer cone
    /// are attenuated by `outerGain`, and sounds in between are interpolated.
    /// Default: inner and outer angles of 2 pi (360 degrees, so no attenuation) and an
    /// `outerGain` of 0.
    void setCone( float innerAngle, float outerAngle, float outerGain ) noexcept
    {
        audio_listener_set_cone( device.get(), index, innerAngle, outerAngle, outerGain );
    }

    /// Get the listener cone (see `setCone`).
    void getCone( float& innerAngle, float& outerAngle, float& outerGain ) const noexcept
    {
        audio_listener_get_cone( device.get(), index, &innerAngle, &outerAngle, &outerGain );
    }

    /// The index of this listener on its device [0 .. 3], or 4 for an invalid listener.
    uint32_t getIndex() const noexcept
    {
        return index;
    }

    /// `true` if this refers to a listener that exists on its device.
    explicit operator bool() const noexcept
    {
        return index < audio_device_get_listener_count( device.get() );
    }

private:
    friend class Device;
    friend Listener getListener( uint32_t ) noexcept;

    Listener( std::shared_ptr<audio_device> device, uint32_t index ) noexcept
    : device { std::move( device ) }
    , index { index }
    {}

    std::shared_ptr<audio_device> device;  // Empty = the default device.
    uint32_t                      index = AUDIO_MAX_LISTENERS;
};

}  // namespace Audio
