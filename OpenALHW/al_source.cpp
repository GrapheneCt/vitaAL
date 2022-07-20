#include <heatwave.h>
#include <string.h>
#include <float.h>

#include "common.h"
#include "context.h"
#include "named_object.h"

using namespace al;

Source::BufferConsumeState Source::consumeBuffer(Buffer *in, ALchar **out, ALuint needBytes, ALuint *remainBytes)
{
	*remainBytes = 0;

	if (needBytes < in->trackSize)
	{
		memcpy(*out, in->trackStorage, needBytes);
		*out += needBytes;
		in->trackStorage += needBytes;
		in->trackSize -= needBytes;
		return BufferConsumeState_Remain;
	}
	else if (needBytes == in->trackSize)
	{
		memcpy(*out, in->trackStorage, needBytes);
		*out += needBytes;
		in->trackStorage = NULL;
		in->trackSize = 0;
		return BufferConsumeState_Exact;
	}
	else if (needBytes > in->trackSize)
	{
		memcpy(*out, in->trackStorage, in->trackSize);
		*out += in->trackSize;
		*remainBytes = needBytes - in->trackSize;
		in->trackStorage = NULL;
		in->trackSize = 0;
		return BufferConsumeState_NeedMore;
	}

	return BufferConsumeState_Exact;
}

void Source::runtimeStreamEntry(SceHwSourceRuntimeStreamCallbackInfo *pCallbackInfo, void *pUserData, ScewHwSourceRuntimeStreamCallbackType reason)
{
	Source *src = (Source *)pUserData;

	BufferConsumeState ret;
	Buffer *curBuf = NULL;
	ALuint remBytes = 0;
	ALuint requestedSize = 0;
	void *writePtr = NULL;

	//unsigned int t1 = sceKernelGetProcessTimeLow();
	if (reason == eStreamRender)
	{
		sceKernelLockLwMutex(src->lock, 1, NULL);

		requestedSize = pCallbackInfo->nBufferSizeBytes;
		remBytes = requestedSize;
		writePtr = pCallbackInfo->pcBuffer;

		if (src->type == AL_STREAMING)
		{
			//printf("requested %u\n", requestedSize);

		consumeNext:

			if (src->queuedBuffers.size() == 0)
			{
				pCallbackInfo->nBytesWrittenOut = pCallbackInfo->nBufferSizeBytes - remBytes;
				return;
			}

			/*
			if (seekPos != 0)
			{

			}
			*/

			curBuf = *src->queuedBuffers.begin();
			ret = consumeBuffer(curBuf, (ALchar **)&writePtr, requestedSize, &remBytes);
			//printf("buffer at state %d\n", ret);
			switch (ret) {
			case BufferConsumeState_Remain:
				break;
			case BufferConsumeState_Exact:
				curBuf->deref();
				src->queuedBuffers.erase(src->queuedBuffers.begin());
				src->processedBuffers.push_back(curBuf);
				break;
			case BufferConsumeState_NeedMore:
				curBuf->deref();
				src->queuedBuffers.erase(src->queuedBuffers.begin());
				src->processedBuffers.push_back(curBuf);
				requestedSize = remBytes;
				goto consumeNext;
			}

			pCallbackInfo->nBytesWrittenOut = pCallbackInfo->nBufferSizeBytes;

			//printf("written %u\n", (char *)writePtr - pCallbackInfo->pcBuffer);
		}
		else if (src->type == AL_STATIC)
		{
			if (src->staticBuffer->trackSize == 0)
			{
				pCallbackInfo->nBytesWrittenOut = 0;
				return;
			}

			ret = consumeBuffer(src->staticBuffer, (ALchar **)&writePtr, requestedSize, &remBytes);
			//printf("STATIC buffer at state %d\n", ret);
			switch (ret) {
			case BufferConsumeState_Remain:
				break;
			case BufferConsumeState_Exact:
				src->staticBuffer->deref();
				break;
			case BufferConsumeState_NeedMore:
				src->staticBuffer->deref();
				pCallbackInfo->nBytesWrittenOut = pCallbackInfo->nBufferSizeBytes - remBytes;
				return;
			}

			pCallbackInfo->nBytesWrittenOut = pCallbackInfo->nBufferSizeBytes;
		}

		sceKernelUnlockLwMutex(src->lock, 1);
	}
	//printf("time taken %u\n", sceKernelGetProcessTimeLow() - t1);
}

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
	needOffsetsReset(AL_FALSE),
	lock(NULL),
	streamFrequency(0),
	streamChannels(0)
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

	lock = (SceKernelLwMutexWork *)malloc(sizeof(SceKernelLwMutexWork));

	sceKernelCreateLwMutex(lock, "ALSound", SCE_KERNEL_LW_MUTEX_ATTR_TH_FIFO, 0, NULL);

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

	sceKernelLockLwMutex(lock, 1, NULL);
	sceKernelUnlockLwMutex(lock, 1);

	sceKernelDeleteLwMutex(lock);

	free(lock);

	m_initialized = AL_FALSE;

	return AL_NO_ERROR;

