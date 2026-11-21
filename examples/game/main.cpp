// Brick Bounce: a small game that shows how to use the Audio library in a game.
//
//  - Background music that streams from a file, and fades when the game is paused
//    or over.
//  - Sound effects that are panned to where they happen, with a pitch that changes
//    with the game (bricks hit in a row sound higher and higher).
//  - Short tunes made with a Waveform, without any sound files.
//
// Controls: move with the mouse or the arrow keys, launch the ball with a click or
// space, pause with P or Escape.

#include "font.h"
#include "platform.h"

#include <Audio/Audio.hpp>

#include <algorithm>
#include <cmath>
#include <random>
#include <string>
#include <string_view>
#include <vector>

using Platform::Height;
using Platform::Key;
using Platform::Width;

// ---------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------

void clear( uint32_t color )
{
    std::fill_n( Platform::pixels(), Width * Height, color );
}

void fillRect( int x, int y, int width, int height, uint32_t color )
{
    const int x0 = std::max( x, 0 );
    const int y0 = std::max( y, 0 );
    const int x1 = std::min( x + width, Width );
    const int y1 = std::min( y + height, Height );

    uint32_t* pixels = Platform::pixels();
    for ( int py = y0; py < y1; ++py )
        std::fill( pixels + py * Width + x0, pixels + py * Width + std::max( x0, x1 ), color );
}

int textWidth( std::string_view text, int scale )
{
    return static_cast<int>( text.size() ) * ( Font::GlyphWidth + 1 ) * scale - scale;
}

void drawText( std::string_view text, int x, int y, int scale, uint32_t color )
{
    for ( char c: text )
    {
        if ( const uint8_t* glyph = Font::getGlyph( c ) )
        {
            for ( int row = 0; row < Font::GlyphHeight; ++row )
                for ( int column = 0; column < Font::GlyphWidth; ++column )
                    if ( glyph[row] & ( 0b10000 >> column ) )
                        fillRect( x + column * scale, y + row * scale, scale, scale, color );
        }
        x += ( Font::GlyphWidth + 1 ) * scale;
    }
}

void drawTextCentered( std::string_view text, int y, int scale, uint32_t color )
{
    drawText( text, ( Width - textWidth( text, scale ) ) / 2, y, scale, color );
}

// ---------------------------------------------------------------------------
// Audio
// ---------------------------------------------------------------------------

// Plays a short tune: a list of notes, using a Waveform.
class Jingle
{
public:
    struct Note
    {
        float frequency;  // In Hz. 0 is a rest.
        float duration;   // In seconds.
    };

    explicit Jingle( Audio::Waveform::Type type, float amplitude )
    : waveform { type, amplitude }
    {}

    void play( std::vector<Note> tune )
    {
        notes = std::move( tune );
        index = 0;
        time  = 0.0f;
        startNote();
    }

    // Call once per frame.
    void update( float deltaTime )
    {
        if ( index >= notes.size() )
            return;

        time += deltaTime;
        if ( time >= notes[index].duration )
        {
            time -= notes[index].duration;
            ++index;
            startNote();
        }
    }

private:
    void startNote()
    {
        if ( index < notes.size() && notes[index].frequency > 0.0f )
        {
            waveform.setFrequency( notes[index].frequency );
            waveform.start();
        }
        else
        {
            waveform.stop();
        }
    }

    Audio::Waveform   waveform;
    std::vector<Note> notes;
    size_t            index = 0;
    float             time  = 0.0f;
};

// Note frequencies, see https://en.wikipedia.org/wiki/Piano_key_frequencies
constexpr float C5 = 523.25f, E5 = 659.26f, G5 = 783.99f, C6 = 1046.50f;
constexpr float G4 = 392.00f, Eb4 = 311.13f, C4 = 261.63f, G3 = 196.00f;

Audio::Sound music;
Audio::Sound paddleSound;
Audio::Sound wallSound;
Audio::Sound brickSound;
Audio::Sound loseSound;
Jingle       jingle { Audio::Waveform::Type::Square, 0.12f };

// The music fades between these volumes.
constexpr float MusicVolume       = 1.0f;
constexpr float MusicPausedVolume = 0.25f;

std::mt19937 rng { std::random_device {}() };

