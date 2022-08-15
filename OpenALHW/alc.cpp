#include <kernel.h>
#include <ngs.h>
#include <string.h>
#include <ctype.h>

#include "common.h"
#include "device.h"

using namespace al;

static ALCboolean s_deviceExists = ALC_FALSE;
static ALCboolean s_deviceCaptureExists = ALC_FALSE;
static ALCcontext *s_currentContext = NULL;

static ALCchar s_alcNoDeviceExtList[] =
	"ALC_ENUMERATE_ALL_EXT "
	"ALC_ENUMERATION_EXT "
	"AL_EXT_EXPONENT_DISTANCE "
	"AL_EXT_LINEAR_DISTANCE "
	"ALC_EXT_CAPTURE "
	"ALC_NGS_MEMORY_FUNCTIONS";

//Each device name will be separated by a single NULL character and the list will be terminated with two NULL characters
static ALCchar s_alcAllDevicesList[] =
	AL_DEVICE_NAME
	"\0";

static ALCchar s_alcAllCaptureDevicesList[] =
	AL_CAPTURE_DEVICE_NAME
	"\0";

ALC_API ALCdevice* ALC_APIENTRY alcCaptureOpenDevice(const ALCchar *devicename, ALCuint frequency, ALCenum format, ALCsizei buffersize)
{
	DeviceAudioIn *dev = NULL;
	ALCint ret = ALC_NO_ERROR;

	AL_TRACE_CALL

	if (devicename)
	{
		if (strncmp(AL_CAPTURE_DEVICE_NAME, devicename, sizeof(AL_CAPTURE_DEVICE_NAME) - 1))
		{
			AL_SET_ERROR(ALC_INVALID_VALUE);
			return NULL;
		}
	}

	if (s_deviceCaptureExists == ALC_TRUE)
	{
		AL_SET_ERROR(ALC_INVALID_VALUE);
		return NULL;
	}

	dev = new DeviceAudioIn();

	if (!dev->validateAttributes(frequency, format, buffersize))
	{
		delete dev;
		AL_SET_ERROR(ALC_INVALID_VALUE);
		return NULL;
	}

	dev->setAttributes(frequency, format, buffersize);

	ret = dev->createContext();
	if (ret != ALC_NO_ERROR)
	{
		delete dev;
		AL_SET_ERROR(ret);
		return NULL;
	}

	s_deviceCaptureExists = ALC_TRUE;

	return (ALCdevice *)dev;
}

ALC_API ALCboolean ALC_APIENTRY alcCaptureCloseDevice(ALCdevice *device)
{
	DeviceAudioIn *dev = NULL;

	AL_TRACE_CALL

	if (!DeviceAudioIn::validate(device))
	{
		AL_SET_ERROR(ALC_INVALID_DEVICE);
		return NULL;
	}

	dev = (DeviceAudioIn *)device;

	delete dev;

	s_deviceCaptureExists = ALC_FALSE;

	return ALC_TRUE;
}

ALC_API void ALC_APIENTRY alcCaptureStart(ALCdevice *device)
{
	DeviceAudioIn *dev = NULL;

	AL_TRACE_CALL

	if (!DeviceAudioIn::validate(device))
	{
		AL_SET_ERROR(ALC_INVALID_DEVICE);
		return;
	}

	if (!dev->start())
	{
		AL_SET_ERROR(ALC_INVALID_VALUE);
		return;
	}
}

ALC_API void ALC_APIENTRY alcCaptureStop(ALCdevice *device)
{
	DeviceAudioIn *dev = NULL;

	AL_TRACE_CALL

	if (!DeviceAudioIn::validate(device))
	{
		AL_SET_ERROR(ALC_INVALID_DEVICE);
		return;
	}

	if (!dev->stop())
	{
		AL_SET_ERROR(ALC_INVALID_VALUE);
		return;
	}
}

ALC_API void ALC_APIENTRY alcCaptureSamples(ALCdevice *device, ALCvoid *buffer, ALCsizei samples)
{
	DeviceAudioIn *dev = NULL;

	AL_TRACE_CALL

	if (!DeviceAudioIn::validate(device))
	{
		AL_SET_ERROR(ALC_INVALID_DEVICE);
		return;
	}

	if (!dev->captureSamples(buffer, samples))
	{
		AL_SET_ERROR(ALC_INVALID_VALUE);
		return;
	}
}

ALC_API ALCdevice *ALC_APIENTRY alcOpenDevice(const ALCchar *devicename)
{
	DeviceNGS *dev = NULL;

	AL_TRACE_CALL

	if (devicename)
	{
		if (strncmp(AL_DEVICE_NAME, devicename, sizeof(AL_DEVICE_NAME) - 1))
		{
			AL_SET_ERROR(ALC_INVALID_VALUE);
			return NULL;
		}
	}

	if (s_deviceExists == ALC_TRUE)
	{
		AL_SET_ERROR(ALC_INVALID_VALUE);
		return NULL;
	}

	dev = new DeviceNGS();

	s_deviceExists = ALC_TRUE;

	return (ALCdevice *)dev;
}

