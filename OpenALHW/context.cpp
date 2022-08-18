#include <kernel.h>
#include <audioout.h>
#include <string.h>
#include <ngs.h>

#include "common.h"
#include "device.h"
#include "panner.h"
#include "context.h"

using namespace al;

Context::Context(Device *device)
{
	m_dev = device;
	m_voiceDef = sceNgsVoiceDefGetSimpleVoice();
	m_sysMem = NULL;
	m_masterRackMem.data = NULL;
	m_masterRackMem.size = NULL;
	m_ngsOutThread = SCE_UID_INVALID_UID;
	m_outActive = ALC_TRUE;

	m_listenerPosition.x = 0.0f;
	m_listenerPosition.y = 0.0f;
	m_listenerPosition.z = 0.0f;

	m_listenerVelocity.x = 0.0f;
	m_listenerVelocity.y = 0.0f;
	m_listenerVelocity.z = 0.0f;

	m_listenerForward.x = 0.0f;
	m_listenerForward.y = 0.0f;
	m_listenerForward.z = -1.0f;

	m_listenerUp.x = 0.0f;
	m_listenerUp.y = 1.0f;
	m_listenerUp.z = 0.0f;

	m_listenerGain = 1.0f;

	DeviceNGS *ngsDev = (DeviceNGS *)m_dev;
	SceInt32 ret = SCE_OK;
	SceSize reqSize = 0;
	SceNgsRackDescription masterRackDesc;
	SceNgsRackDescription sourceRackDesc;
	SceNgsSystemInitParams initParams;
	SceInt32 totalVoiceCount = ngsDev->getMaxMonoVoiceCount() + ngsDev->getMaxStereoVoiceCount();

	m_voiceUsed = (bool *)AL_MALLOC(sizeof(bool) * totalVoiceCount);

	memset(m_voiceUsed, 0, sizeof(bool) * totalVoiceCount);

	initParams.nMaxRacks = 2;
	initParams.nMaxVoices = totalVoiceCount + 1;
	initParams.nGranularity = NGS_SYSTEM_GRANULARITY;
	initParams.nSampleRate = ngsDev->getSamplingFrequency();
	initParams.nMaxModules = 14;

	ret = sceNgsSystemGetRequiredMemorySize(&initParams, &reqSize);
	if (ret != SCE_NGS_OK)
	{
		AL_SET_ERROR(_alErrorNgs2Al(ret));
		return;
	}

	m_sysMem = AL_MEMALIGN(SCE_NGS_MEMORY_ALIGN_SIZE, reqSize);
	if (m_sysMem == NULL)
	{
		AL_SET_ERROR(ALC_OUT_OF_MEMORY);
		return;
	}

	ret = sceNgsSystemInit(m_sysMem, reqSize, &initParams, &m_system);
	if (ret != SCE_NGS_OK)
	{
		AL_SET_ERROR(_alErrorNgs2Al(ret));
		return;
	}

	sourceRackDesc.nChannelsPerVoice = 2;
	sourceRackDesc.nVoices = totalVoiceCount;
	sourceRackDesc.pVoiceDefn = sceNgsVoiceDefGetSimpleVoice();
	sourceRackDesc.nMaxPatchesPerInput = 0;
	sourceRackDesc.nPatchesPerOutput = 1;

	ret = sceNgsRackGetRequiredMemorySize(m_system, &sourceRackDesc, &m_sourceRackMem.size);
	if (ret != SCE_NGS_OK)
	{
		AL_SET_ERROR(_alErrorNgs2Al(ret));
		return;
	}

	m_sourceRackMem.data = AL_MEMALIGN(SCE_NGS_MEMORY_ALIGN_SIZE, m_sourceRackMem.size);
	if (m_sourceRackMem.data == NULL)
	{
		AL_SET_ERROR(ALC_OUT_OF_MEMORY);
		return;
	}

	memset(m_sourceRackMem.data, 0, m_sourceRackMem.size);

	ret = sceNgsRackInit(m_system, &m_sourceRackMem, &sourceRackDesc, &m_sourceRack);
	if (ret != SCE_NGS_OK)
	{
		AL_SET_ERROR(_alErrorNgs2Al(ret));
		return;
	}

	masterRackDesc.nChannelsPerVoice = 2;
	masterRackDesc.nVoices = 1;
	masterRackDesc.pVoiceDefn = sceNgsVoiceDefGetMasterBuss();
	masterRackDesc.nMaxPatchesPerInput = totalVoiceCount;
	masterRackDesc.nPatchesPerOutput = 0;

	ret = sceNgsRackGetRequiredMemorySize(m_system, &masterRackDesc, &m_masterRackMem.size);
	if (ret != SCE_NGS_OK)
	{
		AL_SET_ERROR(_alErrorNgs2Al(ret));
		return;
	}

	m_masterRackMem.data = AL_MEMALIGN(SCE_NGS_MEMORY_ALIGN_SIZE, m_masterRackMem.size);
	if (m_masterRackMem.data == NULL)
	{
		AL_SET_ERROR(ALC_OUT_OF_MEMORY);
		return;
	}

	memset(m_masterRackMem.data, 0, m_masterRackMem.size);

	ret = sceNgsRackInit(m_system, &m_masterRackMem, &masterRackDesc, &m_masterRack);
	if (ret != SCE_NGS_OK)
	{
		AL_SET_ERROR(_alErrorNgs2Al(ret));
		return;
	}

	ret = sceNgsRackGetVoiceHandle(m_masterRack, 0, &m_masterVoice);
	if (ret != SCE_NGS_OK)
	{
		AL_SET_ERROR(_alErrorNgs2Al(ret));
		return;
	}

	ret = sceNgsVoicePlay(m_masterVoice);
	if (ret != SCE_NGS_OK)
	{
		AL_SET_ERROR(_alErrorNgs2Al(ret));
		return;
	}

	m_ngsOutThread = sceKernelCreateThread("OpenALHW::NGSOut", outputThread, SCE_KERNEL_HIGHEST_PRIORITY_USER, SCE_KERNEL_4KiB, 0, ngsDev->getOutputThreadAffinity(), NULL);
	if (m_ngsOutThread <= 0)
	{
		AL_SET_ERROR(ALC_INVALID_VALUE);
		return;
	}

	m_ngsUpdateThread = sceKernelCreateThread("OpenALHW::NGSUpdate", updateThread, SCE_KERNEL_HIGHEST_PRIORITY_USER + 1, SCE_KERNEL_4KiB, 0, ngsDev->getUpdateThreadAffinity(), NULL);
	if (m_ngsOutThread <= 0)
	{
		AL_SET_ERROR(ALC_INVALID_VALUE);
		return;
	}

	Context *argptr = this;

	ret = sceKernelStartThread(m_ngsOutThread, sizeof(Context **), &argptr);
	if (ret != SCE_OK)
	{
		AL_SET_ERROR(ALC_INVALID_VALUE);
		return;
	}

	ret = sceKernelStartThread(m_ngsUpdateThread, sizeof(Context **), &argptr);
	if (ret != SCE_OK)
	{
		AL_SET_ERROR(ALC_INVALID_VALUE);
		return;
	}

	ret = sceKernelCreateLwMutex(&m_lock, "OpenALHW::SysMtx", 0, 0, NULL);
	if (ret != SCE_OK)
	{
		AL_SET_ERROR(ALC_INVALID_VALUE);
		return;
	}
}

