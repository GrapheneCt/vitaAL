#include <kernel.h>
#include <libsysmodule.h>
#include <ngs.h>
#include <audioin.h>
#include <string.h>

#include "common.h"
#include "device.h"

using namespace al;

Device::Device()
	: m_magic(AL_INTERNAL_MAGIC),
	m_isInitialized(AL_FALSE),
	m_type(DeviceType_None),
	m_ctx(NULL)
{

}

Device::~Device()
{

}

ALCboolean Device::isValid()
{
	if (m_magic == AL_INTERNAL_MAGIC)
		return AL_TRUE;

	return AL_FALSE;
}

ALCboolean Device::isInitialized()
{
	return m_isInitialized;
}

DeviceType Device::getType()
{
	return m_type;
}

Context *Device::getContext()
{
	return m_ctx;
}

ALCboolean DeviceNGS::validate(ALCdevice *device)
{
	DeviceNGS *dev = NULL;

	if (!device)
	{
		return ALC_FALSE;
	}

	dev = (DeviceNGS *)device;

	if (dev->getType() != DeviceType_NGS)
	{
		return ALC_FALSE;
	}

	return ALC_TRUE;
}

DeviceNGS::DeviceNGS()
	: m_maxMonoVoices(k_maxMonoChannels),
	m_maxStereoVoices(k_maxStereoChannels),
	m_outputThreadAffinity(SCE_KERNEL_CPU_MASK_USER_2)
{
	m_type = DeviceType_NGS;
}

DeviceNGS::~DeviceNGS()
{
	destroyContext();
}

ALCboolean DeviceNGS::validateAttributes(const ALCint* attrlist)
{
	ALCint currAttr = *attrlist;

	while (currAttr != 0)
	{
		attrlist += sizeof(ALCint);

		switch (currAttr) {
		case ALC_MONO_SOURCES:
			if (*attrlist > k_maxMonoChannels)
			{
				return ALC_FALSE;
			}
			break;
		case ALC_STEREO_SOURCES:
			if (*attrlist > k_maxStereoChannels)
			{
				return ALC_FALSE;
			}
			break;
		case ALC_SYNC:
			if (*attrlist != ALC_FALSE)
			{
				return ALC_FALSE;
			}
			break;
		}

		attrlist += sizeof(ALCint);
		currAttr = *attrlist;
	}

	return ALC_TRUE;
}

ALCvoid DeviceNGS::setAttributes(const ALCint* attrlist)
{
	ALCint currAttr = *attrlist;

	while (currAttr != 0)
	{
		attrlist += sizeof(ALCint);

		switch (currAttr) {
		case ALC_MONO_SOURCES:
			m_maxMonoVoices = *attrlist;
			break;
		case ALC_STEREO_SOURCES:
			m_maxStereoVoices = *attrlist;
			break;
		}

		attrlist += sizeof(ALCint);
		currAttr = *attrlist;
	}
}

ALCint DeviceNGS::createContext()
{
	SceInt32 ret = SCE_OK;

	if (sceSysmoduleIsLoaded(SCE_SYSMODULE_NGS))
	{
		ret = sceSysmoduleLoadModule(SCE_SYSMODULE_NGS);
		if (ret != SCE_OK)
		{
			return ALC_INVALID_VALUE;
		}
	}

	m_ctx = new Context(this);

	m_isInitialized = ALC_TRUE;
	
	return ALC_NO_ERROR;
}

ALCvoid DeviceNGS::destroyContext()
{
	if (m_ctx != NULL)
	{
		delete m_ctx;
	}

	if (m_isInitialized == AL_TRUE)
	{
		if (!sceSysmoduleIsLoaded(SCE_SYSMODULE_NGS))
		{
			sceSysmoduleUnloadModule(SCE_SYSMODULE_NGS);
		}
	}

	m_isInitialized = ALC_FALSE;
}

const ALCchar *DeviceNGS::getExtensionList()
{
	return "ALC_NGS_THREAD_AFFINITY";
}

const ALCchar *DeviceNGS::getName()
{
	return AL_DEVICE_NAME;
}

ALCint DeviceNGS::getAttributeCount()
{
	return 5;
}

ALCint DeviceNGS::getAttribute(ALCenum attr)
{
	ALCint ret = -1000;

	switch (attr) {
	case ALC_FREQUENCY:
		ret = m_samplingFrequency;
		break;
	case ALC_MONO_SOURCES:
		ret = m_maxMonoVoices;
		break;
	case ALC_REFRESH:
		ret = m_refreshRate;
		break;
	case ALC_STEREO_SOURCES:
		ret = m_maxStereoVoices;
		break;
	case ALC_SYNC:
		ret = m_sync;
		break;
	}

	return ret;
}

