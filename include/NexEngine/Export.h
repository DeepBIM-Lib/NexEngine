#ifndef NEXENGINE_EXPORT_H
#define NEXENGINE_EXPORT_H

#if defined(NexEngine_BUILD_SHARED)
#    if defined(_WIN32) || defined(__CYGWIN__)
#        if defined(NexEngine_EXPORTS)
#            define NEXENGINE_API __declspec(dllexport)
#        else
#            define NEXENGINE_API __declspec(dllimport)
#        endif
#        define NEXENGINE_LOCAL
#    elif defined(__GNUC__) || defined(__clang__)
#        define NEXENGINE_API __attribute__((visibility("default")))
#        define NEXENGINE_LOCAL __attribute__((visibility("hidden")))
#    else
#        define NEXENGINE_API
#        define NEXENGINE_LOCAL
#    endif
#else
#    define NEXENGINE_API
#    define NEXENGINE_LOCAL
#endif

#if defined(__GNUC__) || defined(__clang__)
#    define NEXENGINE_DEPRECATED __attribute__((deprecated))
#elif defined(_MSC_VER)
#    define NEXENGINE_DEPRECATED __declspec(deprecated)
#else
#    define NEXENGINE_DEPRECATED
#endif

#define NEXENGINE_DEPRECATED_API NEXENGINE_DEPRECATED NEXENGINE_API
#define NEXENGINE_DEPRECATED_NO_EXPORT NEXENGINE_DEPRECATED

namespace Nex {}

#endif // NEXENGINE_EXPORT_H
