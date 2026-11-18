#pragma once

/// A 3D vector type used for positions, directions and velocities.

#include "audio.h"

namespace Audio
{

/// A 3D vector. Layout compatible with `audio_vec3`.
struct Vector
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

namespace detail
{
static_assert( sizeof( Vector ) == sizeof( audio_vec3 ), "Audio::Vector must match audio_vec3." );

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
