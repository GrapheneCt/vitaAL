#ifndef AL_PANNER_H
#define AL_PANNER_H

#include <kernel.h>

#include "AL/al.h"
#include "AL/alc.h"

namespace al {
	class SourceParams;

	class Panner
	{
	public:

		enum SpeakerID
		{
			Speaker_FL = 0,
			Speaker_FR = 1,
			Speaker_C = 2,
			Speaker_LFE = 3,
			Speaker_SL = 4,
			Speaker_SR = 5,
			Speaker_BL = 6,
			Speaker_BR = 7,
			Speaker_MAX = 8
		};

		Panner();
		~Panner();

		ALint setListenerPosition(SceFVector4 vPosition);
		ALint setListenerVelocity(SceFVector4 vVelocity);
		ALint setListenerOrientation(SceFVector4 vForward, SceFVector4 vUp);

		ALint setDopplerFactor(float32_t fFactor);
		ALint getDopplerFactor(float32_t *pfFactor);
		ALint setSpeedOfSound(float32_t fSpeed);
		ALint getSpeedOfSound(float32_t *pfSpeed);

		ALint setListenerGain(float32_t fGain);
		ALint getListenerGain(float32_t *pfGain);

		ALint setDistanceModel(int32_t nModel);
		ALint getDistanceModel(int32_t *pnModel);

		ALint calculate(SourceParams* pParams, uint32_t uNumVolumes, float32_t *pVolumeMatrixOut, float32_t *pDopplerShiftOut, float32_t *pLowPassOut);

		ALint getListenerRelativePosition(SceFVector4 vPosition, SceFVector4 *pRelativePositionOut);
		ALint calcDistanceGain(SceFVector4 vRelativePostion, float32_t fMinDistance, float32_t fMaxDistance, float32_t fDistanceFactor, float32_t *pDistanceGainOut);
		ALint calcDopplerShift(SceFVector4 vRelativePosition, SceFVector4 vVelocity, float32_t *pDopplerShiftOut);
		ALint calcCone(SceFVector4 vRelativePosition, SceFVector4 vForward, float32_t fInsideAngle, float32_t fOutsideAngle, float32_t fOutsideGain, float32_t fOutsideLowpass, float32_t *pGainOut, float32_t *pFilterLowpassOut);
		ALint calcSpeakerVolumes(SceFVector4 vRelativePosition, float32_t *pVolumeMatrixOut, uint32_t uNumVolumes);

		// Listener params and globals
		SceFVector4 m_position;
		SceFVector4 m_velocity;
		SceFVector4 m_forward;
		SceFVector4 m_up;
		float32_t m_dopplerFactor;
		float32_t m_speedOfSound;
		float32_t m_gain;
		int32_t m_distanceModel;

	private:

		// Listener params and globals
		/*SceFVector4 m_position;
		SceFVector4 m_velocity;
		SceFVector4 m_forward;
		SceFVector4 m_up;
		float32_t m_dopplerFactor;
		float32_t m_speedOfSound;
		float32_t m_gain;
		int32_t m_distanceModel;*/
	};
}

#endif
