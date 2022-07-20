#ifndef AL_NAMED_OBJECT_H
#define AL_NAMED_OBJECT_H

#include <kernel.h>
#include <vector>

namespace al {

	enum ObjectType
	{
		ObjectType_None,
		ObjectType_Buffer,
		ObjectType_Source
	};

	class NamedObject
	{
	public:

		NamedObject() : m_type(ObjectType_None), m_magic(SCE_HEATWAVE_API_VERSION), m_initialized(AL_FALSE)
		{

		}

		virtual ~NamedObject()
		{

		}

		virtual ALint init() =0;
		virtual ALint release() =0;

		ObjectType getType()
		{
			return m_type;
		}

		ALboolean isValid()
		{
			if (m_magic == SCE_HEATWAVE_API_VERSION)
				return AL_TRUE;

			return AL_FALSE;
		}

		ALboolean isInitialized()
		{
			return m_initialized;
		}


	protected:

		ALint m_magic;
		ObjectType m_type;
		ALboolean m_initialized;
	};

	class Buffer : public NamedObject
	{
	public:

		static ALboolean validate(Buffer *buf);

		Buffer();
		~Buffer();

		ALint init();
		ALint release();

		ALvoid ref();
		ALvoid deref();

		ALint frequency;
		ALint bits;
		ALint channels;

		ALint data;			// the useless one

		ALint size;
		ALvoid *storage;	// the actual one

		ALint trackSize;
		ALchar *trackStorage;

		ALint state;

		ALint refCounter;

	};

	class Source : public NamedObject
	{
	public:

		enum BufferConsumeState
		{
			BufferConsumeState_Remain,
			BufferConsumeState_Exact,
			BufferConsumeState_NeedMore
		};

		static ALboolean validate(Source *src);

		Source();
		~Source();

		ALint init();
		ALint release();
		ALvoid dropAllBuffers();
		ALvoid switchToStaticBuffer(Buffer *buf);
		ALint reloadRuntimeStream(ALint frequency, ALint channels);

		SceHwSource hwSrc;
		SceHwSound hwSnd;
		SceHwSfx hwSfx;

		ALfloat minDistance;
		ALfloat maxDistance;
		ALfloat roloffFactor;
		ALfloat minGain;
		ALfloat maxGain;
		ALfloat coneOuterGain;
		ALfloat coneInnerAngle;
		ALfloat coneOuterAngle;
		ALfloat coneOuterFreq;
		SceHwVector position;
		SceHwVector velocity;
		SceHwVector direction;
		ALfloat offsetSec;
		ALfloat offsetSample;
		ALfloat offsetByte;
		ALboolean positionRelative;
		ALboolean looping;
		ALint type;
		Buffer *staticBuffer;
		std::vector<Buffer*> queuedBuffers;
		std::vector<Buffer*> processedBuffers;
		SceKernelLwMutexWork *lock;
		ALint streamFrequency;
		ALint streamChannels;

		ALboolean needOffsetsReset;

	private:

		static void runtimeStreamEntry(SceHwSourceRuntimeStreamCallbackInfo *pCallbackInfo, void *pUserData, ScewHwSourceRuntimeStreamCallbackType reason);
		static BufferConsumeState consumeBuffer(Buffer *in, ALchar **out, ALuint needBytes, ALuint *remainBytes);
	};
}

#endif
