#include <heatwave.h>
#include <string.h>
#include <float.h>

#include "common.h"
#include "context.h"
#include "named_object.h"

using namespace al;

ALboolean Source::validate(Source *src)
{
	if (src == NULL)
	{
		return AL_FALSE;
	}

	if (src->getType() != ObjectType_Source)
	{
		return AL_FALSE;
	}

	if (src->isInitialized() != AL_TRUE)
	{
		return AL_FALSE;
	}

	return AL_TRUE;
}

Source::Source()
	: hwSrc(AL_INVALID_HW_HANDLE),
	hwSnd(AL_INVALID_HW_HANDLE),
	hwSfx(AL_INVALID_HW_HANDLE),
	minDistance(1.0f),
	maxDistance(FLT_MAX),
	roloffFactor(1.0f),
	minGain(kfSfxVolumeMin),
	maxGain(kfSfxVolumeMax),
	coneOuterGain(1.0f),
	coneInnerAngle(0.0f),
	coneOuterAngle(360.0f),
	coneOuterFreq(1.0f),
	offsetSec(0.0f),
	offsetSample(0.0f),
	offsetByte(0.0f),
	positionRelative(AL_FALSE),
	type(AL_UNDETERMINED),
	staticBuffer(NULL),
	looping(AL_FALSE),
	needOffsetsReset(AL_FALSE)
{
	m_type = ObjectType_Source;
}

Source::~Source()
{

}

ALint Source::init()
{
	SceHwError ret = SCE_HEATWAVE_API_OK;

	ret = sceHeatWaveCreateSound(&hwSnd);
	if (SCE_HEATWAVE_API_FAILED(ret))
	{
		goto error;
	}

	ret = sceHeatWaveSoundCreateSource(hwSnd, &hwSrc);
	if (SCE_HEATWAVE_API_FAILED(ret))
	{
		goto error;
	}

	ret = sceHeatWaveSoundCreateSfx(hwSnd, &hwSfx);
	if (SCE_HEATWAVE_API_FAILED(ret))
	{
		goto error;
	}

	m_initialized = AL_TRUE;

	return AL_NO_ERROR;

error:

	return _alErrorHw2Al(ret);
}

ALint Source::release()
{
	SceHwError ret = SCE_HEATWAVE_API_OK;

	ret = sceHeatWaveSfxStop(hwSfx);
	if (SCE_HEATWAVE_API_FAILED(ret))
	{
		goto error;
	}

	ret = sceHeatWaveReleaseSfx(hwSfx);
	if (SCE_HEATWAVE_API_FAILED(ret))
	{
		goto error;
	}

	ret = sceHeatWaveReleaseSource(hwSrc);
	if (SCE_HEATWAVE_API_FAILED(ret))
	{
		goto error;
	}

	ret = sceHeatWaveReleaseSound(hwSnd);
	if (SCE_HEATWAVE_API_FAILED(ret))
	{
		goto error;
	}

	m_initialized = AL_FALSE;

	return AL_NO_ERROR;

error:

	return _alErrorHw2Al(ret);
}

AL_API void AL_APIENTRY alGenSources(ALsizei n, ALuint* sources)
{
	Source **ppSrc = NULL;
	Source *pSrc = NULL;
	ALint ret = AL_NO_ERROR;
	Context *ctx = (Context *)alcGetCurrentContext();

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	if (sources == NULL)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	for (int i = 0; i < n; i++)
	{
		pSrc = new Source();

		ret = pSrc->init();
		if (ret != AL_NO_ERROR)
		{
			AL_SET_ERROR(ret);
			return;
		}

		ppSrc = (Source **)&sources[i];
		*ppSrc = pSrc;
	}
}

AL_API void AL_APIENTRY alDeleteSources(ALsizei n, const ALuint* sources)
{
	Source *pSrc = NULL;
	ALint ret = AL_NO_ERROR;
	Context *ctx = (Context *)alcGetCurrentContext();

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	if (sources == NULL)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	for (int i = 0; i < n; i++)
	{
		pSrc = (Source *)sources[i];

		if (!Source::validate(pSrc))
		{
			AL_SET_ERROR(AL_INVALID_NAME);
			return;
		}

		ret = pSrc->release();
		if (ret != AL_NO_ERROR)
		{
			AL_SET_ERROR(ret);
			return;
		}

		delete pSrc;
	}
}

