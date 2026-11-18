<!-- markdownlint-disable MD033 -->
# Audio Library

[![Alt text](docs/images/Audio-Library.png)](https://youtu.be/sXPN-QH1kwg)

A super minimalistic C/C++ audio library built on [miniaudio](https://miniaud.io/index.html).

## Features

Should work on most platforms, but I've only tested it on Windows.

The library is written in C99 and exposes a C API ([audio.h](inc/Audio/audio.h)) with a thin, header-only C++ wrapper on top. Include [Audio.hpp](inc/Audio/Audio.hpp) to get the whole C++ API, or include only what you need (for example `<Audio/Sound.hpp>`). Because only C crosses the library boundary, a compiled `Audio.lib` works with any version of Visual Studio's C++ standard library and can be linked into both Debug and Release builds of your game, with either the static (`/MT`) or DLL (`/MD`) C runtime.

There is no need to create or destroy an audio device: it is created the first time you play a sound and shut down automatically when your program exits.

The `Audio::Sound` class can be used to play one-shot sound effects and can also be used to stream background music to minimize memory use.

The `Sound` class supports the following file types:

* WAV (`.wav`): Waveform Audio File Format
* MP3 (`.mp3`): MPEG-1/2 Audio Layer III
* FLAC (`.flac`): Free Lossless Audio Codec
* OGG (`.ogg`): Free, Open Container Format ([Xiph.org](https://xiph.org/))

The `Audio::Waveform` class can be used to play waveform sounds. See [MarioCoin.hpp](example/MarioCoin.hpp) and [MarioCoin.cpp](example/MarioCoin.cpp) for an example of creating a complex sound effect using Waveforms.

## Installation

This project comes with both a Visual Studio 2022 solution and project file and a `CMakeLists.txt` file for use with CMake.

### Visual Studio 2022

Open the `Audio.sln` file in the root directory of this repository. You should see two projects in the solution:

1. Audio (Static library): This project generates the `Audio.lib` file that you must link to in your own project.
2. example (Executable): This is an example showing how to work with the Audio library. You can inspect the project settings of this project to see how to integrate the `Audio.lib` file into your own projects.

Running the example project in Visual Studio should play a narration and show a few examples of working with `Waveform`s.

### CMake

#### Windows

You should have (at least) CMake 3.23.0 installed (and added to your `PATH` environment variable in Windows). If you don't have CMake, please download it from [cmake.org](https://cmake.org/download/). Make sure you download the installer for your platform (don't download the source distributions).

To generate the Visual Studio solution and project files, run the [GenerateProjectFiles.bat](GenerateProjectFiles.bat) batch file on Windows. This script uses [vswhere](https://github.com/microsoft/vswhere) to detect the latest version of Visual Studio you have installed and will generate the solution and project files for the version of Visual Studio you have installed.

Alternatively, you can open the Audio folder directly in the CMake GUI to generate the solution and project files:

![CMake GUI](docs/images/CMakeGUI.png)

There is a preset (use the preset drop-down menu in the CMake GUI) for Visual Studio 2019 and 2022 that you can use to generate the solution and project files.

In the CMake GUI, use the **Configure**, **Generate**, and **Open Project** buttons (in that order) to generate and open the Visual Studio solution file.

#### Linux Based Platforms

CMake can usually detect your build toolchain on Linux based operating systems. To configure and build this project on Linux run the following commands from this project's root folder:

```bash
mkdir build
cd build
cmake ../
cmake --build .
```

This will build the **Debug** build by default. To build the **Release** library, use the following command:

```bash
cmake --build . --config Release
```

> **Note**: Do not distribute the CMake generated project files! These files contain absolute paths and will only work for your file structure.

## Build

Open either the existing [Audio.sln](Audio.sln) file, or the CMake generated solution file in Visual Studio.

![Visual Studio 2022](docs/images/Visual%20Studio%202022.png)

Select **Build > Build Solution**  (<kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>B</kbd>) from the main menu to build the `Audio.lib` and `example.exe`.

The `Audio.lib` file will go to the `lib` folder in the root of this repository depending on the current configuration.

* **Debug**: `lib\Debug\Audio.lib`
* **Release**: `lib\Release\Audio.lib`

> **Note**: Do not distribute the generated library file with your project or commit it to GitHub. The library file should always be compiled for the toolchain version where it will be used.

The `example.exe` file will be placed in the `bin` folder.

* **Debug**: `bin\Debug\example.exe`
* **Release**: `bin\Release\example.exe`

The example project should run directly from Visual Studio, but the example tries to open the [narrator.flac](example/narrator.flac) audio file in the [example](example/) folder. When running the `example.exe` file directly from the command line, you should also specify the [example](example/) folder as the current working directory when running:

```sh
.\bin\Release\example.exe -cwd ".\example"
```

If you see `Failed to initialize sound from source: narrator.flac` printed to the console, then the wrong working directory is being used to run the example. In this case, you will still hear the waveforms (since they don't rely on loading any files from disk), but you won't hear the narrator audio file.

## Using Prebuilt Packages

Prebuilt packages are available on the [Releases](https://github.com/3dgep/Audio/releases) page. Download the ZIP that matches your compiler and configuration:

* `Audio-<version>-<platform>-<compiler>-static.zip` — static library (`.lib`)
* `Audio-<version>-<platform>-<compiler>-shared.zip` — shared library (`.dll` + import `.lib`)

Each package contains both Debug and Release binaries:

```txt
├── include
│   └── Audio
│       ├── audio.h
│       ├── Audio.hpp
│       ├── Config.hpp
│       ├── Device.hpp
│       ├── Listener.hpp
│       ├── Sound.hpp
│       ├── Vector.hpp
│       └── Waveform.hpp
├── lib
│   ├── Debug
│   │   └── Audio.lib
│   ├── Release
│   │   └── Audio.lib
│   └── cmake
│       └── Audio
│           ├── AudioConfig.cmake
│           ├── AudioConfigVersion.cmake
│           └── AudioTargets.cmake
```

Shared packages additionally contain:

```txt
├── bin
│   ├── Debug
│   │   └── Audio.dll
│   └── Release
│       └── Audio.dll
```

### Using with CMake (Recommended)

Extract the package and use `find_package` to integrate it into your CMake project:

```cmake
# Point CMake to the extracted package
list(APPEND CMAKE_PREFIX_PATH "path/to/Audio-<version>-<platform>-<compiler>-static")

find_package(Audio REQUIRED)

add_executable(MyApp main.cpp)
target_link_libraries(MyApp PRIVATE Audio::Audio)
```

CMake will automatically set up include paths and link the correct library for your build configuration.

### Using with FetchContent

You can also pull the library directly from GitHub without downloading a package:

```cmake
include(FetchContent)

FetchContent_Declare(
    Audio
    GIT_REPOSITORY https://github.com/3dgep/Audio.git
    GIT_TAG        v2.0.0
)

FetchContent_MakeAvailable(Audio)

add_executable(MyApp main.cpp)
target_link_libraries(MyApp PRIVATE Audio::Audio)
```

## Integrating Into Your Own Project (Manual)

To use the Audio library in your own projects, copy the `lib` and `inc` folders to a folder in your own project. For example, suppose you keep your 3rd party files in a folder called `extern` or `3rdParty`.

Create a subfolder called `Audio` and copy the contents of the `lib` and `inc` folders to the `Audio` subfolder. You should have a folder structure that is similar to what is shown in the screenshot below:

![Folder Structure](docs/images/Folder%20Structure.png)

Or if you prefer a textual view:

```txt
├── MyDemoGame
│   ├── extern
│   │   ├── Audio
│   │   │   ├── inc
│   │   │   │   ├── Audio
│   │   │   │   │   ├── audio.h
│   │   │   │   │   ├── Audio.hpp
│   │   │   │   │   ├── Config.hpp
│   │   │   │   │   ├── Device.hpp
│   │   │   │   │   ├── Listener.hpp
│   │   │   │   │   ├── Sound.hpp
│   │   │   │   │   ├── Vector.hpp
│   │   │   │   │   ├── Waveform.hpp
│   │   │   ├── lib
│   │   │   │   ├── Debug
│   │   │   │   │   ├── Audio.lib
│   │   │   │   │   ├── Audio.pdb
│   │   │   │   ├── Release
│   │   │   │   │   ├── Audio.lib
│   │   │   │   │   ├── Audio.pdb
```

Alternatively, you can just copy the contents of the [src](src/) folder to where you keep your source files (`.c`, or `.cpp` files) for your game project, and the contents of the [inc](inc/) folder to where you keep your header files (`.h` or `.hpp` files), then add all of the `.c` files directly to your game project. Doing this does not require to generate the `Audio.lib` file and you can skip the next section.

### C/C++ Settings

In order for the compiler to find the include files, you need to tell the compiler where to find the additional header files.

Open the project settings for your game project (either by right-clicking on the project in Visual Studio, or select **Project > Properties** from the main menu in Visual Studio).

Expand the **C/C++** node in the Property Pages dialog box and select the **General** subnode.

![C/C++ Settings](docs/images/Property%20Pages%20Dialog%20Box.png)

In the **Additional Include Directories**, specify the folder to the `Audio\inc` folder you copied the header files to.

> **Note**: Make sure you set the **Configuration** to **All Configurations** to apply the include directory to all build configurations!

### Linker Settings

You must tell the linker where to find the `Audio.lib` file depending on the build configuration being used.

Open the project settings for your game project (either by right-clicking on the project in Visual Studio, or select **Project > Properties** from the main menu in Visual Studio).

Expand the **Linker** node in the Property Pages dialog box and select the **General** subnode.

![Linker/General Settings](docs/images/Linker%20General%20Settings.png)

In the **Additional Library Directories** setting, specify the folder to the `Audio\lib` folder you copied the `Audio.lib` files to.

> **Note**: The `$(Configuration)` macro shown in the screenshot will resolve to `Debug` when creating a debug build, and `Release` when building a release build. This way, you can specify the same path for all configurations.

Now select the **Input** subnode in the project's property pages dialog:

![Linker/Input Settings](docs/images/Linker%20Input%20Settings.png)

In the **Additional Dependencies** settings, specify `Audio.lib` for both the **Debug** and **Release** configurations.

You should be able to compile your project with the Audio library. If you have any issues, please join our Discord server: [3dgep.com Discord Server](https://discord.gg/gsxxaxc)

## Playing Sounds

The very easiest way to play a sound effect is "fire and forget". The same sound can overlap itself, but you can't stop it once it has started:

```cpp
#include <Audio/Audio.hpp>
...
Audio::playSound( "explosion.wav" );
```

For more control, load a sound effect file into an `Audio::Sound` object and use the `Sound::play` or `Sound::replay` (the latter will automatically force the sound to play from the beginning in case it is already playing).

```cpp
#include <Audio/Audio.hpp>
...
// Create a Sound effect from a wav file:
Audio::Sound coin { "coin.wav" };
```

Later, the sound effect can be played using the `Sound::play` method:

```cpp
// Play the coin sound effect:
coin.play();
```

Or, if there is a chance the sound is already playing, it will not be restarted using the `Sound::play` method. This can happen if you play many coin sounds in rapid succession. To force the sound effect to play from the beginning, use the `Sound::replay` method:

```cpp
// Force the coin sound to play from the beginning:
coin.replay();
```

Use the `Sound::stop` method to force the sound effect to stop playing:

```cpp
// Stop the sound effect:
coin.stop()
```

> **Note**: Stopping a sound effect does not automatically rewind the sound effect to the beginning of the sound. Use the `Sound::seek` method to seek to the beginning of the sound. You can also use `Sound::replay` to automatically rewind the sound to the beginning.

### Copying Sounds

Copying an `Audio::Sound` does not copy the sound: just like objects in C#, Java, Python or JavaScript, both copies refer to the *same* sound. This means you can pass sounds to functions and store them in other objects without worrying about it:

```cpp
void playHitSound( Audio::Sound sound )
{
    sound.play(); // Keeps playing after this function returns.
}
```

The sound is destroyed when the last copy is gone. If you need two independent sounds from the same file (for example, to set a different volume on each), load the file twice. The audio data is shared between them, so this is cheap.

### Checking for Errors

If a sound fails to load, an error is printed to the console and the sound is empty. Using an empty sound is safe (it just doesn't make any noise). You can check if a sound loaded correctly:

```cpp
Audio::Sound coin { "coin.wav" };
if ( !coin )
{
    // The sound failed to load.
}
```

## Spatial Audio

Sound effects can make use of spatial sound effects. A `Sound` has a position in 3D space relative to a `Listener`. In order to hear the correct spatial sounds, both the `Sound` and `Listener` must be set the correct position.

```cpp
#include <Audio/Audio.hpp>
...
// Create a Sound effect from a wav file:
Audio::Sound coin { "coin.wav" };
// Set the position of the sound in the world.
coin.setPosition( {x, y, z} );
...
// Get a listener.
Audio::Listener listener = Audio::getListener();
...
// Position the listener based on the position of the player.
listener.setPosition( player.getPosition() );
// Optionally, you can also set the direction of the listener.
listener.setDirection( player.getDirection() );
```

Spatial audio allows you to specify the attenuation model for the sound effect. There are 4 attenuation models:

1. **Off**: No attenuation. This is equivalent to disabling spatialization for the sound effect.
2. **Inverse**: Default attenuation that models natural sound attenuation.
3. **Linear**: Linear attenuation falloff (less natural, but might be better for 2D sound effects).
4. **Exponential**: Exponential attenuation.

The code snippet shows how to disable attenuation for a sound effect:

```cpp
sound.setAttenuationModel( Audio::Sound::AttenuationModel::Off );
```

By default, both sounds and the listener have a position of {0, 0, 0}. In this case, the sounds will not exhibit any spatial attenuation.

## Playing Music

Short, one-shot sound effects are loaded into memory and decoded on creation. To minimize the impact on loading larger files, it is recommended to stream in the files and decode the audio file "on the fly" while playing. The reduces the time to load the file as well as reduced the amount of memory required to store the audio file.

```cpp
#include <Audio/Audio.hpp>
...
// Load background music, streamed while it plays:
Audio::Sound bgMusic { "Background_Music.mp3", Audio::Sound::Type::Music };
```

The only difference between loading background music and one-shot sound effects is the additional `Audio::Sound::Type::Music` parameter in the constructor (`Type::Stream` and `Type::Background` are aliases). Playing, stopping, and restarting of streamed sounds is the same as one-shot sound effects.

## Playing Waveforms

An `Audio::Waveform` class can be used to play waveform audio. Many early video games simulated sound effects using waveforms or [MIDI](https://en.wikipedia.org/wiki/MIDI) audio because it was much easier to store and synthesize the audio than use WAV files.

The `Audio::Waveform` class has two properties that can be modified at runtime to achieve the desired sound effect:

* **Frequency**: The "speed" of the waveform determines the apparent pitch of the sound. This value is measured in Hz.
* **Amplitude**: The "strength" of the sound and determines how loud it sounds. This value is a floating-point value that is measured in the range from 0 (no sound) to 1 (100% volume). Amplitude is modulated by the master volume (see `Audio::setMasterVolume`).

A waveform is silent until you call `Waveform::start`.

```cpp
Audio::Waveform beep { Audio::Waveform::Type::Square, 0.2f, 440.0f };
beep.start();
```

See [MarioCoin.cpp](example/MarioCoin.cpp) for an example of how to use a Waveform.

## Using the C API

The C API in [audio.h](inc/Audio/audio.h) can be used from C, or from any language that can call C functions. Functions that take an `audio_device*` accept `NULL` to use the default device. See [cExample.c](example/cExample.c) for a complete example.

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
audio_shutdown(); // Optional when the program exits normally.
```

## Upgrading From v1

* The headers have the same names as in v1 (`<Audio/Sound.hpp>`, `<Audio/Device.hpp>`, ...), so existing `#include` lines keep working. You can also include `<Audio/Audio.hpp>` to get everything.
* `Audio::Device::playSound`, `setMasterVolume` and `getListener` are now free functions: `Audio::playSound`, `Audio::setMasterVolume` and `Audio::getListener`.
* `Sound::loadSound( path )` and `Sound::loadMusic( path )` are replaced by `Sound::load( path, type )`, which returns an `Audio::Result`.
* `Sound::setStartTime` and `Sound::setStopTime` now take a delay from *now* (in v1, the value was incorrectly treated as an absolute time).
* A `Waveform`'s default frequency is now 440 Hz instead of 0 Hz, and `Waveform::setSampleRate` was removed (waveforms always play at the device's sample rate).
* `Audio::Device` is now only needed if you want to create an audio device with specific settings.
