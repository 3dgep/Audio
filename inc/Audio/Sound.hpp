#pragma once

#include "Config.hpp"
#include "Listener.hpp"
#include "Vector.hpp"

#include <cstddef>
#include <cstdint>
#include <chrono>
#include <filesystem>
#include <memory>

namespace Audio
{
class SoundImpl;

class AUDIO_API Sound
{
public:
    enum class Type
    {
        Sound,               ///< A short sound effect.
        Music,               ///< A longer sound like background music.
        Background = Music,  ///< An alias for Music.
        Stream     = Music,  ///< An alias for Music.
    };

    enum class AttenuationModel
    {
        NoAttenuation,         ///< No distance attenuation and no specialization.
        Inverse,      ///< Equivalent to OpenAL's AL_INVERSE_DISTANCE_CLAMPED.
        Linear,       ///< Linear attenuation. Equivalent to OpenAL's AL_LINEAR_DISTANCE_CLAMPED.
        Exponential,  ///< Exponential attenuation. Equivalent to OpenAL's AL_EXPONENT_DISTANCE_CLAMPED.
    };

    explicit Sound( const std::filesystem::path& filePath, Type type = Type::Sound );

    /// <summary>
    /// Load a sound effect from a file.
    /// Use this to load short sounds like sound effects.
    /// </summary>
    /// <param name="filePath">The path to the sound file.</param>
    void loadSound( const std::filesystem::path& filePath );

    /// <summary>
    /// Load a music file.
    /// Use this to load longer sounds like background music.
    /// </summary>
    /// <param name="filePath">The path to the music file.</param>
    void loadMusic( const std::filesystem::path& filePath );

    /// <summary>
    /// Start playing the sound.
    /// </summary>
    void play();

    /// <summary>
    /// Stop the playing sound.
    /// Note: Non-looping sounds will stop automatically.
    /// </summary>
    void stop();

    /// <summary>
    /// Pause the playing sound.
    /// Note: This is the same as Sound::stop().
    /// </summary>
    void pause();

    /// <summary>
    /// Get the duration of the sound in seconds.
    /// </summary>
    /// <returns>The duration of the sound (in seconds).</returns>
    float getDurationInSeconds() const;

    /// <summary>
    /// Get the current cursor position of the sound in seconds.
    /// </summary>
    /// <returns>The current cursor position of the sound (in seconds).</returns>
    float getCursorInSeconds() const;

    /// <summary>
    /// Seek to a specific position in the sound.
    /// Note: `duration` must be less than the duration of the sound.
    /// </summary>
    /// <typeparam name="Rep">The representation of the duration.</typeparam>
    /// <typeparam name="Period">The period of the duration.</typeparam>
    /// <param name="duration">The duration to seek to.</param>
    template<typename Rep, typename Period = std::ratio<1>>
    void seek( const std::chrono::duration<Rep, Period>& duration );

    /// <summary>
    /// Seek to a specific position (in milliseconds) in the sound.
    /// </summary>
    /// <param name="milliseconds">(optional) The milliseconds to seek to. Default: 0</param>
    void seek( uint64_t milliseconds = 0ull );

    /// <summary>
    /// Restart the playing sound from the beginning.
    /// This is useful if the sound is already playing, but you want to play it again from the beginning.
    /// If you only want to seek to the beginning without playing the sound, use `seek(0)`.
    /// </summary>
    void replay();

    /// <summary>
    /// Check if the sound is currently playing.
    /// </summary>
    /// <returns>`true` if the sound is playing, false otherwise.</returns>
    bool isPlaying() const;

    /// <summary>
    /// Check if the sound is finished playing.
    /// Note: Looping sounds will always return false.
    /// </summary>
    /// <returns>`true` if the sound is finished playing, `false` otherwise.</returns>
    bool isEnd() const;

    /// <summary>
    /// Loop the sound.
    /// </summary>
    /// <param name="looping">`true` to make the sound loop, `false` to disable looping.</param>
    void setLooping( bool looping );

    /// <summary>
    /// Check if the sound is looping.
    /// </summary>
    /// <returns>`true` if the sound is looping, `false` otherwise.</returns>
    bool isLooping() const;