// Play a sound effect, panned to where it happens (x), with a pitch.
void playEffect( Audio::Sound& sound, float x, float pitch = 1.0f )
{
    // A little random variation, so the same sound doesn't get boring.
    std::uniform_real_distribution<float> variation { 0.97f, 1.03f };

    sound.setPan( ( x / Width * 2.0f - 1.0f ) * 0.75f );  // -0.75 (left) to 0.75 (right).
    sound.setPitch( pitch * variation( rng ) );
    sound.replay();
}

// ---------------------------------------------------------------------------
// Game
// ---------------------------------------------------------------------------

enum class State
{
    Title,
    Serve,  // The ball is on the paddle, waiting to be launched.
    Playing,
    Paused,
    LevelClear,
    GameOver,
};

struct Rect
{
    float x, y, width, height;

    bool overlaps( const Rect& other ) const
    {
        return x < other.x + other.width && x + width > other.x && y < other.y + other.height && y + height > other.y;
    }
    float centerX() const
    {
        return x + width / 2.0f;
    }
    bool contains( int px, int py ) const
    {
        return px >= x && px < x + width && py >= y && py < y + height;
    }
};

struct Brick
{
    Rect     rect;
    uint32_t color;
    int      points;
    bool     alive;
};

constexpr int      Top             = 40;  // The playing field starts below the score.
constexpr int      BrickColumns    = 10;
constexpr int      BrickRows       = 6;
constexpr float    PaddleSpeed     = 520.0f;
constexpr float    BallSize        = 8.0f;
constexpr float    MaxBallSpeed    = 650.0f;
constexpr uint32_t BackgroundColor = 0x101828;
constexpr uint32_t RowColors[BrickRows] = { 0xFF5050, 0xFF9F40, 0xFFE050, 0x60D060, 0x50A0FF, 0xB070FF };

State              state = State::Title;
std::vector<Brick> bricks;
Rect               paddle { Width / 2.0f - 40.0f, Height - 40.0f, 80.0f, 12.0f };
Rect               ball { 0.0f, 0.0f, BallSize, BallSize };
float              velocityX  = 0.0f;
float              velocityY  = 0.0f;
float              ballSpeed  = 0.0f;
int                score      = 0;
int                lives      = 0;
int                level      = 0;
int                combo      = 0;  // Bricks hit since the ball last touched the paddle.
float              stateTime  = 0.0f;
float              totalTime  = 0.0f;

// The buttons in the top-right corner of the score bar: fullscreen (where it's
// possible) and pause (while playing). They are larger than their icons, so
// they're easy to tap.
constexpr float ButtonSize = 44.0f;

bool hasFullscreenButton()
{
    return Platform::canToggleFullscreen();
}

bool hasPauseButton()
{
    return state == State::Playing || state == State::Paused;
}

Rect fullscreenButton()
{
    return { Width - ButtonSize, 0.0f, ButtonSize, Top - 2.0f };
}

Rect pauseButton()
{
    const float right = hasFullscreenButton() ? Width - ButtonSize : Width;
    return { right - ButtonSize, 0.0f, ButtonSize, Top - 2.0f };
}

bool isMouseOver( const Rect& button )
{
    return button.contains( Platform::mouseX(), Platform::mouseY() );
}

// Two bars while playing, or a triangle ("play") while paused.
void drawPauseIcon( int x, int y, bool paused, uint32_t color )
{
    if ( paused )
    {
        for ( int row = 0; row < 20; ++row )
            fillRect( x + 3, y + row, std::min( row, 19 - row ) * 3 / 2 + 1, 1, color );
    }
    else
    {
        fillRect( x + 3, y, 5, 20, color );
        fillRect( x + 12, y, 5, 20, color );
    }
}

// Four corners: pointing out to go fullscreen, pointing in to leave it.
void drawFullscreenIcon( int x, int y, bool leave, uint32_t color )
{
    constexpr int size = 20, arm = 7, line = 3;
    for ( int corner = 0; corner < 4; ++corner )
    {
        const bool right  = corner & 1;
        const bool bottom = corner & 2;
        // The point of the corner, and the direction its arms go.
        int cx = x + ( right ? size - line : 0 );
        int cy = y + ( bottom ? size - line : 0 );
        int dx = right ? -1 : 1;
        int dy = bottom ? -1 : 1;
        if ( leave )
        {
            // Move the point towards the center, and flip the arms.
            cx += dx * ( arm - line );
            cy += dy * ( arm - line );
            dx = -dx;
            dy = -dy;
        }
        fillRect( dx > 0 ? cx : cx - arm + line, cy, arm, line, color );  // Horizontal arm.
        fillRect( cx, dy > 0 ? cy : cy - arm + line, line, arm, color );  // Vertical arm.
    }
}