Context::~Context()
{
	sceNgsVoiceKeyOff(m_masterVoice);

	m_outActive = ALC_FALSE;

	sceKernelWaitThreadEnd(m_ngsOutThread, NULL, NULL);
	sceKernelWaitThreadEnd(m_ngsUpdateThread, NULL, NULL);

	sceNgsSystemRelease(m_system);

	sceKernelDeleteLwMutex(&m_lock);

	if (m_sysMem)
		AL_FREE(m_sysMem);
	if (m_masterRackMem.data)
		AL_FREE(m_masterRackMem.data);
	if (m_sourceRackMem.data)
		AL_FREE(m_sourceRackMem.data);
	if (m_voiceUsed)
		AL_FREE(m_voiceUsed);
}

SceInt32 Context::outputThread(SceSize argSize, void *pArgBlock)
{
	Context *ctx = *(Context **)pArgBlock;
	DeviceNGS *ngsDev = (DeviceNGS *)ctx->getDevice();
	SceInt32 portId = -1;
	ScePVoid outputBuffer[2];
	SceInt32 bufferIndex = 0;

	portId = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_MAIN,
		NGS_SYSTEM_GRANULARITY,
		ngsDev->getSamplingFrequency(),
		SCE_AUDIO_OUT_PARAM_FORMAT_S16_STEREO);

	SceInt32 volume[2] = { SCE_AUDIO_VOLUME_0dB, SCE_AUDIO_VOLUME_0dB };
	sceAudioOutSetVolume(portId, (SCE_AUDIO_VOLUME_FLAG_L_CH | SCE_AUDIO_VOLUME_FLAG_R_CH), volume);

	for (int i = 0; i < 2; i++) {
		outputBuffer[i] = (short *)AL_MALLOC(NGS_SYSTEM_GRANULARITY * 2 * sizeof(short));
	}

	while ((volatile ALCboolean)ctx->m_outActive)
	{
		sceNgsSystemUpdate(ctx->m_system);

		sceNgsVoiceGetStateData(ctx->m_masterVoice, SCE_NGS_MASTER_BUSS_OUTPUT_MODULE,
			outputBuffer[bufferIndex], sizeof(short) * NGS_SYSTEM_GRANULARITY * 2);

		sceAudioOutOutput(portId, outputBuffer[bufferIndex]);

		bufferIndex ^= 1;
	}

	sceAudioOutReleasePort(portId);

	for (int i = 0; i < 2; i++) {
		AL_FREE(outputBuffer[i]);
	}

	return sceKernelExitDeleteThread(0);
}