    /// <summary>
    /// By default, sounds will be spatialized based on the closest listener.
    /// Sounds can be pinned to a specific listener using this function.
    /// </summary>
    /// <param name="listener">The listener to pin this sound to.</param>
    void setPinnedListener( const Listener& listener );

    /// <summary>
    /// Set the volume of this sound.
    /// </summary>
    /// <param name="volume">The volume to set this sound to (in the range [0 .. 1])</param>
    void setVolume( float volume );

    /// <summary>
    /// Get the volume of this sound.
    /// </summary>
    /// <returns>The current volume of this sound.</returns>
    float getVolume() const;

    /// <summary>
    /// Set the pan of this sound.
    /// A value of 0 disables panning.
    /// A value of -1 pans the sound to the left.
    /// A value of 1 pans the sound to the right.
    /// </summary>
    /// <param name="pan">The pan value for the sound.</param>
    void setPan( float pan );

    /// <summary>
    /// Get the current pan of this sound.
    /// </summary>
    /// <returns>The current pan of this sound.</returns>
    float getPan() const;

    /// <summary>
    /// Set the pitch of this sound.
    /// The pitch must be greater than 1.
    /// A larger value will result in a higher pitch.
    /// </summary>
    /// <param name="pitch">The pitch to set this sound to.</param>
    void setPitch( float pitch );

    /// <summary>
    /// Get the current pitch of this sound.
    /// </summary>
    /// <returns>The current pitch of this sound.</returns>
    float getPitch() const;

    /// <summary>
    /// Set the position of this spatialized sounds.
    /// </summary>
    /// <param name="position">The position to set this sound to.</param>
    void setPosition( const Vector& position );

    /// <summary>
    /// Get the position of this sound.
    /// </summary>
    /// <returns>The position of this sound in world space.</returns>
    Vector getPosition() const;

    /// <summary>
    /// Set the forward direction of this spatialized sounds.
    /// </summary>
    /// <param name="direction">The forward direction of this sound.</param>
    void setDirection( const Vector& direction );

    /// <summary>
    /// Get the forward direction of this spatialized sound.
    /// </summary>
    /// <returns>The forward direction of this sound.</returns>
    Vector getDirection() const;

    /// <summary>
    /// Set the velocity of this spatialized sound.
    /// </summary>
    /// <param name="velocity">The velocity of this sound.</param>
    void setVelocity( const Vector& velocity );

    /// <summary>
    /// Get the velocity of this spatialized sound.
    /// </summary>
    /// <returns>The velocity of this sound.</returns>
    Vector getVelocity() const;

    /// <summary>
    /// Set the cone used to control attenuation of this sound.
    /// </summary>
    /// <param name="innerAngle">The inner cone angle (in radians).</param>
    /// <param name="outerAngle">The outer cone angle (in radians).</param>
    /// <param name="outerGain">The gain to use when the listener is outside the `outerAngle`.</param>
    void setCone( float innerAngle, float outerAngle, float outerGain );

    /// <summary>
    /// Get the cone used to control attenuation of this sound.
    /// </summary>
    /// <param name="innerAngle">A reference to the inner cone angle (in radians).</param>
    /// <param name="outerAngle">A reference to the outer cone angle (in radians).</param>
    /// <param name="outerGain">A reference to the gain when the listener is outside the `outerAngle`.</param>
    void getCone( float& innerAngle, float& outerAngle, float& outerGain ) const;

    /// <summary>
    /// Set the attenuation model for this sound.
    /// </summary>
    /// <param name="attenuation">The attenuation model to use for this sound.</param>
    void setAttenuationModel( AttenuationModel attenuation );

    /// <summary>
    /// Get the attenuation model for this sound.
    /// </summary>
    /// <returns>The attenuation model for this sound.</returns>
    AttenuationModel getAttenuationModel() const;

    /// <summary>
    /// Set the roll off factor for this sound.
    /// Roll off controls how quickly the sound attenuates as it moves away from the listener.
    /// </summary>
    /// <param name="rollOff">The roll off factor for this sound.</param>
    void setRollOff( float rollOff );