void setState( State newState )
{
    state     = newState;
    stateTime = 0.0f;
}

void resetBricks()
{
    constexpr float width = 56.0f, height = 18.0f, gap = 4.0f;
    constexpr float left  = ( Width - ( BrickColumns * width + ( BrickColumns - 1 ) * gap ) ) / 2.0f;

    bricks.clear();
    for ( int row = 0; row < BrickRows; ++row )
        for ( int column = 0; column < BrickColumns; ++column )
            bricks.push_back( { { left + column * ( width + gap ), 70.0f + row * ( height + gap ), width, height }, RowColors[row], ( BrickRows - row ) * 10, true } );
}

void serve()
{
    combo = 0;
    setState( State::Serve );
}

void startLevel( int newLevel )
{
    level     = newLevel;
    ballSpeed = 300.0f + 30.0f * static_cast<float>( level );
    resetBricks();
    serve();
}

void newGame()
{
    score = 0;
    lives = 3;
    startLevel( 1 );
    music.setFade( MusicVolume, 1000 );
}

void launchBall()
{
    std::uniform_real_distribution<float> angle { -0.5f, 0.5f };  // Radians, about 30 degrees.
    const float                           a = angle( rng );
    velocityX                               = ballSpeed * std::sin( a );
    velocityY                               = -ballSpeed * std::cos( a );
    setState( State::Playing );
}

void togglePause()
{
    if ( state == State::Paused )
    {
        setState( State::Playing );
        music.setFade( MusicVolume, 300 );
    }
    else
    {
        setState( State::Paused );
        music.setFade( MusicPausedVolume, 300 );
    }
}

// Returns true if the ball hit a brick.
bool hitBrick()
{
    for ( Brick& brick: bricks )
    {
        if ( brick.alive && ball.overlaps( brick.rect ) )
        {
            brick.alive = false;
            ++combo;
            score += brick.points * combo;

            // Each brick in a row sounds a bit higher.
            playEffect( brickSound, brick.rect.centerX(), 1.0f + 0.06f * static_cast<float>( std::min( combo, 15 ) ) );
            return true;
        }
    }
    return false;
}

// Move the ball one small step, and bounce it off walls, bricks and the paddle.
void moveBall( float deltaTime )
{
    // Move horizontally, then vertically, so we know which way to bounce.
    ball.x += velocityX * deltaTime;
    if ( ball.x < 0.0f || ball.x + ball.width > Width )
    {
        ball.x    = std::clamp( ball.x, 0.0f, Width - ball.width );
        velocityX = -velocityX;
        playEffect( wallSound, ball.centerX() );
    }
    else if ( hitBrick() )
    {
        ball.x -= velocityX * deltaTime;
        velocityX = -velocityX;
    }

    ball.y += velocityY * deltaTime;
    if ( ball.y < Top )
    {
        ball.y    = Top;
        velocityY = -velocityY;
        playEffect( wallSound, ball.centerX(), 0.8f );
    }
    else if ( hitBrick() )
    {
        ball.y -= velocityY * deltaTime;
        velocityY = -velocityY;
    }
    else if ( velocityY > 0.0f && ball.overlaps( paddle ) )
    {
        // Bounce at an angle that depends on where the ball hits the paddle.
        const float offset = std::clamp( ( ball.centerX() - paddle.centerX() ) / ( paddle.width / 2.0f ), -1.0f, 1.0f );
        const float angle  = offset * 1.05f;  // Up to 60 degrees.

        ballSpeed = std::min( ballSpeed + 5.0f, MaxBallSpeed );
        velocityX = ballSpeed * std::sin( angle );
        velocityY = -ballSpeed * std::cos( angle );
        ball.y    = paddle.y - ball.height;
        combo     = 0;

        // Hitting the paddle with its edge sounds a bit higher.
        playEffect( paddleSound, ball.centerX(), 1.0f + 0.1f * std::abs( offset ) );
    }
}

