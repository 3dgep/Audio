#pragma once

#include <Audio/Listener.hpp>
#include <Audio/Sound.hpp>

#include "miniaudio.h"

#include <chrono>
#include <filesystem>

namespace Audio
{
class DeviceImpl;

class SoundImpl
{
public:
    SoundImpl( std::shared_ptr<DeviceImpl> device, const std::filesystem::path& filePath, ma_engine* pEngine, ma_sound_group* pGroup = nullptr, uint32_t flags = 0 );
    ~SoundImpl();

    void play();
    void stop();

    float getDurationInSeconds() const;

    float getCursorInSeconds() const;

    void seek( uint64_t milliseconds );

    bool isPlaying() const;
    bool isEnd() const;

    void setLooping( bool looping );
    bool isLooping() const;

    void setPinnedListener( const Listener& listener );

    void  setVolume( float volume );
    float getVolume() const;

    void  setPan( float pan );
    float getPan() const;

    void  setPitch( float pitch );
    float getPitch() const;

    void   setPosition( const Vector& pos );
    Vector getPosition() const;

    void   setDirection( const Vector& dir );
    Vector getDirection() const;

    void   setVelocity( const Vector& vel );
    Vector getVelocity() const;

    void setCone( float innerConeAngle, float outerConeAngle, float outerGain );
    void getCone( float& innerConeAngle, float& outerConeAngle, float& outerGain ) const;

    void                    setAttenuationModel( Sound::AttenuationModel attenuation );
    Sound::AttenuationModel getAttenuationModel() const;

    void  setRollOff( float rollOff );
    float getRollOff() const;

    void  setMinGain( float minGain );
    float getMinGain() const;

    void  setMaxGain( float maxGain );
    float getMaxGain() const;

    void  setMinDistance( float minDistance );
    float getMinDistance() const;

    void  setMaxDistance( float maxDistance );
    float getMaxDistance() const;

    void  setDopplerFactor( float dopplerFactor );
    float getDopplerFactor() const;

    void setFade( float endVolume, uint64_t milliseconds );
    void setFade( float beginVolume, float endVolume, uint64_t milliseconds ); // an added fade to correct issues with the current fade

    void setStartTime( uint64_t milliseconds );
    void setStopTime( uint64_t milliseconds );

private:
    std::shared_ptr<DeviceImpl> device;
    ma_engine*                  engine = nullptr;
    ma_sound_group*             group  = nullptr;
    ma_sound                    sound {};
};

}  // namespace Audio
