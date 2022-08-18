#include <kernel.h>
#include <ngs.h>
#include <string.h>
#include <ctype.h>

#include "common.h"
#include "context.h"
#include "panner.h"
#include "device.h"

using namespace al;

AL_API void AL_APIENTRY alEnable(ALenum capability)
{
	Context *ctx = (Context *)alcGetCurrentContext();

	AL_TRACE_CALL

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	AL_SET_ERROR(AL_INVALID_ENUM);
}

AL_API void AL_APIENTRY alDisable(ALenum capability)
{
	Context *ctx = (Context *)alcGetCurrentContext();

	AL_TRACE_CALL

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	AL_SET_ERROR(AL_INVALID_ENUM);
}

AL_API ALboolean AL_APIENTRY alIsEnabled(ALenum capability)
{
	ALboolean value = AL_FALSE;
	Context *ctx = (Context *)alcGetCurrentContext();

	AL_TRACE_CALL

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return value;
	}

	AL_SET_ERROR(AL_INVALID_ENUM);

	return value;
}

AL_API const ALchar* AL_APIENTRY alGetString(ALenum param)
{
	const ALCchar *value = NULL;
	Context *ctx = (Context *)alcGetCurrentContext();

	AL_TRACE_CALL

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return value;
	}

	switch (param)
	{
	case AL_NO_ERROR:
		value = "AL_NO_ERROR";
		break;
	case AL_INVALID_NAME:
		value = "AL_INVALID_NAME";
		break;
	case AL_INVALID_ENUM:
		value = "AL_INVALID_ENUM";
		break;
	case AL_INVALID_VALUE:
		value = "AL_INVALID_VALUE";
		break;
	case AL_INVALID_OPERATION:
		value = "AL_INVALID_OPERATION";
		break;
	case AL_OUT_OF_MEMORY:
		value = "AL_OUT_OF_MEMORY";
		break;
	case AL_VERSION:
		value = AL_VERSION_STRING;
		break;
	case AL_RENDERER:
		value = AL_RENDERER_STRING;
		break;
	case AL_VENDOR:
		value = AL_VENDOR_STRING;
		break;
	case AL_EXTENSIONS:
		value = ctx->getDevice()->getExtensionList();
		break;
	default:
		AL_SET_ERROR(AL_INVALID_ENUM);
		break;
	}

	return value;
}

AL_API void AL_APIENTRY alGetBooleanv(ALenum param, ALboolean* data)
{
	AL_TRACE_CALL

	if (data == NULL)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	*data = alGetBoolean(param);
}

AL_API void AL_APIENTRY alGetIntegerv(ALenum param, ALint* data)
{
	AL_TRACE_CALL

	if (data == NULL)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	*data = alGetInteger(param);
}

AL_API void AL_APIENTRY alGetFloatv(ALenum param, ALfloat* data)
{
	AL_TRACE_CALL

	if (data == NULL)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	*data = alGetFloat(param);
}

AL_API void AL_APIENTRY alGetDoublev(ALenum param, ALdouble* data)
{
	AL_TRACE_CALL

	if (data == NULL)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	*data = alGetDouble(param);
}

AL_API ALboolean AL_APIENTRY alGetBoolean(ALenum param)
{
	ALboolean value = AL_FALSE;
	Context *ctx = (Context *)alcGetCurrentContext();

	AL_TRACE_CALL

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return value;
	}

	switch (param)
	{
	case AL_DOPPLER_FACTOR:
		value = (ctx->m_panner.m_dopplerFactor != 0.0f);
		break;
	case AL_DISTANCE_MODEL:
		if (ctx->m_panner.m_distanceModel != -1)
		{
			value = AL_TRUE;
		}
		else
		{
			value = AL_FALSE;
		}
		break;
	case AL_SPEED_OF_SOUND:
		value = (ctx->m_panner.m_speedOfSound != 0.0f);
		break;
	default:
		AL_SET_ERROR(AL_INVALID_ENUM);
		break;
	}

	return value;
}

AL_API ALint AL_APIENTRY alGetInteger(ALenum param)
{
	AL_TRACE_CALL

	return (ALint)alGetFloat(param);
}

AL_API ALfloat AL_APIENTRY alGetFloat(ALenum param)
{
	ALfloat value = 0.0f;
	Context *ctx = (Context *)alcGetCurrentContext();

	AL_TRACE_CALL

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return value;
	}

	switch (param)
	{
	case AL_DOPPLER_FACTOR:
		value = ctx->m_panner.m_dopplerFactor;
		break;
	case AL_DISTANCE_MODEL:
		value = (ALfloat)ctx->m_panner.m_distanceModel;
		break;
	case AL_SPEED_OF_SOUND:
		value = ctx->m_panner.m_speedOfSound;
		break;
	default:
		AL_SET_ERROR(AL_INVALID_ENUM);
		break;
	}

	return value;
}