SceInt32 Context::updateThread(SceSize argSize, void *pArgBlock)
{
	Context *ctx = *(Context **)pArgBlock;
	DeviceNGS *ngsDev = (DeviceNGS *)ctx->getDevice();

	while ((volatile ALCboolean)ctx->m_outActive)
	{
		sceKernelLockLwMutex(&ctx->m_lock, 1, NULL);
		for (Source *src : ctx->m_sourceStack)
		{
			src->update();
		}
		sceKernelUnlockLwMutex(&ctx->m_lock, 1);

		sceKernelDelayThread(NGS_UPDATE_INTERVAL_US);
	}

	return sceKernelExitDeleteThread(0);
}

ALvoid Context::beginParamUpdate()
{
	sceKernelLockLwMutex(&m_lock, 1, NULL);
}

ALvoid Context::endParamUpdate()
{
	markAllAsDirty();
	sceKernelUnlockLwMutex(&m_lock, 1);
}

ALvoid Context::markAllAsDirty()
{
	for (Source *src : m_sourceStack)
	{
		src->m_paramsDirty = AL_TRUE;
	}
}

ALint Context::suspend()
{
	return _alErrorNgs2Al(sceNgsSystemLock(m_system));
}

ALint Context::resume()
{
	return _alErrorNgs2Al(sceNgsSystemUnlock(m_system));
}

ALCboolean Context::validate(ALCcontext *context)
{
	Context *ctx = NULL;

	if (!context)
	{
		return ALC_FALSE;
	}

	ctx = (Context *)context;

	if (ctx->getDevice() == NULL)
	{
		return ALC_FALSE;
	}

	return ALC_TRUE;
}

Device *Context::getDevice()
{
	return m_dev;
}