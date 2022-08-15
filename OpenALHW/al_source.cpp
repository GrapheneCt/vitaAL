#include <ngs.h>
#include <string.h>
#include <float.h>
#include <algorithm>
#include <sce_atomic.h>

#include "common.h"
#include "context.h"
#include "device.h"
#include "named_object.h"

using namespace al;

SourceParams::SourceParams()
{
	vPosition.x = 0.0f;
	vPosition.y = 0.0f;
	vPosition.z = 0.0f;

	vVelocity.x = 0.0f;
	vVelocity.y = 0.0f;
	vVelocity.z = 0.0f;

	vForward.x = 0.0f;
	vForward.y = 0.0f;
	vForward.z = -1.0f;

	fMinDistance = 1.0f;
	fMaxDistance = FLT_MAX;

	fInsideAngle = 0.0f;
	fOutsideAngle = 360.0f;
	fOutsideGain = 1.0f;
	fOutsideFreq = 1.0f;

	bListenerRelative = false;

	fDistanceFactor = 1.0f;
	fPitchMul = 1.0f;
	fGainMul = 1.0f;
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

ALvoid Source::streamCallback(const SceNgsCallbackInfo *pCallbackInfo)
{
	SceInt32 ret = SCE_NGS_OK;
	SceNgsBufferInfo   bufferInfo;
	SceNgsPlayerParams *pPcmParams;
	Source *src = (Source *)pCallbackInfo->pUserData;
	ALint bufferFlag = 0;

	if (pCallbackInfo->nCallbackData == SCE_NGS_PLAYER_SWAPPED_BUFFER)
	{
		if ((volatile ALboolean)src->m_looping != AL_TRUE)
		{
			bufferFlag = (1 << pCallbackInfo->nCallbackData2);

			ret = sceNgsVoiceLockParams(pCallbackInfo->hVoiceHandle, SCE_NGS_SIMPLE_VOICE_PCM_PLAYER, SCE_NGS_PLAYER_PARAMS_STRUCT_ID, &bufferInfo);
			if (ret != SCE_NGS_OK)
			{
				AL_WARNING("Error has occured in streamCallback: 0x%08X\n", ret);
				return;
			}

			pPcmParams = (SceNgsPlayerParams *)bufferInfo.data;
			pPcmParams->buffs[pCallbackInfo->nCallbackData2].pBuffer = NULL;
			pPcmParams->buffs[pCallbackInfo->nCallbackData2].nNumBytes = 0;
			pPcmParams->buffs[pCallbackInfo->nCallbackData2].nLoopCount = 0;
			pPcmParams->buffs[pCallbackInfo->nCallbackData2].nNextBuff = SCE_NGS_PLAYER_NO_NEXT_BUFFER;

			ret = sceNgsVoiceUnlockParams(pCallbackInfo->hVoiceHandle, SCE_NGS_SIMPLE_VOICE_PCM_PLAYER);
			if (ret != SCE_NGS_OK)
			{
				AL_WARNING("Error has occured in streamCallback: 0x%08X\n", ret);
				return;
			}

			src->m_queueBuffers &= ~bufferFlag;
			src->m_processedBuffers |= bufferFlag;
		}
	}
}

Source::Source(Context *ctx)
	: m_voice(AL_INVALID_NGS_HANDLE),
	m_patch(AL_INVALID_NGS_HANDLE),
	m_voiceIdx(-1),
	m_minGain(0.0f),
	m_maxGain(1.0f),
	m_offsetSec(0.0f),
	m_offsetSample(0.0f),
	m_offsetByte(0.0f),
	m_altype(AL_UNDETERMINED),
	m_looping(AL_FALSE),
	m_needOffsetsReset(AL_FALSE),
	m_paramsDirty(AL_FALSE),
	m_queueBuffers(0),
	m_processedBuffers(0),
	m_lastPushedIdx(3)
{
	m_ctx = ctx;
	m_type = ObjectType_Source;
}

Source::~Source()
{

}

ALint Source::init()
{
	SceInt32 ret = SCE_NGS_OK;
	DeviceNGS *ngsDev = (DeviceNGS *)m_ctx->getDevice();
	SceNgsBufferInfo   bufferInfo;
	SceNgsPlayerParams *pPcmParams;
	SceNgsPatchSetupInfo patchInfo;
	SceNgsPatchRouteInfo patchRouteInfo;

	for (int i = 0; i < ngsDev->getMaxMonoVoiceCount() + ngsDev->getMaxStereoVoiceCount(); i++)
	{
		if (m_ctx->m_voiceUsed[i] == false)
		{
			m_voiceIdx = i;
			break;
		}
	}

	if (m_voiceIdx == -1)
	{
		return AL_OUT_OF_MEMORY;
	}

	ret = sceNgsRackGetVoiceHandle(m_ctx->m_sourceRack, m_voiceIdx, &m_voice);
	if (ret != SCE_NGS_OK)
	{
		return _alErrorNgs2Al(ret);
	}

	sceNgsVoiceBypassModule(m_voice, SCE_NGS_SIMPLE_VOICE_EQ, SCE_NGS_MODULE_FLAG_BYPASSED);

	ret = sceNgsVoiceLockParams(m_voice, SCE_NGS_SIMPLE_VOICE_PCM_PLAYER, SCE_NGS_PLAYER_PARAMS_STRUCT_ID, &bufferInfo);
	if (ret != SCE_NGS_OK)
	{
		return _alErrorNgs2Al(ret);
	}

	memset(bufferInfo.data, 0, bufferInfo.size);
	pPcmParams = (SceNgsPlayerParams *)bufferInfo.data;
	pPcmParams->desc.id = SCE_NGS_PLAYER_PARAMS_STRUCT_ID;
	pPcmParams->desc.size = sizeof(SceNgsPlayerParams);

	pPcmParams->fPlaybackScalar = 1.0f;
	pPcmParams->nLeadInSamples = 0;
	pPcmParams->nLimitNumberOfSamplesPlayed = 0;
	pPcmParams->nChannels = 1;

	pPcmParams->nType = SCE_NGS_PLAYER_TYPE_PCM;
	pPcmParams->nStartBuffer = 0;
	pPcmParams->nStartByte = 0;

	ret = sceNgsVoiceUnlockParams(m_voice, SCE_NGS_SIMPLE_VOICE_PCM_PLAYER);
	if (ret != SCE_NGS_OK)
	{
		return _alErrorNgs2Al(ret);
	}

	patchInfo.hVoiceSource = m_voice;
	patchInfo.nSourceOutputIndex = 0;
	patchInfo.nSourceOutputSubIndex = SCE_NGS_VOICE_PATCH_AUTO_SUBINDEX;
	patchInfo.hVoiceDestination = m_ctx->m_masterVoice;
	patchInfo.nTargetInputIndex = 0;

	ret = sceNgsPatchCreateRouting(&patchInfo, &m_patch);
	if (ret != SCE_NGS_OK)
	{
		return _alErrorNgs2Al(ret);
	}

	ret = sceNgsPatchGetInfo(m_patch, &patchRouteInfo, NULL);
	if (ret != SCE_NGS_OK)
	{
		return _alErrorNgs2Al(ret);
	}

	if (patchRouteInfo.nOutputChannels == 1)
	{
		patchRouteInfo.vols.m[0][0] = 1.0f; // left to left
		patchRouteInfo.vols.m[0][1] = 1.0f; // left to right
	}
	else
	{
		patchRouteInfo.vols.m[0][0] = 1.0f; // left to left
		patchRouteInfo.vols.m[0][1] = 0.0f; // left to right
		patchRouteInfo.vols.m[1][0] = 0.0f; // right to left
		patchRouteInfo.vols.m[1][1] = 1.0f; // right to right
	}

	ret = sceNgsVoicePatchSetVolumesMatrix(m_patch, &patchRouteInfo.vols);
	if (ret != SCE_NGS_OK)
	{
		return _alErrorNgs2Al(ret);
	}

	sceKernelCreateLwMutex(&m_lock, "OpenALHW::SrcMtx", 0, 0, NULL);

	m_ctx->m_voiceUsed[m_voiceIdx] = true;

	m_initialized = AL_TRUE;

	return AL_NO_ERROR;
}

ALint Source::release()
{
	sceNgsSystemLock(m_ctx->m_system);
	sceNgsVoiceKill(m_voice);
	sceNgsVoiceSetModuleCallback(m_voice, SCE_NGS_SIMPLE_VOICE_PCM_PLAYER, SCE_NGS_NO_CALLBACK, NULL);
	sceNgsPatchRemoveRouting(m_patch);
	sceNgsSystemUnlock(m_ctx->m_system);

	if (m_voiceIdx != -1)
		m_ctx->m_voiceUsed[m_voiceIdx] = false;

	sceKernelDeleteLwMutex(&m_lock);

	m_initialized = AL_FALSE;

	return AL_NO_ERROR;
}

ALint Source::dropAllBuffers()
{
	SceInt32 ret = SCE_NGS_OK;
	Buffer *buf = NULL;
	SceNgsBufferInfo   bufferInfo;
	SceNgsPlayerParams *pPcmParams;

	sceNgsVoiceSetModuleCallback(m_voice, SCE_NGS_SIMPLE_VOICE_PCM_PLAYER, SCE_NGS_NO_CALLBACK, NULL);

	sceNgsVoiceKeyOff(m_voice);

	m_offsetSec = 0.0f;
	m_offsetSample = 0.0f;
	m_offsetByte = 0.0f;

	ret = sceNgsVoiceLockParams(m_voice, SCE_NGS_SIMPLE_VOICE_PCM_PLAYER, SCE_NGS_PLAYER_PARAMS_STRUCT_ID, &bufferInfo);
	if (ret != SCE_NGS_OK)
	{
		return _alErrorNgs2Al(ret);
	}

	pPcmParams = (SceNgsPlayerParams *)bufferInfo.data;

	for (int i = 0; i < SCE_NGS_PLAYER_MAX_BUFFERS; i++)
	{
		if (pPcmParams->buffs[i].pBuffer != NULL)
		{
			for (Buffer *buf : m_ctx->m_bufferStack)
			{
				if (pPcmParams->buffs[i].pBuffer == buf->m_storage)
				{
					buf->deref();
					if (buf->m_refCounter == 0)
					{
						buf->m_state = AL_UNUSED;
					}
					break;
				}
			}
		}

		pPcmParams->buffs[i].pBuffer = NULL;
		pPcmParams->buffs[i].nNumBytes = 0;
		pPcmParams->buffs[i].nLoopCount = 0;
		pPcmParams->buffs[i].nNextBuff = SCE_NGS_PLAYER_NO_NEXT_BUFFER;
	}

	ret = sceNgsVoiceUnlockParams(m_voice, SCE_NGS_SIMPLE_VOICE_PCM_PLAYER);
	if (ret != SCE_NGS_OK)
	{
		return _alErrorNgs2Al(ret);
	}

	m_altype = AL_UNDETERMINED;
	m_lastPushedIdx = 3;
	sceAtomicStore32AcqRel(&m_processedBuffers, 0);
	sceAtomicStore32AcqRel(&m_queueBuffers, 0);

	return AL_NO_ERROR;
}

ALint Source::switchToStaticBuffer(ALint frequency, ALint channels, Buffer *buf)
{
	SceInt32 ret = SCE_NGS_OK;
	Buffer *obuf = NULL;
	SceNgsBufferInfo   bufferInfo;
	SceNgsPlayerParams *pPcmParams;

	ret = dropAllBuffers();
	if (ret != AL_NO_ERROR)
	{
		return ret;
	}

	ret = sceNgsVoiceLockParams(m_voice, SCE_NGS_SIMPLE_VOICE_PCM_PLAYER, SCE_NGS_PLAYER_PARAMS_STRUCT_ID, &bufferInfo);
	if (ret != SCE_NGS_OK)
	{
		return _alErrorNgs2Al(ret);
	}

	pPcmParams = (SceNgsPlayerParams *)bufferInfo.data;

	pPcmParams->fPlaybackFrequency = frequency;
	pPcmParams->nChannels = channels;
	if (channels == 1)
	{
		pPcmParams->nChannelMap[0] = SCE_NGS_PLAYER_LEFT_CHANNEL;
		pPcmParams->nChannelMap[1] = SCE_NGS_PLAYER_LEFT_CHANNEL;
	}
	else
	{
		pPcmParams->nChannelMap[0] = SCE_NGS_PLAYER_LEFT_CHANNEL;
		pPcmParams->nChannelMap[1] = SCE_NGS_PLAYER_RIGHT_CHANNEL;
	}

	pPcmParams->nStartBuffer = 0;
	pPcmParams->nStartByte = 0;

	pPcmParams->buffs[0].pBuffer = buf->m_storage;
	pPcmParams->buffs[0].nNumBytes = buf->m_size;
	if (m_looping == AL_TRUE)
	{
		pPcmParams->buffs[0].nLoopCount = SCE_NGS_PLAYER_LOOP_CONTINUOUS;
	}
	else
	{
		pPcmParams->buffs[0].nLoopCount = 0;
	}
	pPcmParams->buffs[0].nNextBuff = SCE_NGS_PLAYER_NO_NEXT_BUFFER;

	ret = sceNgsVoiceUnlockParams(m_voice, SCE_NGS_SIMPLE_VOICE_PCM_PLAYER);
	if (ret != SCE_NGS_OK)
	{
		return _alErrorNgs2Al(ret);
	}

	m_altype = AL_STATIC;

	return AL_NO_ERROR;
}

ALvoid Source::beginParamUpdate()
{
	sceKernelLockLwMutex(&m_lock, 1, NULL);
}

ALvoid Source::endParamUpdate()
{
	m_paramsDirty = AL_TRUE;
	sceKernelUnlockLwMutex(&m_lock, 1);
}

ALvoid Source::update()
{
	SceInt32 ret = SCE_NGS_OK;
	SceNgsBufferInfo   bufferInfo;
	SceNgsPlayerParams *pPcmParams;
	SceNgsFilterParams *pFilterParams;
	SceNgsPatchRouteInfo patchRouteInfo;
	float32_t volumeMatrix[2];
	float32_t lowpassCutoff = 1.0f;
	float32_t dopplerShift = 1.0f;

	if (m_paramsDirty) {

		sceKernelLockLwMutex(&m_lock, 1, NULL);

		m_ctx->m_panner.calculate(&m_params, 2, volumeMatrix, &dopplerShift, &lowpassCutoff);

		volumeMatrix[0] *= m_params.fGainMul;
		volumeMatrix[1] *= m_params.fGainMul;

		ret = sceNgsVoiceLockParams(m_voice, SCE_NGS_SIMPLE_VOICE_PCM_PLAYER, SCE_NGS_PLAYER_PARAMS_STRUCT_ID, &bufferInfo);
		if (ret != SCE_NGS_OK)
		{
			goto updateFilter;
		}

		pPcmParams = (SceNgsPlayerParams *)bufferInfo.data;
		pPcmParams->fPlaybackScalar = dopplerShift * m_params.fPitchMul;

		if (m_looping == AL_TRUE)
		{
			if (m_altype == AL_STATIC)
			{
				pPcmParams->buffs[0].nLoopCount = SCE_NGS_PLAYER_LOOP_CONTINUOUS;
			}
			else if (m_altype == AL_STREAMING)
			{
				ALint flags = sceAtomicLoad32AcqRel(&m_queueBuffers);
				ALint searchIdx = m_lastPushedIdx - 1;
				if (searchIdx < 0)
				{
					searchIdx = 3;
				}

				while (searchIdx != m_lastPushedIdx)
				{
					if ((flags & (1 << searchIdx)) == 0)
					{
						break;
					}

					searchIdx--;
					if (searchIdx < 0)
					{
						searchIdx = 3;
					}
				}

				searchIdx++;
				if (searchIdx > 3)
				{
					searchIdx = 0;
				}

				pPcmParams->buffs[m_lastPushedIdx].nNextBuff = searchIdx;
			}
		}
		else
		{
			if (m_altype == AL_STATIC)
			{
				pPcmParams->buffs[0].nLoopCount = 0;
			}
			else if (m_altype == AL_STREAMING)
			{
				pPcmParams->buffs[m_lastPushedIdx].nNextBuff = SCE_NGS_PLAYER_NO_NEXT_BUFFER;
			}
		}

		sceNgsVoiceUnlockParams(m_voice, SCE_NGS_SIMPLE_VOICE_PCM_PLAYER);

	updateFilter:

		ret = sceNgsVoiceLockParams(m_voice, SCE_NGS_SIMPLE_VOICE_SEND_1_FILTER, SCE_NGS_FILTER_PARAMS_STRUCT_ID, &bufferInfo);
		if (ret != SCE_NGS_OK)
		{
			goto updatePatch;
		}

		pFilterParams = (SceNgsFilterParams *)bufferInfo.data;

		for (uint32_t chan = 0; chan < bufferInfo.size / sizeof(SceNgsFilterParams); chan++)
		{
			pFilterParams->eFilterMode = SCE_NGS_FILTER_LOWPASS_ONEPOLE;
			pFilterParams->fResonance = 1.0f;
			pFilterParams->fFrequency = 20.0f + ((22000.0f - 20.0f) * lowpassCutoff);

			pFilterParams++;
		}

		sceNgsVoiceUnlockParams(m_voice, SCE_NGS_SIMPLE_VOICE_SEND_1_FILTER);

	updatePatch:

		ret = sceNgsPatchGetInfo(m_patch, &patchRouteInfo, NULL);
		if (ret != SCE_NGS_OK)
		{
			goto updateEnd;
		}

		if (patchRouteInfo.nOutputChannels == 1)
		{
			patchRouteInfo.vols.m[0][0] = volumeMatrix[0]; // left to left
			patchRouteInfo.vols.m[0][1] = volumeMatrix[1]; // left to right
		}
		else
		{
			patchRouteInfo.vols.m[0][0] = volumeMatrix[0]; // left to left
			patchRouteInfo.vols.m[0][1] = 0.0f; // nothing
			patchRouteInfo.vols.m[1][0] = 0.0f; // nothing
			patchRouteInfo.vols.m[1][1] = volumeMatrix[1]; // right to right
		}

		sceNgsVoicePatchSetVolumesMatrix(m_patch, &patchRouteInfo.vols);

	updateEnd:

		m_paramsDirty = AL_FALSE;
		sceKernelUnlockLwMutex(&m_lock, 1);
	}
}

ALint Source::processedBufferCount()
{
	ALint ret = 0;
	ALint flags = sceAtomicLoad32AcqRel(&m_processedBuffers);

	if (flags & NGS_BUFFER_IDX_0)
	{
		ret++;
	}

	if (flags & NGS_BUFFER_IDX_1)
	{
		ret++;
	}

	if (flags & NGS_BUFFER_IDX_2)
	{
		ret++;
	}

	if (flags & NGS_BUFFER_IDX_3)
	{
		ret++;
	}

	return ret;
}

ALint Source::queuedBufferCount()
{
	ALint ret = 0;
	ALint flags = sceAtomicLoad32AcqRel(&m_queueBuffers);

	if (flags & NGS_BUFFER_IDX_0)
	{
		ret++;
	}

	if (flags & NGS_BUFFER_IDX_1)
	{
		ret++;
	}

	if (flags & NGS_BUFFER_IDX_2)
	{
		ret++;
	}

	if (flags & NGS_BUFFER_IDX_3)
	{
		ret++;
	}

	return ret;
}

ALint Source::bqPush(ALint frequency, ALint channels, Buffer *buf)
{
	SceInt32 ret = SCE_NGS_OK;
	SceInt32 nextPushIdx = 0;
	SceNgsBufferInfo   bufferInfo;
	SceNgsPlayerParams *pPcmParams;

	nextPushIdx = m_lastPushedIdx + 1;
	if (nextPushIdx == 4)
	{
		nextPushIdx = 0;
	}

	ret = sceNgsVoiceLockParams(m_voice, SCE_NGS_SIMPLE_VOICE_PCM_PLAYER, SCE_NGS_PLAYER_PARAMS_STRUCT_ID, &bufferInfo);
	if (ret != SCE_NGS_OK)
	{
		return _alErrorNgs2Al(ret);
	}

	pPcmParams = (SceNgsPlayerParams *)bufferInfo.data;

	if (pPcmParams->fPlaybackFrequency != 0)
	{
		if (pPcmParams->fPlaybackFrequency != (SceFloat32)frequency || pPcmParams->nChannels != (SceInt8)channels)
		{
			ret = sceNgsVoiceUnlockParams(m_voice, SCE_NGS_SIMPLE_VOICE_PCM_PLAYER);
			if (ret != SCE_NGS_OK)
			{
				return _alErrorNgs2Al(ret);
			}

			return AL_INVALID_VALUE;
		}
	}
	else {
		pPcmParams->fPlaybackFrequency = (SceFloat32)frequency;
		pPcmParams->nChannels = (SceInt8)channels;
		if (channels == 1)
		{
			pPcmParams->nChannelMap[0] = SCE_NGS_PLAYER_LEFT_CHANNEL;
			pPcmParams->nChannelMap[1] = SCE_NGS_PLAYER_LEFT_CHANNEL;
		}
		else
		{
			pPcmParams->nChannelMap[0] = SCE_NGS_PLAYER_LEFT_CHANNEL;
			pPcmParams->nChannelMap[1] = SCE_NGS_PLAYER_RIGHT_CHANNEL;
		}
	}

	pPcmParams->buffs[m_lastPushedIdx].nNextBuff = nextPushIdx;

	pPcmParams->buffs[nextPushIdx].pBuffer = buf->m_storage;
	pPcmParams->buffs[nextPushIdx].nNumBytes = buf->m_size;
	pPcmParams->buffs[nextPushIdx].nLoopCount = 0;
	pPcmParams->buffs[nextPushIdx].nNextBuff = SCE_NGS_PLAYER_NO_NEXT_BUFFER;

	m_queueBuffers |= (1 << nextPushIdx);
	m_lastPushedIdx = nextPushIdx;

	ret = sceNgsVoiceUnlockParams(m_voice, SCE_NGS_SIMPLE_VOICE_PCM_PLAYER);
	if (ret != SCE_NGS_OK)
	{
		return _alErrorNgs2Al(ret);
	}
}

AL_API void AL_APIENTRY alGenSources(ALsizei n, ALuint* sources)
{
	Source *pSrc = NULL;
	ALint ret = AL_NO_ERROR;
	Context *ctx = (Context *)alcGetCurrentContext();

	AL_TRACE_CALL

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
		pSrc = new Source(ctx);

		ret = pSrc->init();
		if (ret != AL_NO_ERROR)
		{
			AL_SET_ERROR(ret);
			return;
		}

		ctx->m_sourceStack.push_back(pSrc);

		sources[i] = _alNamedObjectAdd((ALint)pSrc);
	}
}

