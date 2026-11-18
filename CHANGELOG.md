# Changelog

All notable changes to the Audio library are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased] - 2.0.0

Version 2 moves the library behind a stable C API. A compiled `Audio.lib` or `Audio.dll` no longer depends on a particular version of the C++ standard library, and the same library can be linked into both Debug and Release builds. The C++ API is now a thin, header-only wrapper around the C API and works the same way as in v1: there is still no need to manage the audio device, and copying a sound still shares it.

See [Upgrading From v1](README.md#upgrading-from-v1) in the README.

### Added

- A C99 API in `<Audio/audio.h>`, usable from C and from any language that can call C functions.
- `Audio.hpp` umbrella header that includes the whole C++ API.
- `Audio::playSound`, `Audio::setMasterVolume`, `Audio::getMasterVolume` and `Audio::getListener` free functions for the default device.
- `Audio::Device` can be created explicitly with a configuration (listener count, sample rate, channel count). Sounds and waveforms keep their device alive.
- `Audio::Result` error codes. `Sound::load`, `Sound::play`, `Sound::seek`, `Waveform::create`, `Device::playSound` and others now report errors.
- A log callback (`audio_set_log_callback`) to redirect or silence error messages. By default, errors and warnings are printed to `stderr`.
- `audio_get_version` to check the version of the linked library at runtime.
- `Waveform::isPlaying`.
- `Device::getSampleRate`, `Device::getChannels` and `Device::getListenerCount`.
- A C example (`example/cExample.c`).

### Changed

- **Breaking:** `Audio::Device::playSound`, `setMasterVolume` and `getListener` are replaced by the free functions `Audio::playSound`, `Audio::setMasterVolume` and `Audio::getListener`. `Audio::Device` is now an object for explicitly created devices.
- **Breaking:** `Sound::loadSound( path )` and `Sound::loadMusic( path )` are replaced by `Sound::load( path, type )`.
- **Breaking:** `Sound::setStartTime` and `Sound::setStopTime` now take a delay from now.
- **Breaking:** `Waveform`'s default frequency is now 440 Hz instead of 0 Hz.
- The library is implemented in C99 instead of C++.
- The static library is built with `/MT /Zl` on MSVC, so it links into programs using `/MD`, `/MDd`, `/MT` or `/MTd`.
- The Visual Studio project no longer uses whole program optimization (`/GL`) for the library, so the library is not tied to one exact compiler version.
- Calling functions on an empty or failed `Sound`, `Waveform` or `Listener` is now a safe no-op instead of a crash.
- Shared libraries only export the Audio API (miniaudio's symbols are hidden).
- File paths are UTF-8 on all platforms, so file names with non-ASCII characters can be loaded on Windows.
- Waveforms always generate 32-bit float samples at the device's sample rate.
- Linux builds link `pthread`, `libm` and `libdl`.
- Updated miniaudio to 0.11.26.

### Removed

- `Waveform::setSampleRate` and `Waveform::getSampleRate`. Waveforms always play at the device's sample rate.
- The `AUDIO_EXTERN` macro. The `AUDIO_API` export macro is now defined in `<Audio/audio.h>`.

### Fixed

- The audio device is shut down correctly when the library is built as a shared library (DLL). In v1, shutdown hung, so the engine was never uninitialized.
- `Sound::setStartTime` and `Sound::setStopTime` passed the delay to miniaudio as an absolute engine time.
- `Sound::setPinnedListener` with an invalid listener now resets the sound to use the closest listener. In v1, a pinned sound could not be unpinned.
- A sound that failed to load no longer appears valid (`if ( sound )` now returns `false`).
- `Sound::setPitch` ignores pitch values of 0 or less.
- The documentation of `Sound::setPitch` stated that the pitch must be greater than 1 (it must be greater than 0).

## [1.1.0] - 2026-09-09

### Added

- vcpkg port (`ports/audio`).
- Clang build presets ([#2](https://github.com/3dgep/Audio/pull/2)).
- `Sound::setFade( beginVolume, endVolume, duration )` to fade from an explicit starting volume ([#3](https://github.com/3dgep/Audio/pull/3)).
- Fade example (`example/fadeExample.cpp`).
- `AUDIO_INSTALL_CONFIG_SUBDIRS` CMake option to install libraries into per-configuration subdirectories.

### Changed

- **Breaking:** `Sound::AttenuationModel::None` is renamed to `Sound::AttenuationModel::Off`, because X11 defines `None` as a macro ([#3](https://github.com/3dgep/Audio/pull/3)).
- CMake presets are renamed to the `<generator>-<static|shared>` pattern.
- Releases are built with Visual Studio 2026 (vs18).

### Fixed

- Build errors with Clang, and the "type attributes ignored after type is already defined" warning.
- The vcpkg license is set to Unlicense.

## [1.0.2] - 2026-03-06

### Added

- CPack packaging and a GitHub Actions workflow that publishes static and shared packages on release.
- Per-configuration install directories.

### Changed

- Updated the Visual Studio project for Visual Studio 2026.
- The minimum CMake version for the examples is 4.0, to use `DEBUGGER_WORKING_DIRECTORY`.
- Removed the Visual Studio 2019 (vs16) preset.

### Fixed

- `Sound::getCone` returned the outer gain instead of the outer cone angle.

## [1.0.1] - 2025-10-11

### Added

- OGG Vorbis support (via stb_vorbis).

### Changed

- Updated miniaudio to 0.11.23.

## [1.0.0] - 2025-10-11

### Added

- `Device::playSound` to play one-shot sound effects without creating a `Sound`.
- `Sound::getCursorInSeconds`.
- `Sound::reset` and `Listener::reset`.
- `Audio::Audio` CMake alias target.
- CMake install and `find_package` support.
- Linux build instructions and a spatial audio example in the README.

### Changed

- Waveforms are created in a stopped state.

### Fixed

- Destruction order of the audio device.

## [0.0.1] - 2023-05-04

### Added

- Initial release: `Audio::Device`, `Audio::Sound`, `Audio::Listener` and `Audio::Waveform`, built on miniaudio.
- WAV, MP3 and FLAC playback, with streaming for background music.
- Spatial (3D) audio.
- Option to build the library as a shared library (DLL).
- Visual Studio 2022 solution and CMake build.
- Example application with narration and a waveform coin sound effect.

[Unreleased]: https://github.com/3dgep/Audio/compare/v1.1.0...v2
[1.1.0]: https://github.com/3dgep/Audio/compare/v1.0.2...v1.1.0
[1.0.2]: https://github.com/3dgep/Audio/compare/v1.0.1...v1.0.2
[1.0.1]: https://github.com/3dgep/Audio/compare/v1.0.0...v1.0.1
[1.0.0]: https://github.com/3dgep/Audio/compare/v0.0.1...v1.0.0
[0.0.1]: https://github.com/3dgep/Audio/releases/tag/v0.0.1
