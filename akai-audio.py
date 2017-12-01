SCR_NAME    = "akai-audio"
SCR_AUTHOR  = "Gavin Stark <gstark31897@gmail.com>"
SCR_VERSION = "0.1.0"
SCR_LICENSE = "GPL3"
SCR_DESC    = "Plays encoded audio submited in IRC channels"
SCR_COMMAND = "akai-audio"

CONFIG_AUDIO_INPUT = "pulse"
CONFIG_AUDIO_OUTPUT = "pulse"


try:
    import weechat
    import_ok = True
except:
    print "Script must be run under weechat. http://www.weechat.org"
    import_ok = False


import pyaudio
import threading
import sys
import time

CHUNK_SIZE = 4096
FORMAT = pyaudio.paInt16
CHANNELS = 2
RATE = 44100

input_stream = None
output_stream = None
audio_buffer = {}


def fn_modify(data, modifier, modifier_data, string):
    return string

def audio_input_callback(in_data, frame_count, time_info, status):
    weechat.prnt("", "input")
    if not "local" in audio_buffer:
        audio_buffer["local"] = b""
    audio_buffer["local"] += in_data
    return (b"", pyaudio.paContinue)

def audio_output_callback(in_data, frame_count, time_info, status):
    requested_size = frame_count * CHANNELS * 2
    weechat.prnt("", "output")
    if "local" in audio_buffer and len(audio_buffer["local"]) >= requested_size:
        weechat.prnt("", "audio")
        value = audio_buffer["local"][:requested_size]
        audio_buffer["local"] = audio_buffer["local"][requested_size:]
    else:
        weechat.prnt("", "no audio")
        value = b"\x00" * requested_size
    return (value, pyaudio.paContinue)

if __name__ == "__main__" and import_ok:
    if weechat.register(SCR_NAME, SCR_AUTHOR, SCR_VERSION, SCR_LICENSE, SCR_DESC, "", ""):
        weechat.hook_modifier("weechat_print", "fn_modify", "")
        weechat.prnt("", sys.executable)

        weechat.prnt("", "Setting up audio")
        audio = pyaudio.PyAudio()
        input_device = 0
        output_device = 0

        audio_info = audio.get_host_api_info_by_index(0)
        for i in range(0, audio_info['deviceCount']):
            device = audio.get_device_info_by_host_api_device_index(0, i)
            if device['maxInputChannels'] > 0 and device['name']  == CONFIG_AUDIO_INPUT:
                input_device = i
            if device['maxOutputChannels'] > 0 and device['name']  == CONFIG_AUDIO_OUTPUT:
                output_device = i

        weechat.prnt("", "Using input: {} output: {}".format(input_device, output_device))
        input_stream = audio.open(format=FORMAT, channels=CHANNELS, rate=RATE, input=True,
            input_device_index=input_device, frames_per_buffer=CHUNK_SIZE, stream_callback=audio_input_callback)
        output_stream = audio.open(format=FORMAT, channels=CHANNELS, rate=RATE, output=True,
            output_device_index=output_device, frames_per_buffer=CHUNK_SIZE, stream_callback=audio_output_callback)
        weechat.prnt("", "starting streaming")
        input_stream.start_stream()
        output_stream.start_stream()
        weechat.prnt("", "streaming started")
        weechat.prnt("", str(input_stream.is_active()) + ":" + str(output_stream.is_active()))
        #while input_stream.is_active() and output_stream.is_active():
        #    time.sleep(0.1)
        #weechat.prnt("", "stopping streaming")
        #input_stream.stop_stream()
        #output_stream.stop_stream()
        #weechat.prnt("", "streaming stopped")

