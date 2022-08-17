#include <stdlib.h>
#include <string.h>
#include <algorithm>
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
	: m_frequency(0),
	m_bits(0),
	m_channels(0),
	m_data(0),
	m_size(0),
	m_storage(NULL),
	m_state(AL_UNUSED),
	m_refCounter(0)
{
	m_type = ObjectType_Buffer;
}

Buffer::~Buffer()
{

}

ALint Buffer::init()
{
	m_initialized = AL_TRUE;

	m_storage = AL_MALLOC(64);
	if (m_storage == NULL)
	{
		return AL_OUT_OF_MEMORY;
	}

	memset(m_storage, 0, 64);

	m_frequency = 44100;
	m_channels = 1;
	m_data = (ALint)m_storage;
	m_size = 64;

	return AL_NO_ERROR;
}

ALint Buffer::release()
{
	if (m_storage != NULL)
	{
		AL_FREE(m_storage);
		m_storage = NULL;
	}

	m_initialized = AL_FALSE;

	return AL_NO_ERROR;
}

ALvoid Buffer::ref()
{
	sceAtomicIncrement32AcqRel(&m_refCounter);
	m_state = AL_PENDING;
}

ALvoid Buffer::deref()
{
	if (sceAtomicDecrement32AcqRel(&m_refCounter) == 1)
	{
		m_state = AL_PROCESSED;
	}
}

AL_API void AL_APIENTRY alGenBuffers(ALsizei n, ALuint* buffers)
{
	Buffer *pBuf = NULL;
	ALint ret = AL_NO_ERROR;
	Context *ctx = (Context *)alcGetCurrentContext();

	AL_TRACE_CALL

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

		ctx->m_bufferStack.push_back(pBuf);

		buffers[i] = _alNamedObjectAdd((ALint)pBuf);
	}
}

AL_API void AL_APIENTRY alDeleteBuffers(ALsizei n, const ALuint* buffers)
{
	Buffer *pBuf = NULL;
	ALint ret = AL_NO_ERROR;
	Context *ctx = (Context *)alcGetCurrentContext();

	AL_TRACE_CALL

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
		pBuf = (Buffer *)_alNamedObjectGet(buffers[i]);

		if (!Buffer::validate(pBuf))
		{
			AL_SET_ERROR(AL_INVALID_NAME);
			return;
		}

		if (pBuf->m_state != AL_UNUSED)
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

		_alNamedObjectRemove(buffers[i]);

		ctx->m_bufferStack.erase(std::remove(ctx->m_bufferStack.begin(), ctx->m_bufferStack.end(), pBuf), ctx->m_bufferStack.end());

		delete pBuf;
	}
}

AL_API ALboolean AL_APIENTRY alIsBuffer(ALuint bid)
{
	Context *ctx = (Context *)alcGetCurrentContext();

	AL_TRACE_CALL

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return AL_FALSE;
	}

	if (Buffer::validate((Buffer *)_alNamedObjectGet(bid)))
	{
		return AL_TRUE;
	}

	return AL_FALSE;
}

AL_API void AL_APIENTRY alBufferData(ALuint bid, ALenum format, const ALvoid* data, ALsizei size, ALsizei freq)
{
	Buffer *buf = NULL;
	Context *ctx = (Context *)alcGetCurrentContext();

	AL_TRACE_CALL

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	buf = (Buffer *)_alNamedObjectGet(bid);

	if (!Buffer::validate(buf))
	{
		AL_SET_ERROR(AL_INVALID_NAME);
		return;
	}

	if (buf->m_state != AL_UNUSED)
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

	if (freq > 192000)
	{
		AL_SET_ERROR(AL_INVALID_VALUE);
		return;
	}

	if (buf->m_storage != NULL)
	{
		AL_FREE(buf->m_storage);
		buf->m_storage = NULL;
	}

	switch (format)
	{
	case AL_FORMAT_MONO16:
		buf->m_bits = 16;
		buf->m_channels = 1;
		break;
	case AL_FORMAT_STEREO16:
		buf->m_bits = 16;
		buf->m_channels = 2;
		break;
	}

	buf->m_storage = AL_MALLOC(size);

	if (buf->m_storage == NULL)
	{
		AL_SET_ERROR(AL_OUT_OF_MEMORY);
		return;
	}

	buf->m_size = size;
	buf->m_data = (ALint)data;
	buf->m_frequency = freq;

	memcpy(buf->m_storage, data, size);
}

AL_API void AL_APIENTRY alBufferf(ALuint bid, ALenum param, ALfloat value)
{
	Buffer *buf = NULL;
	Context *ctx = (Context *)alcGetCurrentContext();

	AL_TRACE_CALL

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	buf = (Buffer *)_alNamedObjectGet(bid);

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

	AL_TRACE_CALL

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	buf = (Buffer *)_alNamedObjectGet(bid);

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

	AL_TRACE_CALL

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	buf = (Buffer *)_alNamedObjectGet(bid);

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

	AL_TRACE_CALL

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	buf = (Buffer *)_alNamedObjectGet(bid);

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

	AL_TRACE_CALL

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	buf = (Buffer *)_alNamedObjectGet(bid);

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

	AL_TRACE_CALL

	if (ctx == NULL)
	{
		AL_SET_ERROR(AL_INVALID_OPERATION);
		return;
	}

	buf = (Buffer *)_alNamedObjectGet(bid);

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

	AL_TRACE_CALL

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

	buf = (Buffer *)_alNamedObjectGet(bid);

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

	AL_TRACE_CALL

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

	buf = (Buffer *)_alNamedObjectGet(bid);

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

	AL_TRACE_CALL

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

	buf = (Buffer *)_alNamedObjectGet(bid);

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

	AL_TRACE_CALL

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

	buf = (Buffer *)_alNamedObjectGet(bid);

	if (!Buffer::validate(buf))
	{
		AL_SET_ERROR(AL_INVALID_NAME);
		return;
	}

	switch (param)
	{
	case AL_FREQUENCY:
		*value = buf->m_frequency;
		break;
	case AL_BITS:
		*value = buf->m_bits;
		break;
	case AL_CHANNELS:
		*value = buf->m_channels;
		break;
	case AL_SIZE:
		*value = buf->m_size;
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

	AL_TRACE_CALL

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

	buf = (Buffer *)_alNamedObjectGet(bid);

	if (!Buffer::validate(buf))
	{
		AL_SET_ERROR(AL_INVALID_NAME);
		return;
	}

	AL_SET_ERROR(AL_INVALID_ENUM);
}

AL_API void AL_APIENTRY alGetBufferiv(ALuint bid, ALenum param, ALint* values)
{
	AL_TRACE_CALL

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