    /// <summary>
    /// Get the roll off factor for this sound.
    /// </summary>
    /// <returns>The roll off factor for this sound.</returns>
    float getRollOff() const;

    /// <summary>
    /// The minimum gain to apply for spatialization for this sound.
    /// </summary>
    /// <param name="minGain">The minimum gain factor.</param>
    void setMinGain( float minGain );

    /// <summary>
    /// Get the minimum gain to apply for spatialization for this sound.
    /// </summary>
    /// <returns>The minimum gain factor for this sound.</returns>
    float getMinGain() const;

    /// <summary>
    /// Set the maximum gain to apply for spatialization for this sound.
    /// </summary>
    /// <param name="maxGain">The maximum gain factor.</param>
    void setMaxGain( float maxGain );

    /// <summary>
    /// Get the maximum gain to apply for spatialization for this sound.
    /// </summary>
    /// <returns>The maximum gain factor for this sound.</returns>
    float getMaxGain() const;

    /// <summary>
    /// Set the minimum distance for attenuation of this sound.
    /// </summary>
    /// <param name="minDistance">The minimum attenuation distance.</param>
    void setMinDistance( float minDistance );

    /// <summary>
    /// Get the minimum distance for attenuation of this sound.
    /// </summary>
    /// <returns>The minimum attenuation distance of this sound.</returns>
    float getMinDistance() const;

    /// <summary>
    /// Set the maximum distance for attenuation of this sound.
    /// </summary>
    /// <param name="maxDistance">The maximum distance for attenuation of this sound.</param>
    void setMaxDistance( float maxDistance );

    /// <summary>
    /// Get the maximum distance for attenuation of this sound.
    /// </summary>
    /// <returns>The maximum distance for attenuation of this sound.</returns>
    float getMaxDistance() const;

    /// <summary>
    /// Set the factor for the doppler effect of this sound.
    /// </summary>
    /// <param name="dopplerFactor">The doppler effect factor to apply to this sound.</param>
    void setDopplerFactor( float dopplerFactor );

    /// <summary>
    /// Get the factor for the doppler effect of this sound.
    /// </summary>
    /// <returns>The doppler effect factor.</returns>
    float getDopplerFactor() const;

    /// <summary>
    /// Fade this sound in or out over a period of time.
    /// </summary>
    /// <typeparam name="Rep">The representation of the duration.</typeparam>
    /// <typeparam name="Period">The duration period.</typeparam>
    /// <param name="endVolume">The volume to fade this sound to.</param>
    /// <param name="duration">The time to get to the `endVolume`.</param>
    template<class Rep, class Period = std::ratio<1>>
    void setFade( float endVolume, const std::chrono::duration<Rep, Period>& duration );

    /// <summary>
    /// Fade this sound in or out over a number of milliseconds.
    /// </summary>
    /// <param name="endVolume">The volume to fade this sound to.</param>
    /// <param name="milliseconds">The duration in milliseconds to fade to `endVolume`.</param>
    void setFade( float endVolume, uint64_t milliseconds );

    /// <summary>
    /// Fade this sound from a starting volume to an ending volume over a period of time.
    /// </summary>
    /// <typeparam name="Rep">The representation of the duration.</typeparam>
    /// <typeparam name="Period">The duration period.</typeparam>
    /// <param name="beginVolume">The volume to start this fade from.</param>
    /// <param name="endVolume">The volume to fade this sound to.</param>
    /// <param name="duration">The time to get to the `endVolume`.</param>
    template<class Rep, class Period = std::ratio<1>>
    void setFade( float beginVolume, float endVolume, const std::chrono::duration<Rep, Period>& duration );

    /// <summary>
    /// Fade this sound from a starting volume to an ending volume over a number of milliseconds.
    /// </summary>
    /// <param name="beginVolume">The volume to start this fade from.</param>
    /// <param name="endVolume">The volume to fade this sound to.</param>
    /// <param name="milliseconds">The duration in milliseconds to fade to `endVolume`.</param>
    void setFade( float beginVolume, float endVolume, uint64_t milliseconds );

