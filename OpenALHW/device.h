#include "AL/al.h"
#include "AL/alc.h"
#include "context.h"

#ifndef AL_DEVICE_H
#define AL_DEVICE_H

namespace al {

	enum DeviceType
	{
		DeviceType_None,
		DeviceType_HeatWave,
		DeviceType_AudioIn
	};

	class Device
	{
	public:

		Device();
		virtual ~Device();

		virtual ALCint createContext() =0;
		virtual ALCvoid destroyContext() =0;
		virtual const ALCchar *getExtensionList() =0;
		virtual const ALCchar *getName() =0;
		virtual ALCint getAttributeCount() =0;
		virtual ALCint getAttribute(ALCenum attr) =0;

		ALCboolean isValid();
		ALCboolean isInitialized();
		DeviceType getType();
		Context *getContext();

	protected:

		ALCint m_magic;
		DeviceType m_type;
		ALCboolean m_isInitialized;
		Context *m_ctx;
	};

	class DeviceAudioIn : public Device
	{
	public:

		static ALCboolean validate(ALCdevice *device);

		DeviceAudioIn();
		~DeviceAudioIn();

		ALCint createContext();
		ALCvoid destroyContext();
		const ALCchar *getExtensionList();
		const ALCchar *getName();
		ALCint getAttributeCount();
		ALCint getAttribute(ALCenum attr);

		ALCboolean validateAttributes(ALCuint frequency, ALCenum format, ALCsizei buffersize);
		ALCvoid setAttributes(ALCuint frequency, ALCenum format, ALCsizei buffersize);
		ALCboolean start();
		ALCboolean stop();
		ALCboolean captureSamples(ALCvoid *buffer, ALCsizei samples);

	private:

		static constexpr ALCchar k_alcExtensionList[] =
			"";

		ALCuint m_samplingFrequency;
		const ALCenum m_format = AL_FORMAT_MONO16;
		ALCsizei m_buffersize;
		ALCint m_port;
		ALCvoid *m_buffer;
	};

	class DeviceHeatWave : public Device
	{
	public:

		static ALCboolean validate(ALCdevice *device);

		DeviceHeatWave();
		~DeviceHeatWave();

		ALCint createContext();
		ALCvoid destroyContext();
		const ALCchar *getExtensionList();
		const ALCchar *getName();
		ALCint getAttributeCount();
		ALCint getAttribute(ALCenum attr);

		ALCboolean validateAttributes(const ALCint* attrlist);
		ALCvoid setAttributes(const ALCint* attrlist);
		ALCvoid setMp3ChannelCount(ALCint count);
		ALCvoid setAt9ChannelCount(ALCint count);


	private:

		static constexpr ALCchar k_alcExtensionList[] =
			"ALC_NGS_DECODE_CHANNEL_COUNT";

		ALCint m_mp3ChCount;
		ALCint m_at9ChCount;

		const ALCint k_maxMonoChannels = 64;
		const ALCint k_maxStereoChannels = 64;
		const ALCint m_samplingFrequency = 48000;
		const ALCint m_monoSources = k_maxMonoChannels;
		const ALCint m_stereoSources = k_maxStereoChannels;
		const ALCint m_refreshRate = 60;
		const ALCint m_sync = 0;
	};
}

#endif