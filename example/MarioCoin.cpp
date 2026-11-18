#include "MarioCoin.hpp"

#include <Audio/Audio.hpp>

#include <span>

/// <summary>
/// Defines a point along a curve.
/// </summary>
struct Point
{
    float t;  ///< The time (or x) value.
    float v;  ///< The value at time (t).
};

using namespace Audio;

// Piano frequencies.
// Source: https://en.wikipedia.org/wiki/Piano_key_frequencies
static constexpr float B4 = 493.8833f;
static constexpr float B5 = 987.7666f;
static constexpr float E5 = 659.2551f;
static constexpr float E6 = 1318.510f;

// Amplitude curve.
static constexpr Point amplitude[] = {
    { 0.0f, 1.0f },  // Attack
    { 1.0f, 0.0f }   // Release
};

// Frequency curve.
static constexpr Point frequency[] = {
    { 0.0f, B5 },  // B5
    { 0.1f, B5 },  // Hold for 0.1 seconds.
    { 0.1f, E6 },  // E6
    { 1.0f, E6 }   // Hold until end.
};

/// <summary>
/// Helper function to interpolate between two values.
/// </summary>
constexpr float lerp( float v0, float v1, float t )
{
    return ( 1.0f - t ) * v0 + t * v1;
}

/// <summary>
/// Helper function to interpolate between two points.
/// </summary>
constexpr float lerp( const Point& p0, const Point& p1, float t )
{
    return lerp( p0.v, p1.v, t );
}

/// <summary>
/// Compute the ratio between the min and max points at time t.
/// </summary>
/// <returns>The ratio between min and max at time t.</returns>
constexpr float ratio( float min, float max, float t )
{
    return ( t - min ) / ( max - min );
}

/// <summary>
/// Helper function to interpolate a set of points at time t.
/// </summary>
float interpolate( std::span<const Point> points, float t )
{
    if ( points.empty() )
        return 0.0f;

    if ( points.size() == 1 )
        return points[0].v;

    size_t numPoints = points.size();
    auto   prev      = points.end() - 1;

    // Go backwards looking for the element that comes before t.
    while ( prev->t > t && prev != points.begin() )
    {
        --prev;
    }

    auto next = prev + 1;
    if ( next == points.end() )
    {
        next = prev;  // Both are the last point.
    }

    // Lerp the two points.
    return lerp( *prev, *next, ratio( prev->t, next->t, t ) );
}

MarioCoin::MarioCoin()
: waveform { Waveform::Type::Triangle, 0.0f, 0.0f }
{}

void MarioCoin::update( float deltaTime )
{
    if ( isPlaying )
    {
        const float f = interpolate( ::frequency, totalTime );
        const float a = interpolate( ::amplitude, totalTime );

        waveform.setFrequency( f );
        waveform.setAmplitude( a * amplitude );
        waveform.start();

        totalTime += deltaTime;
        if ( totalTime > duration )
            stop();
    }
}

void MarioCoin::play()
{
    totalTime = 0.0f;
    isPlaying = true;
}

void MarioCoin::stop()
{
    waveform.stop();
    isPlaying = false;
}
