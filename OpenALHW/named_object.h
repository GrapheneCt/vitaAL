#ifndef AL_NAMED_OBJECT_H
#define AL_NAMED_OBJECT_H

#include <kernel.h>
#include <ngs.h>
#include <vector>

#include "common.h"

namespace al {

	class Context;

	enum ObjectType
	{
		ObjectType_None = AL_INTERNAL_MAGIC - 1,
		ObjectType_Buffer = AL_INTERNAL_MAGIC - 2,
		ObjectType_Source = AL_INTERNAL_MAGIC - 3
	};

	class SourceParams
	{
	public:
		SourceParams();

		SceFVector4 vPosition;
		SceFVector4 vVelocity;
		SceFVector4 vForward;
		float32_t   fMinDistance;
		float32_t   fMaxDistance;
		float32_t   fInsideAngle;
		float32_t   fOutsideAngle;
		float32_t   fOutsideGain;
		float32_t   fOutsideFreq;
		bool        bListenerRelative;
		float32_t   fDistanceFactor;
		float32_t   fPitchMul;
		float32_t   fGainMul;
	};

	class NamedObject
	{
	public:

		NamedObject() : m_type(ObjectType_None), m_magic(AL_INTERNAL_MAGIC), m_initialized(AL_FALSE)
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
			if (m_magic == AL_INTERNAL_MAGIC)
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

		ALint m_frequency;
		ALint m_bits;
		ALint m_channels;

		ALint m_data;			// the useless one

		ALint m_size;
		ALvoid *m_storage;	// the actual one

		ALint m_state;

		ALint m_refCounter;

	};

	class Source : public NamedObject
	{
	public:

		#define NGS_BUFFER_IDX_0		(1)
		#define NGS_BUFFER_IDX_1		(2)
		#define NGS_BUFFER_IDX_2		(4)
		#define NGS_BUFFER_IDX_3		(8)

		static ALboolean validate(Source *src);
		static ALvoid streamCallback(const SceNgsCallbackInfo *pCallbackInfo);

		Source(Context *ctx);
		~Source();

		ALint init();
		ALint release();
		ALint dropAllBuffers();
		ALint bqPush(ALint frequency, ALint channels, Buffer *buf);
		ALint switchToStaticBuffer(ALint frequency, ALint channels, Buffer *buf);
		ALvoid update();
		ALvoid beginParamUpdate();
		ALvoid endParamUpdate();
		ALint processedBufferCount();
		ALint queuedBufferCount();

		SceNgsHVoice m_voice;
		SceNgsHPatch m_patch;
		SourceParams m_params;

		ALint m_voiceIdx;

		ALfloat m_minGain;
		ALfloat m_maxGain;
		ALfloat m_offsetSec;
		ALfloat m_offsetSample;
		ALfloat m_offsetByte;
		ALboolean m_looping;
		ALint m_altype;
		SceKernelLwMutexWork m_lock;

		volatile ALint m_processedBuffers;
		volatile ALint m_queueBuffers;

		ALint m_lastPushedIdx;
		ALint m_curIdx;

		ALboolean m_needOffsetsReset;
		ALboolean m_paramsDirty;

	private:

		Context *m_ctx;
	};
}

#endif
