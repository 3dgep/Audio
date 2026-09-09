// Demonstrates Sound::setFade.
//
// The two-argument setFade( endVolume, duration ) starts the ramp from the
// sound's *current* fade gain. That gain defaults to 1.0, so a sound that has
// not been faded yet cannot be faded in - there is nowhere to climb from.
//
// The three-argument setFade( beginVolume, endVolume, duration ) takes an
// explicit starting gain, so it can ramp up from silence regardless of the
// current gain.
//
// Note that the fade is a gain applied on top of the sound's volume: what you
// hear is volume x gain. The base volume below is deliberately less than 1.0,
// so a gain of 1.0 is comfortable rather than full scale.

#include <Audio/Sound.hpp>

#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>

using namespace std::chrono;

// Base volume for the music. All fade gains below are relative to this.
constexpr float baseVolume = 0.5f;

enum class State
{
    TwoArgFadeIn,      // The broken case: two-arg setFade cannot fade in.
    ThreeArgFadeDown,  // Three-arg, explicit 1.0 -> 0.0.
    ThreeArgFadeUp,    // Three-arg, 0.0 -> 1.0. The case two-arg cannot do.
    ThreeArgPartial,   // Three-arg with an arbitrary range.
    ChronoOverload,    // Three-arg taking a std::chrono::duration.
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

    Audio::Sound music { "Rondo_Alla_Turka.ogg", Audio::Sound::Type::Stream };

    music.setLooping( true );
    music.setVolume( baseVolume );

    // Set the fade before starting playback. The fader only advances as frames
    // are read, so the ramp begins exactly when the sound does.
    music.setFade( 1.0f, 2000 );

    std::cout << "1. setFade( 1.0f, 2000 ) before play, gain is already 1.0.\n"
              << "   Expect NO fade in - the music starts at full base volume." << std::endl;

    music.play();

    steady_clock::time_point t0        = steady_clock::now();
    double                   totalTime = 0.0;

    State state = State::TwoArgFadeIn;

    while ( state != State::Done )
    {
        steady_clock::time_point t1 = steady_clock::now();

        auto elapsedTime = duration<double>( t1 - t0 );
        t0               = t1;

        totalTime += elapsedTime.count();

        switch ( state )
        {
        case State::TwoArgFadeIn:
            if ( totalTime > 3.0 )
            {
                totalTime = 0.0;
                state     = State::ThreeArgFadeDown;

                music.setFade( 1.0f, 0.0f, 1500 );
                std::cout << "\n2. setFade( 1.0f, 0.0f, 1500 )\n"
                          << "   Expect a fade down to silence." << std::endl;
            }
            break;
        case State::ThreeArgFadeDown:
            if ( totalTime > 2.5 )
            {
                totalTime = 0.0;
                state     = State::ThreeArgFadeUp;

                music.setFade( 0.0f, 1.0f, 3000 );
                std::cout << "\n3. setFade( 0.0f, 1.0f, 3000 )\n"
                          << "   Expect a fade up from silence. This is what the two-arg form cannot do." << std::endl;
            }
            break;
        case State::ThreeArgFadeUp:
            if ( totalTime > 4.0 )
            {
                totalTime = 0.0;
                state     = State::ThreeArgPartial;

                music.setFade( 1.0f, 0.3f, 1500 );
                std::cout << "\n4. setFade( 1.0f, 0.3f, 1500 )\n"
                          << "   Expect a partial fade down to 30% gain, not to silence." << std::endl;
            }
            break;
        case State::ThreeArgPartial:
            if ( totalTime > 2.5 )
            {
                totalTime = 0.0;
                state     = State::ChronoOverload;

                // The std::chrono overload: seconds are converted to milliseconds.
                music.setFade( 0.3f, 1.0f, seconds( 2 ) );
                std::cout << "\n5. setFade( 0.3f, 1.0f, seconds( 2 ) )\n"
                          << "   Expect a fade back up to full gain, timed by the chrono overload." << std::endl;
            }
            break;
        case State::ChronoOverload:
            if ( totalTime > 3.0 )
            {
                totalTime = 0.0;
                state     = State::FadeOut;

                music.setFade( 1.0f, 0.0f, 1500 );
                std::cout << "\n6. setFade( 1.0f, 0.0f, 1500 )\n"
                          << "   Fading out." << std::endl;
            }
            break;
        case State::FadeOut:
            if ( totalTime > 2.0 )
            {
                state = State::Done;
            }
            break;
        case State::Done:
            break;
        }

        // Nothing here needs to run at frame rate; the fades are driven by the
        // audio thread.
        std::this_thread::sleep_for( milliseconds( 10 ) );
    }

    music.stop();

    std::cout << "\nDone." << std::endl;

    return 0;
}