ALCvoid DeviceNGS::setThreadAffinity(ALCuint outputThreadAffinity)
{
	m_outputThreadAffinity = outputThreadAffinity;
}

ALCuint DeviceNGS::getOutputThreadAffinity()
{
	return m_outputThreadAffinity;
}

ALCint DeviceNGS::getMaxMonoVoiceCount()
{
	return m_maxMonoVoices;
}

ALCint DeviceNGS::getMaxStereoVoiceCount()
{
	return m_maxStereoVoices;
}

ALCint DeviceNGS::getSamplingFrequency()
{
	return m_samplingFrequency;
}

ALCboolean DeviceAudioIn::validate(ALCdevice *device)
{
	DeviceAudioIn *dev = NULL;

	if (!device)
	{
		return ALC_FALSE;
	}

	dev = (DeviceAudioIn *)device;

	if (dev->getType() != DeviceType_AudioIn)
	{
		return ALC_FALSE;
	}

	return ALC_TRUE;
}

DeviceAudioIn::DeviceAudioIn()
	: m_samplingFrequency(48000),
	m_buffersize(768 * sizeof(int16_t)),
	m_port(-1)
{
	m_type = DeviceType_AudioIn;
}

DeviceAudioIn::~DeviceAudioIn()
{
	destroyContext();
}

ALCboolean DeviceAudioIn::validateAttributes(ALCuint frequency, ALCenum format, ALCsizei buffersize)
{
	if (frequency != 16000 && frequency != 48000)
	{
		return ALC_FALSE;
	}

	if (format != AL_FORMAT_MONO16)
	{
		return ALC_FALSE;
	}

	if (frequency == 16000 && buffersize != 256 * sizeof(int16_t))
	{
		return ALC_FALSE;
	}

	if (frequency == 48000 && buffersize != 768 * sizeof(int16_t))
	{
		return ALC_FALSE;
	}

	return ALC_TRUE;
}

ALCvoid DeviceAudioIn::setAttributes(ALCuint frequency, ALCenum format, ALCsizei buffersize)
{
	m_samplingFrequency = frequency;
	m_buffersize = buffersize;
}

ALCboolean DeviceAudioIn::start()
{
	m_port = sceAudioInOpenPort(SCE_AUDIO_IN_PORT_TYPE_RAW, m_buffersize / sizeof(int16_t), m_samplingFrequency, SCE_AUDIO_IN_PARAM_FORMAT_S16_MONO);
	if (m_port <= 0)
	{
		return ALC_FALSE;
	}

	return ALC_TRUE;
}

ALCboolean DeviceAudioIn::stop()
{
	int ret = 0;

	ret = sceAudioInReleasePort(m_port);
	if (ret != SCE_OK)
	{
		return ALC_FALSE;
	}

	m_port = -1;

	return ALC_TRUE;
}

ALCboolean DeviceAudioIn::captureSamples(ALCvoid *buffer, ALCsizei samples)
{
	int ret = 0;
	ALCsizei byteSize = samples * sizeof(int16_t);

	if (byteSize > m_buffersize)
	{
		return ALC_FALSE;
	}

	ret = sceAudioInInput(m_port, m_buffer);
	if (ret != SCE_OK)
	{
		return ALC_FALSE;
	}

	memcpy(buffer, m_buffer, byteSize);

	return ALC_TRUE;
}

ALCint DeviceAudioIn::createContext()
{
	m_buffer = AL_MALLOC(m_buffersize);
	if (!m_buffer)
	{
		return ALC_OUT_OF_MEMORY;
	}

	m_isInitialized = ALC_TRUE;

	return ALC_NO_ERROR;
}

ALCvoid DeviceAudioIn::destroyContext()
{
	if (m_isInitialized == ALC_TRUE)
	{
		stop();
		AL_FREE(m_buffer);
	}

	m_isInitialized = ALC_FALSE;
}

const ALCchar *DeviceAudioIn::getExtensionList()
{
	return "";
}

const ALCchar *DeviceAudioIn::getName()
{
	return AL_CAPTURE_DEVICE_NAME;
}

ALCint DeviceAudioIn::getAttributeCount()
{
	return 0;
}

ALCint DeviceAudioIn::getAttribute(ALCenum attr)
{
	ALCint ret = -1000;

	return ret;
}