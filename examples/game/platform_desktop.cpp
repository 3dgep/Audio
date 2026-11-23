// The desktop implementation of platform.h (Windows, Linux and macOS), using
// fenster for the window: https://github.com/zserge/fenster

#include "platform.h"

#if defined( _WIN32 )
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
#endif

// Only the declarations: the implementation is in fenster.c, which is C.
#define FENSTER_HEADER
#define FENSTER_API extern "C"
#include "fenster/fenster.h"

#include <chrono>
#include <cstring>
#include <thread>

#if defined( _WIN32 )
    #include <timeapi.h>  // timeBeginPeriod (winmm.lib).
#elif !defined( __APPLE__ )
    #include <X11/Xutil.h>  // XSizeHints.
#endif

namespace
{
using namespace Platform;
using Clock = std::chrono::steady_clock;

uint32_t pixelBuffer[Width * Height];  // 0xRRGGBB, the same format fenster uses.

// The other fields (input and platform handles) start at zero.
#if defined( __GNUC__ )
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
struct fenster window = { .title = "", .width = Width, .height = Height, .buf = pixelBuffer };
#if defined( __GNUC__ )
    #pragma GCC diagnostic pop
#endif

// Input of the current and the previous frame, to find out what changed.
bool keyDown[static_cast<int>( Key::Count )]     = {};
bool keyWasDown[static_cast<int>( Key::Count )]  = {};
int  mousePositionX = 0, mousePositionY = 0;
int  previousMouseX = 0, previousMouseY = 0;
bool mouseDown = false, mouseWasDown = false;

// Set when the window loses focus (Windows only), and reported for one frame.
bool focusLost = false, focusLostThisFrame = false;

bool cursorVisible = true;

// fenster's key codes: mostly ASCII (upper case letters), the arrows are 17-20.
constexpr int KeyUp = 17, KeyDown = 18, KeyRight = 19, KeyLeft = 20;
constexpr int KeyEnter = 10, KeyEscape = 27, KeySpace = 32;

#if defined( _WIN32 )
// On Windows, the game is scaled to fit the window (fenster always draws it at its
// original size). This is where it's drawn: as large as possible, keeping its
// shape. The rest of the window is black.
RECT gameRect( HWND hwnd )
{
    RECT client;
    GetClientRect( hwnd, &client );
    int width  = client.right;
    int height = client.right * Height / Width;
    if ( height > client.bottom )
    {
        height = client.bottom;
        width  = client.bottom * Width / Height;
    }
    const int x = ( client.right - width ) / 2;
    const int y = ( client.bottom - height ) / 2;
    return { x, y, x + width, y + height };
}

// fenster's window procedure. Ours handles painting, and passes everything else on.
WNDPROC fensterWindowProc = nullptr;

LRESULT CALLBACK windowProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    switch ( message )
    {
    case WM_ERASEBKGND:
        return 1;  // WM_PAINT draws the whole window.

    case WM_PAINT:
    {
        PAINTSTRUCT paint;
        HDC         dc = BeginPaint( hwnd, &paint );

        // Black bars around the game.
        RECT       client;
        const RECT game = gameRect( hwnd );
        GetClientRect( hwnd, &client );
        ExcludeClipRect( dc, game.left, game.top, game.right, game.bottom );
        FillRect( dc, &client, static_cast<HBRUSH>( GetStockObject( BLACK_BRUSH ) ) );
        SelectClipRgn( dc, nullptr );

        // The pixels, scaled to the size of the game rectangle. COLORONCOLOR keeps
        // the pixels sharp.
        BITMAPINFO info {};
        info.bmiHeader.biSize        = sizeof( BITMAPINFOHEADER );
        info.bmiHeader.biWidth       = Width;
        info.bmiHeader.biHeight      = -Height;  // Negative: the first row is the top.
        info.bmiHeader.biPlanes      = 1;
        info.bmiHeader.biBitCount    = 32;
        info.bmiHeader.biCompression = BI_RGB;  // 0x00RRGGBB, like pixelBuffer.
        SetStretchBltMode( dc, COLORONCOLOR );
        StretchDIBits( dc, game.left, game.top, game.right - game.left, game.bottom - game.top, 0, 0, Width, Height, pixelBuffer, &info, DIB_RGB_COLORS, SRCCOPY );

        EndPaint( hwnd, &paint );
        return 0;
    }

    case WM_ACTIVATE:
        if ( LOWORD( wParam ) == WA_INACTIVE )
            focusLost = true;
        break;  // fenster handles the rest.

    case WM_SETCURSOR:
        // Inside the window: show the arrow, or no cursor at all (setCursorVisible).
        if ( LOWORD( lParam ) == HTCLIENT )
        {
            SetCursor( cursorVisible ? LoadCursor( nullptr, IDC_ARROW ) : nullptr );
            return TRUE;
        }
        break;

    case WM_DPICHANGED:
    {
        // The window moved to a screen with a different scale: use the size Windows suggests.
        const RECT* suggested = reinterpret_cast<const RECT*>( lParam );
        SetWindowPos( hwnd, nullptr, suggested->left, suggested->top, suggested->right - suggested->left, suggested->bottom - suggested->top, SWP_NOZORDER | SWP_NOACTIVATE );
        return 0;
    }
    }
    return CallWindowProc( fensterWindowProc, hwnd, message, wParam, lParam );
}

