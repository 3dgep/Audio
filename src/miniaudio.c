/*
 * The miniaudio implementation, with OGG Vorbis support from stb_vorbis.
 * AUDIO_USE_SYSTEM_DEPS: use an installed miniaudio and stb_vorbis instead of the
 * copies in src/.
 */
#define STB_VORBIS_HEADER_ONLY
#if defined( AUDIO_USE_SYSTEM_DEPS )
    #include <stb_vorbis.c>
#else
    #include "stb_vorbis.c"
#endif

#define MINIAUDIO_IMPLEMENTATION
#if defined( AUDIO_USE_SYSTEM_DEPS )
    #include <miniaudio.h>
#else
    #include "miniaudio.h"
#endif

#include "audio_internal.h"

/*
 * Workaround: ma_sound_set_pinned_listener_index rejects MA_LISTENER_INDEX_CLOSEST
 * (it is not less than the listener count), so a pinned sound cannot be unpinned
 * through the public API. This needs miniaudio's internal atomics, which are only
 * available in the translation unit that contains the implementation.
 */
void audio__ma_sound_unpin_listener( ma_sound* sound )
{
    ma_atomic_exchange_explicit_32( &sound->engineNode.pinnedListenerIndex, MA_LISTENER_INDEX_CLOSEST, ma_atomic_memory_order_release );
}