AL_API ALboolean AL_APIENTRY alIsSource(ALuint sid)
{
	Context *ctx = (Context *)alcGetCurrentContext();

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return AL_FALSE;
	}

	if (Source::validate((Source *)sid))
	{
		return AL_TRUE;
	}

	return AL_FALSE;
}

AL_API void AL_APIENTRY alSourcef(ALuint sid, ALenum param, ALfloat value)
{
	ALint ret = AL_NO_ERROR;
	Source *src = NULL;
	Context *ctx = (Context *)alcGetCurrentContext();

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	src = (Source *)sid;

	if (!Source::validate(src))
	{
		AL_SET_ERROR(AL_INVALID_NAME);
		return;
	}

	switch (param)
	{
	case AL_PITCH:
		ret = _alErrorHw2Al(sceHeatWaveSfxSetPitchMultiplier(src->hwSfx, value));
		if (ret != AL_NO_ERROR)
		{
			AL_SET_ERROR(ret);
			return;
		}
		break;
	case AL_GAIN:
		if (value < src->minGain)
		{
			value = src->minGain;
		}
		if (value > src->maxGain)
		{
			value = src->maxGain;
		}
		ret = _alErrorHw2Al(sceHeatWaveSfxSetVolume(src->hwSfx, value));
		if (ret != AL_NO_ERROR)
		{
			AL_SET_ERROR(ret);
			return;
		}
		break;
	case AL_MAX_DISTANCE:
		ret = _alErrorHw2Al(sceHeatWaveSourceSetRolloffDistances(src->hwSrc, src->minDistance, value));
		if (ret != AL_NO_ERROR)
		{
			AL_SET_ERROR(ret);
			return;
		}
		src->maxDistance = value;
		break;
	case AL_ROLLOFF_FACTOR:
		ret = _alErrorHw2Al(sceHeatWaveSourceSetDistanceFactor(src->hwSrc, value));
		if (ret != AL_NO_ERROR)
		{
			AL_SET_ERROR(ret);
			return;
		}
		src->roloffFactor = value;
		break;
	case AL_REFERENCE_DISTANCE:
		ret = _alErrorHw2Al(sceHeatWaveSourceSetRolloffDistances(src->hwSrc, value, src->maxDistance));
		if (ret != AL_NO_ERROR)
		{
			AL_SET_ERROR(ret);
			return;
		}
		src->minDistance = value;
		break;
	case AL_MIN_GAIN:
		if (value < kfSfxVolumeMin || value > src->maxGain)
		{
			AL_SET_ERROR(AL_INVALID_VALUE);
			return;
		}
		src->minGain = value;
		break;
	case AL_MAX_GAIN:
		if (value > kfSfxVolumeMax || value < src->minGain)
		{
			AL_SET_ERROR(AL_INVALID_VALUE);
			return;
		}
		src->maxGain = value;
		break;
	case AL_CONE_OUTER_GAIN:
		ret = _alErrorHw2Al(sceHeatWaveSourceSetCone(src->hwSrc, src->coneInnerAngle, src->coneOuterAngle, value, src->coneOuterFreq));
		if (ret != AL_NO_ERROR)
		{
			AL_SET_ERROR(ret);
			return;
		}
		src->coneOuterGain = value;
		break;
	case AL_CONE_INNER_ANGLE:
		ret = _alErrorHw2Al(sceHeatWaveSourceSetCone(src->hwSrc, value, src->coneOuterAngle, src->coneOuterGain, src->coneOuterFreq));
		if (ret != AL_NO_ERROR)
		{
			AL_SET_ERROR(ret);
			return;
		}
		src->coneInnerAngle = value;
		break;
	case AL_CONE_OUTER_ANGLE:
		ret = _alErrorHw2Al(sceHeatWaveSourceSetCone(src->hwSrc, src->coneInnerAngle, value, src->coneOuterGain, src->coneOuterFreq));
		if (ret != AL_NO_ERROR)
		{
			AL_SET_ERROR(ret);
			return;
		}
		src->coneOuterAngle = value;
		break;
	case AL_SEC_OFFSET:
		if (value < 0.0f)
		{
			AL_SET_ERROR(AL_INVALID_VALUE);
			return;
		}
		sceHeatWaveSfxPause(src->hwSfx);
		src->offsetSec = value;
		sceHeatWaveSfxResume(src->hwSfx);
		break;
	case AL_SAMPLE_OFFSET:
		if (value < 0.0f)
		{
			AL_SET_ERROR(AL_INVALID_VALUE);
			return;
		}
		sceHeatWaveSfxPause(src->hwSfx);
		src->offsetSample = value;
		sceHeatWaveSfxResume(src->hwSfx);
		break;
	case AL_BYTE_OFFSET:
		if (value < 0.0f)
		{
			AL_SET_ERROR(AL_INVALID_VALUE);
			return;
		}
		sceHeatWaveSfxPause(src->hwSfx);
		src->offsetByte = value;
		sceHeatWaveSfxResume(src->hwSfx);
		break;
	default:
		AL_SET_ERROR(AL_INVALID_ENUM);
		break;
	}
}

