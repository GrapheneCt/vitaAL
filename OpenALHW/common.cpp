#include <heatwave.h>
#include <string.h>

#include "common.h"

int32_t g_lastError = AL_NO_ERROR;

#define DECL(x) { #x, reinterpret_cast<void*>(x) }
const struct {
	const char *funcName;
	void *address;
} s_alcFunctions[] = {
	DECL(alcCreateContext),
	DECL(alcMakeContextCurrent),
	DECL(alcProcessContext),
	DECL(alcSuspendContext),
	DECL(alcDestroyContext),
	DECL(alcGetCurrentContext),
	DECL(alcGetContextsDevice),
	DECL(alcOpenDevice),
	DECL(alcCloseDevice),
	DECL(alcGetError),
	DECL(alcIsExtensionPresent),
	DECL(alcGetProcAddress),
	DECL(alcGetEnumValue),
	DECL(alcGetString),
	DECL(alcGetIntegerv),
	DECL(alcCaptureOpenDevice),
	DECL(alcCaptureCloseDevice),
	DECL(alcCaptureStart),
	DECL(alcCaptureStop),
	DECL(alcCaptureSamples),

	//DECL(alEnable),
	//DECL(alDisable),
	//DECL(alIsEnabled),
	//DECL(alGetString),
	//DECL(alGetBooleanv),
	//DECL(alGetIntegerv),
	//DECL(alGetFloatv),
	//DECL(alGetDoublev),
	//DECL(alGetBoolean),
	//DECL(alGetInteger),
	//DECL(alGetFloat),
	//DECL(alGetDouble),
	//DECL(alGetError),
	//DECL(alIsExtensionPresent),
	//DECL(alGetProcAddress),
	//DECL(alGetEnumValue),
	//DECL(alListenerf),
	//DECL(alListener3f),
	//DECL(alListenerfv),
	//DECL(alListeneri),
	//DECL(alListener3i),
	//DECL(alListeneriv),
	//DECL(alGetListenerf),
	//DECL(alGetListener3f),
	//DECL(alGetListenerfv),
	//DECL(alGetListeneri),
	//DECL(alGetListener3i),
	//DECL(alGetListeneriv),
	//DECL(alGenSources),
	//DECL(alDeleteSources),
	//DECL(alIsSource),
	//DECL(alSourcef),
	//DECL(alSource3f),
	//DECL(alSourcefv),
	//DECL(alSourcei),
	//DECL(alSource3i),
	//DECL(alSourceiv),
	DECL(alGetSourcef),
	DECL(alGetSource3f),
	DECL(alGetSourcefv),
	DECL(alGetSourcei),
	DECL(alGetSource3i),
	DECL(alGetSourceiv),
	DECL(alSourcePlayv),
	DECL(alSourceStopv),
	DECL(alSourceRewindv),
	DECL(alSourcePausev),
	DECL(alSourcePlay),
	DECL(alSourceStop),
	DECL(alSourceRewind),
	DECL(alSourcePause),
	DECL(alSourceQueueBuffers),
	DECL(alSourceUnqueueBuffers),
	DECL(alGenBuffers),
	DECL(alDeleteBuffers),
	DECL(alIsBuffer),
	DECL(alBufferData),
	DECL(alBufferf),
	DECL(alBuffer3f),
	DECL(alBufferfv),
	DECL(alBufferi),
	DECL(alBuffer3i),
	DECL(alBufferiv),
	DECL(alGetBufferf),
	DECL(alGetBuffer3f),
	DECL(alGetBufferfv),
	DECL(alGetBufferi),
	DECL(alGetBuffer3i),
	DECL(alGetBufferiv),
	//DECL(alDopplerFactor),
	//DECL(alDopplerVelocity),
	//DECL(alSpeedOfSound),
	//DECL(alDistanceModel),

	DECL(alcSetMp3ChannelCountNGS),
	DECL(alcSetAt9ChannelCountNGS),
};
#undef DECL

