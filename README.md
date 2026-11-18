<!-- markdownlint-disable MD033 -->
# Audio Library

[![Alt text](docs/images/Audio-Library.png)](https://youtu.be/sXPN-QH1kwg)

A super minimalistic audio library for games, built on [miniaudio](https://miniaud.io/index.html).

```cpp
#include <Audio/Audio.hpp>

int main()
{
    Audio::Sound music { "music.ogg", Audio::Sound::Type::Music };
    music.setLooping( true );
    music.play();

    Audio::playSound( "coin.wav" );  // Fire and forget.
    ...
}
```

There is no audio device to set up or shut down. The library does that for you.

## Contents

* [Features](#features)
* [Getting the Library](#getting-the-library)
* [Adding the Library to a Visual Studio Project](#adding-the-library-to-a-visual-studio-project)
* [Building From Source](#building-from-source)
* [Playing Sounds](#playing-sounds)
* [Playing Music](#playing-music)
* [Spatial Audio](#spatial-audio)
* [Playing Waveforms](#playing-waveforms)
* [Using a Specific Device](#using-a-specific-device)
* [Using the C API](#using-the-c-api)
* [Upgrading From v1](#upgrading-from-v1)

## Features

* Play sound effects and stream background music.
* Supported file types: WAV (`.wav`), MP3 (`.mp3`), FLAC (`.flac`) and OGG (`.ogg`).
* Volume, panning, pitch, looping, fading and scheduled start/stop.
* 3D (spatial) audio with up to 4 listeners.
* Generated waveforms (sine, square, triangle and sawtooth) for retro sound effects. See [MarioCoin.cpp](examples/getting_started/MarioCoin.cpp).
* One `Audio.lib` for both the **Debug** and **Release** builds of your game.
* Works on Windows and Linux.

Your project must use C++17 or newer.

## Getting the Library

Choose one of the following:

* **Prebuilt package**: download it from the [Releases](https://github.com/3dgep/Audio/releases) page, then follow [Adding the Library to a Visual Studio Project](#adding-the-library-to-a-visual-studio-project) or [CMake: find_package](#cmake-find_package).
* **CMake FetchContent**: let CMake download and build the library. See [CMake: FetchContent](#cmake-fetchcontent).
* **vcpkg**: see [vcpkg](#vcpkg).
* **Build it yourself**: see [Building From Source](#building-from-source).

### Prebuilt Packages

There are two kinds of packages. If you are not sure, use the static one:

* `Audio-<version>-windows-x64-static.zip`: a static library (`Audio.lib`).
* `Audio-<version>-windows-x64-shared.zip`: a shared library (`Audio.dll`). Copy `Audio.dll` next to your game's `.exe`.

Both packages work with Visual Studio 2022 and newer.

### CMake: find_package

Extract a prebuilt package and add it to your `CMakeLists.txt`:

```cmake
# Where you extracted the package.
list(APPEND CMAKE_PREFIX_PATH "path/to/Audio-<version>-windows-x64-static")

find_package(Audio REQUIRED)

add_executable(MyGame main.cpp)
target_link_libraries(MyGame PRIVATE Audio::Audio)
```

### CMake: FetchContent

Download and build the library as part of your project:

```cmake
include(FetchContent)

FetchContent_Declare(
    Audio
    GIT_REPOSITORY https://github.com/3dgep/Audio.git
    GIT_TAG        v2.0.0
)

FetchContent_MakeAvailable(Audio)

add_executable(MyGame main.cpp)
target_link_libraries(MyGame PRIVATE Audio::Audio)
```

### vcpkg

Install the port from the [ports](ports/) folder of this repository:

```sh
vcpkg install 3dgep-audio --overlay-ports=path/to/Audio/ports
```

Then use it in CMake:

```cmake
find_package(Audio CONFIG REQUIRED)
target_link_libraries(MyGame PRIVATE Audio::Audio)
```

## Adding the Library to a Visual Studio Project

Copy the `include` and `lib` folders of the package (or of this repository, if you [built it yourself](#building-from-source)) to a folder in your project, for example `extern\Audio`:

![Folder Structure](docs/images/Folder%20Structure.png)

```txt
├── MyDemoGame
│   ├── extern
│   │   ├── Audio
│   │   │   ├── include
│   │   │   │   ├── Audio
│   │   │   │   │   ├── audio.h
│   │   │   │   │   ├── Audio.hpp
│   │   │   │   │   ├── ...
│   │   │   ├── lib
│   │   │   │   ├── Debug
│   │   │   │   │   ├── Audio.lib
│   │   │   │   ├── Release
│   │   │   │   │   ├── Audio.lib
```

Then open your project's settings (right-click the project and select **Properties**). Set **Configuration** to **All Configurations** at the top of the dialog, so the settings apply to both Debug and Release.

### C/C++ Settings

![C/C++ Settings](docs/images/Property%20Pages%20Dialog%20Box.png)

1. Under **C/C++ > General**, add the `Audio\include` folder to **Additional Include Directories**.
2. Under **C/C++ > Language**, set **C++ Language Standard** to **ISO C++17 Standard** or newer.

### Linker Settings

![Linker/General Settings](docs/images/Linker%20General%20Settings.png)

1. Under **Linker > General**, add `Audio\lib\$(Configuration)` to **Additional Library Directories**. `$(Configuration)` becomes `Debug` or `Release`, depending on what you are building.
2. Under **Linker > Input**, add `Audio.lib` to **Additional Dependencies**.

![Linker/Input Settings](docs/images/Linker%20Input%20Settings.png)

You should now be able to build your project. If you have any issues, join our Discord server: [3dgep.com Discord Server](https://discord.gg/gsxxaxc)

> **Tip**: Instead of using `Audio.lib`, you can also copy the [include](include/) and [src](src/) folders into your project and add the `.c` files from `src` to it.

## Building From Source

You need Visual Studio 2022 or newer with the **Desktop development with C++** workload on Windows, or CMake 3.25 or newer on Linux. To build the examples with CMake, you need CMake 4.0 or newer.

### Visual Studio

Open [Audio.slnx](Audio.slnx) in Visual Studio 2026 and select **Build > Build Solution** (<kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>B</kbd>). This builds `Audio.lib` and the three [examples](#examples). You don't need CMake for this. To run a different example, right-click it in **Solution Explorer** and select **Set as Startup Project**.

> **Note**: Visual Studio 2022 can't open `.slnx` solutions by default. With Visual Studio 2022, use [GenerateProjectFiles.bat](GenerateProjectFiles.bat) instead.

![Visual Studio 2022](docs/images/Visual%20Studio%202022.png)

### CMake on Windows

Run [GenerateProjectFiles.bat](GenerateProjectFiles.bat). It creates a Visual Studio solution (with the examples) in the `out\build` folder and opens it.

Or use a preset from the command line:

```sh
cmake --preset vs18-static -DAUDIO_BUILD_EXAMPLES=ON
cmake --build --preset vs18-static-release
```

Use `vs17-*` presets for Visual Studio 2022, and `*-shared` presets to build `Audio.dll`.

### CMake on Linux

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DAUDIO_BUILD_EXAMPLES=ON
cmake --build build
```

Use `-DCMAKE_BUILD_TYPE=Debug` for a Debug build, and add `-DBUILD_SHARED_LIBS=ON` to build a shared library.

### Build Output

* The library goes to the `lib` folder (`lib\Debug\Audio.lib` and `lib\Release\Audio.lib`). On Linux, it is `lib/Debug/libAudio.a` and `lib/Release/libAudio.a`, or `bin/Debug/libAudio.so` and `bin/Release/libAudio.so` for the shared library.
* The examples go to the `bin` folder (for example `bin\Debug\getting_started.exe` and `bin\Release\getting_started.exe`).

### Examples

The [examples](examples/) folder contains three examples:

* [getting_started](examples/getting_started/main.cpp): narration, background music and a waveform sound effect.
* [fade](examples/fade/main.cpp): fading sounds in and out.
* [c_api](examples/c_api/main.c): using the library from C.

They load their audio files from the [examples/assets](examples/assets/) folder. They work when you run them from Visual Studio. From the command line, use `-cwd` to tell them where the files are:

```sh
.\bin\Release\getting_started.exe -cwd ".\examples\assets"
```

If you see `[Audio] Error: Failed to load sound 'narrator.flac'`, the example can't find its audio files.

## Playing Sounds

The easiest way to play a sound effect is "fire and forget". You can't stop the sound once it has started:

```cpp
#include <Audio/Audio.hpp>
...
Audio::playSound( "explosion.wav" );
```

For more control, load the sound into an `Audio::Sound`:

```cpp
Audio::Sound coin { "coin.wav" };

coin.play();    // Play the sound. Does nothing if it is already playing.
coin.replay();  // Play the sound from the beginning.
coin.stop();    // Stop the sound. Calling play() again continues where it stopped.
```

### Volume, Pan and Pitch

```cpp
coin.setVolume( 0.5f );  // 0 is silent, 1 is 100%, above 1 is louder.
coin.setPan( -1.0f );    // -1 is left, 0 is centered, 1 is right.
coin.setPitch( 1.5f );   // 1 is the original pitch. Higher values sound higher.

Audio::setMasterVolume( 0.8f );  // The volume of all sounds.
```

### Copying Sounds

Copies of an `Audio::Sound` are the *same* sound, just like objects in C#, Java, Python or JavaScript. You can pass sounds to functions and store them in other objects:

```cpp
void playHitSound( Audio::Sound sound )
{
    sound.play();  // Keeps playing after this function returns.
}
```

To play the same file with different settings (for example, a different volume), load it into two separate sounds.

### Checking for Errors

If a sound can't be loaded, an error is printed to the console. The sound is then empty: using it is safe, but it doesn't make any noise. To check if a sound loaded:

```cpp
Audio::Sound coin { "coin.wav" };
if ( !coin )
{
    // The sound failed to load.
}
```

Or use the `Audio::Result` that `load` returns:

```cpp
Audio::Sound coin;
Audio::Result result = coin.load( "coin.wav" );
if ( result != Audio::Result::Success )
{
    std::cout << "Could not load coin.wav: " << Audio::toString( result ) << std::endl;
}
```

## Playing Music

Load long sounds, like background music, as `Music`. Music is streamed from the file while it plays, so it loads faster and uses less memory:

```cpp
Audio::Sound music { "Background_Music.mp3", Audio::Sound::Type::Music };
music.setLooping( true );
music.play();
```

Otherwise, music works the same as sound effects.

### Fading

```cpp
using namespace std::chrono_literals;

music.setFade( 0.0f, 2s );        // Fade out from the current volume over 2 seconds.
music.setFade( 0.0f, 1.0f, 2s );  // Fade in from silence over 2 seconds.
```

See the [fade](examples/fade/main.cpp) example for more.

## Spatial Audio

Sounds can be placed in 3D space. What you hear depends on where the sound is relative to the `Listener` (usually the player or the camera):

```cpp
Audio::Sound coin { "coin.wav" };
coin.setPosition( { x, y, z } );  // Where the sound is in the world.
...
Audio::Listener listener = Audio::getListener();
listener.setPosition( player.getPosition() );
listener.setDirection( player.getDirection() );  // Optional.
```

The attenuation model controls how a sound gets quieter with distance: `Inverse` (the default, sounds natural), `Linear`, `Exponential`, or `Off` (no change with distance):

```cpp
coin.setAttenuationModel( Audio::Sound::AttenuationModel::Linear );
```

> **Note**: Music is not spatialized.

### Multiple Listeners

There are 4 listeners (`Audio::getListener( 0 )` to `Audio::getListener( 3 )`), for example for split-screen games. A sound is heard by the closest listener, unless you pin it to a specific one:

```cpp
coin.setPinnedListener( Audio::getListener( 1 ) );
```

## Playing Waveforms

An `Audio::Waveform` generates a tone instead of playing a file, like the sound effects of early video games. There are 4 types: `Sine`, `Square`, `Triangle` and `Sawtooth`. Change the **frequency** (in Hz) to change the pitch, and the **amplitude** (0 to 1) to change the volume:

```cpp
// A square wave with an amplitude of 0.2, at 440 Hz.
Audio::Waveform beep { Audio::Waveform::Type::Square, 0.2f, 440.0f };
beep.start();
...
beep.setFrequency( 880.0f );  // One octave higher.
...
beep.stop();
```

See [MarioCoin.cpp](examples/getting_started/MarioCoin.cpp) for a more complex sound effect made with a waveform.

## Using a Specific Device

Most games don't need this. To change the audio device's settings, create an `Audio::Device` and pass it when you load sounds:

```cpp
Audio::Device::Config config;
config.listenerCount = 2;
config.sampleRate    = 48000;

Audio::Device device { config };

Audio::Sound    coin { device, "coin.wav" };
Audio::Listener listener = device.getListener( 0 );
```

## Using the C API

The library can also be used from C with [audio.h](include/Audio/audio.h). Pass `NULL` as the device to use the default device. See the [c_api](examples/c_api/main.c) example for a complete program.

```c
#include <Audio/audio.h>
...
audio_play_sound( NULL, "coin.wav" );

audio_sound* music = NULL;
if ( audio_sound_create( NULL, "music.ogg", AUDIO_SOUND_FLAGS_MUSIC, &music ) == AUDIO_SUCCESS )
{
    audio_sound_play( music );
}
...
audio_sound_destroy( music );
```

## Upgrading From v1

The most important changes (see the [CHANGELOG](CHANGELOG.md) for all of them):

* `Audio::Device::playSound`, `setMasterVolume` and `getListener` are now `Audio::playSound`, `Audio::setMasterVolume` and `Audio::getListener`.
* `Sound::loadSound( path )` and `Sound::loadMusic( path )` are replaced by `Sound::load( path, type )`.
* `Sound::setStartTime` and `Sound::setStopTime` now take a delay from *now*.
* A `Waveform`'s default frequency is now 440 Hz, and `Waveform::setSampleRate` was removed.
* You only need `Audio::Device` if you want to change the device's settings.