void update( float deltaTime )
{
    stateTime += deltaTime;
    totalTime += deltaTime;
    jingle.update( deltaTime );

    // The buttons in the score bar. A click on a button doesn't count as an action.
    const bool clicked           = Platform::wasMouseClicked();
    const bool clickedFullscreen = clicked && hasFullscreenButton() && isMouseOver( fullscreenButton() );
    const bool clickedPause      = clicked && hasPauseButton() && isMouseOver( pauseButton() );
    const bool action            = Platform::wasKeyPressed( Key::Space ) || ( clicked && !clickedFullscreen && !clickedPause );

    if ( hasFullscreenButton() && ( clickedFullscreen || Platform::wasKeyPressed( Key::Fullscreen ) ) )
        Platform::toggleFullscreen();

    // Hide the mouse cursor over the playing field while playing; the paddle shows where it is.
    const bool playing = state == State::Serve || state == State::Playing;
    Platform::setCursorVisible( !playing || Platform::mouseY() < Top );

    if ( ( Platform::wasKeyPressed( Key::Pause ) || clickedPause ) && ( state == State::Playing || state == State::Paused ) )
        togglePause();

    // Pause when the player switches to another window or browser tab.
    if ( Platform::wasFocusLost() && state == State::Playing )
        togglePause();

    // Move the paddle, unless the game is paused.
    if ( state == State::Serve || state == State::Playing )
    {
        if ( Platform::hasMouseMoved() )
            paddle.x = static_cast<float>( Platform::mouseX() ) - paddle.width / 2.0f;
        if ( Platform::isKeyDown( Key::Left ) )
            paddle.x -= PaddleSpeed * deltaTime;
        if ( Platform::isKeyDown( Key::Right ) )
            paddle.x += PaddleSpeed * deltaTime;
        paddle.x = std::clamp( paddle.x, 0.0f, Width - paddle.width );
    }

    switch ( state )
    {
    case State::Title:
        if ( action )
            newGame();
        break;

    case State::Serve:
        ball.x = paddle.centerX() - ball.width / 2.0f;
        ball.y = paddle.y - ball.height;
        if ( action )
            launchBall();
        break;

    case State::Playing:
    {
        // Move in small steps, so a fast ball can't pass through a brick.
        const int steps = static_cast<int>( std::ceil( deltaTime / ( 1.0f / 240.0f ) ) );
        for ( int i = 0; i < steps && state == State::Playing; ++i )
        {
            moveBall( deltaTime / static_cast<float>( steps ) );

            if ( ball.y > Height )
            {
                --lives;
                playEffect( loseSound, ball.centerX() );
                if ( lives > 0 )
                {
                    serve();
                }
                else
                {
                    setState( State::GameOver );
                    music.setFade( 0.0f, 2000 );
                    jingle.play( { { G4, 0.2f }, { Eb4, 0.2f }, { C4, 0.2f }, { G3, 0.6f } } );
                }
            }
            else if ( std::none_of( bricks.begin(), bricks.end(), []( const Brick& b ) { return b.alive; } ) )
            {
                setState( State::LevelClear );
                music.setFade( MusicPausedVolume, 300 );
                jingle.play( { { C5, 0.1f }, { E5, 0.1f }, { G5, 0.1f }, { C6, 0.4f } } );
            }
        }
        break;
    }

    case State::Paused:
        if ( action )
            togglePause();
        break;

    case State::LevelClear:
        if ( stateTime > 2.0f )
        {
            music.setFade( MusicVolume, 500 );
            startLevel( level + 1 );
        }
        break;

    case State::GameOver:
        if ( stateTime > 1.0f && action )
            newGame();
        break;
    }
}