error:

	return _alErrorHw2Al(ret);
}

ALvoid Source::dropAllBuffers()
{
	Buffer *buf = NULL;

	sceHeatWaveSfxStop(hwSfx);

	while (queuedBuffers.size() != 0)
	{
		buf = queuedBuffers.back();
		buf->deref();
		if (buf->refCounter == 0)
		{
			buf->state = AL_UNUSED;
		}
		queuedBuffers.pop_back();
	}

	while (processedBuffers.size() != 0)
	{
		buf = processedBuffers.back();
		if (buf->refCounter == 0)
		{
			buf->state = AL_UNUSED;
		}
		processedBuffers.pop_back();
	}

	if (staticBuffer != NULL)
	{
		staticBuffer->deref();
		if (staticBuffer->refCounter == 0)
		{
			staticBuffer->state = AL_UNUSED;
		}
		staticBuffer = NULL;
	}

	type = AL_UNDETERMINED;
}

ALvoid Source::switchToStaticBuffer(Buffer *buf)
{
	Buffer *obuf = NULL;

	sceKernelLockLwMutex(lock, 1, NULL);

	while (queuedBuffers.size() != 0)
	{
		obuf = queuedBuffers.back();
		obuf->deref();
		if (obuf->refCounter == 0)
		{
			obuf->state = AL_UNUSED;
		}
		queuedBuffers.pop_back();
	}

	while (processedBuffers.size() != 0)
	{
		obuf = processedBuffers.back();
		if (obuf->refCounter == 0)
		{
			obuf->state = AL_UNUSED;
		}
		processedBuffers.pop_back();
	}

	if (staticBuffer != NULL)
	{
		staticBuffer->deref();
		if (staticBuffer->refCounter == 0)
		{
			staticBuffer->state = AL_UNUSED;
		}
		staticBuffer = NULL;
	}

	staticBuffer = buf;
	staticBuffer->ref();

	type = AL_STATIC;

	sceKernelUnlockLwMutex(lock, 1);
}

ALint Source::reloadRuntimeStream(ALint frequency, ALint channels)
{
	SceHwSourceRuntimeStreamParam sparam;
	SceHwSfxState state = kSfxState_Playing;

	sceHeatWaveSfxGetState(hwSfx, &state);

	if (state != kSfxState_Ready)
	{
		return AL_INVALID_OPERATION;
	}

	sparam.nSampleRate = frequency;
	sparam.nChannelCount = channels;

	sceHeatWaveSourceLoadRuntimeStream(hwSrc, &sparam, runtimeStreamEntry, this);

	streamFrequency = frequency;
	streamChannels = channels;

	return AL_NO_ERROR;
}

