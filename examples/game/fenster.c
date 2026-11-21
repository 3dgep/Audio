/*
 * The implementation of fenster (https://github.com/zserge/fenster), a minimal
 * cross-platform window with a pixel buffer. It is compiled as C, because its
 * Windows code doesn't compile as C++. platform_desktop.cpp uses it.
 *
 * fenster/fenster.h is an unmodified copy of fenster.h at commit
 * e700581dfb7956dd161aee44fc0cff0663e789a1 (MIT license, see fenster/LICENSE).
 */
#if defined( _MSC_VER )
    /* fenster_sleep passes an int64_t to Sleep, which takes a DWORD. */
    #pragma warning( disable : 4244 )
#endif

#include "fenster/fenster.h"