void draw()
{
    clear( BackgroundColor );

    // The buttons in the score bar. They light up under the mouse.
    const bool mouse = !Platform::isUsingTouch();
    if ( hasFullscreenButton() )
    {
        const Rect button = fullscreenButton();
        drawFullscreenIcon( static_cast<int>( button.x ) + 12, 9, Platform::isFullscreen(), mouse && isMouseOver( button ) ? 0xFFFFFF : 0xA0A8C0 );
    }
    if ( hasPauseButton() )
    {
        const Rect button = pauseButton();
        drawPauseIcon( static_cast<int>( button.x ) + 12, 9, state == State::Paused, mouse && isMouseOver( button ) ? 0xFFFFFF : 0xA0A8C0 );
    }

    // Score bar. LIVES stays left of the pause button's place, also when the button
    // isn't shown, so it doesn't jump around.
    if ( state != State::Title )
    {
        drawText( "SCORE " + std::to_string( score ), 12, 12, 2, 0xFFFFFF );
        drawTextCentered( "LEVEL " + std::to_string( level ), 12, 2, 0xFFFFFF );
        const std::string livesText = "LIVES " + std::to_string( lives );
        drawText( livesText, static_cast<int>( pauseButton().x ) - 8 - textWidth( livesText, 2 ), 12, 2, 0xFFFFFF );
    }
    fillRect( 0, Top - 2, Width, 2, 0x303850 );

    for ( const Brick& brick: bricks )
    {
        if ( !brick.alive )
            continue;
        const int x = static_cast<int>( brick.rect.x ), y = static_cast<int>( brick.rect.y );
        const int w = static_cast<int>( brick.rect.width ), h = static_cast<int>( brick.rect.height );
        fillRect( x, y, w, h, brick.color );
        fillRect( x, y + h - 4, w, 4, ( brick.color >> 1 ) & 0x7F7F7F );  // Darker bottom edge.
    }

    if ( state != State::Title )
    {
        fillRect( static_cast<int>( paddle.x ), static_cast<int>( paddle.y ), static_cast<int>( paddle.width ), static_cast<int>( paddle.height ), 0xD0D8E8 );
        if ( state != State::GameOver )
            fillRect( static_cast<int>( ball.x ), static_cast<int>( ball.y ), static_cast<int>( ball.width ), static_cast<int>( ball.height ), 0xFFFFFF );
    }

    const bool blink = std::fmod( totalTime, 1.0f ) < 0.6f;

    // On a touch screen, you tap instead of click.
    const bool        touch = Platform::isUsingTouch();
    const std::string click = touch ? "TAP" : "CLICK";

    switch ( state )
    {
    case State::Title:
        drawTextCentered( "BRICK BOUNCE", 230, 6, 0xFFFFFF );
        if ( blink )
            drawTextCentered( click + " TO PLAY", 310, 3, 0xFFE050 );
        if ( touch )
        {
            drawTextCentered( "MOVE: DRAG YOUR FINGER", 380, 2, 0xA0A8C0 );
            drawTextCentered( "LAUNCH: TAP", 410, 2, 0xA0A8C0 );
        }
        else
        {
            drawTextCentered( "MOVE: MOUSE OR ARROW KEYS", 380, 2, 0xA0A8C0 );
            drawTextCentered( "LAUNCH: CLICK OR SPACE    PAUSE: P", 410, 2, 0xA0A8C0 );
        }
        break;
    case State::Serve:
        if ( blink )
            drawTextCentered( click + " TO LAUNCH", 300, 3, 0xFFE050 );
        break;
    case State::Paused:
        drawTextCentered( "PAUSED", 280, 5, 0xFFFFFF );
        drawTextCentered( click + " TO CONTINUE", 340, 3, 0xA0A8C0 );
        break;
    case State::LevelClear:
        drawTextCentered( "LEVEL CLEAR!", 280, 5, 0xFFE050 );
        break;
    case State::GameOver:
        drawTextCentered( "GAME OVER", 260, 6, 0xFF5050 );
        if ( stateTime > 1.0f && blink )
            drawTextCentered( click + " TO PLAY AGAIN", 340, 3, 0xFFE050 );
        break;
    default:
        break;
    }
}

void frame( float deltaTime )
{
    update( deltaTime );
    draw();
}

int main( int argc, char* argv[] )
{
    // The audio files are loaded from the working directory. Use -cwd to load them
    // from another folder: game -cwd path/to/examples/assets
    for ( int i = 1; i + 1 < argc; ++i )
    {
        if ( std::string_view( argv[i] ) == "-cwd" )
            std::filesystem::current_path( argv[i + 1] );
    }

    // Music is streamed from the file while it plays, instead of loading it all into memory.
    music.load( "chiptune_theme.ogg", Audio::Sound::Type::Music );
    music.setLooping( true );
    music.setVolume( 0.4f );
    music.play();  // On the web, the browser starts the sound after the first click.

    paddleSound.load( "paddle.wav" );
    wallSound.load( "wall.wav" );
    wallSound.setVolume( 0.6f );
    brickSound.load( "brick.wav" );
    loseSound.load( "lose.wav" );

    resetBricks();

    Platform::run( "Brick Bounce", frame );
    return 0;
}
