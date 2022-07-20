#include <heatwave.h>
#include <string.h>
#include <ctype.h>

#include "common.h"
#include "context.h"
#include "device.h"

using namespace al;

AL_API void AL_APIENTRY alEnable(ALenum capability)
{
	Context *ctx = (Context *)alcGetCurrentContext();

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
	if (data == NULL)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	*data = alGetBoolean(param);
}

AL_API void AL_APIENTRY alGetIntegerv(ALenum param, ALint* data)
{
	if (data == NULL)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	*data = alGetInteger(param);
}

AL_API void AL_APIENTRY alGetFloatv(ALenum param, ALfloat* data)
{
	if (data == NULL)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	*data = alGetFloat(param);
}

AL_API void AL_APIENTRY alGetDoublev(ALenum param, ALdouble* data)
{
	if (data == NULL)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	*data = alGetDouble(param);
}

AL_API ALboolean AL_APIENTRY alGetBoolean(ALenum param)
{
	ALfloat res = 0.0f;
	ALint ires = 0;
	ALboolean value = AL_FALSE;
	Context *ctx = (Context *)alcGetCurrentContext();

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return value;
	}

	switch (param)
	{
	case AL_DOPPLER_FACTOR:
		sceHeatWaveGetDopplerFactor(&res);
		value = (res != 0.0f);
		break;
	case AL_DISTANCE_MODEL:
		sceHeatWaveGetDistanceModel(&ires);
		if (ires != SCE_HEATWAVE_API_DISTANCE_MODEL_DISABLE)
		{
			value = AL_TRUE;
		}
		else
		{
			value = AL_FALSE;
		}
		break;
	case AL_SPEED_OF_SOUND:
		sceHeatWaveGetSpeedOfSound(&res);
		value = (res != 0.0f);
		break;
	default:
		AL_SET_ERROR(AL_INVALID_ENUM);
		break;
	}

	return value;
}

AL_API ALint AL_APIENTRY alGetInteger(ALenum param)
{
	return (ALint)alGetFloat(param);
}

AL_API ALfloat AL_APIENTRY alGetFloat(ALenum param)
{
	ALfloat res = 0.0f;
	ALint ires = 0;
	ALfloat value = 0.0f;
	Context *ctx = (Context *)alcGetCurrentContext();

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return value;
	}

	switch (param)
	{
	case AL_DOPPLER_FACTOR:
		sceHeatWaveGetDopplerFactor(&res);
		value = res;
		break;
	case AL_DISTANCE_MODEL:
		sceHeatWaveGetDistanceModel(&ires);
		value = (ALfloat)ires;
		break;
	case AL_SPEED_OF_SOUND:
		sceHeatWaveGetSpeedOfSound(&res);
		value = res;
		break;
	default:
		AL_SET_ERROR(AL_INVALID_ENUM);
		break;
	}

	return value;
}

AL_API ALdouble AL_APIENTRY alGetDouble(ALenum param)
{
	return (ALdouble)alGetFloat(param);
}

AL_API ALenum AL_APIENTRY alGetError(void)
{
	return _alGetError();
}

AL_API ALboolean AL_APIENTRY alIsExtensionPresent(const ALchar* extname)
{
	const ALCchar *ptr = NULL;
	size_t len = 0;
	Context *ctx = (Context *)alcGetCurrentContext();

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
	if (!fname)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return NULL;
	}

	return _alGetProcAddress(fname);
}

AL_API ALenum AL_APIENTRY alGetEnumValue(const ALchar* ename)
{
	if (!ename)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return 0;
	}

	return _alGetEnumValue(ename);
}

AL_API void AL_APIENTRY alDopplerFactor(ALfloat value)
{
	Context *ctx = (Context *)alcGetCurrentContext();

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

	sceHeatWaveSetDopplerFactor(value);
}

AL_API void AL_APIENTRY alDopplerVelocity(ALfloat value)
{
	Context *ctx = (Context *)alcGetCurrentContext();

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
	Context *ctx = (Context *)alcGetCurrentContext();

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

	sceHeatWaveSetSpeedOfSound(value);
}

AL_API void AL_APIENTRY alDistanceModel(ALenum distanceModel)
{
	ALint model = AL_NONE;

	if (alcGetCurrentContext() == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	switch (distanceModel) {
	case AL_NONE:
		model = SCE_HEATWAVE_API_DISTANCE_MODEL_DISABLE;
		break;
	case AL_INVERSE_DISTANCE:
		model = SCE_HEATWAVE_API_DISTANCE_MODEL_INVERSE;
		break;
	case AL_INVERSE_DISTANCE_CLAMPED:
		model = SCE_HEATWAVE_API_DISTANCE_MODEL_INVERSE_CLAMPED;
		break;
	case AL_LINEAR_DISTANCE:
		model = SCE_HEATWAVE_API_DISTANCE_MODEL_LINEAR;
		break;
	case AL_LINEAR_DISTANCE_CLAMPED:
		model = SCE_HEATWAVE_API_DISTANCE_MODEL_LINEAR_CLAMPED;
		break;
	case AL_EXPONENT_DISTANCE:
		model = SCE_HEATWAVE_API_DISTANCE_MODEL_EXPONENT;
		break;
	case AL_EXPONENT_DISTANCE_CLAMPED:
		model = SCE_HEATWAVE_API_DISTANCE_MODEL_EXPONENT_CLAMPED;
		break;
	default:
		AL_SET_ERROR(AL_INVALID_ENUM);
		break;
	}

	sceHeatWaveSetDistanceModel(model);
}