void setUpWindow()
{
    fensterWindowProc = reinterpret_cast<WNDPROC>( SetWindowLongPtr( window.hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>( windowProc ) ) );

    // fenster makes the whole window Width x Height, including its border. Resize
    // it, so the inside of the window is Width x Height, times the scale of the
    // screen (for example 960 x 720 at 150%).
    const UINT dpi  = GetDpiForWindow( window.hwnd );
    RECT       rect { 0, 0, MulDiv( Width, dpi, 96 ), MulDiv( Height, dpi, 96 ) };
    AdjustWindowRectExForDpi( &rect, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_CLIENTEDGE, dpi );
    SetWindowPos( window.hwnd, nullptr, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOMOVE | SWP_NOZORDER );
}

// Convert a mouse position in the window to a position in the game.
void windowToGame( int& x, int& y )
{
    const RECT game = gameRect( window.hwnd );
    if ( game.right > game.left && game.bottom > game.top )
    {
        x = ( x - game.left ) * Width / ( game.right - game.left );
        y = ( y - game.top ) * Height / ( game.bottom - game.top );
    }
}

// Fullscreen: a window without a border that covers the whole screen. The game
// scales to fit (see WM_PAINT). The window's old size and position are restored
// when leaving fullscreen.
bool            fullscreen = false;
WINDOWPLACEMENT windowedPlacement { sizeof( WINDOWPLACEMENT ) };

void setFullscreen( bool enable )
{
    HWND       hwnd    = window.hwnd;
    const LONG style   = GetWindowLong( hwnd, GWL_STYLE );
    const LONG exStyle = GetWindowLong( hwnd, GWL_EXSTYLE );

    if ( enable && !fullscreen )
    {
        MONITORINFO monitor { sizeof( MONITORINFO ) };
        if ( !GetWindowPlacement( hwnd, &windowedPlacement ) || !GetMonitorInfo( MonitorFromWindow( hwnd, MONITOR_DEFAULTTONEAREST ), &monitor ) )
            return;

        const RECT& screen = monitor.rcMonitor;
        SetWindowLong( hwnd, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW );
        SetWindowLong( hwnd, GWL_EXSTYLE, exStyle & ~WS_EX_CLIENTEDGE );
        SetWindowPos( hwnd, HWND_TOP, screen.left, screen.top, screen.right - screen.left, screen.bottom - screen.top, SWP_NOOWNERZORDER | SWP_FRAMECHANGED );
        fullscreen = true;
    }
    else if ( !enable && fullscreen )
    {
        SetWindowLong( hwnd, GWL_STYLE, style | WS_OVERLAPPEDWINDOW );
        SetWindowLong( hwnd, GWL_EXSTYLE, exStyle | WS_EX_CLIENTEDGE );
        SetWindowPlacement( hwnd, &windowedPlacement );
        SetWindowPos( hwnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED );
        fullscreen = false;
    }
}

