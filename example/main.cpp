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

    background.setLooping( true );
    background.setVolume( 0.2f );
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