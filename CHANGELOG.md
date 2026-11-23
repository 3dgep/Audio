# Changelog

All notable changes to the Audio library are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Changed

- `AUDIO_INSTALL_CONFIG_SUBDIRS` is now only ON by default for multi-config generators (Visual Studio, Ninja Multi-Config, Xcode), which can build Debug and Release into one install folder. With a single-config generator (Make, Ninja), the library installs to the standard `lib/` and `bin/` folders, so the linker and the dynamic loader find it after `cmake --install`. Set the option to keep the old layout.

## [2.1.1] - 2026-11-23

### Fixed

- One-shots (`Sound::playOneShot`) piled up while the audio device wasn't running, for example in a browser before the first click or tap. The copies never finished, so they were never freed, every call got slower, and they all played at once when the audio started. A one-shot now plays right away or not at all: it is skipped (and still returns success) while the device isn't running, and while its group is paused or stopped.
- At most 64 one-shots play at once per device; starting another one stops the oldest. Many one-shots in a single frame no longer pile up and play as one loud burst.

## [2.1.0] - 2026-11-23

### Added

- Groups (`Audio::Group`, `audio_group_*`): channels that sounds and waveforms play through, with their own volume, pan, pitch and fade. Stopping a group pauses everything in it. For example, a music and an effects group let the player set their volumes separately. Groups can contain other groups.
- `Sound( group, path, type )`, `Waveform( group, ... )` and `Audio::playSound( path, group )` to play through a group (C: `audio_sound_create_in_group`, `audio_waveform_create_in_group`, `audio_play_sound_in_group`).
- Move sounds and waveforms between groups after they were created, also while they play: `group.add( sound )`, `group.remove( sound )`, `sound.setGroup( group )` and `sound.getGroup()`, and the same for waveforms. `Group::none()` is "no group" (the device's output), and `==` compares groups. (C: `audio_sound_set_group`, `audio_sound_get_group`, `audio_waveform_set_group`, `audio_waveform_get_group`, `audio_group_retain`.)
- `Sound::playOneShot` (`audio_sound_play_one_shot`): play an overlapping copy of a sound, with its current volume, pan, pitch and 3D position. For sound effects that can play several times at once.
- `Sound::stop( fade )` and `Group::stop( fade )` (`audio_sound_stop_with_fade`, `audio_group_stop_with_fade`): fade out, then stop. Playing again restores the volume from before the fade.
- The game example has a music and an effects group with a volume slider for each on the pause screen, plays its sound effects as one-shots, and fades out the music on game over. The fade example shows `stop( fade )`.

### Fixed

- After a stop time set with `Sound::setStopTime` had passed, `play` did nothing: the sound stayed stopped.

## [2.0.1] - 2026-11-22

### Added

- Documentation for all public functions, types and constants in `<Audio/audio.h>` and the C++ headers: defaults, units, and what happens with empty objects and out-of-range values.
- The game example has touch controls in the browser (drag to move, tap to start and launch), and a pause and a fullscreen button in its score bar (fullscreen also with F, in the browser and on Windows). It pauses when it loses focus.

### Fixed

- The README said that a key press starts the sound in a browser. Browsers only allow it after a click or tap.

## [2.0.0] - 2026-11-21

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
- A C example (`examples/c_api`).
- A game example (`examples/game`): a small brick game with streamed music, panned sound effects and waveform jingles. It runs on Windows, Linux, macOS (using [fenster](https://github.com/zserge/fenster)) and in a web browser (Emscripten).
- Web (WebAssembly) builds with Emscripten: the `AUDIO_BUILD_WEB` CMake option and the `emscripten` preset. CMake uses an installed Emscripten SDK, or downloads one into `out/emsdk`. Web builds go to `lib/web` and `bin/web`.
- A website for the library (`website`), with the game example playable in the browser. The `website` CMake target (in web builds) puts it together, and the `Website` workflow publishes it on GitHub Pages.
- CMake options `AUDIO_USE_SYSTEM_DEPS` (use an installed miniaudio and stb_vorbis) and `AUDIO_MSVC_ANY_CRT` (turn off the `/MT /Zl` build of the static library).

### Changed

- **Breaking:** `Audio::Device::playSound`, `setMasterVolume` and `getListener` are replaced by the free functions `Audio::playSound`, `Audio::setMasterVolume` and `Audio::getListener`. `Audio::Device` is now an object for explicitly created devices.
- **Breaking:** `Sound::loadSound( path )` and `Sound::loadMusic( path )` are replaced by `Sound::load( path, type )`.
- **Breaking:** `Sound::setStartTime` and `Sound::setStopTime` now take a delay from now.
- **Breaking:** `Waveform`'s default frequency is now 440 Hz instead of 0 Hz.
- **Breaking:** The `inc` folder is renamed to `include`, the same name as in the packages. If you copied the headers into your project by hand, update the include path. CMake, FetchContent and vcpkg users are not affected.
- The library is implemented in C99 instead of C++.
- The static library is built with `/MT /Zl` on MSVC, so it links into programs using `/MD`, `/MDd`, `/MT` or `/MTd`.
- The Visual Studio project no longer uses whole program optimization (`/GL`) for the library, so the library is not tied to one exact compiler version.
- Calling functions on an empty or failed `Sound`, `Waveform` or `Listener` is now a safe no-op instead of a crash.
- Shared libraries only export the Audio API (miniaudio's symbols are hidden).
- File paths are UTF-8 on all platforms, so file names with non-ASCII characters can be loaded on Windows.
- Waveforms always generate 32-bit float samples at the device's sample rate.
- Linux builds link `pthread`, `libm` and `libdl`.
- Updated miniaudio to 0.11.26.
- **Breaking:** The Visual Studio solution is now `Audio.slnx` (the XML solution format of Visual Studio 2026) instead of `Audio.sln`, and the projects use the Visual Studio 2026 toolset (v145). Visual Studio 2022 can't open `.slnx` solutions by default; use `GenerateProjectFiles.bat` instead. If the projects are opened in Visual Studio 2022, they use its own toolset (v143).
- **Breaking:** The examples moved from `example` to `examples`, with a folder per example: `getting_started` (was `example`), `fade` (was `fadeExample`) and `c_api` (was `cExample`). Their audio files are in `examples/assets`, which is the working directory when they run from Visual Studio.
- `Audio.slnx` contains all examples, not only `getting_started`, in an `examples` solution folder. `game` is the startup project, so F5 runs it.
- Release packages are built with Visual Studio 2022, so they work with Visual Studio 2022 and newer. The compiler is no longer part of the package name, and the platform is named `windows-x64` instead of `Windows-AMD64` (for example `Audio-2.0.0-windows-x64-static.zip`).
- `GeneratePackages.bat` builds the packages with the Visual Studio 2022 toolset when it is available (warning otherwise), uses the CMake that comes with Visual Studio if CMake is not installed, works from any folder, and collects the packages in `out\packages`.
- **Breaking:** The vcpkg port is renamed from `audio` to `3dgep-audio`. It now depends on vcpkg's `miniaudio` and `stb` ports, uses the triplet's C runtime, and installs PDBs and the usage file.

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
- With single-configuration generators (such as Unix Makefiles or Ninja), Debug and Release builds wrote to the same `lib` and `bin` folders and overwrote each other. All generators now use `lib/<Config>` and `bin/<Config>`.
- The Visual Studio projects put their intermediate files in an `Audio` folder (and `example\x64`) inside the repository. They now go to `out\obj`, which is ignored by git. The projects also build on their own, outside the solution.
- `GenerateProjectFiles.bat` used CMake presets that no longer existed after the presets were renamed in 1.1.0. It now uses the `vs18-static` and `vs17-static` presets and includes the examples. It also requires only the "Desktop development with C++" workload (instead of "Game development with C++"), uses the CMake that comes with Visual Studio if CMake is not installed, and no longer fails with a syntax error when no Visual Studio is found.

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

[Unreleased]: https://github.com/3dgep/Audio/compare/v2.1.1...main
[2.1.1]: https://github.com/3dgep/Audio/compare/v2.1.0...v2.1.1
[2.1.0]: https://github.com/3dgep/Audio/compare/v2.0.1...v2.1.0
[2.0.1]: https://github.com/3dgep/Audio/compare/v2.0.0...v2.0.1
[2.0.0]: https://github.com/3dgep/Audio/compare/v1.1.0...v2.0.0
[1.1.0]: https://github.com/3dgep/Audio/compare/v1.0.2...v1.1.0
[1.0.2]: https://github.com/3dgep/Audio/compare/v1.0.1...v1.0.2
[1.0.1]: https://github.com/3dgep/Audio/compare/v1.0.0...v1.0.1
[1.0.0]: https://github.com/3dgep/Audio/compare/v0.0.1...v1.0.0
[0.0.1]: https://github.com/3dgep/Audio/releases/tag/v0.0.1
