#include <kernel.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <AL/al.h>
#include <AL/alc.h>

unsigned int sceLibcHeapSize = 200 * 1024 * 1024;

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

	ALCboolean hasNgsExt = alcIsExtensionPresent(device, "ALC_NGS_DECODE_CHANNEL_COUNT");
	if (!hasNgsExt)
	{
		printf("Didn't find ALC_NGS_DECODE_CHANNEL_COUNT. Wrong device?\n");
		abort();
	}

	void(*setAt9Ch)(ALCdevice *dev, ALCint ch) = NULL;
	void(*setMp3Ch)(ALCdevice *dev, ALCint ch) = NULL;

	setAt9Ch = alcGetProcAddress(device, "alcSetAt9ChannelCountNGS");
	if (!setAt9Ch)
	{
		printf("Didn't find alcSetAt9ChannelCountNGS\n");
		abort();
	}

	setMp3Ch = alcGetProcAddress(device, "alcSetMp3ChannelCountNGS");
	if (!setMp3Ch)
	{
		printf("Didn't find alcSetAt9ChannelCountNGS\n");
		abort();
	}

	setAt9Ch(device, 0);
	setMp3Ch(device, 0);

	context = alcCreateContext(device, NULL);
	alcMakeContextCurrent(context);

	ALuint staticBuffer;
	ALuint source;

	alGetError(); // clear error code

	alGenBuffers(1, &staticBuffer);
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

	alBufferData(staticBuffer, AL_FORMAT_STEREO16, audioFile, audioFileSize, 44100);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		printf("alBufferData failed: %s", alGetString(error));
		abort();
	}

	alSourcei(source, AL_BUFFER, staticBuffer);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		printf("alSourcei failed: %s", alGetString(error));
		abort();
	}

	alSourcePlay(source);

	ALint state = AL_STOPPED;

	while (1) {

		alGetSourcei(source, AL_SOURCE_STATE, &state);

		printf("src state: 0x%X\n", state);

		sceKernelDelayThread(10000);
	}

	return returnCode;
}