AL_API void AL_APIENTRY alDeleteSources(ALsizei n, const ALuint* sources)
{
	Source *pSrc = NULL;
	ALint ret = AL_NO_ERROR;
	Context *ctx = (Context *)alcGetCurrentContext();

	AL_TRACE_CALL

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
		pSrc = (Source *)_alNamedObjectGet(sources[i]);

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

		_alNamedObjectRemove(sources[i]);

		ctx->m_sourceStack.erase(std::remove(ctx->m_sourceStack.begin(), ctx->m_sourceStack.end(), pSrc), ctx->m_sourceStack.end());

		delete pSrc;
	}
}

AL_API ALboolean AL_APIENTRY alIsSource(ALuint sid)
{
	Context *ctx = (Context *)alcGetCurrentContext();

	AL_TRACE_CALL

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return AL_FALSE;
	}

	if (Source::validate((Source *)_alNamedObjectGet(sid)))
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

	AL_TRACE_CALL

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	src = (Source *)_alNamedObjectGet(sid);

	if (!Source::validate(src))
	{
		AL_SET_ERROR(AL_INVALID_NAME);
		return;
	}

	if (value < 0.0f)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	switch (param)
	{
	case AL_PITCH:
		if (value < 0.5f || value > 2.0f)
		{
			AL_SET_ERROR(AL_INVALID_VALUE);
			return;
		}
		src->beginParamUpdate();
		src->m_params.fPitchMul = value;
		src->endParamUpdate();
		break;
	case AL_GAIN:
		if (value < src->m_minGain)
		{
			value = src->m_minGain;
		}
		if (value > src->m_maxGain)
		{
			value = src->m_maxGain;
		}
		src->beginParamUpdate();
		src->m_params.fGainMul = value;
		src->endParamUpdate();
		break;
	case AL_MAX_DISTANCE:
		src->beginParamUpdate();
		src->m_params.fMaxDistance = value;
		src->endParamUpdate();
		break;
	case AL_ROLLOFF_FACTOR:
		src->beginParamUpdate();
		src->m_params.fDistanceFactor = value;
		src->endParamUpdate();
		break;
	case AL_REFERENCE_DISTANCE:
		src->beginParamUpdate();
		src->m_params.fMinDistance = value;
		src->endParamUpdate();
		break;
	case AL_MIN_GAIN:
		if (value > src->m_maxGain)
		{
			AL_SET_ERROR(AL_INVALID_VALUE);
			return;
		}
		src->m_minGain = value;
		break;
	case AL_MAX_GAIN:
		if (value < src->m_minGain)
		{
			AL_SET_ERROR(AL_INVALID_VALUE);
			return;
		}
		src->m_maxGain = value;
		break;
	case AL_CONE_OUTER_GAIN:
		src->beginParamUpdate();
		src->m_params.fOutsideGain = value;
		src->endParamUpdate();
		break;
	case AL_CONE_INNER_ANGLE:
		if (value > 360.0f)
		{
			AL_SET_ERROR(AL_INVALID_VALUE);
			return;
		}
		src->beginParamUpdate();
		src->m_params.fInsideAngle = value;
		src->endParamUpdate();
		break;
	case AL_CONE_OUTER_ANGLE:
		if (value > 360.0f)
		{
			AL_SET_ERROR(AL_INVALID_VALUE);
			return;
		}
		src->beginParamUpdate();
		src->m_params.fOutsideAngle = value;
		src->endParamUpdate();
		break;
	case AL_SEC_OFFSET:
		src->beginParamUpdate();
		AL_WARNING("AL_SEC_OFFSET is not implemented\n");
		src->m_offsetSec = value;
		src->endParamUpdate();
		break;
	case AL_SAMPLE_OFFSET:
		src->beginParamUpdate();
		AL_WARNING("AL_SAMPLE_OFFSET is not implemented\n");
		src->m_offsetSample = value;
		src->endParamUpdate();
		break;
	case AL_BYTE_OFFSET:
		src->beginParamUpdate();
		AL_WARNING("AL_BYTE_OFFSET is not implemented\n");
		src->m_offsetByte = value;
		src->endParamUpdate();
		break;
	default:
		AL_SET_ERROR(AL_INVALID_ENUM);
		break;
	}
}

