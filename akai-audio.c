#include <stdlib.h>
#include <weechat/weechat-plugin.h>

WEECHAT_PLUGIN_NAME("akai-audio");
WEECHAT_PLUGIN_DESCRIPTION("Audio plugin for Weechat");
WEECHAT_PLUGIN_AUTHOR("Gavin Stark <gstark31897@gmail.com>");
WEECHAT_PLUGIN_VERSION("0.1");
WEECHAT_PLUGIN_LICENSE("GPL3");

struct t_weechat_plugin *weechat_plugin = NULL;
struct t_hook *weechat_audio_hook = NULL;


#include <AL/al.h>
#include <AL/alc.h>

#define FREQUENCY 44100
#define FRAMES_PER_SEC 10
#define BUFFER_SIZE FREQUENCY/FRAMES_PER_SEC
#define DOUBLE_BUFFER_SIZE FREQUENCY/FRAMES_PER_SEC*2
#define BUFFER_COUNT 2

ALCcontext *audioContext = NULL;
ALCdevice *audioDevice = NULL;
ALCdevice *inputDevice = NULL;
ALuint buffers[BUFFER_COUNT], source;
int buffersActive[BUFFER_COUNT];
short buffer[DOUBLE_BUFFER_SIZE];
ALenum errorCode = 0;
int bufferState = 0;

#define NEXT_BUFFER_STATE (bufferState + 1) % BUFFER_COUNT


int check_al_error()
{
    errorCode = alGetError();
    return 0;
}


void init_audio()
{
    audioDevice = alcOpenDevice(NULL); // Request default audio device
    errorCode = alcGetError(audioDevice);
    audioContext = alcCreateContext(audioDevice,NULL); // Create the audio context
    alcMakeContextCurrent(audioContext);
    errorCode = alcGetError(audioDevice);
    inputDevice = alcCaptureOpenDevice(NULL,FREQUENCY,AL_FORMAT_MONO16,BUFFER_SIZE);
    errorCode = alcGetError(inputDevice);
    alcCaptureStart(inputDevice); // Begin capturing
    errorCode = alcGetError(inputDevice);

    alGenBuffers(BUFFER_COUNT, buffers);
    check_al_error();

    alGenSources(1, &source);
    check_al_error();

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
    bufferState = NEXT_BUFFER_STATE;
    weechat_printf(NULL, "doing audio buffer: %d", bufferState);
    if (buffersActive[bufferState] == 1)
    {
        alSourceUnqueueBuffers(source, 1, &buffers[bufferState]);
        buffersActive[bufferState] = 0;
    }

    alcCaptureSamples(inputDevice, buffer, BUFFER_SIZE);

    alBufferData(buffers[bufferState], AL_FORMAT_MONO16, buffer, BUFFER_SIZE * sizeof(short), FREQUENCY);
    alSourceQueueBuffers(source, 1, &buffers[bufferState]);
    buffersActive[bufferState] = 1;

    ALint sourceState = 0;
    alGetSourcei(source, AL_SOURCE_STATE, &sourceState);
    if (sourceState != AL_PLAYING) {
        alSourcePlay(source);
    }

    time_t date = time(NULL);
    weechat_printf(NULL, "date: %d", date);
    return WEECHAT_RC_OK;
}


int
command_double_cb (const void *pointer, void *data,
                   struct t_gui_buffer *buffer,
                   int argc, char **argv, char **argv_eol)
{
    /* make C compiler happy */
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
    /* make C compiler happy */
    (void) plugin;

    end_audio();

    return WEECHAT_RC_OK;
}

