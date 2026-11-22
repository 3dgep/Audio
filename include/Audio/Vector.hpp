#pragma once

/// A 3D vector type used for positions, directions and velocities.

#include "audio.h"

namespace Audio
{

/// A 3D vector, for example `Audio::Vector { 1.0f, 0.0f, -2.0f }`. Layout compatible
/// with `audio_vec3`.
///
/// The library uses a right-handed coordinate system: by default, +y is up and -z is
/// forward. Use the same units as your game world (for example, meters).
struct Vector
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

namespace detail
{
static_assert( sizeof( Vector ) == sizeof( audio_vec3 ), "Audio::Vector must match audio_vec3." );

/// Convert between `Audio::Vector` and the C API's `audio_vec3`.
inline audio_vec3 toC( const Vector& v ) noexcept
{
    return { v.x, v.y, v.z };
}

inline Vector fromC( const audio_vec3& v ) noexcept
{
    return { v.x, v.y, v.z };
}
}  // namespace detail

}  // namespace Audio
