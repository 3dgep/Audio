#pragma once

/// Audio::Listener: the "ears" in the 3D world.

#include "Config.hpp"
#include "Vector.hpp"

#include <cstdint>
#include <memory>
#include <utility>

namespace Audio
{

/// A listener (the "ears" in the 3D world).
///
/// A small value type that refers to a listener on a device. Copies refer to the same
/// listener. A listener keeps its device alive.
class Listener
{
public:
    /// An invalid listener.
    Listener() noexcept = default;

    void setPosition( const Vector& position ) noexcept
    {
        audio_listener_set_position( device.get(), index, detail::toC( position ) );
    }

    Vector getPosition() const noexcept
    {
        return detail::fromC( audio_listener_get_position( device.get(), index ) );
    }

    /// Set the forward direction of the listener.
    void setDirection( const Vector& direction ) noexcept
    {
        audio_listener_set_direction( device.get(), index, detail::toC( direction ) );
    }

    Vector getDirection() const noexcept
    {
        return detail::fromC( audio_listener_get_direction( device.get(), index ) );
    }

    /// Set the up direction of the listener. Default: (0, 1, 0).
    void setUp( const Vector& up ) noexcept
    {
        audio_listener_set_world_up( device.get(), index, detail::toC( up ) );
    }

    Vector getUp() const noexcept
    {
        return detail::fromC( audio_listener_get_world_up( device.get(), index ) );
    }

    /// Set the listener cone (angles in radians).
    /// Sounds outside the outer cone are attenuated by `outerGain`.
    void setCone( float innerAngle, float outerAngle, float outerGain ) noexcept
    {
        audio_listener_set_cone( device.get(), index, innerAngle, outerAngle, outerGain );
    }

    void getCone( float& innerAngle, float& outerAngle, float& outerGain ) const noexcept
    {
        audio_listener_get_cone( device.get(), index, &innerAngle, &outerAngle, &outerGain );
    }

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
