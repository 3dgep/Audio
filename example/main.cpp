#include "MarioCoin.hpp"

#include <Audio/Sound.hpp>
#include <Audio/Waveform.hpp>

#include <chrono>
#include <cmath>
#include <cstring>
#include <future>
#include <iostream>
#include <numbers>

using namespace std::chrono;
using std::numbers::pi;

constexpr double pi_over_2 = pi / 2.0;
constexpr double two_pi    = 2.0 * pi;

enum class State
{
    NarratorPart1,
    Waveform,
    NarratorPart2,
    MarioCoin,
    Wait,
    NarratorPart3,
    FadeOut,
    Done
};

int main( int argc, char* argv[] )
{
    // Parse command-line arguments.
    if ( argc > 1 )
    {
        for ( int i = 0; i < argc; ++i )
        {
            if ( strcmp( argv[i], "-cwd" ) == 0 )
            {
                std::string workingDirectory = argv[++i];
                std::filesystem::current_path( workingDirectory );
            }
        }
    }

    Audio::Sound    background { "Rondo_Alla_Turka.ogg", Audio::Sound::Type::Stream };
    Audio::Sound    narrator { "narrator.flac", Audio::Sound::Type::Stream };
    Audio::Waveform waveform { Audio::Waveform::Type::Sine, 0.2f };
    MarioCoin       marioCoin;

    steady_clock::time_point t0        = steady_clock::now();
    double                   totalTime = 0.0;

    State state = State::NarratorPart1;
// an enhancement to setfade has been added, normally you cannot fade up a sound that has been created with soundvolume at 1.0, or used setVolume(x.x), 
// you can fade down but need to allow it to play long enough to hear it.. not ideal.

    background.setLooping( true );


// Toggle this to switch between the broken and fixed fade-in.
#define BROKEN_FADE  // comment out this line to use the working three-arg fade

#ifdef BROKEN_FADE
    // The intuitive-but-broken path: two-arg setFade starts from the current
    // volume, so a sound already at full has nowhere to climb — no fade-up.
    background.setVolume( 1.0f );      // audible base, so a working fade WOULD be heard
    background.setFade( 1.0f, 3000 );  // won't fade up (already at 1.0); will however fade down and can use fade up when done
    std::cout << "Background starts at 1 and stays there. There's no fade in\n";
#else
    // The fix: three-arg setFade takes an explicit begin volume, so it ramps
    // from 0 to full regardless of current volume. Result is scaled by the base
    // volume (fade is volume x gain), so keep base at 1.0 for full range.
    background.setVolume( 1.0f );            // audible base, so a working fade WOULD be heard
    background.setFade( 0.0f, 1.0f, 3000 );  // fades up from silence to full, you can also go higher than 1.0f but risk clipping
    std::cout << "Background has a base of 1.0 but fade range of 0 to 1.0 creates fade in.\n";
#endif

/*
    A point to note: the 2-arg form WILL fade up if you set a low non-zero base with
    setVolume (e.g. 0.01f) and a much larger endVolume in setFade (e.g. 1000.0f),
    because the fade gain then has room to climb. But this relies on endVolume being
    an unclamped multiplier, and the product (base x gain) easily exceeds 1.0 and
    clips — e.g. 0.01 x 1000 = 10, ten times full scale. It breaks the intended
    0..1 volume model. The three-arg form gives the same fade-up cleanly and in range.
 
  */
    
    background.play();

    std::cout << "Hello, and welcome to the Audio library.\n";
    std::cout << "In a few seconds, you will hear a waveform audio sample that tests the human audible range between 20 to 20,000 Hz" << std::endl;

    while ( state != State::Done )
    {
        steady_clock::time_point t1 = steady_clock::now();
 
        auto elapsedTime = duration<double>( t1 - t0 );
        t0               = t1;

        totalTime += elapsedTime.count();

        switch ( state )
        {
        case State::NarratorPart1:
            narrator.play();
            if ( narrator.getCursorInSeconds() > 11.5 )
            {
                totalTime = 0.0;
                narrator.pause();
                background.setFade( 0.0f, 500 );
                state = State::Waveform;
            }
            break;
        case State::Waveform:
        {
            waveform.start();
            const auto f = ( std::sin( totalTime - pi_over_2 ) + 1.0 ) / 2.0 * 10000.0 + 20.0;
            std::cout << "Frequency: " << f << " Hz" << std::endl;

            // Adjust the frequency of the waveform.
            waveform.setFrequency( static_cast<float>( f ) );

            if ( totalTime > two_pi )
            {
                totalTime = 0.0;
                waveform.stop();
                background.setFade( 1.0f, 500 );
                state = State::NarratorPart2;

                std::cout << "\nWaveforms can also be used to create sound effects for use in popular retro video games.\nPerhaps you will recognize this sound effect." << std::endl;
            }
        }
        break;
        case State::NarratorPart2:
            narrator.play();
            if ( totalTime > 7.6 )
            {
                totalTime = 0.0;
                narrator.pause();
                background.setFade( 0.0f, 500 );
                state = State::MarioCoin;
            }
            break;
        case State::MarioCoin:
            marioCoin.play();
            state = State::Wait;
            break;
        case State::Wait:
            marioCoin.update( static_cast<float>( elapsedTime.count() ) );
            if ( totalTime > 1.0 )
            {
                totalTime = 0.0;
                state     = State::NarratorPart3;
                background.setFade( 1.0f, 500 );
                std::cout << "\nThanks for listening." << std::endl;
            }
            break;
        case State::NarratorPart3:
            narrator.play();
            if ( totalTime > 1.5 )
            {
                background.setFade( 0.0f, 1000 );
                narrator.stop();
                totalTime = 0.0f;
                state     = State::FadeOut;
            }
            break;
        case State::FadeOut:
            if ( totalTime > 1.0 )
            {
                state = State::Done;
            }
            break;
        }
    }

    return 0;
}