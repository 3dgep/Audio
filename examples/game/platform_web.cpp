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
bool usingTouch     = false;
bool focusLost      = false;

// Where the current touch started, to tell a tap from a drag.
int touchStartX = 0;
int touchStartY = 0;

// clang-format off
// Does the device have a touch screen as its main pointer (a phone or a tablet)?
EM_JS( bool, hasTouchScreen, (), {
    return window.matchMedia( '(pointer: coarse)' ).matches;
} );

// Copy the pixels to the canvas.
EM_JS( void, presentCanvas, ( const uint32_t* pixels, int width, int height ), {
    if ( !Module.canvasContext ) {
        Module.canvasContext = Module.canvas.getContext( '2d' );
        Module.canvasImage   = Module.canvasContext.createImageData( width, height );
    }
    Module.canvasImage.data.set( HEAPU8.subarray( pixels, pixels + width * height * 4 ) );
    Module.canvasContext.putImageData( Module.canvasImage, 0, 0 );
} );

EM_JS( void, setCanvasCursor, ( bool visible ), {
    Module.canvas.style.cursor = visible ? 'default' : 'none';
} );

// Fullscreen for the whole page (the canvas scales to fit). In an <iframe>, this
// only works if the iframe has allow="fullscreen". Safari on iPhone can't do it.
EM_JS( bool, canFullscreen, (), {
    return !!( document.fullscreenEnabled || document.webkitFullscreenEnabled );
} );

EM_JS( bool, inFullscreen, (), {
    return !!( document.fullscreenElement || document.webkitFullscreenElement );
} );

EM_JS( void, toggleFullscreenJS, (), {
    if ( document.fullscreenElement || document.webkitFullscreenElement ) {
        ( document.exitFullscreen || document.webkitExitFullscreen ).call( document );
    } else {
        const page    = document.documentElement;
        const request = page.requestFullscreen || page.webkitRequestFullscreen;
        const result  = request.call( page );
        if ( result && result.catch )
            result.catch( () => {} );  // The browser refused: stay as we are.
    }
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
    else if ( !strcmp( code, "KeyF" ) )
        key = Key::Fullscreen;
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
    usingTouch = false;
    return false;
}

bool onMouseDown( int, const EmscriptenMouseEvent* e, void* )
{
    setMousePosition( e->targetX, e->targetY );
    usingTouch = false;
    if ( e->button == 0 )
        mouseClicked = true;
    return false;
}

// Touch works like the mouse: dragging a finger moves the mouse, and a tap (a
// touch that doesn't move much) is a click.
bool onTouch( int eventType, const EmscriptenTouchEvent* e, void* )
{
    for ( int i = 0; i < e->numTouches; ++i )
    {
        const EmscriptenTouchPoint& touch = e->touches[i];
        if ( !touch.isChanged )
            continue;

        setMousePosition( touch.targetX, touch.targetY );
        usingTouch = true;

        if ( eventType == EMSCRIPTEN_EVENT_TOUCHSTART )
        {
            touchStartX = mousePositionX;
            touchStartY = mousePositionY;
        }
        else if ( eventType == EMSCRIPTEN_EVENT_TOUCHEND )
        {
            const int dx = mousePositionX - touchStartX;
            const int dy = mousePositionY - touchStartY;
            if ( dx * dx + dy * dy < 20 * 20 )
                mouseClicked = true;
        }
        break;  // Only the first finger.
    }
    // Don't scroll or zoom the page, and don't send mouse events as well.
    return true;
}

bool onFocusLost( int, const EmscriptenFocusEvent*, void* )
{
    focusLost = true;
    return false;
}

bool onVisibilityChange( int, const EmscriptenVisibilityChangeEvent* e, void* )
{
    if ( e->hidden )
        focusLost = true;
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
    focusLost    = false;
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
    emscripten_set_touchstart_callback( "#canvas", nullptr, true, onTouch );
    emscripten_set_touchmove_callback( "#canvas", nullptr, true, onTouch );
    emscripten_set_touchend_callback( "#canvas", nullptr, true, onTouch );
    emscripten_set_blur_callback( EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, true, onFocusLost );
    emscripten_set_visibilitychange_callback( nullptr, true, onVisibilityChange );

    usingTouch    = hasTouchScreen();
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

bool isUsingTouch()
{
    return usingTouch;
}

bool wasFocusLost()
{
    return focusLost;
}

void setCursorVisible( bool visible )
{
    static int current = -1;  // Only change the style when it changes.
    if ( current != static_cast<int>( visible ) )
    {
        current = visible;
        setCanvasCursor( visible );
    }
}

bool canToggleFullscreen()
{
    return canFullscreen();
}

bool isFullscreen()
{
    return inFullscreen();
}

void toggleFullscreen()
{
    // Browsers only allow this right after a click, tap or key press. The game
    // calls this in the frame after the click, which is soon enough.
    toggleFullscreenJS();
}

}  // namespace Platform
