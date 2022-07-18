#include <heatwave.h>

#include "AL/al.h"
#include "AL/alc.h"

#ifndef AL_CONTEXT_H
#define AL_CONTEXT_H

namespace al {

	class Device;

	class Context
	{
	public:

		static ALCboolean validate(ALCcontext *ctx);

		Context(Device *device);
		~Context();

		Device *getDevice();

		const ALenum distanceModel = AL_EXPONENT_DISTANCE;

		float32_t listenerGain;
		SceHwVector listenerPosition;
		SceHwVector listenerVelocity;
		SceHwVector listenerForward;
		SceHwVector listenerUp;

	private:

		Device *m_dev;
	};
}

#endif
