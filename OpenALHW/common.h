#include <heatwave.h>
#include <libdbg.h>
#include <stdio.h>

#include "AL/al.h"
#include "AL/alc.h"
#include "AL/alext.h"

#ifndef AL_COMMON_H
#define AL_COMMON_H

extern int32_t g_lastError; // Located in alc.cpp

#define AL_DEVICE_NAME "HeatWave"
#define AL_CAPTURE_DEVICE_NAME "AudioIn"

#define AL_VERSION_STRING "1.1 SCE " AL_DEVICE_NAME "/" AL_CAPTURE_DEVICE_NAME
#define AL_RENDERER_STRING "SCE " AL_DEVICE_NAME
#define AL_VENDOR_STRING "SCE"

#define AL_INTERNAL_MAJOR_VERSION (1)
#define AL_INTERNAL_MINOR_VERSION (1)

#define AL_SET_ERROR(x) { g_lastError = x; SCE_DBG_LOG_ERROR("AL_SET_ERROR: 0x%X\n", g_lastError); }
#define AL_WARNING SCE_DBG_LOG_WARNING

#define AL_INVALID_HW_HANDLE (0xffffffffffffffff)

ALint _alErrorHw2Al(int error);
ALvoid *_alGetProcAddress(const ALchar *funcName);
ALint _alGetError();
ALCenum _alGetEnumValue(const ALCchar *enumname);
ALint _alSourceStateHw2Al(SceHwSfxState state);

#endif