AL_API void AL_APIENTRY alSource3f(ALuint sid, ALenum param, ALfloat value1, ALfloat value2, ALfloat value3)
{
	SceHwVector value;
	ALint ret = AL_NO_ERROR;
	Source *src = NULL;
	Context *ctx = (Context *)alcGetCurrentContext();

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	src = (Source *)sid;

	if (!Source::validate(src))
	{
		AL_SET_ERROR(AL_INVALID_NAME);
		return;
	}

	switch (param)
	{
	case AL_POSITION:
		if (src->positionRelative == AL_TRUE)
		{
			value.fX = ctx->listenerPosition.fX + value1;
			value.fY = ctx->listenerPosition.fY + value2;
			value.fZ = ctx->listenerPosition.fZ + value3;
		}
		else
		{
			value.fX = value1;
			value.fY = value2;
			value.fZ = value3;
		}
		ret = _alErrorHw2Al(sceHeatWaveSfxSetPosition(src->hwSfx, value));
		if (ret != AL_NO_ERROR)
		{
			AL_SET_ERROR(ret);
			return;
		}
		src->position = value;
		break;
	case AL_VELOCITY:
		if (src->positionRelative == AL_TRUE)
		{
			value.fX = ctx->listenerVelocity.fX + value1;
			value.fY = ctx->listenerVelocity.fY + value2;
			value.fZ = ctx->listenerVelocity.fZ + value3;
		}
		else
		{
			value.fX = value1;
			value.fY = value2;
			value.fZ = value3;
		}
		ret = _alErrorHw2Al(sceHeatWaveSfxSetVelocity(src->hwSfx, value));
		if (ret != AL_NO_ERROR)
		{
			AL_SET_ERROR(ret);
			return;
		}
		src->velocity = value;
		break;
	case AL_DIRECTION:
		if (src->positionRelative == AL_TRUE)
		{
			value.fX = ctx->listenerForward.fX + value1;
			value.fY = ctx->listenerForward.fY + value2;
			value.fZ = ctx->listenerForward.fZ + value3;
		}
		else
		{
			value.fX = value1;
			value.fY = value2;
			value.fZ = value3;
		}
		ret = _alErrorHw2Al(sceHeatWaveSfxSetForward(src->hwSfx, value));
		if (ret != AL_NO_ERROR)
		{
			AL_SET_ERROR(ret);
			return;
		}
		src->direction = value;
		break;
	default:
		AL_SET_ERROR(AL_INVALID_ENUM);
		break;
	}
}

