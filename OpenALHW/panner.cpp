#include <kernel.h>
#include <math.h>
#include <float.h>

#include "panner.h"
#include "named_object.h"
#include "common.h"

namespace al {

	#define AL_PI					(3.14159265358979323846)
	#define AL_2PI					(2.0 * AL_PI)
	#define AL_DEGREE_TO_RADIAN		(AL_PI / 180.0)
	#define AL_HALF_PI				(0.5 * AL_PI)

	typedef struct _speakerinfo
	{
		float32_t angle;
		Panner::SpeakerID id;
	} _speakerinfo;

	static ALint isVectorFinite(SceFVector4 vVec)
	{
		if (!isfinite(vVec.x))
		{
			return AL_INVALID_VALUE;
		}
		if (!isfinite(vVec.y))
		{
			return AL_INVALID_VALUE;
		}
		if (!isfinite(vVec.z))
		{
			return AL_INVALID_VALUE;
		}

		return AL_NO_ERROR;
	}

	static SceVoid _normaliseVector(SceFVector4 *pVector)
	{
		float32_t length = 0.0f;
		float32_t inverseLen = 1.0f;

		if (pVector == NULL)
		{
			return;
		}

		length = sqrtf(pVector->x*pVector->x + pVector->y*pVector->y + pVector->z*pVector->z);
		if (length == 0.0f)
		{
			return;
		}
		inverseLen = 1.0f / length;

		pVector->x *= inverseLen;
		pVector->y *= inverseLen;
		pVector->z *= inverseLen;
	}

	static inline float32_t _dotProduct(SceFVector4 vIn1, SceFVector4 vIn2)
	{
		const float32_t result = (vIn1.x * vIn2.x + vIn1.y * vIn2.y + vIn1.z * vIn2.z);
		return result;
	}

	static inline float32_t _minf(float32_t a, float32_t b)
	{
		return ((a > b) ? b : a);
	}

	static inline float32_t _maxf(float32_t a, float32_t b)
	{
		return ((a > b) ? a : b);
	}

	static inline float32_t _clampf(float32_t val, float32_t min, float32_t max)
	{
		return _minf(max, _maxf(min, val));
	}

	static inline float32_t _lerpf(float32_t val1, float32_t val2, float32_t mu)
	{
		return val1 + (val2 - val1)*mu;
	}

	static ALint _getSpeakerPair(const float32_t fAngle, Panner::SpeakerID *pSpeakerOut1, Panner::SpeakerID *pSpeakerOut2, float32_t *pInterpolateOut)
	{
		const _speakerinfo ksSpeakersNoCenter[] = {
			{ (float32_t)(-30.0  * AL_DEGREE_TO_RADIAN), Panner::Speaker_FL },
			{ (float32_t)(30.0  * AL_DEGREE_TO_RADIAN), Panner::Speaker_FR },
			{ (float32_t)(90.0  * AL_DEGREE_TO_RADIAN), Panner::Speaker_SR },
			{ (float32_t)(135.0  * AL_DEGREE_TO_RADIAN), Panner::Speaker_BR },
			{ (float32_t)(225.0  * AL_DEGREE_TO_RADIAN), Panner::Speaker_BL },
			{ (float32_t)(270.0  * AL_DEGREE_TO_RADIAN), Panner::Speaker_SL },
			{ (float32_t)(330.0  * AL_DEGREE_TO_RADIAN), Panner::Speaker_FL },
			{ (float32_t)(390.0  * AL_DEGREE_TO_RADIAN), Panner::Speaker_FR },
		};

		int32_t numberOfSpeakers = 0;
		const _speakerinfo *ksSpeakers = NULL;
		const _speakerinfo *pSpeakerLeft = NULL;
		const _speakerinfo *pSpeakerRight = NULL;

		if (fAngle < 0.0f || fAngle >(float32_t)AL_2PI)
		{
			return AL_INVALID_VALUE;
		}
		if (pSpeakerOut1 == NULL)
		{
			return AL_INVALID_VALUE;
		}
		if (pSpeakerOut2 == NULL)
		{
			return AL_INVALID_VALUE;
		}

		ksSpeakers = ksSpeakersNoCenter;
		numberOfSpeakers = (int32_t)(sizeof(ksSpeakersNoCenter) / sizeof(_speakerinfo));

		if (ksSpeakers == NULL || numberOfSpeakers == 0)
		{
			return AL_INVALID_VALUE;
		}

		pSpeakerLeft = &ksSpeakers[0];
		pSpeakerRight = &ksSpeakers[1];

		for (int32_t i = 0; i < numberOfSpeakers - 1; i++)
		{
			if (fAngle >= pSpeakerLeft->angle && fAngle < pSpeakerRight->angle)
			{
				break;
			}

			pSpeakerRight++;
			pSpeakerLeft++;
		}

		*pSpeakerOut1 = pSpeakerLeft->id;
		*pSpeakerOut2 = pSpeakerRight->id;

		*pInterpolateOut = (fAngle - pSpeakerLeft->angle) / (pSpeakerRight->angle - pSpeakerLeft->angle); /* distance between the speaker pair 'coneAngle' is at */

		if (*pInterpolateOut  < 0.0f || *pInterpolateOut > 1.0f)
		{
			return AL_INVALID_VALUE;
		}

		return AL_NO_ERROR;
	}