ALC_API ALCboolean ALC_APIENTRY alcCloseDevice(ALCdevice *device)
{
	DeviceNGS *dev = NULL;

	AL_TRACE_CALL

	if (!DeviceNGS::validate(device))
	{
		AL_SET_ERROR(ALC_INVALID_DEVICE);
		return ALC_FALSE;
	}

	dev = (DeviceNGS *)device;

	delete dev;

	s_deviceExists = ALC_FALSE;

	return ALC_TRUE;
}

ALC_API ALCcontext *ALC_APIENTRY alcCreateContext(ALCdevice *device, const ALCint* attrlist)
{
	DeviceNGS *dev = NULL;
	ALCint ret = ALC_NO_ERROR;

	AL_TRACE_CALL

	if (!DeviceNGS::validate(device))
	{
		AL_SET_ERROR(ALC_INVALID_DEVICE);
		return NULL;
	}

	dev = (DeviceNGS *)device;

	if (dev->getContext() != NULL)
	{
		AL_SET_ERROR(ALC_INVALID_VALUE);
		return NULL;
	}

	if (attrlist != NULL)
	{
		if (!dev->validateAttributes(attrlist))
		{
			AL_SET_ERROR(ALC_INVALID_VALUE);
			return NULL;
		}

		dev->setAttributes(attrlist);
	}

	ret = dev->createContext();
	if (ret != ALC_NO_ERROR)
	{
		AL_SET_ERROR(ret);
		return NULL;
	}

	return (ALCcontext *)dev->getContext();
}

ALC_API ALCboolean ALC_APIENTRY alcMakeContextCurrent(ALCcontext *context)
{
	AL_TRACE_CALL

	if (!Context::validate(context))
	{
		AL_SET_ERROR(ALC_INVALID_CONTEXT);
		return ALC_FALSE;
	}

	s_currentContext = context;

	return ALC_TRUE;
}

ALC_API void ALC_APIENTRY alcProcessContext(ALCcontext *context)
{
	AL_TRACE_CALL

	if (!Context::validate(context))
	{
		AL_SET_ERROR(ALC_INVALID_CONTEXT);
		return;
	}
}

ALC_API void ALC_APIENTRY alcSuspendContext(ALCcontext *context)
{
	AL_TRACE_CALL

	if (!Context::validate(context))
	{
		AL_SET_ERROR(ALC_INVALID_CONTEXT);
		return;
	}
}

ALC_API void ALC_APIENTRY alcDestroyContext(ALCcontext *context)
{
	Context *ctx = NULL;

	AL_TRACE_CALL

	if (!Context::validate(context))
	{
		AL_SET_ERROR(ALC_INVALID_CONTEXT);
		return;
	}

	ctx = (Context *)context;

	ctx->getDevice()->destroyContext();

	s_currentContext = NULL;
}

ALC_API ALCcontext *ALC_APIENTRY alcGetCurrentContext(void)
{
	AL_TRACE_CALL

	return s_currentContext;
}

ALC_API ALCdevice *ALC_APIENTRY alcGetContextsDevice(ALCcontext *context)
{
	Context *ctx = NULL;

	AL_TRACE_CALL

	if (!Context::validate(context))
	{
		AL_SET_ERROR(ALC_INVALID_CONTEXT);
		return ALC_FALSE;
	}

	ctx = (Context *)context;

	return (ALCdevice *)ctx->getDevice();
}

ALC_API ALCenum ALC_APIENTRY alcGetError(ALCdevice *device)
{
	AL_TRACE_CALL

	return _alGetError();
}

ALC_API ALCboolean ALC_APIENTRY alcIsExtensionPresent(ALCdevice *device, const ALCchar *extname)
{
	Device *dev = NULL;
	const ALCchar *ptr = NULL;
	size_t len = 0;

	AL_TRACE_CALL

	if (!extname)
	{
		AL_SET_ERROR(ALC_INVALID_VALUE);
		return ALC_FALSE;
	}

	if (device)
	{
		dev = (Device *)device;
		ptr = dev->getExtensionList();
	}
	else
	{
		ptr = s_alcNoDeviceExtList;
	}

	len = strlen(extname);

	while (ptr && *ptr)
	{
		if (strncasecmp(ptr, extname, len) == 0 && (ptr[len] == '\0' || isspace(ptr[len])))
			return ALC_TRUE;

		if ((ptr = strchr(ptr, ' ')) != NULL)
		{
			do {
				++ptr;
			} while (isspace(*ptr));
		}
	}

	return ALC_FALSE;
}

ALC_API ALCvoid* ALC_APIENTRY alcGetProcAddress(ALCdevice *device, const ALCchar *funcName)
{
	AL_TRACE_CALL

	if (!funcName)
	{
		AL_SET_ERROR(ALC_INVALID_VALUE);
		return NULL;
	}

	return _alGetProcAddress(funcName);
}