AL_API void AL_APIENTRY alGenSources(ALsizei n, ALuint* sources)
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
		pSrc = new Source();

		ret = pSrc->init();
		if (ret != AL_NO_ERROR)
		{
			AL_SET_ERROR(ret);
			return;
		}

		sources[i] = (ALuint)pSrc;
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
		sceKernelLockLwMutex(src->lock, 1, NULL);
		src->offsetSec = value;
		sceKernelUnlockLwMutex(src->lock, 1);
		break;
	case AL_SAMPLE_OFFSET:
		if (value < 0.0f)
		{
			AL_SET_ERROR(AL_INVALID_VALUE);
			return;
		}
		sceKernelLockLwMutex(src->lock, 1, NULL);
		src->offsetSample = value;
		sceKernelUnlockLwMutex(src->lock, 1);
		break;
	case AL_BYTE_OFFSET:
		if (value < 0.0f)
		{
			AL_SET_ERROR(AL_INVALID_VALUE);
			return;
		}
		sceKernelLockLwMutex(src->lock, 1, NULL);
		src->offsetByte = value;
		sceKernelUnlockLwMutex(src->lock, 1);
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
			AL_SET_ERROR(AL_INVALID_NAME);
			return;
		}
		if (src->streamFrequency != buf->frequency || src->streamChannels != buf->channels)
		{
			ret = src->reloadRuntimeStream(buf->frequency, buf->channels);
			if (ret != AL_NO_ERROR)
			{
				AL_SET_ERROR(ret);
				return;
			}
		}
		src->switchToStaticBuffer(buf);
		break;
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
		*value = src->queuedBuffers.size();
		break;
	case AL_BUFFERS_PROCESSED:
		*value = src->processedBuffers.size();
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
	ALint ret = AL_NO_ERROR;
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

	sceKernelLockLwMutex(src->lock, 1, NULL);
	src->offsetSec = 0.0f;
	src->offsetSample = 0.0f;
	src->offsetByte = 0.0f;
	sceKernelUnlockLwMutex(src->lock, 1);
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

AL_API void AL_APIENTRY alSourceQueueBuffers(ALuint sid, ALsizei numEntries, const ALuint *bids)
{
	Source *src = NULL;
	Buffer *buf = NULL;
	Context *ctx = (Context *)alcGetCurrentContext();
	ALint ret = AL_NO_ERROR;
	ALint frequency = 0;
	ALint bits = 0;
	ALint channels = 0;

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	if (bids == NULL)
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

	if (src->type == AL_STATIC)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	for (int i = 0; i < numEntries; i++)
	{
		if (bids[i] == 0)
			continue;

		if (buf == NULL)
		{
			buf = (Buffer *)bids[i];
			frequency = buf->frequency;
			bits = buf->bits;
			channels = buf->channels;
		}
		else
		{
			buf = (Buffer *)bids[i];
			if (frequency != buf->frequency || bits != buf->bits || channels != buf->channels)
			{
				AL_SET_ERROR(AL_INVALID_VALUE);
				return;
			}
		}
	}

	buf = NULL;

	if (src->streamFrequency != frequency || src->streamChannels != channels)
	{
		ret = src->reloadRuntimeStream(frequency, channels);
		if (ret != AL_NO_ERROR)
		{
			AL_SET_ERROR(ret);
			return;
		}
	}

	sceKernelLockLwMutex(src->lock, 1, NULL);
	for (int i = 0; i < numEntries; i++)
	{
		if (bids[i] == 0)
			continue;

		buf = (Buffer *)bids[i];

		buf->ref();
		src->queuedBuffers.push_back(buf);
	}
	sceKernelUnlockLwMutex(src->lock, 1);

	src->type = AL_STREAMING;
}

AL_API void AL_APIENTRY alSourceUnqueueBuffers(ALuint sid, ALsizei numEntries, ALuint *bids)
{
	Source *src = NULL;
	Buffer *buf = NULL;
	Context *ctx = (Context *)alcGetCurrentContext();

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	if (bids == NULL)
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

	if (src->type == AL_STATIC)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	sceKernelLockLwMutex(src->lock, 1, NULL);
	if (src->processedBuffers.size() < numEntries)
	{
		sceKernelUnlockLwMutex(src->lock, 1);
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	std::vector<Buffer*>::iterator it = src->processedBuffers.begin();

	for (int i = 0; i < numEntries; i++)
	{
		bids[i] = (ALuint)*it;
		buf = (Buffer *)bids[i];
		if (buf->refCounter == 0)
		{
			buf->state = AL_UNUSED;
		}
		it = src->processedBuffers.erase(it);
	}
	sceKernelUnlockLwMutex(src->lock, 1);
}