AL_API void AL_APIENTRY alSourcefv(ALuint sid, ALenum param, const ALfloat* values)
{
	if (values == NULL)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	switch (param)
	{
	case AL_PITCH:
	case AL_GAIN:
	case AL_MAX_DISTANCE:
	case AL_ROLLOFF_FACTOR:
	case AL_REFERENCE_DISTANCE:
	case AL_MIN_GAIN:
	case AL_MAX_GAIN:
	case AL_CONE_OUTER_GAIN:
	case AL_CONE_INNER_ANGLE:
	case AL_CONE_OUTER_ANGLE:
	case AL_SEC_OFFSET:
	case AL_SAMPLE_OFFSET:
	case AL_BYTE_OFFSET:
		alSourcef(sid, param, values[0]);
		break;
	case AL_POSITION:
	case AL_VELOCITY:
	case AL_DIRECTION:
		alSource3f(sid, param, values[0], values[1], values[2]);
		break;
	default:
		AL_SET_ERROR(AL_INVALID_ENUM);
		break;
	}
}

AL_API void AL_APIENTRY alSourcei(ALuint sid, ALenum param, ALint value)
{
	ALint ret = AL_NO_ERROR;
	Source *src = NULL;
	Buffer *buf = NULL;
	SceHwSfxState state;
	Context *ctx = (Context *)alcGetCurrentContext();

	switch (param)
	{
	case AL_MAX_DISTANCE:
	case AL_ROLLOFF_FACTOR:
	case AL_REFERENCE_DISTANCE:
	case AL_CONE_INNER_ANGLE:
	case AL_CONE_OUTER_ANGLE:
	case AL_SEC_OFFSET:
	case AL_SAMPLE_OFFSET:
	case AL_BYTE_OFFSET:
		alSourcef(sid, param, (ALfloat)value);
		return;
	default:
		break;
	}

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	src = (Source *)sid;

	if (!Source::validate(src))
	{
		AL_SET_ERROR(AL_INVALID_NAME);
		return;
	}

	switch (param)
	{
	case AL_SOURCE_RELATIVE:
		src->positionRelative = value;
		break;
	case AL_LOOPING:
		src->looping = value;
		break;
	case AL_BUFFER:
		if (value == 0)
		{
			src->dropAllBuffers();
			return;
		}
		buf = (Buffer *)value;
		if (!Buffer::validate(buf))
		{
			AL_SET_ERROR(AL_INVALID_VALUE);
			return;
		}
		src->dropStreamBuffers();
		src->setStaticBuffer(buf);
		break;
		/*
	case AL_SOURCE_STATE:
		switch (value)
		{
		case AL_INITIAL:
		case AL_STOPPED:
			sceHeatWaveSfxStop(src->hwSfx);
			break;
		case AL_PAUSED:
			sceHeatWaveSfxPause(src->hwSfx);
			break;
		case AL_PLAYING:
			ret = _alErrorHw2Al(sceHeatWaveSfxGetState(src->hwSfx, &state));
			if (ret != AL_NO_ERROR)
			{
				AL_SET_ERROR(ret);
				return;
			}
			if (state == kSfxState_Paused)
			{
				sceHeatWaveSfxResume(src->hwSfx);
			}
			else
			{
				sceHeatWaveSfxPlay(src->hwSfx, NULL);
			}
			break;
		default:
			AL_SET_ERROR(AL_INVALID_VALUE);
			break;
		}
		break;
	case AL_BUFFERS_QUEUED:
		n;
		break;
	case AL_BUFFERS_PROCESSED:
		n;
		break;
		*/
	default:
		AL_SET_ERROR(AL_INVALID_ENUM);
		break;
	}
}

AL_API void AL_APIENTRY alSource3i(ALuint sid, ALenum param, ALint value1, ALint value2, ALint value3)
{
	switch (param)
	{
	case AL_DIRECTION:
		alSource3f(sid, param, (ALfloat)value1, (ALfloat)value2, (ALfloat)value3);
		break;
	default:
		AL_SET_ERROR(AL_INVALID_ENUM);
		break;
	}
}

