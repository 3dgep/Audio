#include "SoundImpl.hpp"
#include "ListenerImpl.hpp"

#include <iostream>

using namespace Audio;

SoundImpl::SoundImpl( std::shared_ptr<DeviceImpl> device, const std::filesystem::path& filePath, ma_engine* pEngine, ma_sound_group* pGroup, uint32_t flags )
: device { std::move( device ) }
, engine { pEngine }
, group { pGroup }
{

    if ( ma_sound_init_from_file_w( engine, filePath.wstring().c_str(), flags, group, nullptr, &sound ) != MA_SUCCESS )
    {
        std::cerr << "Failed to initialize sound from source: " << filePath.string() << std::endl;
    }
}

SoundImpl::~SoundImpl()
{
    ma_sound_uninit( &sound );
}

void SoundImpl::play()
{
    ma_sound_start( &sound );
}

void SoundImpl::stop()
{
    ma_sound_stop( &sound );
}

float SoundImpl::getDurationInSeconds() const
{
    float duration = 0.0f;
    // ma_sound_get_length_in_seconds takes a non-const pointer (not sure why).
    // To get around this, we need a const_cast on the this pointer :(.
    ma_sound_get_length_in_seconds( &const_cast<SoundImpl*>( this )->sound, &duration );
    return duration;
}

float SoundImpl::getCursorInSeconds() const
{
    float cursor = 0.0f;
    ma_sound_get_cursor_in_seconds( &const_cast<SoundImpl*>( this )->sound, &cursor );
    return cursor;
}

void SoundImpl::seek( uint64_t milliseconds )
{
    ma_uint32 sampleRate;
    ma_sound_get_data_format( &sound, nullptr, nullptr, &sampleRate, nullptr, 0 );
    const ma_uint64 pcmFrame = ( sampleRate * milliseconds ) / 1000;
    ma_sound_seek_to_pcm_frame( &sound, pcmFrame );
}

bool SoundImpl::isPlaying() const
{
    return ma_sound_is_playing( &sound ) == MA_TRUE;
}

bool SoundImpl::isEnd() const
{
    return ma_sound_at_end( &sound ) == MA_TRUE;
}

void SoundImpl::setLooping( bool looping )
{
    ma_sound_set_looping( &sound, looping ? MA_TRUE : MA_FALSE );
}

bool SoundImpl::isLooping() const
{
    return ma_sound_is_looping( &sound ) == MA_TRUE;
}

void SoundImpl::setPinnedListener( const Listener& listener )
{
    if ( const auto listenerImpl = listener.get() )
    {
        const uint32_t index = listenerImpl->getIndex();
        ma_sound_set_pinned_listener_index( &sound, index );
    }
}

void SoundImpl::setVolume( float volume )
{
    ma_sound_set_volume( &sound, volume );
}

float SoundImpl::getVolume() const
{
    return ma_sound_get_volume( &sound );
}

void SoundImpl::setPan( float pan )
{
    ma_sound_set_pan( &sound, pan );
}

float SoundImpl::getPan() const
{
    return ma_sound_get_pan( &sound );
}

void SoundImpl::setPitch( float pitch )
{
    ma_sound_set_pitch( &sound, pitch );
}

float SoundImpl::getPitch() const
{
    return ma_sound_get_pitch( &sound );
}

void SoundImpl::setPosition( const Vector& pos )
{
    ma_sound_set_position( &sound, pos.x, pos.y, pos.z );
}

Vector SoundImpl::getPosition() const
{
    auto pos = ma_sound_get_position( &sound );
    return { pos.x, pos.y, pos.z };
}

void SoundImpl::setDirection( const Vector& dir )
{
    ma_sound_set_direction( &sound, dir.x, dir.y, dir.z );
}

Vector SoundImpl::getDirection() const
{
    auto dir = ma_sound_get_direction( &sound );
    return { dir.x, dir.y, dir.z };
}

