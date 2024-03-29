#include "al.h"
#include "alc.h"

#ifndef AL_ALEXT_H
#define AL_ALEXT_H

#if defined(__cplusplus)
extern "C" {
#endif

/*
*
* OpenAL-Soft
*
*/

#define AL_DEFERRED_UPDATES_SOFT                 0xC002

AL_API void AL_APIENTRY alDeferUpdatesSOFT(void);
AL_API void AL_APIENTRY alProcessUpdatesSOFT(void);

typedef void           (AL_APIENTRY *LPALDEFERUPDATESSOFT)(void);
typedef void           (AL_APIENTRY *LPALPROCESSUPDATESSOFT)(void);

/*
*
* NGS
*
*/

typedef void*(*AlMemoryAllocNGS)(size_t size);
typedef void*(*AlMemoryAllocAlignNGS)(size_t align, size_t size);
typedef void(*AlMemoryFreeNGS)(void *ptr);

AL_API void AL_APIENTRY alcSetThreadAffinityNGS(ALCdevice *device, ALCuint outputThreadAffinity, ALCuint updateThreadAffinity);
AL_API void AL_APIENTRY alcSetMemoryFunctionsNGS(AlMemoryAllocNGS alloc, AlMemoryAllocAlignNGS allocAlign, AlMemoryFreeNGS free);

typedef void           (AL_APIENTRY *LPALCSETTHREADAFFINITYNGS)( ALCdevice *device, ALCuint outputThreadAffinity, ALCuint updateThreadAffinity );
typedef void           (AL_APIENTRY *LPALCSETMEMORYFUNCTIONSNGS)( AlMemoryAllocNGS alloc, AlMemoryAllocAlignNGS allocAlign, AlMemoryFreeNGS free );

#if defined(__cplusplus)
}
#endif

#endif /* AL_ALEXT_H */