AL_API ALdouble AL_APIENTRY alGetDouble(ALenum param)
{
	AL_TRACE_CALL

	return (ALdouble)alGetFloat(param);
}

AL_API ALenum AL_APIENTRY alGetError(void)
{
	AL_TRACE_CALL

	return _alGetError();
}

AL_API ALboolean AL_APIENTRY alIsExtensionPresent(const ALchar* extname)
{
	const ALCchar *ptr = NULL;
	size_t len = 0;
	Context *ctx = (Context *)alcGetCurrentContext();

	AL_TRACE_CALL

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return AL_FALSE;
	}

	if (!extname)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return AL_FALSE;
	}

	ptr = ctx->getDevice()->getExtensionList();
	len = strlen(extname);

	while (ptr && *ptr)
	{
		if (strncasecmp(ptr, extname, len) == 0 && (ptr[len] == '\0' || isspace(ptr[len])))
			return AL_TRUE;

		if ((ptr = strchr(ptr, ' ')) != NULL)
		{
			do {
				++ptr;
			} while (isspace(*ptr));
		}
	}

	return AL_FALSE;
}

AL_API void* AL_APIENTRY alGetProcAddress(const ALchar* fname)
{
	AL_TRACE_CALL

	if (!fname)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return NULL;
	}

	return _alGetProcAddress(fname);
}

AL_API ALenum AL_APIENTRY alGetEnumValue(const ALchar* ename)
{
	AL_TRACE_CALL

	if (!ename)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return 0;
	}

	return _alGetEnumValue(ename);
}

AL_API void AL_APIENTRY alDopplerFactor(ALfloat value)
{
	ALint ret = AL_NO_ERROR;
	Context *ctx = (Context *)alcGetCurrentContext();

	AL_TRACE_CALL

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	if (value < 0.0f)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	ctx->beginParamUpdate();
	ret = ctx->m_panner.setDopplerFactor(value);
	ctx->endParamUpdate();
	if (ret != AL_NO_ERROR)
	{
		AL_SET_ERROR(ret);
		return;
	}
}

AL_API void AL_APIENTRY alDopplerVelocity(ALfloat value)
{
	Context *ctx = (Context *)alcGetCurrentContext();

	AL_TRACE_CALL

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	if (value < 0.0f)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	AL_WARNING("alDopplerVelocity is not supported, use alSpeedOfSound instead\n");
}

AL_API void AL_APIENTRY alSpeedOfSound(ALfloat value)
{
	ALint ret = AL_NO_ERROR;
	Context *ctx = (Context *)alcGetCurrentContext();

	AL_TRACE_CALL

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	if (value < 0.0f)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	ctx->beginParamUpdate();
	ret = ctx->m_panner.setSpeedOfSound(value);
	ctx->endParamUpdate();
	if (ret != AL_NO_ERROR)
	{
		AL_SET_ERROR(ret);
		return;
	}
}

AL_API void AL_APIENTRY alDistanceModel(ALenum distanceModel)
{
	ALint ret = AL_NO_ERROR;
	Context *ctx = (Context *)alcGetCurrentContext();

	AL_TRACE_CALL

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	ctx->beginParamUpdate();
	ret = ctx->m_panner.setDistanceModel(distanceModel);
	ctx->endParamUpdate();
	if (ret != AL_NO_ERROR)
	{
		AL_SET_ERROR(ret);
		return;
	}
}

/*
*
* OpenAL-Soft
*
*/

AL_API void AL_APIENTRY alDeferUpdatesSOFT(void)
{
	ALint ret = AL_NO_ERROR;
	Context *ctx = (Context *)alcGetCurrentContext();

	AL_TRACE_CALL

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	ret = ctx->suspend();
	if (ret != AL_NO_ERROR)
	{
		AL_SET_ERROR(ret);
		return;
	}
}

AL_API void AL_APIENTRY alProcessUpdatesSOFT(void)
{
	ALint ret = AL_NO_ERROR;
	Context *ctx = (Context *)alcGetCurrentContext();

	AL_TRACE_CALL

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	ret = ctx->resume();
	if (ret != AL_NO_ERROR)
	{
		AL_SET_ERROR(ret);
		return;
	}
}