#include <ngs.h>
#include <vector>

#include "AL/al.h"
#include "AL/alc.h"
#include "named_object.h"
#include "panner.h"

#ifndef AL_CONTEXT_H
#define AL_CONTEXT_H

#define NGS_SYSTEM_GRANULARITY (512)
#define NGS_UPDATE_INTERVAL_US (5000)

namespace al {

	class Device;

	class Context
	{
	public:

		static ALCboolean validate(ALCcontext *ctx);

		Context(Device *device);
		~Context();

		Device *getDevice();
		ALvoid beginParamUpdate();
		ALvoid endParamUpdate();
		ALvoid markAllAsDirty();
		ALint suspend();
		ALint resume();

		float32_t m_listenerGain;
		SceFVector4 m_listenerPosition;
		SceFVector4 m_listenerVelocity;
		SceFVector4 m_listenerForward;
		SceFVector4 m_listenerUp;

		ALCboolean m_outActive;
		SceNgsHSynSystem m_system;
		SceNgsHVoice m_masterVoice;
		SceNgsHRack m_sourceRack;
		std::vector<Source *> m_sourceStack;
		std::vector<Buffer *> m_bufferStack;
		bool *m_voiceUsed;
		SceKernelLwMutexWork m_lock;
		Panner m_panner;

	private:

		static SceInt32 outputThread(SceSize argSize, void *pArgBlock);
		static SceInt32 updateThread(SceSize argSize, void *pArgBlock);

		Device *m_dev;
		ALCvoid *m_sysMem;
		SceNgsBufferInfo m_masterRackMem;
		SceNgsBufferInfo m_sourceRackMem;
		SceNgsHRack m_masterRack;
		SceUID m_ngsOutThread;
		SceUID m_ngsUpdateThread;
		const SceNgsVoiceDefinition *m_voiceDef;
	};
}

#endif
