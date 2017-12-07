#include <stdlib.h>
#include <weechat/weechat-plugin.h>
#include <AL/al.h>
#include <AL/alc.h>

#define FREQUENCY 44100
#define FRAMES_PER_SEC 20
#define BUFFER_SIZE FREQUENCY/FRAMES_PER_SEC
#define DOUBLE_BUFFER_SIZE FREQUENCY/FRAMES_PER_SEC*2
#define BUFFER_COUNT 4
#define NEXT_BUFFER_STATE (bufferState + 1) % BUFFER_COUNT

WEECHAT_PLUGIN_NAME("akai-audio");
WEECHAT_PLUGIN_DESCRIPTION("Audio plugin for Weechat");
WEECHAT_PLUGIN_AUTHOR("Gavin Stark <gstark31897@gmail.com>");
WEECHAT_PLUGIN_VERSION("0.1");
WEECHAT_PLUGIN_LICENSE("GPL3");

struct t_weechat_plugin *weechat_plugin = NULL;
struct t_hook *weechat_audio_hook = NULL;

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
        weechat_printf(NULL, "%s: invalid name", location);
    }
    else if (error == AL_INVALID_ENUM)
    {
        weechat_printf(NULL, "%s: invalid enum", location);
    }
    else if (error == AL_INVALID_OPERATION)
    {
        weechat_printf(NULL, "%s: invalid operation", location);
    }
    else if (error == AL_OUT_OF_MEMORY)
    {
        weechat_printf(NULL, "%s: out of memory", location);
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



int
timer_audio_cb(const void *pointer, void *data, int remaining_calls)
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
    return WEECHAT_RC_OK;
}


int
command_double_cb (const void *pointer, void *data,
                   struct t_gui_buffer *buffer,
                   int argc, char **argv, char **argv_eol)
{
    (void) data;
    (void) buffer;
    (void) argv;

    if (argc > 1)
    {
        weechat_command (NULL, argv_eol[1]);
        weechat_command (NULL, argv_eol[1]);
    }

    return WEECHAT_RC_OK;
}


int
weechat_plugin_init (struct t_weechat_plugin *plugin,
                     int argc, char *argv[])
{
    weechat_plugin = plugin;

    weechat_hook_command ("akai-audio",
                          "Display two times a message "
                          "or execute two times a command",
                          "message | command",
                          "message: message to display two times\n"
                          "command: command to execute two times",
                          NULL,
                          &command_double_cb, NULL, NULL);

    weechat_audio_hook = weechat_hook_timer(1000/FRAMES_PER_SEC, 0, 0, &timer_audio_cb, NULL, NULL);

    init_audio();

    return WEECHAT_RC_OK;
}

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    (void) plugin;

    end_audio();

    return WEECHAT_RC_OK;
}