AL_API void AL_APIENTRY alSourceiv(ALuint sid, ALenum param, const ALint* values)
{
	if (values == NULL)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	switch (param)
	{
	case AL_MAX_DISTANCE:
	case AL_ROLLOFF_FACTOR:
	case AL_REFERENCE_DISTANCE:
	case AL_CONE_INNER_ANGLE:
	case AL_CONE_OUTER_ANGLE:
	case AL_SOURCE_RELATIVE:
	case AL_LOOPING:
	case AL_BUFFER:
	case AL_SEC_OFFSET:
	case AL_SAMPLE_OFFSET:
	case AL_BYTE_OFFSET:
		alSourcei(sid, param, values[0]);
		break;
	case AL_DIRECTION:
		alSource3i(sid, param, values[0], values[1], values[2]);
		break;
	default:
		AL_SET_ERROR(AL_INVALID_ENUM);
		break;
	}
}

AL_API void AL_APIENTRY alGetSourcef(ALuint sid, ALenum param, ALfloat* value)
{
	ALint ret = AL_NO_ERROR;
	Source *src = NULL;
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

	src = (Source *)sid;

	if (!Source::validate(src))
	{
		AL_SET_ERROR(AL_INVALID_NAME);
		return;
	}

	switch (param)
	{
	case AL_PITCH:
		ret = _alErrorHw2Al(sceHeatWaveSfxGetPitchMultiplier(src->hwSfx, value));
		if (ret != AL_NO_ERROR)
		{
			AL_SET_ERROR(ret);
			return;
		}
		break;
	case AL_GAIN:
		ret = _alErrorHw2Al(sceHeatWaveSfxGetVolume(src->hwSfx, value));
		if (ret != AL_NO_ERROR)
		{
			AL_SET_ERROR(ret);
			return;
		}
		break;
	case AL_MAX_DISTANCE:
		*value = src->maxDistance;
		break;
	case AL_ROLLOFF_FACTOR:
		*value = src->roloffFactor;
		break;
	case AL_REFERENCE_DISTANCE:
		*value = src->minDistance;
		break;
	case AL_MIN_GAIN:
		*value = src->minGain;
		break;
	case AL_MAX_GAIN:
		*value = src->maxGain;
		break;
	case AL_CONE_OUTER_GAIN:
		*value = src->coneOuterGain;
		break;
	case AL_CONE_INNER_ANGLE:
		*value = src->coneInnerAngle;
		break;
	case AL_CONE_OUTER_ANGLE:
		*value = src->coneOuterAngle;
		break;
	case AL_SEC_OFFSET:
		*value = src->offsetSec;
		break;
	case AL_SAMPLE_OFFSET:
		*value = src->offsetSample;
		break;
	case AL_BYTE_OFFSET:
		*value = src->offsetByte;
		break;
	default:
		AL_SET_ERROR(AL_INVALID_ENUM);
		break;
	}
}

AL_API void AL_APIENTRY alGetSource3f(ALuint sid, ALenum param, ALfloat* value1, ALfloat* value2, ALfloat* value3)
{
	ALint ret = AL_NO_ERROR;
	Source *src = NULL;
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

	src = (Source *)sid;

	if (!Source::validate(src))
	{
		AL_SET_ERROR(AL_INVALID_NAME);
		return;
	}

	switch (param)
	{
	case AL_POSITION:
		*value1 = src->position.fX;
		*value2 = src->position.fY;
		*value3 = src->position.fZ;
		break;
	case AL_VELOCITY:
		*value1 = src->velocity.fX;
		*value2 = src->velocity.fY;
		*value3 = src->velocity.fZ;
		break;
	case AL_DIRECTION:
		*value1 = src->direction.fX;
		*value2 = src->direction.fY;
		*value3 = src->direction.fZ;
		break;
	default:
		AL_SET_ERROR(AL_INVALID_ENUM);
		break;
	}
}

