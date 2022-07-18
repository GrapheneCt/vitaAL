#include <heatwave.h>
#include <string.h>

#include "common.h"
#include "context.h"

using namespace al;

AL_API void AL_APIENTRY alListenerf(ALenum param, ALfloat value)
{
	ALint ret = AL_NO_ERROR;
	Context *ctx = (Context *)alcGetCurrentContext();

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	switch (param)
	{
	case AL_GAIN:
		ret = _alErrorHw2Al(sceHeatWaveSetListener(ctx->listenerPosition, ctx->listenerVelocity, ctx->listenerForward, ctx->listenerUp, value));
		if (ret != AL_NO_ERROR)
		{
			AL_SET_ERROR(ret);
			return;
		}
		ctx->listenerGain = value;
		break;
	default:
		AL_SET_ERROR(AL_INVALID_ENUM);
		break;
	}
}

AL_API void AL_APIENTRY alListener3f(ALenum param, ALfloat value1, ALfloat value2, ALfloat value3)
{
	ALint ret = AL_NO_ERROR;
	SceHwVector value;
	Context *ctx = (Context *)alcGetCurrentContext();

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	switch (param)
	{
	case AL_POSITION:
		value.fX = value1;
		value.fY = value2;
		value.fZ = value3;
		ret = _alErrorHw2Al(sceHeatWaveSetListener(value, ctx->listenerVelocity, ctx->listenerForward, ctx->listenerUp, ctx->listenerGain));
		if (ret != AL_NO_ERROR)
		{
			AL_SET_ERROR(ret);
			return;
		}
		ctx->listenerPosition = value;
		break;
	case AL_VELOCITY:
		value.fX = value1;
		value.fY = value2;
		value.fZ = value3;
		ret = _alErrorHw2Al(sceHeatWaveSetListener(ctx->listenerPosition, value, ctx->listenerForward, ctx->listenerUp, ctx->listenerGain));
		if (ret != AL_NO_ERROR)
		{
			AL_SET_ERROR(ret);
			return;
		}
		ctx->listenerVelocity = value;
		break;
	default:
		AL_SET_ERROR(AL_INVALID_ENUM);
		break;
	}
}

AL_API void AL_APIENTRY alListenerfv(ALenum param, const ALfloat* values)
{
	ALint ret = AL_NO_ERROR;
	SceHwVector value1;
	SceHwVector value2;
	Context *ctx = (Context *)alcGetCurrentContext();

	if (values == NULL)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	switch (param)
	{
	case AL_GAIN:
		alListenerf(param, values[0]);
		return;
	case AL_POSITION:
	case AL_VELOCITY:
		alListener3f(param, values[0], values[1], values[2]);
		return;
	}

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	switch (param)
	{
	case AL_ORIENTATION:
		value1.fX = values[0];
		value1.fY = values[1];
		value1.fZ = values[2];
		value2.fX = values[3];
		value2.fY = values[4];
		value2.fZ = values[5];
		ret = _alErrorHw2Al(sceHeatWaveSetListener(ctx->listenerPosition, ctx->listenerVelocity, value1, value2, ctx->listenerGain));
		if (ret != AL_NO_ERROR)
		{
			AL_SET_ERROR(ret);
			return;
		}
		ctx->listenerForward = value1;
		ctx->listenerUp = value2;
		break;
	default:
		AL_SET_ERROR(AL_INVALID_ENUM);
		break;
	}
}

AL_API void AL_APIENTRY alListeneri(ALenum param, ALint value)
{
	alListenerf(param, (ALfloat)value);
}

AL_API void AL_APIENTRY alListener3i(ALenum param, ALint value1, ALint value2, ALint value3)
{
	alListener3f(param, (ALfloat)value1, (ALfloat)value2, (ALfloat)value3);
}

