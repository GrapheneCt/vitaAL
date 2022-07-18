#include "al.h"
#include "alc.h"

#ifndef AL_ALEXT_H
#define AL_ALEXT_H

#if defined(__cplusplus)
extern "C" {
#endif

AL_API void AL_APIENTRY alcSetMp3ChannelCountNGS( ALCdevice *device, ALCint count );
AL_API void AL_APIENTRY alcSetAt9ChannelCountNGS( ALCdevice *device, ALCint count );

/*
 * Pointer-to-function types, useful for dynamically getting AL entry points.
 */
typedef void           (AL_APIENTRY *LPALCSETMP3CHANNELCOUNTNGS)( ALCdevice *device, ALCint count );
typedef void           (AL_APIENTRY *LPALCSETAT9CHANNELCOUNTNGS)( ALCdevice *device, ALCint count );

#if defined(__cplusplus)
}
#endif

#endif /* AL_ALEXT_H */