	static ALint _performFoldown(const float *pVoumes, float32_t *pOutputMatrix, const int32_t nTargetChannels)
	{
		if (pVoumes == NULL)
		{
			return AL_INVALID_VALUE;
		}
		if (pOutputMatrix == NULL)
		{
			return AL_INVALID_VALUE;
		}
		if (nTargetChannels != 2)
		{
			return AL_INVALID_VALUE;
		}

		if (nTargetChannels == 2) /* 2.0 */
		{
			/* LEFT = 1.0 x L + 0.707 x C + 0.707 x Ls + 0.707 x Le */
			pOutputMatrix[Panner::Speaker_FL] = (float32_t)(pVoumes[Panner::Speaker_FL]
				+ (0.707 * pVoumes[Panner::Speaker_C])
				+ (0.707 * pVoumes[Panner::Speaker_SL])
				+ (0.707 * pVoumes[Panner::Speaker_BL]));

			/* RIGHT = 1.0 x R + 0.707 x C + 0.707 x Rs + 0.707 x Re */
			pOutputMatrix[Panner::Speaker_FR] = (float32_t)(pVoumes[Panner::Speaker_FR]
				+ (0.707 * pVoumes[Panner::Speaker_C])
				+ (0.707 * pVoumes[Panner::Speaker_SR])
				+ (0.707 * pVoumes[Panner::Speaker_BR]));
		}

		return AL_NO_ERROR;
	}

	Panner::Panner()
	{
		m_position.x = 0.0f;
		m_position.y = 0.0f;
		m_position.z = 0.0f;

		m_velocity.x = 0.0f;
		m_velocity.y = 0.0f;
		m_velocity.z = 0.0f;

		m_forward.x = 0.0f;
		m_forward.y = 0.0f;
		m_forward.z = -1.0f;

		m_up.x = 0.0f;
		m_up.y = 1.0f;
		m_up.z = 0.0f;

		m_dopplerFactor = 1.0f;
		m_speedOfSound = 343.3f;
		m_gain = 1.0f;

		m_distanceModel = AL_INVERSE_DISTANCE_CLAMPED;
	}

	Panner::~Panner()
	{
	}

	ALint Panner::setListenerPosition(SceFVector4 vPosition)
	{
		ALint ret = AL_NO_ERROR;

		ret = isVectorFinite(vPosition);
		if (ret != AL_NO_ERROR)
		{
			return ret;
		}

		m_position.x = vPosition.x;
		m_position.y = vPosition.y;
		m_position.z = vPosition.z;

		return ret;
	}

	ALint Panner::setListenerVelocity(SceFVector4 vVelocity)
	{
		ALint ret = AL_NO_ERROR;
		float32_t   listenerspeedSq = 0.0f;

		const float32_t kSpeedOfSoundSq = m_speedOfSound * m_speedOfSound;

		ret = isVectorFinite(vVelocity);
		if (ret != AL_NO_ERROR)
		{
			return ret;
		}

		/* Clamp to velocity to the speed of sound */
		listenerspeedSq = (vVelocity.x * vVelocity.x) + (vVelocity.y * vVelocity.y) + (vVelocity.z * vVelocity.z);
		if (listenerspeedSq > kSpeedOfSoundSq)
		{
			return AL_INVALID_VALUE;
		}

		m_velocity.x = vVelocity.x;
		m_velocity.y = vVelocity.y;
		m_velocity.z = vVelocity.z;

		return ret;
	}

