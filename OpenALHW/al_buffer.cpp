#include <heatwave.h>
#include <stdlib.h>
#include <string.h>
#include <sce_atomic.h>

#include "common.h"
#include "context.h"
#include "named_object.h"

using namespace al;

ALboolean Buffer::validate(Buffer *buf)
{
	if (buf == NULL)
	{
		return AL_FALSE;
	}

	if (buf->getType() != ObjectType_Buffer)
	{
		return AL_FALSE;
	}

	if (buf->isInitialized() != AL_TRUE)
	{
		return AL_FALSE;
	}

	return AL_TRUE;
}

Buffer::Buffer()
	: frequency(0),
	bits(0),
	channels(0),
	data(0),
	size(0),
	storage(NULL),
	state(AL_UNUSED),
	refCounter(0),
	trackSize(0),
	trackStorage(NULL)
{
	m_type = ObjectType_Buffer;
}

Buffer::~Buffer()
{

}

ALint Buffer::init()
{
	m_initialized = AL_TRUE;

	storage = malloc(256);
	if (storage == NULL)
	{
		return AL_OUT_OF_MEMORY;
	}

	memset(storage, 0, 256);

	frequency = 44100;
	channels = 1;
	data = (ALint)storage;
	size = 256;
	trackSize = size;
	trackStorage = (ALchar *)storage;

	return AL_NO_ERROR;
}

ALint Buffer::release()
{
	if (storage != NULL)
	{
		free(storage);
		storage = NULL;
		trackStorage = NULL;
	}

	m_initialized = AL_FALSE;

	return AL_NO_ERROR;
}

ALvoid Buffer::ref()
{
	sceAtomicIncrement32AcqRel(&refCounter);
	state = AL_PENDING;
}

ALvoid Buffer::deref()
{
	if (sceAtomicDecrement32AcqRel(&refCounter) == 1)
	{
		state = AL_PROCESSED;
	}
}

AL_API void AL_APIENTRY alGenBuffers(ALsizei n, ALuint* buffers)
{
	Buffer *pBuf = NULL;
	ALint ret = AL_NO_ERROR;
	Context *ctx = (Context *)alcGetCurrentContext();

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	if (buffers == NULL)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	for (int i = 0; i < n; i++)
	{
		pBuf = new Buffer();

		ret = pBuf->init();
		if (ret != AL_NO_ERROR)
		{
			AL_SET_ERROR(ret);
			return;
		}

		buffers[i] = (ALuint)pBuf;
	}
}

AL_API void AL_APIENTRY alDeleteBuffers(ALsizei n, const ALuint* buffers)
{
	Buffer *pBuf = NULL;
	ALint ret = AL_NO_ERROR;
	Context *ctx = (Context *)alcGetCurrentContext();

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	if (buffers == NULL)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	for (int i = 0; i < n; i++)
	{
		pBuf = (Buffer *)buffers[i];

		if (!Buffer::validate(pBuf))
		{
			AL_SET_ERROR(AL_INVALID_NAME);
			return;
		}

		if (pBuf->state != AL_UNUSED)
		{
			AL_SET_ERROR(AL_INVALID_OPERATION);
			return;
		}

		ret = pBuf->release();
		if (ret != AL_NO_ERROR)
		{
			AL_SET_ERROR(ret);
			return;
		}

		delete pBuf;
	}
}

AL_API ALboolean AL_APIENTRY alIsBuffer(ALuint bid)
{
	Context *ctx = (Context *)alcGetCurrentContext();

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return AL_FALSE;
	}

	if (Buffer::validate((Buffer *)bid))
	{
		return AL_TRUE;
	}

	return AL_FALSE;
}

AL_API void AL_APIENTRY alBufferData(ALuint bid, ALenum format, const ALvoid* data, ALsizei size, ALsizei freq)
{
	Buffer *buf = NULL;
	Context *ctx = (Context *)alcGetCurrentContext();

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	buf = (Buffer *)bid;

	if (!Buffer::validate(buf))
	{
		AL_SET_ERROR(AL_INVALID_NAME);
		return;
	}

	if (buf->state != AL_UNUSED)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	if (data == NULL || size == 0)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	if (format != AL_FORMAT_MONO16 && format != AL_FORMAT_STEREO16)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	if (freq > 48000)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	if (buf->storage != NULL)
	{
		free(buf->storage);
		buf->storage = NULL;
		buf->trackStorage = NULL;
	}

	switch (format)
	{
	case AL_FORMAT_MONO16:
		buf->bits = 16;
		buf->channels = 1;
		break;
	case AL_FORMAT_STEREO16:
		buf->bits = 16;
		buf->channels = 2;
		break;
	}

	buf->storage = malloc(size);

	if (buf->storage == NULL)
	{
		AL_SET_ERROR(AL_OUT_OF_MEMORY);
		return;
	}

	buf->size = size;
	buf->data = (ALint)data;
	buf->frequency = freq;
	buf->trackSize = buf->size;
	buf->trackStorage = (ALchar *)buf->storage;

	memcpy(buf->storage, data, size);
}

AL_API void AL_APIENTRY alBufferf(ALuint bid, ALenum param, ALfloat value)
{
	Buffer *buf = NULL;
	Context *ctx = (Context *)alcGetCurrentContext();

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	buf = (Buffer *)bid;

	if (!Buffer::validate(buf))
	{
		AL_SET_ERROR(AL_INVALID_NAME);
		return;
	}

	AL_SET_ERROR(AL_INVALID_ENUM);
}