AL_API void AL_APIENTRY alSource3f(ALuint sid, ALenum param, ALfloat value1, ALfloat value2, ALfloat value3)
{
	SceFVector4 value;
	ALint ret = AL_NO_ERROR;
	Source *src = NULL;
	Context *ctx = (Context *)alcGetCurrentContext();

	AL_TRACE_CALL

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	src = (Source *)_alNamedObjectGet(sid);

	if (!Source::validate(src))
	{
		AL_SET_ERROR(AL_INVALID_NAME);
		return;
	}

	value.x = value1;
	value.y = value2;
	value.z = value3;

	src->beginParamUpdate();

	switch (param)
	{
	case AL_POSITION:
		src->m_params.vPosition = value;
		break;
	case AL_VELOCITY:
		src->m_params.vVelocity = value;
		break;
	case AL_DIRECTION:
		src->m_params.vForward = value;
		break;
	default:
		AL_SET_ERROR(AL_INVALID_ENUM);
		break;
	}

	src->endParamUpdate();
}

AL_API void AL_APIENTRY alSourcefv(ALuint sid, ALenum param, const ALfloat* values)
{
	AL_TRACE_CALL

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

	AL_TRACE_CALL

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

	src = (Source *)_alNamedObjectGet(sid);

	if (!Source::validate(src))
	{
		AL_SET_ERROR(AL_INVALID_NAME);
		return;
	}

	switch (param)
	{
	case AL_SOURCE_RELATIVE:
		src->beginParamUpdate();
		src->m_params.bListenerRelative = (bool)value;
		src->endParamUpdate();
		break;
	case AL_LOOPING:
		src->beginParamUpdate();
		src->m_looping = value;
		src->endParamUpdate();
		break;
	case AL_BUFFER:
		src->beginParamUpdate();
		if (value == 0)
		{
			ret = src->dropAllBuffers();
			if (ret != AL_NO_ERROR)
			{
				AL_SET_ERROR(ret);
				src->endParamUpdate();
				return;
			}
			src->endParamUpdate();
			return;
		}
		buf = (Buffer *)_alNamedObjectGet(value);
		if (!Buffer::validate(buf))
		{
			AL_SET_ERROR(AL_INVALID_NAME);
			src->endParamUpdate();
			return;
		}
		ret = src->switchToStaticBuffer(buf->m_frequency, buf->m_channels, buf);
		if (ret != AL_NO_ERROR)
		{
			AL_SET_ERROR(ret);
			src->endParamUpdate();
			return;
		}
		src->endParamUpdate();
		break;
	default:
		AL_SET_ERROR(AL_INVALID_ENUM);
		break;
	}
}

