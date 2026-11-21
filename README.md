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
* Works on Windows, Linux and the web ([Emscripten](#cmake-for-the-web-emscripten)). Try the [demo game](https://www.3dgep.com/Audio/) in your browser.

Your project must use C++17 or newer.

## Getting the Library

Choose one of the following:

* **Prebuilt package**: download it from the [Releases](https://github.com/3dgep/Audio/releases) page, then follow [Adding the Library to a Visual Studio Project](#adding-the-library-to-a-visual-studio-project) or [CMake: find_package](#cmake-find_package).
* **CMake FetchContent**: let CMake download and build the library. See [CMake: FetchContent](#cmake-fetchcontent).
* **vcpkg**: see [vcpkg](#vcpkg).
* **Build it yourself**: see [Building From Source](#building-from-source).

### Prebuilt Packages

There are two kinds of packages. If you are not sure, use the static one: it has no DLL that you need to copy.

* `Audio-<version>-windows-x64-static.zip`: a static library (`Audio.lib`).
* `Audio-<version>-windows-x64-shared.zip`: a shared library (`Audio.dll`). Your game needs `Audio.dll` next to its `.exe` (see [Copy the DLL](#5-copy-the-dll-shared-package-only)).

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

These steps add a [prebuilt package](#prebuilt-packages) to your own Visual Studio project. They use a project called `MyDemoGame` as an example.

### 1. Copy the Library Into Your Project

Extract the package and copy its folders to a new `extern\Audio` folder next to your project file (`MyDemoGame.vcxproj`):

```txt
MyDemoGame
├── MyDemoGame.vcxproj
├── main.cpp
└── extern
    └── Audio
        ├── include          (the header files)
        │   └── Audio
        │       ├── Audio.hpp
        │       └── ...
        ├── lib              (the libraries)
        │   ├── Debug
        │   │   └── Audio.lib
        │   └── Release
        │       └── Audio.lib
        └── bin              (shared package only)
            ├── Debug
            │   └── Audio.dll
            └── Release
                └── Audio.dll
```

![Folder Structure](docs/images/Folder%20Structure.png)

> **Tip**: If you [built the library yourself](#building-from-source), copy the `include` and `lib` folders (and for the shared library, the `bin` folder) from the root of this repository instead.

### 2. Open the Project Properties

In **Solution Explorer**, right-click your project and select **Properties**. At the top of the dialog, set:

* **Configuration** to **All Configurations**, so your settings apply to both Debug and Release.
* **Platform** to **x64**. The library is 64-bit only.

The settings below use `$(ProjectDir)`. Visual Studio replaces it with the folder that contains your project file, so the paths keep working when you move or share your project.

### 3. Add the Include Folder

![C/C++ Settings](docs/images/Property%20Pages%20Dialog%20Box.png)

1. Go to **C/C++ > General**. In **Additional Include Directories**, add:

   ```txt
   $(ProjectDir)extern\Audio\include
   ```

2. Go to **C/C++ > Language**. Set **C++ Language Standard** to **ISO C++17 Standard** or newer.

### 4. Link With the Library

![Linker/General Settings](docs/images/Linker%20General%20Settings.png)

1. Go to **Linker > General**. In **Additional Library Directories**, add:

   ```txt
   $(ProjectDir)extern\Audio\lib\$(Configuration)
   ```

   `$(Configuration)` is replaced with `Debug` or `Release`, depending on what you are building.

2. Go to **Linker > Input**. In **Additional Dependencies**, add `Audio.lib`. If the field already contains something, separate the entries with a semicolon, for example:

   ```txt
   Audio.lib;%(AdditionalDependencies)
   ```

![Linker/Input Settings](docs/images/Linker%20Input%20Settings.png)

### 5. Copy the DLL (Shared Package Only)

Skip this step if you use the static package.

A program that uses `Audio.dll` can only start if `Audio.dll` is in the same folder as the program's `.exe`. For a new Visual Studio project, the `.exe` is in the `x64\Debug` or `x64\Release` folder of your solution, **not** in your project folder.

Let Visual Studio copy the DLL there every time you build: go to **Build Events > Post-Build Event** and set **Command Line** to:

```txt
copy /y "$(ProjectDir)extern\Audio\bin\$(Configuration)\Audio.dll" "$(OutDir)"
```

`$(OutDir)` is the folder where Visual Studio puts your `.exe`.

Click **OK** to save the project settings.

### 6. Add Your Sound Files

Your program loads sound files from its *working directory*:

* When you run your program from Visual Studio (<kbd>F5</kbd>), that is your **project folder** (next to `MyDemoGame.vcxproj`).
* When you run the `.exe` directly (for example by double-clicking it), that is the folder of the `.exe`.

So put your sound files in your project folder, for example in an `assets` folder, and load them with a path relative to the project folder:

```cpp
Audio::Sound coin { "assets/coin.wav" };
```

When you share your game, copy the `assets` folder next to the `.exe` (and `Audio.dll`, if you use the shared package).

### 7. Test It

Replace your `main.cpp` with the following code, and put a sound file called `coin.wav` in the `assets` folder of your project:

```cpp
#include <Audio/Audio.hpp>

#include <iostream>

int main()
{
    Audio::Sound coin { "assets/coin.wav" };
    if ( !coin )
    {
        std::cout << "Could not load assets/coin.wav." << std::endl;
        return 1;
    }

    coin.play();

    // Sounds play in the background. Wait, so the program doesn't end before the sound does.
    std::cout << "Press Enter to quit." << std::endl;
    std::cin.get();
}
```

Press <kbd>F5</kbd>. You should hear the sound.

### Troubleshooting

| Error | Fix |
| --- | --- |
| `cannot open source file "Audio/Audio.hpp"` or `C1083: Cannot open include file: 'Audio/Audio.hpp'` | Check the include folder in [step 3](#3-add-the-include-folder). Make sure **Configuration** was set to **All Configurations** when you changed it. |
| `The contents of <filesystem> are available only with C++17 or later.` | Set the C++ Language Standard to C++17 or newer ([step 3](#3-add-the-include-folder)). |
| `LNK1104: cannot open file 'Audio.lib'` | Check the library folder in [step 4](#4-link-with-the-library), and that `lib\Debug` and `lib\Release` both contain `Audio.lib`. |
| `LNK2019: unresolved external symbol audio_...` | Add `Audio.lib` to **Additional Dependencies** ([step 4](#4-link-with-the-library)). |
| `LNK4272: library machine type 'x64' conflicts with target machine type 'x86'` | Set **Platform** to **x64** ([step 2](#2-open-the-project-properties)) and build for x64 (in the toolbar, next to Debug/Release). |
| `The code execution cannot proceed because Audio.dll was not found` | Copy `Audio.dll` next to your `.exe` ([step 5](#5-copy-the-dll-shared-package-only)). |
| `[Audio] Error: Failed to load sound 'assets/coin.wav'` | The file isn't where your program looks for it. See [step 6](#6-add-your-sound-files). |

If you still have problems, ask on our Discord server: [3dgep.com Discord Server](https://discord.gg/gsxxaxc)

### Using the Source Files Instead

Instead of linking with `Audio.lib`, you can also add the library's source files to your project:

1. Copy the [include](include/) and [src](src/) folders of this repository to `extern\Audio` in your project.
2. Add the include folder ([step 3](#3-add-the-include-folder)). Skip steps 4 and 5.
3. In **Solution Explorer**, right-click your project and select **Add > Existing Item**. Add the five `.c` files from `extern\Audio\src`: `audio_device.c`, `audio_sound.c`, `audio_waveform.c`, `miniaudio.c` and `stb_vorbis.c`.
4. If your project uses precompiled headers (`pch.h`), select the five `.c` files in **Solution Explorer**, open their **Properties**, and set **C/C++ > Precompiled Headers > Precompiled Header** to **Not Using Precompiled Headers**.

## Building From Source

You need Visual Studio 2022 or newer with the **Desktop development with C++** workload on Windows, or CMake 3.25 or newer on Linux. To build the examples with CMake, you need CMake 4.0 or newer.

### Visual Studio

Open [Audio.slnx](Audio.slnx) in Visual Studio 2026 and select **Build > Build Solution** (<kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>B</kbd>). This builds `Audio.lib` and the four [examples](#examples). You don't need CMake for this. Press <kbd>F5</kbd> to play the [game](#examples). To run a different example, right-click it in **Solution Explorer** and select **Set as Startup Project**.

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

### CMake for the Web (Emscripten)

The library can also be built for the web (WebAssembly) with [Emscripten](https://emscripten.org/):

```sh
cmake --preset emscripten
cmake --build --preset emscripten-release
```

The `emscripten` preset sets `AUDIO_BUILD_WEB=ON`. CMake then uses the Emscripten SDK you have installed (from `EMSDK`, `emcc` on the `PATH`, or `~/emsdk`). If you don't have one, it downloads Emscripten into `out/emsdk` the first time (about 1 GB). On Windows, Ninja is used from the `PATH`, or else the one that comes with Visual Studio.

The library goes to `lib/web/Debug` and `lib/web/Release`. The [game](#examples) example goes to `bin/web/Release/game.html` (with `game.js`, `game.wasm` and `game.data`). The other examples are desktop only. To play the game, serve the folder with a web server, for example:

```sh
python -m http.server --directory bin/web/Release
```

and open <http://localhost:8000/game.html>. Opening `game.html` directly from disk doesn't work, because browsers don't let a page load files that way.

> **Note**: Browsers only play sound after the user clicks or taps the page. The library starts the sound automatically after that first click or tap. A key press is not enough, so ask the player to click (for example, "Click to play").

### Build Output

* The library goes to the `lib` folder (`lib\Debug\Audio.lib` and `lib\Release\Audio.lib`). On Linux, it is `lib/Debug/libAudio.a` and `lib/Release/libAudio.a`, or `bin/Debug/libAudio.so` and `bin/Release/libAudio.so` for the shared library.
* The examples go to the `bin` folder (for example `bin\Debug\getting_started.exe` and `bin\Release\getting_started.exe`).

### Examples

The [examples](examples/) folder contains four examples:

* [game](examples/game/main.cpp): a small brick game with music, sound effects and waveform jingles. It runs on Windows, Linux, macOS and in a web browser. On Linux, it needs the X11 development files (`sudo apt install libx11-dev`).
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