AL_API void AL_APIENTRY alListeneriv(ALenum param, const ALint* values)
{
	ALfloat fvalues[6];

	if (values == NULL)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	switch (param)
	{
	case AL_GAIN:
		fvalues[0] = (ALfloat)values[0];
		break;
	case AL_POSITION:
	case AL_VELOCITY:
		fvalues[0] = (ALfloat)values[0];
		fvalues[1] = (ALfloat)values[1];
		fvalues[2] = (ALfloat)values[2];
		break;
	case AL_ORIENTATION:
		fvalues[0] = (ALfloat)values[0];
		fvalues[1] = (ALfloat)values[1];
		fvalues[2] = (ALfloat)values[2];
		fvalues[3] = (ALfloat)values[3];
		fvalues[4] = (ALfloat)values[4];
		fvalues[5] = (ALfloat)values[5];
		break;
	}

	alListenerfv(param, fvalues);
}

AL_API void AL_APIENTRY alGetListenerf(ALenum param, ALfloat* value)
{
	Context *ctx = (Context *)alcGetCurrentContext();

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	if (value == NULL)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	switch (param)
	{
	case AL_GAIN:
		*value = ctx->listenerGain;
		break;
	default:
		AL_SET_ERROR(AL_INVALID_ENUM);
		break;
	}
}

AL_API void AL_APIENTRY alGetListener3f(ALenum param, ALfloat *value1, ALfloat *value2, ALfloat *value3)
{
	Context *ctx = (Context *)alcGetCurrentContext();

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	if (value1 == NULL || value2 == NULL || value3 == NULL)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	switch (param)
	{
	case AL_POSITION:
		*value1 = ctx->listenerPosition.fX;
		*value2 = ctx->listenerPosition.fY;
		*value3 = ctx->listenerPosition.fZ;
		break;
	case AL_VELOCITY:
		*value1 = ctx->listenerVelocity.fX;
		*value2 = ctx->listenerVelocity.fY;
		*value3 = ctx->listenerVelocity.fZ;
		break;
	default:
		AL_SET_ERROR(AL_INVALID_ENUM);
		break;
	}
}

AL_API void AL_APIENTRY alGetListenerfv(ALenum param, ALfloat* values)
{
	Context *ctx = (Context *)alcGetCurrentContext();

	if (values == NULL)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	switch (param)
	{
	case AL_GAIN:
		alGetListenerf(param, values);
		break;
	case AL_POSITION:
	case AL_VELOCITY:
		alGetListener3f(param, &values[0], &values[1], &values[2]);
		return;
	}

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	switch (param)
	{
	case AL_ORIENTATION:
		values[0] = ctx->listenerForward.fX;
		values[1] = ctx->listenerForward.fY;
		values[2] = ctx->listenerForward.fZ;
		values[3] = ctx->listenerUp.fX;
		values[4] = ctx->listenerUp.fY;
		values[5] = ctx->listenerUp.fZ;
		break;
	default:
		AL_SET_ERROR(AL_INVALID_ENUM);
		break;
	}
}

AL_API void AL_APIENTRY alGetListeneri(ALenum param, ALint* value)
{
	ALfloat ret = 0;

	if (value == NULL)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	alGetListenerf(param, &ret);

	*value = (ALint)ret;
}

AL_API void AL_APIENTRY alGetListener3i(ALenum param, ALint *value1, ALint *value2, ALint *value3)
{
	ALfloat ret[3];

	if (value1 == NULL || value2 == NULL || value3 == NULL)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	alGetListener3f(param, &ret[0], &ret[1], &ret[2]);

	*value1 = (ALint)ret[0];
	*value2 = (ALint)ret[1];
	*value3 = (ALint)ret[2];
}

AL_API void AL_APIENTRY alGetListeneriv(ALenum param, ALint* values)
{
	ALfloat fret[6];

	if (values == NULL)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	alGetListenerfv(param, fret);

	switch (param)
	{
	case AL_GAIN:
		values[0] = (ALint)fret[0];
		break;
	case AL_POSITION:
	case AL_VELOCITY:
		values[0] = (ALint)fret[0];
		values[1] = (ALint)fret[1];
		values[2] = (ALint)fret[2];
		break;
	case AL_ORIENTATION:
		values[0] = (ALint)fret[0];
		values[1] = (ALint)fret[1];
		values[2] = (ALint)fret[2];
		values[3] = (ALint)fret[3];
		values[4] = (ALint)fret[4];
		values[5] = (ALint)fret[5];
		break;
	}
}