    /// <summary>
    /// Schedule this sound to start playing at a specific time.
    /// Note: The sound must be explicitly started using `play`.
    /// </summary>
    /// <typeparam name="Rep">The representation of the duration.</typeparam>
    /// <typeparam name="Period">The duration period.</typeparam>
    /// <param name="duration">The duration to wait before playing this sound.</param>
    template<class Rep, class Period = std::ratio<1>>
    void setStartTime( const std::chrono::duration<Rep, Period>& duration );

    /// <summary>
    /// Schedule this sound to start playing at a specific time.
    /// Note: The sound must be explicitly started using `play`.
    /// </summary>
    /// <param name="milliseconds">The duration in milliseconds to start playing this sound.</param>
    void setStartTime( uint64_t milliseconds );

    /// <summary>
    /// Schedule this sound to stop playing at a specific time.
    /// Note: The sound must be explicitly started using `play`.
    /// </summary>
    /// <typeparam name="Rep">The representation of the duration.</typeparam>
    /// <typeparam name="Period">The duration period.</typeparam>
    /// <param name="duration">The duration to wait before stopping this sound.</param>
    template<class Rep, class Period = std::ratio<1>>
    void setStopTime( const std::chrono::duration<Rep, Period>& duration );

    /// <summary>
    /// Schedule this sound to stop playing at a specific time.
    /// Note: The sound must be explicitly started using `play`.
    /// </summary>
    /// <param name="milliseconds">The duration in milliseconds to stop playing this sound.</param>
    void setStopTime( uint64_t milliseconds );

    Sound();
    ~Sound();
    Sound( const Sound& );
    Sound( Sound&& ) noexcept;
    Sound& operator=( const Sound& );
    Sound& operator=( Sound&& ) noexcept;

    /// <summary>
    /// Allow nullptr assignment.
    /// </summary>
    /// <remarks>
    /// Assigning `nullptr` will release the underlying implementation.
    /// This is the same as using the `reset` function on this object.
    /// </remarks>
    Sound& operator=( std::nullptr_t ) noexcept;

    /// <summary>
    /// Allow for null checks.
    /// </summary>
    bool operator==( std::nullptr_t ) const noexcept;
    bool operator!=( std::nullptr_t ) const noexcept;

    /// <summary>
    /// Explicit bool conversion allows to check for a valid object.
    /// </summary>
    /// <returns>`true` if this object contains a valid pointer to implementation. `false` otherwise.</returns>
    explicit operator bool() const noexcept;

    /// <summary>
    /// Get a pointer to the implementation.
    /// </summary>
    /// <returns>A pointer to the Sound implementation.</returns>
    std::shared_ptr<SoundImpl> get() const noexcept;

    /// <summary>
    /// Release the underlying pointer to implementation.
    /// </summary>
    void reset() noexcept;

protected:
    explicit Sound( std::shared_ptr<SoundImpl> impl );

private:
    std::shared_ptr<SoundImpl> impl;
};

template<typename Rep, typename Period>
void Sound::seek( const std::chrono::duration<Rep, Period>& duration )
{
    seek( std::chrono::duration_cast<std::chrono::milliseconds>( duration ).count() );
}

template<class Rep, class Period>
void Sound::setFade( float endVolume, const std::chrono::duration<Rep, Period>& duration )
{
    setFade( endVolume, std::chrono::duration_cast<std::chrono::milliseconds>( duration ).count() );
}

template<class Rep, class Period>
void Sound::setFade( float beginVolume, float endVolume, const std::chrono::duration<Rep, Period>& duration )
{
    setFade( beginVolume, endVolume,
             std::chrono::duration_cast<std::chrono::milliseconds>( duration ).count() );
}

template<class Rep, class Period>
void Sound::setStartTime( const std::chrono::duration<Rep, Period>& duration )
{
    setStartTime( std::chrono::duration_cast<std::chrono::milliseconds>( duration ).count() );
}

template<class Rep, class Period>
void Sound::setStopTime( const std::chrono::duration<Rep, Period>& duration )
{
    setStopTime( std::chrono::duration_cast<std::chrono::milliseconds>( duration ).count() );
}

}  // namespace Audio
