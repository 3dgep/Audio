#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

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