#define DECL(x) { #x, (x) }
constexpr struct {
	const ALCchar *enumName;
	ALCenum value;
} s_alcEnumerations[] = {
	DECL(ALC_INVALID),
	DECL(ALC_FALSE),
	DECL(ALC_TRUE),

	DECL(ALC_MAJOR_VERSION),
	DECL(ALC_MINOR_VERSION),
	DECL(ALC_ATTRIBUTES_SIZE),
	DECL(ALC_ALL_ATTRIBUTES),
	DECL(ALC_DEFAULT_DEVICE_SPECIFIER),
	DECL(ALC_DEVICE_SPECIFIER),
	DECL(ALC_ALL_DEVICES_SPECIFIER),
	DECL(ALC_DEFAULT_ALL_DEVICES_SPECIFIER),
	DECL(ALC_EXTENSIONS),
	DECL(ALC_FREQUENCY),
	DECL(ALC_REFRESH),
	DECL(ALC_SYNC),
	DECL(ALC_MONO_SOURCES),
	DECL(ALC_STEREO_SOURCES),
	DECL(ALC_CAPTURE_DEVICE_SPECIFIER),
	DECL(ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER),
	DECL(ALC_CAPTURE_SAMPLES),

	DECL(ALC_NO_ERROR),
	DECL(ALC_INVALID_DEVICE),
	DECL(ALC_INVALID_CONTEXT),
	DECL(ALC_INVALID_ENUM),
	DECL(ALC_INVALID_VALUE),
	DECL(ALC_OUT_OF_MEMORY),


	DECL(AL_INVALID),
	DECL(AL_NONE),
	DECL(AL_FALSE),
	DECL(AL_TRUE),

	DECL(AL_SOURCE_RELATIVE),
	DECL(AL_CONE_INNER_ANGLE),
	DECL(AL_CONE_OUTER_ANGLE),
	DECL(AL_PITCH),
	DECL(AL_POSITION),
	DECL(AL_DIRECTION),
	DECL(AL_VELOCITY),
	DECL(AL_LOOPING),
	DECL(AL_BUFFER),
	DECL(AL_GAIN),
	DECL(AL_MIN_GAIN),
	DECL(AL_MAX_GAIN),
	DECL(AL_ORIENTATION),
	DECL(AL_REFERENCE_DISTANCE),
	DECL(AL_ROLLOFF_FACTOR),
	DECL(AL_CONE_OUTER_GAIN),
	DECL(AL_MAX_DISTANCE),
	DECL(AL_SEC_OFFSET),
	DECL(AL_SAMPLE_OFFSET),
	DECL(AL_BYTE_OFFSET),
	DECL(AL_SOURCE_TYPE),
	DECL(AL_STATIC),
	DECL(AL_STREAMING),
	DECL(AL_UNDETERMINED),

	DECL(AL_SOURCE_STATE),
	DECL(AL_INITIAL),
	DECL(AL_PLAYING),
	DECL(AL_PAUSED),
	DECL(AL_STOPPED),

	DECL(AL_BUFFERS_QUEUED),
	DECL(AL_BUFFERS_PROCESSED),

	DECL(AL_FORMAT_MONO8),
	DECL(AL_FORMAT_MONO16),
	DECL(AL_FORMAT_STEREO8),
	DECL(AL_FORMAT_STEREO16),

	DECL(AL_FREQUENCY),
	DECL(AL_BITS),
	DECL(AL_CHANNELS),
	DECL(AL_SIZE),

	DECL(AL_UNUSED),
	DECL(AL_PENDING),
	DECL(AL_PROCESSED),

	DECL(AL_NO_ERROR),
	DECL(AL_INVALID_NAME),
	DECL(AL_INVALID_ENUM),
	DECL(AL_INVALID_VALUE),
	DECL(AL_INVALID_OPERATION),
	DECL(AL_OUT_OF_MEMORY),

	DECL(AL_VENDOR),
	DECL(AL_VERSION),
	DECL(AL_RENDERER),
	DECL(AL_EXTENSIONS),

	DECL(AL_DOPPLER_FACTOR),
	DECL(AL_DOPPLER_VELOCITY),
	DECL(AL_DISTANCE_MODEL),
	DECL(AL_SPEED_OF_SOUND),

	DECL(AL_INVERSE_DISTANCE),
	DECL(AL_INVERSE_DISTANCE_CLAMPED),
	DECL(AL_LINEAR_DISTANCE),
	DECL(AL_LINEAR_DISTANCE_CLAMPED),
	DECL(AL_EXPONENT_DISTANCE),
	DECL(AL_EXPONENT_DISTANCE_CLAMPED),
};
#undef DECL

ALint _alGetError()
{
	return g_lastError;
}

ALCenum _alGetEnumValue(const ALCchar *enumname)
{
	for (const auto &enm : s_alcEnumerations)
	{
		if (strcmp(enm.enumName, enumname) == 0)
			return enm.value;
	}

	return 0;
}

ALvoid *_alGetProcAddress(const ALchar *funcName)
{
	for (const auto &func : s_alcFunctions)
	{
		if (strcmp(func.funcName, funcName) == 0)
			return func.address;
	}

	return NULL;
}

ALint _alErrorHw2Al(SceHwError error)
{
	ALint alError = AL_NO_ERROR;

	switch (error) {
	case SCE_HEATWAVE_API_ERROR:
	case SCE_HEATWAVE_API_ERROR_HANDLE:
	case SCE_HEATWAVE_API_ERROR_FILE:
	case SCE_HEATWAVE_API_ERROR_NO_HARDWARE_CONTEXT:
	case SCE_HEATWAVE_API_ERROR_INVALID_OPERATION:
	case SCE_HEATWAVE_API_ERROR_NO_OUTPUT_DEVICE:
	case SCE_HEATWAVE_API_ERROR_UNSUPPORTED_FORMAT:
		alError = AL_INVALID_OPERATION;
		break;
	case SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS:
		alError = AL_OUT_OF_MEMORY;
		break;
	case SCE_HEATWAVE_API_ERROR_PARAM:
		alError = AL_INVALID_VALUE;
		break;
	default:
		break;
	}

	return alError;
}