	ALint Panner::setListenerOrientation(SceFVector4 vForward, SceFVector4 vUp)
	{
		ALint ret = AL_NO_ERROR;
		float forwardnorm = 1.0f;
		float upnorm = 1.0f;

		ret = isVectorFinite(vForward);
		if (ret != AL_NO_ERROR)
		{
			return ret;
		}

		ret = isVectorFinite(vUp);
		if (ret != AL_NO_ERROR)
		{
			return ret;
		}

		forwardnorm = sqrtf((vForward.x * vForward.x) + (vForward.y * vForward.y) + (vForward.z * vForward.z));
		upnorm = sqrtf((vUp.x * vUp.x) + (vUp.y * vUp.y) + (vUp.z * vUp.z));

		if (forwardnorm < FLT_EPSILON && forwardnorm > -FLT_EPSILON)
		{
			return AL_INVALID_VALUE;
		}

		vForward.x = vForward.x / forwardnorm;
		vForward.y = vForward.y / forwardnorm;
		vForward.z = vForward.z / forwardnorm;

		if (upnorm < FLT_EPSILON && upnorm > -FLT_EPSILON)
		{
			return AL_INVALID_VALUE;
		}
		vUp.x = vUp.x / upnorm;
		vUp.y = vUp.y / upnorm;
		vUp.z = vUp.z / upnorm;

		m_up.x = vUp.x;
		m_up.y = vUp.y;
		m_up.z = vUp.z;

		m_forward.x = vForward.x;
		m_forward.y = vForward.y;
		m_forward.z = vForward.z;

		return ret;
	}

	ALint Panner::setDopplerFactor(float32_t fFactor)
	{
		if (fFactor < 0.0f)
		{
			return AL_INVALID_VALUE;
		}

		m_dopplerFactor = fFactor;

		return AL_NO_ERROR;
	}

	ALint Panner::getDopplerFactor(float32_t *pfFactor)
	{
		if (pfFactor == NULL)
		{
			return AL_INVALID_VALUE;
		}

		*pfFactor = m_dopplerFactor;

		return AL_NO_ERROR;
	}

	ALint Panner::setSpeedOfSound(float32_t fSpeed)
	{
		if (fSpeed < 0.0f)
		{
			return AL_INVALID_VALUE;
		}

		m_speedOfSound = fSpeed;

		return AL_NO_ERROR;
	}

	ALint Panner::getSpeedOfSound(float32_t *pfSpeed)
	{
		if (pfSpeed == NULL)
		{
			return AL_INVALID_VALUE;
		}

		*pfSpeed = m_speedOfSound;

		return AL_NO_ERROR;
	}

	ALint Panner::setListenerGain(float32_t fGain)
	{
		if (fGain < 0.0f)
		{
			return AL_INVALID_VALUE;
		}

		m_gain = fGain;

		return AL_NO_ERROR;
	}

	ALint Panner::getListenerGain(float32_t *pfGain)
	{
		if (pfGain == NULL)
		{
			return AL_INVALID_VALUE;
		}

		*pfGain = m_gain;

		return AL_NO_ERROR;
	}

	ALint Panner::setDistanceModel(int32_t nModel)
	{
		switch (nModel)
		{
		case AL_INVERSE_DISTANCE_CLAMPED:
		case AL_INVERSE_DISTANCE:
		case AL_LINEAR_DISTANCE_CLAMPED:
		case AL_LINEAR_DISTANCE:
		case AL_EXPONENT_DISTANCE_CLAMPED:
		case AL_EXPONENT_DISTANCE:
		case -1:
			break;
		default:
			return AL_INVALID_VALUE;
		}

		m_distanceModel = nModel;

		return AL_NO_ERROR;
	}

	ALint Panner::getDistanceModel(int32_t *pnModel)
	{
		if (pnModel == NULL)
		{
			return AL_INVALID_VALUE;
		}

		*pnModel = m_distanceModel;

		return AL_NO_ERROR;
	}

