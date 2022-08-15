#include "al.h"
#include "alc.h"

#ifndef AL_ALEXT_H
#define AL_ALEXT_H

#if defined(__cplusplus)
extern "C" {
#endif

typedef void*(*AlMemoryAllocNGS)(size_t size);
typedef void*(*AlMemoryAllocAlignNGS)(size_t align, size_t size);
typedef void(*AlMemoryFreeNGS)(void *ptr);

AL_API void AL_APIENTRY alcSetThreadAffinityNGS(ALCdevice *device, ALCuint outputThreadAffinity);
AL_API void AL_APIENTRY alcSetMemoryFunctionsNGS(AlMemoryAllocNGS alloc, AlMemoryAllocAlignNGS allocAlign, AlMemoryFreeNGS free);

/*
 * Pointer-to-function types, useful for dynamically getting AL entry points.
 */
typedef void           (AL_APIENTRY *LPALCSETTHREADAFFINITYNGS)( ALCdevice *device, ALCuint outputThreadAffinity );
typedef void           (AL_APIENTRY *LPALCSETMEMORYFUNCTIONSNGS)( AlMemoryAllocNGS alloc, AlMemoryAllocAlignNGS allocAlign, AlMemoryFreeNGS free );

#if defined(__cplusplus)
}
#endif

#endif /* AL_ALEXT_H */
