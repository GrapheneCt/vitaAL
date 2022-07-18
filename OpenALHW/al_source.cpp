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
	coneOuterFreq(1.0f)
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
		ret = _alErrorHw2Al(sceHeatWaveSourceSetRolloffDistances(src->hwSfx, src->minDistance, value));
		if (ret != AL_NO_ERROR)
		{
			AL_SET_ERROR(ret);
			return;
		}
		src->maxDistance = value;
		break;
	case AL_ROLLOFF_FACTOR:
		ret = _alErrorHw2Al(sceHeatWaveSourceSetDistanceFactor(src->hwSfx, value));
		if (ret != AL_NO_ERROR)
		{
			AL_SET_ERROR(ret);
			return;
		}
		src->roloffFactor = value;
		break;
	case AL_REFERENCE_DISTANCE:
		ret = _alErrorHw2Al(sceHeatWaveSourceSetRolloffDistances(src->hwSfx, value, src->maxDistance));
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
		}
		src->minGain = value;
	case AL_MAX_GAIN:
		if (value > kfSfxVolumeMax || value < src->minGain)
		{
			AL_SET_ERROR(AL_INVALID_VALUE);
		}
		src->maxGain = value;
	case AL_CONE_OUTER_GAIN:
		
	default:
		AL_SET_ERROR(AL_INVALID_ENUM);
		break;
	}
}

AL_API void AL_APIENTRY alSource3f(ALuint sid, ALenum param, ALfloat value1, ALfloat value2, ALfloat value3)
{

}

AL_API void AL_APIENTRY alSourcefv(ALuint sid, ALenum param, const ALfloat* values)
{

}

AL_API void AL_APIENTRY alSourcei(ALuint sid, ALenum param, ALint value)
{

}

AL_API void AL_APIENTRY alSource3i(ALuint sid, ALenum param, ALint value1, ALint value2, ALint value3)
{

}

AL_API void AL_APIENTRY alSourceiv(ALuint sid, ALenum param, const ALint* values)
{

}