	ALint Panner::calculate(SourceParams* pParams, uint32_t uNumVolumes, float32_t *pVolumeMatrixOut, float32_t *pDopplerShiftOut, float32_t *pLowPassOut)
	{
		ALint res = AL_NO_ERROR;

		SceFVector4 position    = { 0.0f, 0.0f, 0.0f };
		SceFVector4 velocity    = { 0.0f, 0.0f, 0.0f };
		SceFVector4 direction   = { 0.0f, 0.0f, 0.0f };

		float32_t flAttenuation = 1.0f;

		float32_t coneVolume = 1.0f;
		float32_t coneLowpass = 1.0f;
		float32_t dopplerShift = 1.0f;

		float32_t gain = 0.0f;

		/* Check the parameters */
		if (pParams == NULL)
		{
			return AL_INVALID_VALUE;
		}

		res = isVectorFinite(pParams->vPosition);
		if (res != AL_NO_ERROR)
		{
			return res;
		}

		res = isVectorFinite(pParams->vVelocity);
		if (res != AL_NO_ERROR)
		{
			return res;
		}

		res = isVectorFinite(pParams->vForward);
		if (res != AL_NO_ERROR)
		{
			return res;
		}

		position.x = pParams->vPosition.x;
		position.y = pParams->vPosition.y;
		position.z = pParams->vPosition.z;

		velocity.x = pParams->vVelocity.x;
		velocity.y = pParams->vVelocity.y;
		velocity.z = pParams->vVelocity.z;

		direction.x = pParams->vForward.x;
		direction.y = pParams->vForward.y;
		direction.z = pParams->vForward.z;

		if (pParams->fMinDistance < 0.0f)
		{
			return AL_INVALID_VALUE;
		}
		if (pParams->fMaxDistance < pParams->fMinDistance)
		{
			return AL_INVALID_VALUE;
		}

		if (uNumVolumes == 0 || uNumVolumes > Speaker_MAX)
		{
			return AL_INVALID_VALUE;
		}
		if (pVolumeMatrixOut == NULL)
		{
			return AL_INVALID_VALUE;
		}

		if (pDopplerShiftOut == NULL)
		{
			return AL_INVALID_VALUE;
		}

		/* translate to listener relative position */
		if (!pParams->bListenerRelative)
		{
			getListenerRelativePosition(pParams->vPosition, &position);
		}

		/* Distance gain rolloff */
		res = calcDistanceGain(position, pParams->fMinDistance, pParams->fMaxDistance, pParams->fDistanceFactor, &flAttenuation);
		if (res != AL_NO_ERROR)
		{
			return res;
		}

		/* Doppler */
		res = calcDopplerShift(position, velocity, &dopplerShift);
		if (res != AL_NO_ERROR)
		{
			return res;
		}

		/* Apply the sound cone */
		res = calcCone(position, direction, pParams->fInsideAngle, pParams->fOutsideAngle, pParams->fOutsideGain, pParams->fOutsideFreq, &coneVolume, &coneLowpass);
		if (res != AL_NO_ERROR)
		{
			return res;
		}

		/* Pan */
		res = calcSpeakerVolumes(position, pVolumeMatrixOut, uNumVolumes);
		if (res != AL_NO_ERROR)
		{
			return res;
		}

		/* Apply distance and cone attenuation */
		gain = m_gain * flAttenuation;
		gain *= coneVolume;

		/* Fill out the volume matrix */
		for (uint32_t i = 0; i < uNumVolumes; i++)
		{
			pVolumeMatrixOut[i] *= gain;
		}

		*pDopplerShiftOut = dopplerShift;
		*pLowPassOut = coneLowpass;

		return res;
	}

	ALint Panner::getListenerRelativePosition(SceFVector4 vPosition, SceFVector4 *pvRelativePositionOut)
	{
		ALint nRet = AL_NO_ERROR;
		SceFVector4 vRelativePosition = { 0.0f, 0.0f, 0.0f };
		SceFVector4 vRight = { 0.0f, 0.0f, 0.0f };
		SceFVector4 vAlignedPosition = { 0.0f, 0.0f, 0.0f };
		SceFVector4 vLookAt = { 0.0f, 0.0f, 0.0f };

		nRet = isVectorFinite(vPosition);
		if (nRet != AL_NO_ERROR)
		{
			return nRet;
		}
		if (pvRelativePositionOut == NULL)
		{
			return AL_INVALID_VALUE;
		}

		vRelativePosition.x = vPosition.x - m_position.x;
		vRelativePosition.y = vPosition.y - m_position.y;
		vRelativePosition.z = vPosition.z - m_position.z;

		vRight.x = (m_forward.y * m_up.z) - (m_forward.z * m_up.y);
		vRight.y = (m_forward.z * m_up.x) - (m_forward.x * m_up.z);
		vRight.z = (m_forward.x * m_up.y) - (m_forward.y * m_up.x);

		vAlignedPosition.x = _dotProduct(vRelativePosition, vRight);
		vAlignedPosition.y = _dotProduct(vRelativePosition, m_up);

		vLookAt.x = -m_forward.x;
		vLookAt.y = -m_forward.y;
		vLookAt.z = -m_forward.z;

		vAlignedPosition.z = _dotProduct(vRelativePosition, vLookAt);


		pvRelativePositionOut->x = vAlignedPosition.x;
		pvRelativePositionOut->y = vAlignedPosition.y;
		pvRelativePositionOut->z = vAlignedPosition.z;

		return nRet;
	}