void SoundImpl::setVelocity( const Vector& vel )
{
    ma_sound_set_velocity( &sound, vel.x, vel.y, vel.z );
}

Vector SoundImpl::getVelocity() const
{
    auto vel = ma_sound_get_velocity( &sound );
    return { vel.x, vel.y, vel.z };
}

void SoundImpl::setCone( float innerConeAngle, float outerConeAngle, float outerGain )
{
    ma_sound_set_cone( &sound, innerConeAngle, outerConeAngle, outerGain );
}

void SoundImpl::getCone( float& innerConeAngle, float& outerConeAngle, float& outerGain ) const
{
    ma_sound_get_cone( &sound, &innerConeAngle, &outerConeAngle, &outerGain );
}

void SoundImpl::setAttenuationModel( Sound::AttenuationModel attenuation )
{
    switch ( attenuation )
    {
    case Sound::AttenuationModel::NoAttenuation:
        ma_sound_set_attenuation_model( &sound, ma_attenuation_model_none );
        break;
    case Sound::AttenuationModel::Inverse:
        ma_sound_set_attenuation_model( &sound, ma_attenuation_model_inverse );
        break;
    case Sound::AttenuationModel::Linear:
        ma_sound_set_attenuation_model( &sound, ma_attenuation_model_linear );
        break;
    case Sound::AttenuationModel::Exponential:
        ma_sound_set_attenuation_model( &sound, ma_attenuation_model_exponential );
        break;
    }
}

Sound::AttenuationModel SoundImpl::getAttenuationModel() const
{
    switch ( ma_sound_get_attenuation_model( &sound ) )
    {
    case ma_attenuation_model_none:
        return Sound::AttenuationModel::NoAttenuation;
    case ma_attenuation_model_inverse:
        return Sound::AttenuationModel::Inverse;
    case ma_attenuation_model_linear:
        return Sound::AttenuationModel::Linear;
    case ma_attenuation_model_exponential:
        return Sound::AttenuationModel::Exponential;
    }

    return Sound::AttenuationModel::NoAttenuation;
}

void SoundImpl::setRollOff( float rollOff )
{
    ma_sound_set_rolloff( &sound, rollOff );
}

float SoundImpl::getRollOff() const
{
    return ma_sound_get_rolloff( &sound );
}

void SoundImpl::setMinGain( float minGain )
{
    ma_sound_set_min_gain( &sound, minGain );
}

float SoundImpl::getMinGain() const
{
    return ma_sound_get_min_gain( &sound );
}

void SoundImpl::setMaxGain( float maxGain )
{
    ma_sound_set_max_gain( &sound, maxGain );
}

float SoundImpl::getMaxGain() const
{
    return ma_sound_get_max_gain( &sound );
}

void SoundImpl::setMinDistance( float minDistance )
{
    ma_sound_set_min_distance( &sound, minDistance );
}

float SoundImpl::getMinDistance() const
{
    return ma_sound_get_min_distance( &sound );
}

void SoundImpl::setMaxDistance( float maxDistance )
{
    ma_sound_set_max_distance( &sound, maxDistance );
}

float SoundImpl::getMaxDistance() const
{
    return ma_sound_get_max_distance( &sound );
}

void SoundImpl::setDopplerFactor( float dopplerFactor )
{
    ma_sound_set_doppler_factor( &sound, dopplerFactor );
}

float SoundImpl::getDopplerFactor() const
{
    return ma_sound_get_doppler_factor( &sound );
}

void SoundImpl::setFade( float endVolume, uint64_t milliseconds )
{
    ma_sound_set_fade_in_milliseconds( &sound, -1.0f, endVolume, milliseconds );
}

void SoundImpl::setStartTime( uint64_t milliseconds )
{
    ma_sound_set_start_time_in_milliseconds( &sound, milliseconds );
}

void SoundImpl::setStopTime( uint64_t milliseconds )
{
    ma_sound_set_stop_time_in_milliseconds( &sound, milliseconds );
}