ALC_API const ALCchar *ALC_APIENTRY alcGetString(ALCdevice *device, ALCenum param)
{
	Device *dev = NULL;
	const ALCchar *value = NULL;

	AL_TRACE_CALL

	switch (param)
	{
	case ALC_NO_ERROR:
		value = "AL_NO_ERROR";
	case ALC_INVALID_ENUM:
		value = "ALC_INVALID_ENUM";
	case ALC_INVALID_VALUE:
		value = "ALC_INVALID_VALUE";
	case ALC_INVALID_DEVICE:
		value = "ALC_INVALID_DEVICE";
	case ALC_INVALID_CONTEXT:
		value = "ALC_INVALID_CONTEXT";
	case ALC_OUT_OF_MEMORY:
		value = "ALC_OUT_OF_MEMORY";
	case ALC_DEFAULT_DEVICE_SPECIFIER:
		value = AL_DEVICE_NAME;
		break;
	case ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER:
		value = AL_CAPTURE_DEVICE_NAME;
		break;
	case ALC_DEVICE_SPECIFIER:
		if (device)
		{
			dev = (Device *)device;
			value = dev->getName();
		}
		else
		{
			value = s_alcAllDevicesList;
		}
		break;
	case ALC_CAPTURE_DEVICE_SPECIFIER:
		if (device)
		{
			dev = (Device *)device;
			value = dev->getName();
		}
		else
		{
			value = s_alcAllCaptureDevicesList;
		}
		break;
	case ALC_ALL_DEVICES_SPECIFIER:
		value = s_alcAllDevicesList;
		break;
	case ALC_DEFAULT_ALL_DEVICES_SPECIFIER:
		value = AL_DEVICE_NAME;
		break;
	case ALC_EXTENSIONS:
		if (device)
		{
			dev = (Device *)device;
			value = dev->getExtensionList();
		}
		else
		{
			value = s_alcNoDeviceExtList;
		}
		break;
	default:
		AL_SET_ERROR(ALC_INVALID_ENUM);
		break;
	}

	return value;
}

ALC_API ALCenum ALC_APIENTRY alcGetEnumValue(ALCdevice *device, const ALCchar *enumname)
{
	AL_TRACE_CALL

	if (!enumname)
	{
		AL_SET_ERROR(ALC_INVALID_VALUE);
		return 0;
	}

	return _alGetEnumValue(enumname);
}

ALC_API void ALC_APIENTRY alcGetIntegerv(ALCdevice *device, ALCenum param, ALCsizei size, ALCint *data)
{
	Device *dev = NULL;

	AL_TRACE_CALL

	if (size <= 0 || data == NULL)
	{
		AL_SET_ERROR(ALC_INVALID_VALUE);
		return;
	}

	switch (param)
	{
	case ALC_MAJOR_VERSION:
		data[0] = AL_INTERNAL_MAJOR_VERSION;
		break;
	case ALC_MINOR_VERSION:
		data[0] = AL_INTERNAL_MINOR_VERSION;
		break;
	case ALC_ATTRIBUTES_SIZE:
		if (!device)
		{
			AL_SET_ERROR(ALC_INVALID_DEVICE);
		}
		else
		{
			dev = (Device *)device;
			data[0] = dev->getAttributeCount() * 2 + 1;
		}
		break;
	case ALC_ALL_ATTRIBUTES:
		if (!device)
		{
			AL_SET_ERROR(ALC_INVALID_DEVICE);
		}
		else
		{
			dev = (Device *)device;

			if (size < dev->getAttributeCount() * 2 + 1)
			{
				AL_SET_ERROR(ALC_INVALID_VALUE);
				return;
			}

			data[0] = ALC_FREQUENCY;
			data[1] = dev->getAttribute(ALC_FREQUENCY);
			data[2] = ALC_MONO_SOURCES;
			data[3] = dev->getAttribute(ALC_MONO_SOURCES);
			data[4] = ALC_STEREO_SOURCES;
			data[5] = dev->getAttribute(ALC_STEREO_SOURCES);
			data[6] = ALC_REFRESH;
			data[7] = dev->getAttribute(ALC_REFRESH);
			data[8] = ALC_SYNC;
			data[9] = dev->getAttribute(ALC_SYNC);
			data[10] = 0;
		}
		break;
	default:
		AL_SET_ERROR(ALC_INVALID_ENUM);
		break;
	}
}

AL_API void AL_APIENTRY alcSetThreadAffinityNGS(ALCdevice *device, ALCuint outputThreadAffinity)
{
	DeviceNGS *dev = NULL;

	AL_TRACE_CALL

	if (!DeviceNGS::validate(device))
	{
		AL_SET_ERROR(ALC_INVALID_DEVICE);
		return;
	}

	dev = (DeviceNGS *)device;

	if (dev->getType() != DeviceType_NGS)
	{
		AL_SET_ERROR(ALC_INVALID_DEVICE);
		return;
	}

	dev->setThreadAffinity(outputThreadAffinity);
}

AL_API void AL_APIENTRY alcSetMemoryFunctionsNGS(AlMemoryAllocNGS alloc, AlMemoryAllocAlignNGS allocAlign, AlMemoryFreeNGS free)
{
	AL_TRACE_CALL

	if (alloc == NULL || free == NULL || allocAlign == NULL)
	{
		AL_SET_ERROR(ALC_INVALID_VALUE);
		return;
	}

	g_alloc = alloc;
	g_memalign = allocAlign;
	g_free = free;
}