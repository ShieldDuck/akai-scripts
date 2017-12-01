import pyaudio
import wave
import time

CHUNK_SIZE = 4096
FORMAT = pyaudio.paInt16
CHANNELS = 1
RATE = 44100
RECORD_SECONDS = 5
WAVE_OUTPUT_FILENAME = "output.wav"

import threading

lock = threading.Lock()
frames = []
audio_buffer = {}

def audio_input_callback(in_data, frame_count, time_info, status):
    if not "local" in audio_buffer:
        audio_buffer["local"] = b""
    audio_buffer["local"] += in_data
    return (b"", pyaudio.paContinue)

def audio_output_callback(in_data, frame_count, time_info, status):
    requested_size = frame_count * CHANNELS * 2
    if "local" in audio_buffer and len(audio_buffer["local"]) >= requested_size:
        value = audio_buffer["local"][:requested_size]
        audio_buffer["local"] = audio_buffer["local"][requested_size:]
    else:
        value = b"\x00" * requested_size
    return (value, pyaudio.paContinue)

if __name__ == "__main__":
        print("starting pyaudio")
        audio = pyaudio.PyAudio()
        print("started pyaudio")
        input_device = 0
        output_device = 0

        audio_info = audio.get_host_api_info_by_index(0)
        for i in range(0, audio_info['deviceCount']):
            device = audio.get_device_info_by_host_api_device_index(0, i)
            print(device)
            if device['maxInputChannels'] > 0 and device['name']  == u"pulse":
                input_device = i
            if device['maxOutputChannels'] > 0 and device['name']  == u"pulse":
                output_device = i

        input_stream = audio.open(format=FORMAT, channels=CHANNELS, rate=RATE, input=True,
            input_device_index=input_device, frames_per_buffer=CHUNK_SIZE, stream_callback=audio_input_callback)
        output_stream = audio.open(format=FORMAT, channels=CHANNELS, rate=RATE, output=True,
            output_device_index=output_device, frames_per_buffer=CHUNK_SIZE, stream_callback=audio_output_callback)
        input_stream.start_stream()
        output_stream.start_stream()

        while input_stream.is_active() and output_stream.is_active():
            time.sleep(0.1)

        input_stream.stop_stream()
        output_stream.stop_stream()
