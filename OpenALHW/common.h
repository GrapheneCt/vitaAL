#include <ngs.h>
#include <libdbg.h>
#include <kernel.h>
#include <stdio.h>

#include "AL/al.h"
#include "AL/alc.h"
#include "AL/alext.h"

#ifndef AL_COMMON_H
#define AL_COMMON_H

// Located in common.cpp
extern int32_t g_lastError;
extern AlMemoryAllocNGS g_alloc;
extern AlMemoryAllocAlignNGS g_memalign;
extern AlMemoryFreeNGS g_free;

#define AL_INTERNAL_MAGIC (0xBC24DB7E)

#define AL_NAMED_OBJECT_MAX (65536)

#define AL_MALLOC(x)		g_alloc(x)
#define AL_MEMALIGN(x, y)	g_memalign(x, y)
#define AL_FREE(x)			g_free(x)

#define AL_DEVICE_NAME "SceNgs"
#define AL_CAPTURE_DEVICE_NAME "SceAudioIn"

#define AL_VERSION_STRING "1.1 SCE " AL_DEVICE_NAME "/" AL_CAPTURE_DEVICE_NAME
#define AL_RENDERER_STRING "SCE " AL_DEVICE_NAME
#define AL_VENDOR_STRING "SCE"

#define AL_INTERNAL_MAJOR_VERSION (1)
#define AL_INTERNAL_MINOR_VERSION (1)

#define AL_SET_ERROR(x) { g_lastError = x; SCE_DBG_LOG_ERROR("AL_SET_ERROR: 0x%X\n", g_lastError); }
#if ((SCE_DBG_MINIMUM_LOG_LEVEL == SCE_DBG_LOG_LEVEL_TRACE) && SCE_DBG_LOGGING_ENABLED)
#define AL_TRACE_CALL { sceClibPrintf("AL_TRACE_CALL:  %s\n", __FUNCTION__); }
#else
#define AL_TRACE_CALL
#endif
#define AL_WARNING SCE_DBG_LOG_WARNING

#define AL_INVALID_NGS_HANDLE (0x0)

ALint _alErrorNgs2Al(ALint error);
ALvoid *_alGetProcAddress(const ALchar *funcName);
ALint _alGetError();
ALCenum _alGetEnumValue(const ALCchar *enumname);
ALint _alSourceStateNgs2Al(SceUInt32 state);
ALint _alNamedObjectGet(ALint id);
ALvoid _alNamedObjectRemove(ALint id);
ALint _alNamedObjectAdd(ALint obj);
ALint _alNamedObjectGetName(ALint obj);

inline SceInt32 _alLockNgsResource(SceNgsHVoice hVoiceHandle, const SceUInt32 uModule, const SceNgsParamsID uParamsInterfaceId, SceNgsBufferInfo* pParamsBuffer)
{
	SceInt32 ret = sceNgsVoiceLockParams(hVoiceHandle, uModule, uParamsInterfaceId, pParamsBuffer);
	while (ret == SCE_NGS_ERROR_RESOURCE_LOCKED)
	{
		ret = sceNgsVoiceLockParams(hVoiceHandle, uModule, uParamsInterfaceId, pParamsBuffer);
		sceKernelDelayThread(100);
	}

	return ret;
}

#define _alTryLockNgsResource sceNgsVoiceLockParams

#endif