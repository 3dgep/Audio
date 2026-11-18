#pragma once

/// Audio v2 - C++ API (umbrella header: includes the whole C++ API)
///
/// A header-only C++ wrapper around the C API in <Audio/audio.h>.
///
/// Because this wrapper is compiled into your own program, it always uses your
/// compiler's standard library. Only the C API crosses the library boundary, so a
/// prebuilt Audio library works with any compiler version and in both Debug and
/// Release builds.
///
/// Quick start:
/// \code
///     #include <Audio/Audio.hpp>
///
///     Audio::playSound( "coin.wav" );     // Fire and forget.
///
///     Audio::Sound music { "music.ogg", Audio::Sound::Type::Music };
///     music.setLooping( true );
///     music.play();
/// \endcode
///
/// There is no need to create or destroy a device: the default device is created on
/// first use and shut down automatically when the program exits.

#include "Config.hpp"
#include "Vector.hpp"
#include "Listener.hpp"
#include "Device.hpp"
#include "Sound.hpp"
#include "Waveform.hpp"
