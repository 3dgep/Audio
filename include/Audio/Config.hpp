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

/// The result of an operation that can fail. Use `toString` to get a description.
enum class Result : int32_t
{
    Success          = AUDIO_SUCCESS,            ///< The operation succeeded.
    Error            = AUDIO_ERROR,              ///< An unknown error occurred.
    InvalidArgs      = AUDIO_INVALID_ARGS,       ///< An argument was invalid (for example, an empty device).
    InvalidOperation = AUDIO_INVALID_OPERATION,  ///< The operation is not valid for this object or state.
    OutOfMemory      = AUDIO_OUT_OF_MEMORY,      ///< Memory could not be allocated.
    FileNotFound     = AUDIO_FILE_NOT_FOUND,     ///< The file does not exist (check the path and the working directory).
    InvalidFile      = AUDIO_INVALID_FILE,       ///< The file is corrupt or its format is not supported.
    NoBackend        = AUDIO_NO_BACKEND,         ///< No audio backend is available on this system.
    DeviceError      = AUDIO_DEVICE_ERROR,       ///< The playback device could not be initialized or started.
};

/// Get a human-readable description of a result.
inline const char* toString( Result result ) noexcept
{
    return audio_result_to_string( static_cast<audio_result>( result ) );
}

/// Implementation details of the C++ wrapper. Not part of the API.
namespace detail
{
/// Convert a C result code to a `Result`.
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

/// Convert a duration (for example `2s` or `500ms`) to milliseconds. Negative
/// durations become 0.
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
