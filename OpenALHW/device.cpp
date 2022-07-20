#include <heatwave.h>
#include <stdlib.h>
#include <string.h>
#include <audioin.h>

#include "common.h"
#include "device.h"

using namespace al;

Device::Device()
	: m_magic(SCE_HEATWAVE_API_VERSION),
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
	if (m_magic == SCE_HEATWAVE_API_VERSION)
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

ALCboolean DeviceHeatWave::validate(ALCdevice *device)
{
	DeviceHeatWave *dev = NULL;

	if (!device)
	{
		return ALC_FALSE;
	}

	dev = (DeviceHeatWave *)device;

	if (dev->getType() != DeviceType_HeatWave)
	{
		return ALC_FALSE;
	}

	return ALC_TRUE;
}

DeviceHeatWave::DeviceHeatWave()
	: m_at9ChCount(0),
	m_mp3ChCount(0)
{
	m_type = DeviceType_HeatWave;
}

DeviceHeatWave::~DeviceHeatWave()
{
	destroyContext();
}

ALCvoid DeviceHeatWave::setMp3ChannelCount(ALCint count)
{
	m_mp3ChCount = count;
}

ALCvoid DeviceHeatWave::setAt9ChannelCount(ALCint count)
{
	m_at9ChCount = count;
}

ALCboolean DeviceHeatWave::validateAttributes(const ALCint* attrlist)
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

ALCvoid DeviceHeatWave::setAttributes(const ALCint* attrlist)
{

}

ALCint DeviceHeatWave::createContext()
{
	SceHwError res = SCE_HEATWAVE_API_OK;
	SceHwInitParams iparam;

	memset(&iparam, 0, sizeof(SceHwInitParams));
	iparam.bLoopBack = false;
	iparam.nMaxChannelNumAt9 = m_at9ChCount;
	iparam.nMaxChannelNumMp3 = m_mp3ChCount;

	m_ctx = new Context(this);

	res = sceHeatWaveInit(&iparam);
	if (SCE_HEATWAVE_API_FAILED(res))
	{
		delete m_ctx;
	}

	m_isInitialized = ALC_TRUE;
	
	return _alErrorHw2Al(res);
}

ALCvoid DeviceHeatWave::destroyContext()
{
	if (m_isInitialized == AL_TRUE)
	{
		sceHeatWaveRelease();
	}

	if (m_ctx != NULL)
	{
		delete m_ctx;
	}

	m_isInitialized = ALC_FALSE;
}

const ALCchar *DeviceHeatWave::getExtensionList()
{
	return "ALC_NGS_DECODE_CHANNEL_COUNT";
}

const ALCchar *DeviceHeatWave::getName()
{
	return AL_DEVICE_NAME;
}

ALCint DeviceHeatWave::getAttributeCount()
{
	return 5;
}

ALCint DeviceHeatWave::getAttribute(ALCenum attr)
{
	ALCint ret = -1000;

	switch (attr) {
	case ALC_FREQUENCY:
		ret = m_samplingFrequency;
		break;
	case ALC_MONO_SOURCES:
		ret = m_monoSources;
		break;
	case ALC_REFRESH:
		ret = m_refreshRate;
		break;
	case ALC_STEREO_SOURCES:
		ret = m_stereoSources;
		break;
	case ALC_SYNC:
		ret = m_sync;
		break;
	}

	return ret;
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
	m_buffer = malloc(m_buffersize);
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
		free(m_buffer);
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