	ALint Panner::calcDistanceGain(SceFVector4 vRelativePostion, float32_t fMinDistance, float32_t fMaxDistance, float32_t fDistanceFactor, float32_t *pDistanceGainOut)
	{
		float32_t distance = 0.0f;
		float32_t clampedDist = 0.0f;
		ALint res = AL_NO_ERROR;

		if (fDistanceFactor > 0.0f) {

			res = isVectorFinite(vRelativePostion);
			if (res != AL_NO_ERROR)
			{
				return res;
			}
			if (pDistanceGainOut == NULL)
			{
				return AL_INVALID_VALUE;
			}

			distance = sqrtf(_dotProduct(vRelativePostion, vRelativePostion));
			clampedDist = distance;

			switch (m_distanceModel)
			{
			case AL_INVERSE_DISTANCE_CLAMPED:
				if (fMaxDistance < fMinDistance)
				{
					break;
				}
				clampedDist = _clampf(clampedDist, fMinDistance, fMaxDistance);
				/* fall-through */
			case AL_INVERSE_DISTANCE:
				if (fMinDistance > 0.0f)
				{
					float32_t dist = _lerpf(fMinDistance, clampedDist, fDistanceFactor);
					if (dist > 0.0f)
					{
						*pDistanceGainOut = fMinDistance / dist;
					}
				}
				break;
			case AL_LINEAR_DISTANCE_CLAMPED:
				if (fMaxDistance < fMinDistance)
				{
					break;
				}
				clampedDist = _clampf(clampedDist, fMinDistance, fMaxDistance);
				/* fall-through */
			case AL_LINEAR_DISTANCE:
				if (fMaxDistance != fMinDistance)
				{
					float32_t attn = (clampedDist - fMinDistance) / (fMaxDistance - fMinDistance) * fDistanceFactor;
					*pDistanceGainOut = _maxf(1.0f - attn, 0.0f);
				}
				break;
			case AL_EXPONENT_DISTANCE_CLAMPED:
				if (fMaxDistance < fMinDistance)
				{
					break;
				}
				clampedDist = _clampf(clampedDist, fMinDistance, fMaxDistance);
				/*fall-through*/
			case AL_EXPONENT_DISTANCE:
				if (clampedDist > 0.0f && fMinDistance > 0.0f)
				{
					const float32_t dist_ratio = clampedDist / fMinDistance;
					*pDistanceGainOut = powf(dist_ratio, -fDistanceFactor);
				}
				break;
			case -1:
				*pDistanceGainOut = 1.0f;
				break;
			}
		}
		else
		{
			*pDistanceGainOut = 1.0f;
		}

		return res;
	}

	ALint Panner::calcDopplerShift(SceFVector4 vRelativePosition, SceFVector4 vVelocity, float32_t *pDopplerShiftOut)
	{
		ALint res = AL_NO_ERROR;

		if (pDopplerShiftOut == NULL)
		{
			return AL_INVALID_VALUE;
		}

		if (m_dopplerFactor != 0.0f)
		{
			float32_t fVr = 0.0f;
			float32_t fVs = 0.0f;
			SceFVector4 sourceToListener = { -vRelativePosition.x, -vRelativePosition.y, -vRelativePosition.z };

			res = isVectorFinite(vRelativePosition);
			if (res != AL_NO_ERROR)
			{
				return res;
			}
			res = isVectorFinite(vVelocity);
			if (res != AL_NO_ERROR)
			{
				return res;
			}

			_normaliseVector(&sourceToListener);

			/* Doppler */
			fVr = _dotProduct(m_velocity, sourceToListener);
			fVs = _dotProduct(vVelocity, sourceToListener);

			*pDopplerShiftOut = m_dopplerFactor * ((m_speedOfSound - fVr) / (m_speedOfSound - fVs)); /* Negated doppler calculation due to right hand coordinate system */
		}
		else
		{
			*pDopplerShiftOut = 0.0f;
		}

		return res;
	}

