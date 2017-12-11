#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <AL/al.h>
#include <AL/alc.h>

#define FREQUENCY 44100
#define FRAMES_PER_SEC 20
#define BUFFER_SIZE FREQUENCY/FRAMES_PER_SEC
#define DOUBLE_BUFFER_SIZE FREQUENCY/FRAMES_PER_SEC*2
#define BUFFER_COUNT 16
#define NEXT_BUFFER_STATE (bufferState + 1) % BUFFER_COUNT
#define STDIN 0

ALCcontext *audioContext = NULL;
ALCdevice *audioDevice = NULL;
ALCdevice *inputDevice = NULL;
ALuint buffers[BUFFER_COUNT], source;
int buffersActive[BUFFER_COUNT];
short buffer[DOUBLE_BUFFER_SIZE];
ALenum errorCode = 0;
int bufferState = 0;


void openal_error(char *location)
{
    ALenum error = alGetError();
    if (error == AL_INVALID_NAME)
    {
        printf("%s: invalid name\n", location);
    }
    else if (error == AL_INVALID_ENUM)
    {
        printf("%s: invalid enum\n", location);
    }
    else if (error == AL_INVALID_OPERATION)
    {
        printf("%s: invalid operation\n", location);
    }
    else if (error == AL_OUT_OF_MEMORY)
    {
        printf("%s: out of memory\n", location);
    }
}


void init_audio()
{
    audioDevice = alcOpenDevice(NULL);
    audioContext = alcCreateContext(audioDevice,NULL);
    alcMakeContextCurrent(audioContext);
    openal_error("set context");
    inputDevice = alcCaptureOpenDevice(NULL,FREQUENCY,AL_FORMAT_MONO16,BUFFER_SIZE);
    alcCaptureStart(inputDevice);
    openal_error("start capture");

    alGenBuffers(BUFFER_COUNT, buffers);
    openal_error("gen buffers");

    alGenSources(1, &source);
    openal_error("gen sources");

    for (int i = 0; i < BUFFER_COUNT; ++i)
    {
        buffersActive[i] = 0;
    }
}


void end_audio()
{
    alcCaptureStop(inputDevice);
    alcCaptureCloseDevice(inputDevice);

    alSourceStopv(1, &source);

    alDeleteSources(1, &source);
    alDeleteBuffers(BUFFER_COUNT, buffers);

    alcMakeContextCurrent(NULL);
    alcDestroyContext(audioContext);
    alcCloseDevice(audioDevice);
}



int do_audio()
{
    int samplesIn = 0;
    alcGetIntegerv(inputDevice, ALC_CAPTURE_SAMPLES, 1, &samplesIn);
    if (samplesIn >= BUFFER_SIZE)
    {
        alcCaptureSamples(inputDevice, buffer, BUFFER_SIZE);
        openal_error("capture samples");
        
        long long volume = 0;
        for (int i = 0; i < DOUBLE_BUFFER_SIZE; ++i)
            if (buffer[i] > 0)
                volume += buffer[i];
            else
                volume -= buffer[i];

        bufferState = NEXT_BUFFER_STATE;
        if (buffersActive[bufferState] == 1)
        {
            alSourceUnqueueBuffers(source, 1, &buffers[bufferState]);
            openal_error("unqueue buffers");
            buffersActive[bufferState] = 0;
        }

        alBufferData(buffers[bufferState], AL_FORMAT_MONO16, buffer, BUFFER_SIZE * sizeof(short), FREQUENCY);
        openal_error("buffer data");
        alSourceQueueBuffers(source, 1, &buffers[bufferState]);
        openal_error("queue buffers");
        buffersActive[bufferState] = 1;


        ALint sourceState = 0;
        alGetSourcei(source, AL_SOURCE_STATE, &sourceState);
        openal_error("get source state");
        if (sourceState != AL_PLAYING) {
            alSourcePlay(source);
            openal_error("play source");
        }
    }
    return 0;
}

void read_input()
{
    char buffer[4096];
}

int main(char *argv, int argc)
{
    struct timeval delay;
    delay.tv_sec = 0;

    fd_set fds;
    int maxfd = 1;

    init_audio();
    while (1 == 1)
    {
        FD_ZERO(&fds);
        FD_SET(STDIN, &fds);
        // only set the timeout if we actually timed out
        if (delay.tv_usec == 0)
            delay.tv_usec = 1000000/FRAMES_PER_SEC;
        select(maxfd + 1, &fds, NULL, NULL, &delay);
        if (FD_ISSET(STDIN, &fds))
        {
            printf("reading\n");
            read_input();
        }

        if (delay.tv_usec == 0)
        {
            printf("doing audio\n");
            if (do_audio() != -1)
                break;
        }
    }
    end_audio();
    return 0;
}

