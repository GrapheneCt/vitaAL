#include <kernel.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctrl.h>

#include <AL/al.h>
#include <AL/alc.h>

unsigned int sceLibcHeapSize = 200 * 1024 * 1024;

#define TEST_LOOP
#define TEST_PANNER
//#define TEST_STATES

#define BUFFER_COUNT 4
#define BUFFER_SIZE 4194304

int st = 0;

int main(void)
{
	int returnCode = SCE_OK;
	const char *audioFile = NULL;
	unsigned int audioFileSize = 0;

	SceUID fd = sceIoOpen("app0:test.raw", SCE_O_RDONLY, 0);
	audioFileSize = sceIoLseek(fd, 0, SCE_SEEK_END);
	sceIoLseek(fd, 0, SCE_SEEK_SET);
	audioFile = malloc(audioFileSize);
	sceIoRead(fd, audioFile, audioFileSize);
	sceIoClose(fd);

	printf("file sz %u\n", audioFileSize);
	printf("buf  sz %u\n", BUFFER_SIZE * BUFFER_COUNT);

	returnCode = sceKernelLoadStartModule("app0:module/OpenALHW.suprx", 0, NULL, 0, NULL, NULL);
	if (returnCode <= 0)
	{
		printf("sceKernelLoadStartModule failed: 0x%08X\n", returnCode);
		abort();
	}

	// Initialization
	ALint error = AL_NO_ERROR;
	ALCcontext *context;
	ALCdevice *device = alcOpenDevice(NULL); // select the "preferred device"

	if (!device)
	{
		printf("alcOpenDevice failed\n");
		abort();
	}

	ALCboolean hasNgsExt = alcIsExtensionPresent(device, "ALC_NGS_THREAD_AFFINITY");
	if (!hasNgsExt)
	{
		printf("Didn't find ALC_NGS_THREAD_AFFINITY. Wrong device?\n");
		abort();
	}

	void(*setThreadAffinity)(ALCdevice *device, ALCuint outputThreadAffinity) = NULL;

	setThreadAffinity = alcGetProcAddress(device, "alcSetThreadAffinityNGS");
	if (!setThreadAffinity)
	{
		printf("Didn't find alcSetThreadAffinityNGS\n");
		abort();
	}

	setThreadAffinity(device, SCE_KERNEL_CPU_MASK_USER_2);

	context = alcCreateContext(device, NULL);
	alcMakeContextCurrent(context);

	ALuint buffers[BUFFER_COUNT];
	ALuint source;

	alGetError(); // clear error code

	alGenBuffers(BUFFER_COUNT, buffers);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		printf("alGenBuffers failed: %s", alGetString(error));
		abort();
	}

	alGenSources(1, &source);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		printf("alGenSources failed: %s", alGetString(error));
		abort();
	}

	for (int i = 0; i < BUFFER_COUNT; i++)
	{
		alBufferData(buffers[i], AL_FORMAT_STEREO16, audioFile + BUFFER_SIZE * i, BUFFER_SIZE, 44100);
		if ((error = alGetError()) != AL_NO_ERROR)
		{
			printf("alBufferData failed: %s", alGetString(error));
			abort();
		}
	}

	alSourceQueueBuffers(source, BUFFER_COUNT, buffers);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		printf("alSourceQueueBuffers failed: %s", alGetString(error));
		abort();
	}

#ifdef TEST_LOOP
	alSourcei(source, AL_LOOPING, 1);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		printf("alSourcei failed: %s", alGetString(error));
		abort();
	}
#endif

#ifdef TEST_PANNER
	alListener3f(AL_POSITION, 0.0f, 0.0f, 0.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		printf("alSource3f failed: %s", alGetString(error));
		abort();
	}

	alSourcef(source, AL_CONE_INNER_ANGLE, 90.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		printf("alSourcef failed: %s", alGetString(error));
		abort();
	}

	alSourcef(source, AL_CONE_OUTER_ANGLE, 180.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		printf("alSourcef failed: %s", alGetString(error));
		abort();
	}

	alSourcef(source, AL_MAX_DISTANCE, 10.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		printf("alSourcef failed: %s", alGetString(error));
		abort();
	}

	SceCtrlData cdata;
	SceFVector4 pos = {0.0f};
	sceCtrlSetSamplingMode(SCE_CTRL_MODE_DIGITALANALOG);
#endif

	alSourcePlay(source);

	ALint state = AL_STOPPED;
	ALint procBuffers = 0;

	while (1) {

#ifdef TEST_STATES
		alGetSourcei(source, AL_SOURCE_STATE, &state);
		alGetSourcei(source, AL_BUFFERS_PROCESSED, &procBuffers);

		printf("src state: 0x%X\n", state);
		printf("src processed buffers: %d\n", procBuffers);
#endif

#ifdef TEST_PANNER

		sceCtrlPeekBufferPositive(0, &cdata, 1);

		if (cdata.lx > 140 + 40)
		{
			pos.x += 0.01f;
		}
		else if (cdata.lx < 140 - 40)
		{
			pos.x -= 0.01f;
		}

		if (cdata.ly > 140 + 40)
		{
			pos.z += 0.01f;
		}
		else if (cdata.ly < 140 - 40)
		{
			pos.z -= 0.01f;
		}

		alListener3f(AL_POSITION, pos.x, pos.y, pos.z);
		if ((error = alGetError()) != AL_NO_ERROR)
		{
			printf("alSource3f failed: %s", alGetString(error));
			abort();
		}

		sceClibPrintf("listener pos: (%.02f, %.02f, %.02f)\n", pos.x, pos.y, pos.z);
#endif

		sceKernelDelayThread(10000);
	}

	return returnCode;
}