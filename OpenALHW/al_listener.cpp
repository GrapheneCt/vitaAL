#include <kernel.h>
#include <ngs.h>
#include <string.h>

#include "common.h"
#include "context.h"

using namespace al;

AL_API void AL_APIENTRY alListenerf(ALenum param, ALfloat value)
{
	ALint ret = AL_NO_ERROR;
	Context *ctx = (Context *)alcGetCurrentContext();

	AL_TRACE_CALL

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	switch (param)
	{
	case AL_GAIN:
		ctx->beginParamUpdate();
		ret = ctx->m_panner.setListenerGain(value);
		ctx->endParamUpdate();
		if (ret != AL_NO_ERROR)
		{
			AL_SET_ERROR(ret);
			return;
		}
		break;
	default:
		AL_SET_ERROR(AL_INVALID_ENUM);
		break;
	}
}

AL_API void AL_APIENTRY alListener3f(ALenum param, ALfloat value1, ALfloat value2, ALfloat value3)
{
	ALint ret = AL_NO_ERROR;
	SceFVector4 value;
	Context *ctx = (Context *)alcGetCurrentContext();

	AL_TRACE_CALL

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	switch (param)
	{
	case AL_POSITION:
		value.x = value1;
		value.y = value2;
		value.z = value3;
		ctx->beginParamUpdate();
		ret = ctx->m_panner.setListenerPosition(value);
		ctx->endParamUpdate();
		if (ret != AL_NO_ERROR)
		{
			AL_SET_ERROR(ret);
			return;
		}
		break;
	case AL_VELOCITY:
		value.x = value1;
		value.y = value2;
		value.z = value3;
		ctx->beginParamUpdate();
		ret = ctx->m_panner.setListenerVelocity(value);
		ctx->endParamUpdate();
		if (ret != AL_NO_ERROR)
		{
			AL_SET_ERROR(ret);
			return;
		}
		break;
	default:
		AL_SET_ERROR(AL_INVALID_ENUM);
		break;
	}
}

AL_API void AL_APIENTRY alListenerfv(ALenum param, const ALfloat* values)
{
	ALint ret = AL_NO_ERROR;
	SceFVector4 value1;
	SceFVector4 value2;
	Context *ctx = (Context *)alcGetCurrentContext();

	AL_TRACE_CALL

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
		value1.x = values[0];
		value1.y = values[1];
		value1.z = values[2];
		value2.x = values[3];
		value2.y = values[4];
		value2.z = values[5];
		ctx->beginParamUpdate();
		ret = ctx->m_panner.setListenerOrientation(value1, value2);
		ctx->endParamUpdate();
		if (ret != AL_NO_ERROR)
		{
			AL_SET_ERROR(ret);
			return;
		}
		break;
	default:
		AL_SET_ERROR(AL_INVALID_ENUM);
		break;
	}
}

AL_API void AL_APIENTRY alListeneri(ALenum param, ALint value)
{
	AL_TRACE_CALL

	alListenerf(param, (ALfloat)value);
}

AL_API void AL_APIENTRY alListener3i(ALenum param, ALint value1, ALint value2, ALint value3)
{
	AL_TRACE_CALL

	alListener3f(param, (ALfloat)value1, (ALfloat)value2, (ALfloat)value3);
}

AL_API void AL_APIENTRY alListeneriv(ALenum param, const ALint* values)
{
	ALfloat fvalues[6];

	AL_TRACE_CALL

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

	AL_TRACE_CALL

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
		*value = ctx->m_panner.m_gain;
		break;
	default:
		AL_SET_ERROR(AL_INVALID_ENUM);
		break;
	}
}

AL_API void AL_APIENTRY alGetListener3f(ALenum param, ALfloat *value1, ALfloat *value2, ALfloat *value3)
{
	Context *ctx = (Context *)alcGetCurrentContext();

	AL_TRACE_CALL

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
		*value1 = ctx->m_panner.m_position.x;
		*value2 = ctx->m_panner.m_position.y;
		*value3 = ctx->m_panner.m_position.z;
		break;
	case AL_VELOCITY:
		*value1 = ctx->m_panner.m_velocity.x;
		*value2 = ctx->m_panner.m_velocity.y;
		*value3 = ctx->m_panner.m_velocity.z;
		break;
	default:
		AL_SET_ERROR(AL_INVALID_ENUM);
		break;
	}
}

AL_API void AL_APIENTRY alGetListenerfv(ALenum param, ALfloat* values)
{
	Context *ctx = (Context *)alcGetCurrentContext();

	AL_TRACE_CALL

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
		values[0] = ctx->m_panner.m_forward.x;
		values[1] = ctx->m_panner.m_forward.y;
		values[2] = ctx->m_panner.m_forward.z;
		values[3] = ctx->m_panner.m_up.x;
		values[4] = ctx->m_panner.m_up.y;
		values[5] = ctx->m_panner.m_up.z;
		break;
	default:
		AL_SET_ERROR(AL_INVALID_ENUM);
		break;
	}
}

AL_API void AL_APIENTRY alGetListeneri(ALenum param, ALint* value)
{
	ALfloat ret = 0;

	AL_TRACE_CALL

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

	AL_TRACE_CALL

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

	AL_TRACE_CALL

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