#include <Audio/Device.hpp>
#include <Audio/Sound.hpp>

#include "SoundImpl.hpp"

using namespace Audio;

Sound::Sound()                              = default;
Sound::~Sound()                             = default;
Sound::Sound( const Sound& )                = default;
Sound::Sound( Sound&& ) noexcept            = default;
Sound& Sound::operator=( const Sound& )     = default;
Sound& Sound::operator=( Sound&& ) noexcept = default;

Sound& Sound::operator=( std::nullptr_t ) noexcept
{
    impl = nullptr;
    return *this;
}

bool Sound::operator==( std::nullptr_t ) const noexcept
{
    return impl == nullptr;
}

bool Sound::operator!=( std::nullptr_t ) const noexcept
{
    return impl != nullptr;
}

Sound::operator bool() const noexcept
{
    return impl != nullptr;
}

std::shared_ptr<SoundImpl> Sound::get() const noexcept
{
    return impl;
}

void Sound::reset() noexcept
{
    impl.reset();
}

Sound::Sound( std::shared_ptr<SoundImpl> impl )
: impl { std::move( impl ) }
{}

Sound::Sound( const std::filesystem::path& filePath, Type type )
{
    switch ( type )
    {
    case Type::Sound:
        loadSound( filePath );
        break;
    case Type::Music:
        loadMusic( filePath );
        break;
    }
}

void Sound::loadSound( const std::filesystem::path& filePath )
{
    *this = Device::loadSound( filePath );
}

void Sound::loadMusic( const std::filesystem::path& filePath )
{
    *this = Device::loadMusic( filePath );
}

void Sound::play()
{
    impl->play();
}

void Sound::stop()
{
    impl->stop();
}

void Sound::pause()
{
    stop();
}

float Sound::getDurationInSeconds() const
{
    return impl->getDurationInSeconds();
}

float Sound::getCursorInSeconds() const
{
    return impl->getCursorInSeconds();
}

void Sound::seek( uint64_t milliseconds )
{
    impl->seek( milliseconds );
}

void Sound::replay()
{
    seek( 0ull );
    play();
}

bool Sound::isPlaying() const
{
    return impl->isPlaying();
}

bool Sound::isEnd() const
{
    return impl->isEnd();
}

void Sound::setLooping( bool looping )
{
    return impl->setLooping( looping );
}

bool Sound::isLooping() const
{
    return impl->isLooping();
}

void Sound::setPinnedListener( const Listener& listener )
{
    impl->setPinnedListener( listener );
}

void Sound::setVolume( float volume )
{
    impl->setVolume( volume );
}

float Sound::getVolume() const
{
    return impl->getVolume();
}

void Sound::setPan( float pan )
{
    impl->setPan( pan );
}

float Sound::getPan() const
{
    return impl->getPan();
}

void Sound::setPitch( float pitch )
{
    impl->setPitch( pitch );
}

float Sound::getPitch() const
{
    return impl->getPitch();
}

void Sound::setPosition( const Vector& position )
{
    impl->setPosition( position );
}

Vector Sound::getPosition() const
{
    return impl->getPosition();
}

void Sound::setDirection( const Vector& direction )
{
    impl->setDirection( direction );
}

Vector Sound::getDirection() const
{
    return impl->getDirection();
}

void Sound::setVelocity( const Vector& velocity )
{
    impl->setVelocity( velocity );
}

Vector Sound::getVelocity() const
{
    return impl->getVelocity();
}

void Sound::setCone( float innerAngle, float outerAngle, float outerGain )
{
    impl->setCone( innerAngle, outerAngle, outerGain );
}

void Sound::getCone( float& innerAngle, float& outerAngle, float& outerGain ) const
{
    impl->getCone( innerAngle, outerAngle, outerGain );
}

void Sound::setAttenuationModel( AttenuationModel attenuation )
{
    impl->setAttenuationModel( attenuation );
}

Sound::AttenuationModel Sound::getAttenuationModel() const
{
    return impl->getAttenuationModel();
}

void Sound::setRollOff( float rollOff )
{
    impl->setRollOff( rollOff );
}

float Sound::getRollOff() const
{
    return impl->getRollOff();
}

void Sound::setMinGain( float minGain )
{
    impl->setMinGain( minGain );
}

float Sound::getMinGain() const
{
    return impl->getMinGain();
}

void Sound::setMaxGain( float maxGain )
{
    impl->setMaxGain( maxGain );
}

float Sound::getMaxGain() const
{
    return impl->getMaxGain();
}

void Sound::setMinDistance( float minDistance )
{
    impl->setMinDistance( minDistance );
}

float Sound::getMinDistance() const
{
    return impl->getMinDistance();
}

void Sound::setMaxDistance( float maxDistance )
{
    impl->setMaxDistance( maxDistance );
}

float Sound::getMaxDistance() const
{
    return impl->getMaxDistance();
}

void Sound::setDopplerFactor( float dopplerFactor )
{
    impl->setDopplerFactor( dopplerFactor );
}

float Sound::getDopplerFactor() const
{
    return impl->getDopplerFactor();
}

void Sound::setFade( float endVolume, uint64_t milliseconds )
{
    impl->setFade( endVolume, milliseconds );
}

void Sound::setFade( float beginVolume, float endVolume, uint64_t milliseconds )
{
    impl->setFade( beginVolume, endVolume, milliseconds );
}

void Sound::setStartTime( uint64_t milliseconds )
{
    impl->setStartTime( milliseconds );
}

void Sound::setStopTime( uint64_t milliseconds )
{
    impl->setStopTime( milliseconds );
}
