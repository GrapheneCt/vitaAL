#include <heatwave.h>
#include <string.h>

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
{
	m_type = ObjectType_Buffer;
}

Buffer::~Buffer()
{

}

ALint Buffer::init()
{
	m_initialized = AL_TRUE;

	return AL_NO_ERROR;

error:

	return _alErrorHw2Al(ret);
}

ALint Buffer::release()
{
	m_initialized = AL_FALSE;

	return AL_NO_ERROR;

error:

	return _alErrorHw2Al(ret);
}