AL_API void AL_APIENTRY alSource3i(ALuint sid, ALenum param, ALint value1, ALint value2, ALint value3)
{
	AL_TRACE_CALL

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
	AL_TRACE_CALL

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

	src = (Source *)_alNamedObjectGet(sid);

	if (!Source::validate(src))
	{
		AL_SET_ERROR(AL_INVALID_NAME);
		return;
	}

	switch (param)
	{
	case AL_PITCH:
		*value = src->m_params.fPitchMul;
		break;
	case AL_GAIN:
		*value = src->m_params.fGainMul;
		break;
	case AL_MAX_DISTANCE:
		*value = src->m_params.fMaxDistance;
		break;
	case AL_ROLLOFF_FACTOR:
		*value = src->m_params.fDistanceFactor;
		break;
	case AL_REFERENCE_DISTANCE:
		*value = src->m_params.fMinDistance;
		break;
	case AL_MIN_GAIN:
		*value = src->m_minGain;
		break;
	case AL_MAX_GAIN:
		*value = src->m_maxGain;
		break;
	case AL_CONE_OUTER_GAIN:
		*value = src->m_params.fOutsideGain;
		break;
	case AL_CONE_INNER_ANGLE:
		*value = src->m_params.fInsideAngle;
		break;
	case AL_CONE_OUTER_ANGLE:
		*value = src->m_params.fOutsideAngle;
		break;
	case AL_SEC_OFFSET:
		AL_WARNING("AL_SEC_OFFSET is not implemented\n");
		*value = src->m_offsetSec;
		break;
	case AL_SAMPLE_OFFSET:
		AL_WARNING("AL_SAMPLE_OFFSET is not implemented\n");
		*value = src->m_offsetSample;
		break;
	case AL_BYTE_OFFSET:
		AL_WARNING("AL_BYTE_OFFSET is not implemented\n");
		*value = src->m_offsetByte;
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

	src = (Source *)_alNamedObjectGet(sid);

	if (!Source::validate(src))
	{
		AL_SET_ERROR(AL_INVALID_NAME);
		return;
	}

	switch (param)
	{
	case AL_POSITION:
		*value1 = src->m_params.vPosition.x;
		*value2 = src->m_params.vPosition.y;
		*value3 = src->m_params.vPosition.z;
		break;
	case AL_VELOCITY:
		*value1 = src->m_params.vVelocity.x;
		*value2 = src->m_params.vVelocity.y;
		*value3 = src->m_params.vVelocity.z;
		break;
	case AL_DIRECTION:
		*value1 = src->m_params.vForward.x;
		*value2 = src->m_params.vForward.y;
		*value3 = src->m_params.vForward.z;
		break;
	default:
		AL_SET_ERROR(AL_INVALID_ENUM);
		break;
	}
}

AL_API void AL_APIENTRY alGetSourcefv(ALuint sid, ALenum param, ALfloat* values)
{
	AL_TRACE_CALL

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
	SceNgsVoiceInfo info;
	Context *ctx = (Context *)alcGetCurrentContext();

	AL_TRACE_CALL

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

	src = (Source *)_alNamedObjectGet(sid);

	if (!Source::validate(src))
	{
		AL_SET_ERROR(AL_INVALID_NAME);
		return;
	}

	switch (param)
	{
	case AL_SOURCE_RELATIVE:
		*value = (ALint)src->m_params.bListenerRelative;
		break;
	case AL_SOURCE_TYPE:
		*value = src->m_altype;
		break;
	case AL_LOOPING:
		*value = (ALint)src->m_looping;
		break;
	case AL_BUFFER:
		if (src->m_altype == AL_STATIC)
		{
			SceNgsBufferInfo   bufferInfo;
			SceNgsPlayerParams *pPcmParams;

			ret = sceNgsVoiceLockParams(src->m_voice, SCE_NGS_SIMPLE_VOICE_PCM_PLAYER, SCE_NGS_PLAYER_PARAMS_STRUCT_ID, &bufferInfo);
			if (ret != SCE_NGS_OK)
			{
				AL_SET_ERROR(_alErrorNgs2Al(ret));
				return;
			}

			pPcmParams = (SceNgsPlayerParams *)bufferInfo.data;

			for (Buffer *buf : ctx->m_bufferStack)
			{
				if (pPcmParams->buffs[0].pBuffer == buf->m_storage)
				{
					*value = _alNamedObjectGetName((ALint)buf);
					break;
				}
			}

			ret = sceNgsVoiceUnlockParams(src->m_voice, SCE_NGS_SIMPLE_VOICE_PCM_PLAYER);
			if (ret != SCE_NGS_OK)
			{
				AL_SET_ERROR(_alErrorNgs2Al(ret));
				return;
			}
		}
		else
		{
			AL_SET_ERROR(AL_INVALID_OPERATION);
			return;
		}
		break;
	case AL_SOURCE_STATE:
		ret = _alErrorNgs2Al(sceNgsVoiceGetInfo(src->m_voice, &info));
		if (ret != AL_NO_ERROR)
		{
			AL_SET_ERROR(ret);
			return;
		}
		*value = _alSourceStateNgs2Al(info.uVoiceState);
		break;
	case AL_BUFFERS_QUEUED:
		*value = src->queuedBufferCount();
		break;
	case AL_BUFFERS_PROCESSED:
		*value = src->processedBufferCount();
		break;
	default:
		AL_SET_ERROR(AL_INVALID_ENUM);
		break;
	}
}

AL_API void AL_APIENTRY alGetSource3i(ALuint sid, ALenum param, ALint* value1, ALint* value2, ALint* value3)
{
	ALfloat ret[3];

	AL_TRACE_CALL

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
	AL_TRACE_CALL

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
	Context *ctx = (Context *)alcGetCurrentContext();

	AL_TRACE_CALL

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	if (sids == NULL)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	sceNgsSystemLock(ctx->m_system);
	for (int i = 0; i < ns; i++)
	{
		alSourcePlay(sids[i]);
	}
	sceNgsSystemUnlock(ctx->m_system);
}

AL_API void AL_APIENTRY alSourceStopv(ALsizei ns, const ALuint *sids)
{
	Context *ctx = (Context *)alcGetCurrentContext();

	AL_TRACE_CALL

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	if (sids == NULL)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	sceNgsSystemLock(ctx->m_system);
	for (int i = 0; i < ns; i++)
	{
		alSourceStop(sids[i]);
	}
	sceNgsSystemUnlock(ctx->m_system);
}

AL_API void AL_APIENTRY alSourceRewindv(ALsizei ns, const ALuint *sids)
{
	Context *ctx = (Context *)alcGetCurrentContext();

	AL_TRACE_CALL

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	if (sids == NULL)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	sceNgsSystemLock(ctx->m_system);
	for (int i = 0; i < ns; i++)
	{
		alSourceRewind(sids[i]);
	}
	sceNgsSystemUnlock(ctx->m_system);
}

AL_API void AL_APIENTRY alSourcePausev(ALsizei ns, const ALuint *sids)
{
	Context *ctx = (Context *)alcGetCurrentContext();

	AL_TRACE_CALL

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	if (sids == NULL)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	sceNgsSystemLock(ctx->m_system);
	for (int i = 0; i < ns; i++)
	{
		alSourcePause(sids[i]);
	}
	sceNgsSystemUnlock(ctx->m_system);
}

AL_API void AL_APIENTRY alSourcePlay(ALuint sid)
{
	ALint ret = AL_NO_ERROR;
	SceNgsVoiceInfo info;
	Source *src = NULL;
	Context *ctx = (Context *)alcGetCurrentContext();

	AL_TRACE_CALL

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	src = (Source *)_alNamedObjectGet(sid);

	if (!Source::validate(src))
	{
		AL_SET_ERROR(AL_INVALID_NAME);
		return;
	}

	ret = _alErrorNgs2Al(sceNgsVoiceGetInfo(src->m_voice, &info));
	if (ret != AL_NO_ERROR)
	{
		AL_SET_ERROR(ret);
		return;
	}

	if (_alSourceStateNgs2Al(info.uVoiceState) == AL_PAUSED)
	{
		sceNgsVoiceResume(src->m_voice);
	}
	else
	{
		if (src->m_needOffsetsReset == AL_FALSE)
		{
			src->m_needOffsetsReset = AL_TRUE;
		}
		else
		{
			src->m_offsetSec = 0.0f;
			src->m_offsetSample = 0.0f;
			src->m_offsetByte = 0.0f;
			src->m_needOffsetsReset = AL_FALSE;
		}

		ret = _alErrorNgs2Al(sceNgsVoiceSetModuleCallback(src->m_voice, SCE_NGS_SIMPLE_VOICE_PCM_PLAYER, Source::streamCallback, src));
		if (ret != AL_NO_ERROR)
		{
			AL_SET_ERROR(ret);
			return;
		}

		sceNgsVoicePlay(src->m_voice);
	}
}

AL_API void AL_APIENTRY alSourceStop(ALuint sid)
{
	Source *src = NULL;
	Context *ctx = (Context *)alcGetCurrentContext();

	AL_TRACE_CALL

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	src = (Source *)_alNamedObjectGet(sid);

	if (!Source::validate(src))
	{
		AL_SET_ERROR(AL_INVALID_NAME);
		return;
	}

	sceNgsVoiceSetModuleCallback(src->m_voice, SCE_NGS_SIMPLE_VOICE_PCM_PLAYER, SCE_NGS_NO_CALLBACK, NULL);

	sceNgsVoiceKeyOff(src->m_voice);

	src->m_offsetSec = 0.0f;
	src->m_offsetSample = 0.0f;
	src->m_offsetByte = 0.0f;
}

AL_API void AL_APIENTRY alSourceRewind(ALuint sid)
{
	Source *src = NULL;
	Context *ctx = (Context *)alcGetCurrentContext();

	AL_TRACE_CALL

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	src = (Source *)_alNamedObjectGet(sid);

	if (!Source::validate(src))
	{
		AL_SET_ERROR(AL_INVALID_NAME);
		return;
	}

	AL_WARNING("alSourceRewind is not implemented\n");

	src->m_offsetSec = 0.0f;
	src->m_offsetSample = 0.0f;
	src->m_offsetByte = 0.0f;
}

AL_API void AL_APIENTRY alSourcePause(ALuint sid)
{
	Source *src = NULL;
	Context *ctx = (Context *)alcGetCurrentContext();

	AL_TRACE_CALL

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	src = (Source *)_alNamedObjectGet(sid);

	if (!Source::validate(src))
	{
		AL_SET_ERROR(AL_INVALID_NAME);
		return;
	}

	sceNgsVoicePause(src->m_voice);
}

AL_API void AL_APIENTRY alSourceQueueBuffers(ALuint sid, ALsizei numEntries, const ALuint *bids)
{
	Source *src = NULL;
	Buffer *buf = NULL;
	Context *ctx = (Context *)alcGetCurrentContext();
	ALint ret = AL_NO_ERROR;
	ALint buffersInQueue = 0;
	ALint frequency = 0;
	ALint bits = 0;
	ALint channels = 0;

	AL_TRACE_CALL

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

	src = (Source *)_alNamedObjectGet(sid);

	if (!Source::validate(src))
	{
		AL_SET_ERROR(AL_INVALID_NAME);
		return;
	}

	if (src->m_altype == AL_STATIC)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	buffersInQueue = src->queuedBufferCount();

	if (numEntries > SCE_NGS_PLAYER_MAX_BUFFERS - buffersInQueue)
	{
		AL_WARNING("Attempt to enqueue more than (SCE_NGS_PLAYER_MAX_BUFFERS(%d) - src->queuedBufferCount()(%d)) buffers\n", SCE_NGS_PLAYER_MAX_BUFFERS, buffersInQueue);
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	for (int i = 0; i < numEntries; i++)
	{
		if (bids[i] == 0)
			continue;

		if (buf == NULL)
		{
			buf = (Buffer *)_alNamedObjectGet(bids[i]);
			frequency = buf->m_frequency;
			bits = buf->m_bits;
			channels = buf->m_channels;
		}
		else
		{
			buf = (Buffer *)_alNamedObjectGet(bids[i]);
			if (frequency != buf->m_frequency || bits != buf->m_bits || channels != buf->m_channels)
			{
				AL_SET_ERROR(AL_INVALID_VALUE);
				return;
			}
		}
	}

	buf = NULL;

	for (int i = 0; i < numEntries; i++)
	{
		if (bids[i] == 0)
			continue;

		buf = (Buffer *)_alNamedObjectGet(bids[i]);

		buf->ref();

		ret = src->bqPush(frequency, channels, buf);
		if (ret != AL_NO_ERROR)
		{
			AL_SET_ERROR(ret);
			return;
		}
	}

	src->m_altype = AL_STREAMING;
}

AL_API void AL_APIENTRY alSourceUnqueueBuffers(ALuint sid, ALsizei numEntries, ALuint *bids)
{
	Source *src = NULL;
	Buffer *buf = NULL;
	Context *ctx = (Context *)alcGetCurrentContext();
	SceInt32 ret = SCE_NGS_OK;
	SceNgsBufferInfo   bufferInfo;
	SceNgsPlayerParams *pPcmParams;

	AL_TRACE_CALL

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

	src = (Source *)_alNamedObjectGet(sid);

	if (!Source::validate(src))
	{
		AL_SET_ERROR(AL_INVALID_NAME);
		return;
	}

	if (src->m_altype == AL_STATIC)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	if (src->processedBufferCount() < numEntries)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	ALint flags = sceAtomicLoad32AcqRel(&src->m_processedBuffers);
	ALint flagToCheck = -1;
	ALint outCount = 0;

	ret = sceNgsVoiceLockParams(src->m_voice, SCE_NGS_SIMPLE_VOICE_PCM_PLAYER, SCE_NGS_PLAYER_PARAMS_STRUCT_ID, &bufferInfo);
	if (ret != SCE_NGS_OK)
	{
		AL_SET_ERROR(_alErrorNgs2Al(ret));
		return;
	}

	pPcmParams = (SceNgsPlayerParams *)bufferInfo.data;

	for (int i = 0; i < SCE_NGS_PLAYER_MAX_BUFFERS; i++)
	{
		flagToCheck = 1 << i;

		if (flags & flagToCheck)
		{
			for (Buffer *buf : ctx->m_bufferStack)
			{
				if (pPcmParams->buffs[i].pBuffer == buf->m_storage)
				{
					buf->deref();
					if (buf->m_refCounter == 0)
					{
						buf->m_state = AL_UNUSED;
					}

					bids[outCount] = _alNamedObjectGetName((ALint)buf);
					outCount++;
					break;
				}
			}
			pPcmParams->buffs[i].pBuffer = NULL;
			pPcmParams->buffs[i].nNumBytes = 0;
			pPcmParams->buffs[i].nLoopCount = 0;
			pPcmParams->buffs[i].nNextBuff = SCE_NGS_PLAYER_NO_NEXT_BUFFER;
			src->m_processedBuffers &= ~flagToCheck;
		}
	}

	ret = sceNgsVoiceUnlockParams(src->m_voice, SCE_NGS_SIMPLE_VOICE_PCM_PLAYER);
	if (ret != SCE_NGS_OK)
	{
		AL_SET_ERROR(_alErrorNgs2Al(ret));
		return;
	}
}