AL_API void AL_APIENTRY alGetSourcefv(ALuint sid, ALenum param, ALfloat* values)
{
	if (values == NULL)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	switch (param)
	{
	case AL_PITCH:
	case AL_GAIN:
	case AL_MAX_DISTANCE:
	case AL_ROLLOFF_FACTOR:
	case AL_REFERENCE_DISTANCE:
	case AL_MIN_GAIN:
	case AL_MAX_GAIN:
	case AL_CONE_OUTER_GAIN:
	case AL_CONE_INNER_ANGLE:
	case AL_CONE_OUTER_ANGLE:
	case AL_SEC_OFFSET:
	case AL_SAMPLE_OFFSET:
	case AL_BYTE_OFFSET:
		alGetSourcef(sid, param, values);
		break;
	case AL_POSITION:
	case AL_VELOCITY:
	case AL_DIRECTION:
		alGetSource3f(sid, param, &values[0], &values[1], &values[2]);
		break;
	default:
		AL_SET_ERROR(AL_INVALID_ENUM);
		break;
	}
}

AL_API void AL_APIENTRY alGetSourcei(ALuint sid, ALenum param, ALint* value)
{
	ALint ret = AL_NO_ERROR;
	ALfloat fret = 0.0f;
	Source *src = NULL;
	Buffer *buf = NULL;
	SceHwSfxState state;
	Context *ctx = (Context *)alcGetCurrentContext();

	switch (param)
	{
	case AL_MAX_DISTANCE:
	case AL_ROLLOFF_FACTOR:
	case AL_REFERENCE_DISTANCE:
	case AL_CONE_INNER_ANGLE:
	case AL_CONE_OUTER_ANGLE:
	case AL_SEC_OFFSET:
	case AL_SAMPLE_OFFSET:
	case AL_BYTE_OFFSET:
		alGetSourcef(sid, param, &fret);
		*value = (ALint)fret;
		return;
	default:
		break;
	}

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

	src = (Source *)sid;

	if (!Source::validate(src))
	{
		AL_SET_ERROR(AL_INVALID_NAME);
		return;
	}

	switch (param)
	{
	case AL_SOURCE_RELATIVE:
		*value = (ALint)src->positionRelative;
		break;
	case AL_SOURCE_TYPE:
		*value = src->type;
		break;
	case AL_LOOPING:
		*value = (ALint)src->looping;
		break;
	case AL_BUFFER:
		*value = (ALint)src->staticBuffer;
		break;
	case AL_SOURCE_STATE:
		ret = _alErrorHw2Al(sceHeatWaveSfxGetState(src->hwSfx, &state));
		if (ret != AL_NO_ERROR)
		{
			AL_SET_ERROR(ret);
			return;
		}
		*value = _alSourceStateHw2Al(state);
		break;
	case AL_BUFFERS_QUEUED:
		n;
		break;
	case AL_BUFFERS_PROCESSED:
		n;
		break;
	default:
		AL_SET_ERROR(AL_INVALID_ENUM);
		break;
	}
}

AL_API void AL_APIENTRY alGetSource3i(ALuint sid, ALenum param, ALint* value1, ALint* value2, ALint* value3)
{
	ALfloat ret[3];

	if (value1 == NULL || value2 == NULL || value3 == NULL)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	switch (param)
	{
	case AL_DIRECTION:
		alGetSource3f(sid, param, &ret[0], &ret[1], &ret[2]);
		*value1 = (ALint)ret[0];
		*value2 = (ALint)ret[1];
		*value3 = (ALint)ret[2];
		break;
	default:
		AL_SET_ERROR(AL_INVALID_ENUM);
		break;
	}
}

AL_API void AL_APIENTRY alGetSourceiv(ALuint sid, ALenum param, ALint* values)
{
	if (values == NULL)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	switch (param)
	{
	case AL_MAX_DISTANCE:
	case AL_ROLLOFF_FACTOR:
	case AL_REFERENCE_DISTANCE:
	case AL_CONE_INNER_ANGLE:
	case AL_CONE_OUTER_ANGLE:
	case AL_SOURCE_RELATIVE:
	case AL_SOURCE_TYPE:
	case AL_LOOPING:
	case AL_BUFFER:
	case AL_SOURCE_STATE:
	case AL_BUFFERS_QUEUED:
	case AL_BUFFERS_PROCESSED:
	case AL_SEC_OFFSET:
	case AL_SAMPLE_OFFSET:
	case AL_BYTE_OFFSET:
		alGetSourcei(sid, param, values);
		break;
	case AL_DIRECTION:
		alGetSource3i(sid, param, &values[0], &values[1], &values[2]);
		break;
	default:
		AL_SET_ERROR(AL_INVALID_ENUM);
		break;
	}
}