	ALint Panner::calcCone(SceFVector4 vRelativePosition, SceFVector4 vForward, float32_t fInsideAngle, float32_t fOutsideAngle, float32_t fOutsideGain, float32_t fOutsideLowpass, float32_t *pGainOut, float32_t *pFilterLowpassOut)
	{
		ALint res = AL_NO_ERROR;
		SceFVector4 sourceToListener = { -vRelativePosition.x, -vRelativePosition.y, -vRelativePosition.z };
		float32_t emitterDotProd = 0.0f;
		float32_t coneAngle = 0.0f;
		float32_t coneVolume = 0.0f;
		float32_t coneLowpass = 0.0f;
			
		res = isVectorFinite(vRelativePosition);
		if (res != AL_NO_ERROR)
		{
			return res;
		}
		res = isVectorFinite(vForward);
		if (res != AL_NO_ERROR)
		{
			return res;
		}

		_normaliseVector(&vForward);
		_normaliseVector(&sourceToListener);

		emitterDotProd = _dotProduct(vForward, sourceToListener);
		if (emitterDotProd > -FLT_EPSILON && emitterDotProd < FLT_EPSILON)
		{
			coneAngle = 0;
		}
		else
		{
			coneAngle = (float32_t)((180.0 / AL_PI) * acos(emitterDotProd));
		}

		if (coneAngle >= fInsideAngle && coneAngle <= fOutsideAngle)
		{
			const float32_t coneAngleInterpolated = (coneAngle - fInsideAngle) / (fOutsideAngle - fInsideAngle);

			coneVolume = 1.0f + ((fOutsideGain - 1.0f) * coneAngleInterpolated);
			coneLowpass = 1.0f + ((fOutsideLowpass - 1.0f) * coneAngleInterpolated);
		}
		else if (coneAngle > fOutsideAngle)
		{
			coneVolume = 1.0f + (fOutsideGain - 1.0f);
			coneLowpass = 1.0f + (fOutsideLowpass - 1.0f);
		}
		else
		{
			coneVolume = 1.0f;
			coneLowpass = 1.0f;
		}

		*pGainOut = coneVolume;
		*pFilterLowpassOut = coneLowpass;

		return res;
	}

	ALint Panner::calcSpeakerVolumes(SceFVector4 vRelativePosition, float32_t *pfVolumeMatrixOut, uint32_t uNumVolumes)
	{
		ALint nRet = AL_NO_ERROR;
		SceFVector4 vPosition = { vRelativePosition.x, vRelativePosition.y, vRelativePosition.z };
		SpeakerID eSpeaker1 = Speaker_MAX;
		SpeakerID eSpeaker2 = Speaker_MAX;
		float32_t fEmitterAngle = 0.0f;
		float32_t fInterpolate = 0.0f;
		float32_t afSpeakerGains[Speaker_MAX] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

		/* param validation */
		nRet = isVectorFinite(vPosition);
		if (nRet != AL_NO_ERROR)
		{
			return nRet;
		}
		if (pfVolumeMatrixOut == NULL)
		{
			return AL_INVALID_VALUE;
		}
		if (uNumVolumes == 0 || uNumVolumes > Speaker_MAX)
		{
			return AL_INVALID_VALUE;
		}

		/* convert to unit vector */
		_normaliseVector(&vPosition);

		if (_dotProduct(vPosition, vPosition) == 0.0f) /* Sound is at exactly same position as the listener, emit from the front (not the rear) */
		{
			fEmitterAngle = 0.0f;
		}
		else
		{
			fEmitterAngle = atan2f(vRelativePosition.x, -vRelativePosition.z);
		}

		if (fEmitterAngle < 0.0f) /* wrap */
		{
			fEmitterAngle += (float32_t)AL_2PI;
		}

		/* find the speaker pair the sound will emit from */
		nRet = _getSpeakerPair(fEmitterAngle, &eSpeaker1, &eSpeaker2, &fInterpolate);
		if (nRet != AL_NO_ERROR)
		{
			return nRet;
		}

		/* Constant power pan between the 2 speakers */
		afSpeakerGains[eSpeaker1] = (float32_t)cos(fInterpolate * AL_HALF_PI);
		afSpeakerGains[eSpeaker2] = (float32_t)sin(fInterpolate * AL_HALF_PI);

		/* convert from 7.1 to the destination layout */
		nRet = _performFoldown(afSpeakerGains, pfVolumeMatrixOut, uNumVolumes);
		if (nRet != AL_NO_ERROR)
		{
			return nRet;
		}

		return nRet;
	}
}
