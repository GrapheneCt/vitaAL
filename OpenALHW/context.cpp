#include <kernel.h>

#include "common.h"
#include "context.h"

using namespace al;

Context::Context(Device *device)
{
	m_dev = device;

	listenerPosition.fX = 0.0f;
	listenerPosition.fY = 0.0f;
	listenerPosition.fZ = 0.0f;

	listenerVelocity.fX = 0.0f;
	listenerVelocity.fY = 0.0f;
	listenerVelocity.fZ = 0.0f;

	listenerForward.fX = 0.0f;
	listenerForward.fY = 0.0f;
	listenerForward.fZ = -1.0f;

	listenerUp.fX = 0.0f;
	listenerUp.fY = 1.0f;
	listenerUp.fZ = 0.0f;

	listenerGain = 1.0f;
}

Context::~Context()
{

}

ALCboolean Context::validate(ALCcontext *context)
{
	Context *ctx = NULL;

	if (!context)
	{
		return ALC_FALSE;
	}

	ctx = (Context *)context;

	if (ctx->getDevice() == NULL)
	{
		return ALC_FALSE;
	}

	return ALC_TRUE;
}

Device *Context::getDevice()
{
	return m_dev;
}