AL_API void AL_APIENTRY alBuffer3f(ALuint bid, ALenum param, ALfloat value1, ALfloat value2, ALfloat value3)
{
	Buffer *buf = NULL;
	Context *ctx = (Context *)alcGetCurrentContext();

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	buf = (Buffer *)bid;

	if (!Buffer::validate(buf))
	{
		AL_SET_ERROR(AL_INVALID_NAME);
		return;
	}

	AL_SET_ERROR(AL_INVALID_ENUM);
}

AL_API void AL_APIENTRY alBufferfv(ALuint bid, ALenum param, const ALfloat* values)
{
	Buffer *buf = NULL;
	Context *ctx = (Context *)alcGetCurrentContext();

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	buf = (Buffer *)bid;

	if (!Buffer::validate(buf))
	{
		AL_SET_ERROR(AL_INVALID_NAME);
		return;
	}

	AL_SET_ERROR(AL_INVALID_ENUM);
}

AL_API void AL_APIENTRY alBufferi(ALuint bid, ALenum param, ALint value)
{
	Buffer *buf = NULL;
	Context *ctx = (Context *)alcGetCurrentContext();

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	buf = (Buffer *)bid;

	if (!Buffer::validate(buf))
	{
		AL_SET_ERROR(AL_INVALID_NAME);
		return;
	}

	AL_SET_ERROR(AL_INVALID_ENUM);
}

AL_API void AL_APIENTRY alBuffer3i(ALuint bid, ALenum param, ALint value1, ALint value2, ALint value3)
{
	Buffer *buf = NULL;
	Context *ctx = (Context *)alcGetCurrentContext();

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	buf = (Buffer *)bid;

	if (!Buffer::validate(buf))
	{
		AL_SET_ERROR(AL_INVALID_NAME);
		return;
	}

	AL_SET_ERROR(AL_INVALID_ENUM);
}

AL_API void AL_APIENTRY alBufferiv(ALuint bid, ALenum param, const ALint* values)
{
	Buffer *buf = NULL;
	Context *ctx = (Context *)alcGetCurrentContext();

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	buf = (Buffer *)bid;

	if (!Buffer::validate(buf))
	{
		AL_SET_ERROR(AL_INVALID_NAME);
		return;
	}

	AL_SET_ERROR(AL_INVALID_ENUM);
}

AL_API void AL_APIENTRY alGetBufferf(ALuint bid, ALenum param, ALfloat* value)
{
	Buffer *buf = NULL;
	Context *ctx = (Context *)alcGetCurrentContext();

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	if (value == NULL)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	buf = (Buffer *)bid;

	if (!Buffer::validate(buf))
	{
		AL_SET_ERROR(AL_INVALID_NAME);
		return;
	}

	AL_SET_ERROR(AL_INVALID_ENUM);
}

AL_API void AL_APIENTRY alGetBuffer3f(ALuint bid, ALenum param, ALfloat* value1, ALfloat* value2, ALfloat* value3)
{
	Buffer *buf = NULL;
	Context *ctx = (Context *)alcGetCurrentContext();

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	if (value1 == NULL || value2 == NULL || value3 == NULL)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	buf = (Buffer *)bid;

	if (!Buffer::validate(buf))
	{
		AL_SET_ERROR(AL_INVALID_NAME);
		return;
	}

	AL_SET_ERROR(AL_INVALID_ENUM);
}

AL_API void AL_APIENTRY alGetBufferfv(ALuint bid, ALenum param, ALfloat* values)
{
	Buffer *buf = NULL;
	Context *ctx = (Context *)alcGetCurrentContext();

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	if (values == NULL)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	buf = (Buffer *)bid;

	if (!Buffer::validate(buf))
	{
		AL_SET_ERROR(AL_INVALID_NAME);
		return;
	}

	AL_SET_ERROR(AL_INVALID_ENUM);
}

AL_API void AL_APIENTRY alGetBufferi(ALuint bid, ALenum param, ALint* value)
{
	Buffer *buf = NULL;
	Context *ctx = (Context *)alcGetCurrentContext();

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	if (value == NULL)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	buf = (Buffer *)bid;

	if (!Buffer::validate(buf))
	{
		AL_SET_ERROR(AL_INVALID_NAME);
		return;
	}

	switch (param)
	{
	case AL_FREQUENCY:
		*value = buf->frequency;
		break;
	case AL_BITS:
		*value = buf->bits;
		break;
	case AL_CHANNELS:
		*value = buf->channels;
		break;
	case AL_SIZE:
		*value = buf->size;
		break;
	default:
		AL_SET_ERROR(AL_INVALID_ENUM);
		break;
	}
}

AL_API void AL_APIENTRY alGetBuffer3i(ALuint bid, ALenum param, ALint* value1, ALint* value2, ALint* value3)
{
	Buffer *buf = NULL;
	Context *ctx = (Context *)alcGetCurrentContext();

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	if (value1 == NULL || value2 == NULL || value3 == NULL)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	buf = (Buffer *)bid;

	if (!Buffer::validate(buf))
	{
		AL_SET_ERROR(AL_INVALID_NAME);
		return;
	}

	AL_SET_ERROR(AL_INVALID_ENUM);
}

AL_API void AL_APIENTRY alGetBufferiv(ALuint bid, ALenum param, ALint* values)
{
	switch (param)
	{
	case AL_FREQUENCY:
	case AL_BITS:
	case AL_CHANNELS:
	case AL_SIZE:
		alGetBufferi(bid, param, values);
		break;
	default:
		AL_SET_ERROR(AL_INVALID_ENUM);
		break;
	}
}