#ifndef AL_NAMED_OBJECT_H
#define AL_NAMED_OBJECT_H

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

	class Source : public NamedObject
	{
	public:

		static ALboolean validate(Source *src);

		Source();
		~Source();

		ALint init();
		ALint release();

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
	};

	class Buffer : public NamedObject
	{
	public:

		Buffer();
		~Buffer();

		ALint init();
		ALint release();

	private:

	};
}

#endif
