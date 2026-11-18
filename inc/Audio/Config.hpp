#pragma once

/// Shared definitions for the Audio C++ API: results and internal helpers.
/// Part of the header-only C++ wrapper around <Audio/audio.h>.

#include "audio.h"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

namespace Audio
{

/// The result of an operation that can fail.
enum class Result : int32_t
{
    Success          = AUDIO_SUCCESS,
    Error            = AUDIO_ERROR,
    InvalidArgs      = AUDIO_INVALID_ARGS,
    InvalidOperation = AUDIO_INVALID_OPERATION,
    OutOfMemory      = AUDIO_OUT_OF_MEMORY,
    FileNotFound     = AUDIO_FILE_NOT_FOUND,
    InvalidFile      = AUDIO_INVALID_FILE,
    NoBackend        = AUDIO_NO_BACKEND,
    DeviceError      = AUDIO_DEVICE_ERROR,
};

/// Get a human-readable description of a result.
inline const char* toString( Result result ) noexcept
{
    return audio_result_to_string( static_cast<audio_result>( result ) );
}

namespace detail
{
inline Result toResult( audio_result result ) noexcept
{
    return static_cast<Result>( result );
}

/// Convert a path to a UTF-8 string for the C API.
inline std::string toUtf8( const std::filesystem::path& path )
{
#if defined( __cpp_char8_t )
    const std::u8string s = path.u8string();
    return { s.begin(), s.end() };
#else
    return path.u8string();
#endif
}

/// Take ownership of a C handle. Returns an empty pointer if `handle` is null.
///
/// The wrapper types are shared handles (like `std::shared_ptr`) so that beginners can
/// pass them around by value. Using `std::shared_ptr` here is ABI-safe: this header is
/// compiled into the user's program, so it never crosses the library boundary.
template<class T>
std::shared_ptr<T> adopt( T* handle, void( AUDIO_CALL* destroy )( T* ) )
{
    if ( !handle )
        return {};
    return std::shared_ptr<T>( handle, destroy );
}

template<class Rep, class Period>
uint64_t toMilliseconds( const std::chrono::duration<Rep, Period>& duration ) noexcept
{
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>( duration ).count();
    return ms > 0 ? static_cast<uint64_t>( ms ) : 0u;
}

/// Shuts down the default device when the program exits.
///
/// This lives in the header (and is therefore compiled into the user's program, not
/// the Audio library) so that shutdown runs during normal static destruction of the
/// executable, and never inside DllMain while the loader lock is held.
///
/// It is a function-local static so it is constructed the first time the library is
/// used. Static objects constructed after it (for example, a global `Audio::Sound`)
/// are destroyed before it.
struct DefaultDeviceGuard
{
    DefaultDeviceGuard() = default;
    ~DefaultDeviceGuard()
    {
        audio_shutdown();
    }

    DefaultDeviceGuard( const DefaultDeviceGuard& )            = delete;
    DefaultDeviceGuard& operator=( const DefaultDeviceGuard& ) = delete;
};

/// Returns `device` unchanged. When `device` is null (the default device), makes
/// sure the shutdown guard exists.
inline audio_device* useDevice( audio_device* device ) noexcept
{
    if ( !device )
    {
        static DefaultDeviceGuard guard;
        (void)guard;
    }
    return device;
}
}  // namespace detail

}  // namespace Audio
