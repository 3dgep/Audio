#pragma once

// A minimal platform layer for the game: a window with a pixel buffer, keyboard
// and mouse input, and a frame loop. There is one implementation per platform:
//  - platform_web.cpp:     a <canvas> in a web page (Emscripten).
//  - platform_desktop.cpp: a window on Windows, Linux or macOS.
//
// The game itself (main.cpp) only uses this header, so it runs unchanged on both.

#include <cstdint>

namespace Platform
{
// The size of the pixel buffer.
constexpr int Width  = 640;
constexpr int Height = 480;

enum class Key
{
    Left,
    Right,
    Space,
    Pause,       // P or Escape.
    Fullscreen,  // F.
    Count
};

// Called once per frame with the time since the previous frame, in seconds.
using FrameFunction = void ( * )( float deltaTime );

// Open the window and call frame() once per frame until the window is closed.
// On the web, this returns immediately and the browser calls frame().
void run( const char* title, FrameFunction frame );

// The pixels of the window: Width * Height colors in 0xRRGGBB format, row by row.
// Draw the next frame here; it is shown after frame() returns.
uint32_t* pixels();

// Is the key held down?
bool isKeyDown( Key key );

// Was the key pressed during the last frame?
bool wasKeyPressed( Key key );

// The mouse position in pixels, and whether it moved during the last frame.
int  mouseX();
int  mouseY();
bool hasMouseMoved();

// Was the (left) mouse button clicked during the last frame? On a touch screen,
// a tap is a click, and dragging a finger moves the mouse.
bool wasMouseClicked();

// Is the player using a touch screen (instead of a mouse)?
bool isUsingTouch();

// Did the game lose focus during the last frame (for example, the player switched
// to another window or browser tab)? A good moment to pause the game.
bool wasFocusLost();

// Show or hide the mouse cursor while it is over the game.
void setCursorVisible( bool visible );

// Fullscreen: in the browser and on Windows. canToggleFullscreen() is false where
// it isn't possible (Linux, macOS, and Safari on iPhone).
bool canToggleFullscreen();
bool isFullscreen();
void toggleFullscreen();

}  // namespace Platform
