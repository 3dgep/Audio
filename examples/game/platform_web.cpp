// The web implementation of platform.h: the pixels are drawn to a <canvas>, and
// the browser calls the frame function (emscripten_set_main_loop).

#include "platform.h"

#include <emscripten.h>
#include <emscripten/html5.h>

#include <cstring>

namespace
{
using namespace Platform;

uint32_t      pixelBuffer[Width * Height];  // 0xRRGGBB, what the game draws.
uint32_t      canvasBuffer[Width * Height]; // RGBA bytes, what the canvas needs.
FrameFunction frameFunction = nullptr;
double        previousTime  = 0.0;

bool keyDown[static_cast<int>( Key::Count )]    = {};
bool keyPressed[static_cast<int>( Key::Count )] = {};
int  mousePositionX = 0;
int  mousePositionY = 0;
bool mouseMoved     = false;
bool mouseClicked   = false;

// Copy the pixels to the canvas.
// clang-format off
EM_JS( void, presentCanvas, ( const uint32_t* pixels, int width, int height ), {
    if ( !Module.canvasContext ) {
        Module.canvasContext = Module.canvas.getContext( '2d' );
        Module.canvasImage   = Module.canvasContext.createImageData( width, height );
    }
    Module.canvasImage.data.set( HEAPU8.subarray( pixels, pixels + width * height * 4 ) );
    Module.canvasContext.putImageData( Module.canvasImage, 0, 0 );
} );
// clang-format on

// Map a KeyboardEvent.code to a key. Returns false for keys the game doesn't use.
bool toKey( const char* code, Key& key )
{
    if ( !strcmp( code, "ArrowLeft" ) || !strcmp( code, "KeyA" ) )
        key = Key::Left;
    else if ( !strcmp( code, "ArrowRight" ) || !strcmp( code, "KeyD" ) )
        key = Key::Right;
    else if ( !strcmp( code, "Space" ) || !strcmp( code, "Enter" ) )
        key = Key::Space;
    else if ( !strcmp( code, "KeyP" ) || !strcmp( code, "Escape" ) )
        key = Key::Pause;
    else
        return false;
    return true;
}

bool onKey( int eventType, const EmscriptenKeyboardEvent* e, void* )
{
    Key key;
    if ( !toKey( e->code, key ) )
        return false;  // Let the browser handle other keys.

    const int  index = static_cast<int>( key );
    const bool down  = eventType == EMSCRIPTEN_EVENT_KEYDOWN;
    if ( down && !e->repeat )
        keyPressed[index] = true;
    keyDown[index] = down;
    return true;  // Don't scroll the page with the arrow keys or space.
}

// Convert a position on the canvas (CSS pixels, the canvas may be scaled) to pixels.
void setMousePosition( int x, int y )
{
    double cssWidth, cssHeight;
    emscripten_get_element_css_size( "#canvas", &cssWidth, &cssHeight );
    if ( cssWidth > 0.0 && cssHeight > 0.0 )
    {
        mousePositionX = static_cast<int>( x * Width / cssWidth );
        mousePositionY = static_cast<int>( y * Height / cssHeight );
        mouseMoved     = true;
    }
}

bool onMouseMove( int, const EmscriptenMouseEvent* e, void* )
{
    setMousePosition( e->targetX, e->targetY );
    return false;
}

bool onMouseDown( int, const EmscriptenMouseEvent* e, void* )
{
    setMousePosition( e->targetX, e->targetY );
    if ( e->button == 0 )
        mouseClicked = true;
    return false;
}

void mainLoop()
{
    const double now       = emscripten_get_now();
    float        deltaTime = static_cast<float>( ( now - previousTime ) / 1000.0 );
    previousTime           = now;

    // After the tab was in the background, don't jump ahead.
    if ( deltaTime > 0.1f )
        deltaTime = 0.1f;

    frameFunction( deltaTime );

    // 0xRRGGBB -> R, G, B, A bytes (little endian: 0xAABBGGRR).
    for ( int i = 0; i < Width * Height; ++i )
    {
        const uint32_t p = pixelBuffer[i];
        canvasBuffer[i]  = 0xFF000000u | ( ( p & 0xFFu ) << 16 ) | ( p & 0xFF00u ) | ( ( p >> 16 ) & 0xFFu );
    }
    presentCanvas( canvasBuffer, Width, Height );

    // Input events that happened during this frame have been handled.
    std::memset( keyPressed, 0, sizeof( keyPressed ) );
    mouseMoved   = false;
    mouseClicked = false;
}

}  // namespace

namespace Platform
{
void run( const char* title, FrameFunction frame )
{
    emscripten_set_window_title( title );
    emscripten_set_canvas_element_size( "#canvas", Width, Height );

    emscripten_set_keydown_callback( EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, true, onKey );
    emscripten_set_keyup_callback( EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, true, onKey );
    emscripten_set_mousemove_callback( "#canvas", nullptr, true, onMouseMove );
    emscripten_set_mousedown_callback( "#canvas", nullptr, true, onMouseDown );

    frameFunction = frame;
    previousTime  = emscripten_get_now();

    // Let the browser call mainLoop() before each repaint. This doesn't return.
    emscripten_set_main_loop( mainLoop, 0, true );
}

uint32_t* pixels()
{
    return pixelBuffer;
}

bool isKeyDown( Key key )
{
    return keyDown[static_cast<int>( key )];
}

bool wasKeyPressed( Key key )
{
    return keyPressed[static_cast<int>( key )];
}

int mouseX()
{
    return mousePositionX;
}

int mouseY()
{
    return mousePositionY;
}

bool hasMouseMoved()
{
    return mouseMoved;
}

bool wasMouseClicked()
{
    return mouseClicked;
}

}  // namespace Platform