#elif !defined( __APPLE__ )
// On Linux (X11), fenster always draws the game at its original size, so don't
// let the window be resized. (On macOS, fenster's window can't be resized.)
void setUpWindow()
{
    XSizeHints* hints = XAllocSizeHints();
    hints->flags      = PMinSize | PMaxSize;
    hints->min_width = hints->max_width = Width;
    hints->min_height = hints->max_height = Height;
    XSetWMNormalHints( window.dpy, window.w, hints );
    XFree( hints );
}

void windowToGame( int&, int& ) {}

#else
void setUpWindow() {}
void windowToGame( int&, int& ) {}
#endif

void readInput()
{
    std::memcpy( keyWasDown, keyDown, sizeof( keyDown ) );
    mouseWasDown   = mouseDown;
    previousMouseX = mousePositionX;
    previousMouseY = mousePositionY;
    mousePositionX = window.x;
    mousePositionY = window.y;
    windowToGame( mousePositionX, mousePositionY );

    const int* keys                         = window.keys;
    keyDown[static_cast<int>( Key::Left )]  = keys[KeyLeft] || keys['A'];
    keyDown[static_cast<int>( Key::Right )] = keys[KeyRight] || keys['D'];
    keyDown[static_cast<int>( Key::Up )]    = keys[KeyUp] || keys['W'];
    keyDown[static_cast<int>( Key::Down )]  = keys[KeyDown] || keys['S'];
    keyDown[static_cast<int>( Key::Space )] = keys[KeySpace] || keys[KeyEnter];
    keyDown[static_cast<int>( Key::Pause )] = keys['P'] || keys[KeyEscape];
    keyDown[static_cast<int>( Key::Fullscreen )] = keys['F'];
    mouseDown                               = window.mouse != 0;

    focusLostThisFrame = focusLost;
    focusLost          = false;
}

}  // namespace

namespace Platform
{
void run( const char* title, FrameFunction frame )
{
#if defined( _WIN32 )
    // Use the real pixels of the screen, so Windows doesn't make the window blurry
    // by scaling it up on screens with a scale above 100%.
    SetProcessDpiAwarenessContext( DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 );
#endif

    window.title = title;
    if ( fenster_open( &window ) != 0 )
        return;

    setUpWindow();

#if defined( _WIN32 )
    // Sleep in steps of 1 ms instead of about 15 ms, for a steady frame rate.
    timeBeginPeriod( 1 );
#endif

    constexpr auto frameDuration = std::chrono::microseconds( 1000000 / 60 );  // 60 frames per second.

    auto previousTime = Clock::now();

    // fenster_loop handles the window's events and shows the pixels.
    while ( fenster_loop( &window ) == 0 )
    {
        const auto now       = Clock::now();
        float      deltaTime = std::chrono::duration<float>( now - previousTime ).count();
        previousTime         = now;

        // After the window was dragged or the program was paused in the debugger,
        // don't jump ahead.
        if ( deltaTime > 0.1f )
            deltaTime = 0.1f;

        readInput();
        frame( deltaTime );

        std::this_thread::sleep_until( now + frameDuration );
    }

#if defined( _WIN32 )
    timeEndPeriod( 1 );
#endif
    fenster_close( &window );
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
    return keyDown[static_cast<int>( key )] && !keyWasDown[static_cast<int>( key )];
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
    return mousePositionX != previousMouseX || mousePositionY != previousMouseY;
}

bool isUsingTouch()
{
    return false;  // fenster only has mouse input.
}

bool wasFocusLost()
{
    return focusLostThisFrame;
}

bool wasMouseClicked()
{
    return mouseDown && !mouseWasDown;
}

void setCursorVisible( bool visible )
{
    cursorVisible = visible;  // Used on Windows (WM_SETCURSOR).
}

#if defined( _WIN32 )
bool canToggleFullscreen()
{
    return true;
}

bool isFullscreen()
{
    return fullscreen;
}

void toggleFullscreen()
{
    setFullscreen( !fullscreen );
}
#else
// On Linux and macOS, fenster can't scale the game to fill the screen.
bool canToggleFullscreen()
{
    return false;
}

bool isFullscreen()
{
    return false;
}

void toggleFullscreen() {}
#endif

}  // namespace Platform