AL_API void AL_APIENTRY alSourcePlayv(ALsizei ns, const ALuint *sids)
{
	if (sids == NULL)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	for (int i = 0; i < ns; i++)
	{
		alSourcePlay(sids[i]);
	}
}

AL_API void AL_APIENTRY alSourceStopv(ALsizei ns, const ALuint *sids)
{
	if (sids == NULL)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	for (int i = 0; i < ns; i++)
	{
		alSourceStop(sids[i]);
	}
}

AL_API void AL_APIENTRY alSourceRewindv(ALsizei ns, const ALuint *sids)
{
	if (sids == NULL)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	for (int i = 0; i < ns; i++)
	{
		alSourceRewind(sids[i]);
	}
}

AL_API void AL_APIENTRY alSourcePausev(ALsizei ns, const ALuint *sids)
{
	if (sids == NULL)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	for (int i = 0; i < ns; i++)
	{
		alSourcePause(sids[i]);
	}
}

AL_API void AL_APIENTRY alSourcePlay(ALuint sid)
{
	SceHwSfxState state;
	Source *src = NULL;
	Context *ctx = (Context *)alcGetCurrentContext();

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	src = (Source *)sid;

	if (!Source::validate(src))
	{
		AL_SET_ERROR(AL_INVALID_NAME);
		return;
	}

	ret = _alErrorHw2Al(sceHeatWaveSfxGetState(src->hwSfx, &state));
	if (ret != AL_NO_ERROR)
	{
		AL_SET_ERROR(ret);
		return;
	}
	if (state == kSfxState_Paused)
	{
		sceHeatWaveSfxResume(src->hwSfx);
	}
	else
	{
		if (src->needOffsetsReset == AL_FALSE)
		{
			src->needOffsetsReset = AL_TRUE;
		}
		else
		{
			src->offsetSec = 0.0f;
			src->offsetSample = 0.0f;
			src->offsetByte = 0.0f;
			src->needOffsetsReset = AL_FALSE;
		}

		sceHeatWaveSfxPlay(src->hwSfx, NULL);
	}
}

AL_API void AL_APIENTRY alSourceStop(ALuint sid)
{
	SceHwSfxState state;
	Source *src = NULL;
	Context *ctx = (Context *)alcGetCurrentContext();

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	src = (Source *)sid;

	if (!Source::validate(src))
	{
		AL_SET_ERROR(AL_INVALID_NAME);
		return;
	}

	sceHeatWaveSfxStop(src->hwSfx);

	src->offsetSec = 0.0f;
	src->offsetSample = 0.0f;
	src->offsetByte = 0.0f;
}

AL_API void AL_APIENTRY alSourceRewind(ALuint sid)
{
	Source *src = NULL;
	Context *ctx = (Context *)alcGetCurrentContext();

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	src = (Source *)sid;

	if (!Source::validate(src))
	{
		AL_SET_ERROR(AL_INVALID_NAME);
		return;
	}

	sceHeatWaveSfxPause(src->hwSfx);
	src->offsetSec = 0.0f;
	src->offsetSample = 0.0f;
	src->offsetByte = 0.0f;
	sceHeatWaveSfxResume(src->hwSfx);
}

AL_API void AL_APIENTRY alSourcePause(ALuint sid)
{
	Source *src = NULL;
	Context *ctx = (Context *)alcGetCurrentContext();

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	src = (Source *)sid;

	if (!Source::validate(src))
	{
		AL_SET_ERROR(AL_INVALID_NAME);
		return;
	}

	sceHeatWaveSfxPause(src->hwSfx);
}