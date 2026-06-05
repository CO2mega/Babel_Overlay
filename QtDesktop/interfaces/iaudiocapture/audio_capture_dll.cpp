#include "audio_capture_api.h"
#include "system_audio_capture.hpp"

static SystemAudioCapture::ConversionMode MapMode(AudioConversionMode mode)
{
    switch (mode) {
    case AUDIO_CAPTURE_PASS_THROUGH:
        return SystemAudioCapture::PASS_THROUGH;
    case AUDIO_CAPTURE_STEREO_TO_MONO_AVG:
        return SystemAudioCapture::STEREO_TO_MONO_AVG;
    case AUDIO_CAPTURE_STEREO_TO_MONO_WEIGHT:
        return SystemAudioCapture::STEREO_TO_MONO_WEIGHT;
    case AUDIO_CAPTURE_EXTRACT_LEFT:
        return SystemAudioCapture::EXTRACT_LEFT;
    case AUDIO_CAPTURE_EXTRACT_RIGHT:
        return SystemAudioCapture::EXTRACT_RIGHT;
    default:
        return SystemAudioCapture::PASS_THROUGH;
    }
}

static WAVEFORMATEX MapFormat(AudioCaptureFormat f)
{
    WAVEFORMATEX fmt = {};
    fmt.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    fmt.nChannels = f.num_channels;
    fmt.nSamplesPerSec = f.sample_rate;
    fmt.wBitsPerSample = f.bits_per_sample;
    fmt.nBlockAlign = fmt.nChannels * (fmt.wBitsPerSample / 8);
    fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;
    fmt.cbSize = 0;
    return fmt;
}

extern "C" {

AUDIO_CAPTURE_API AudioCaptureHandle DesktopCapture_Create(
    AudioCaptureFormat format,
    AudioConversionMode mode,
    AudioCaptureCallback callback)
{
    try {
        auto cb = [callback](UINT64 ts, float* data, UINT32 nframes) {
            if (callback)
                callback(ts, data, nframes);
        };
        return new SystemAudioCapture(cb, MapFormat(format), MapMode(mode));
    } catch (...) {
        return nullptr;
    }
}

AUDIO_CAPTURE_API AudioCaptureHandle ProcessCapture_Create(
    unsigned long target_pid,
    AudioCaptureFormat format,
    AudioConversionMode mode,
    AudioCaptureCallback callback)
{
    try {
        auto cb = [callback](UINT64 ts, float* data, UINT32 nframes) {
            if (callback)
                callback(ts, data, nframes);
        };
        return new SystemAudioCapture(
            static_cast<DWORD>(target_pid), cb, MapFormat(format), MapMode(mode));
    } catch (...) {
        return nullptr;
    }
}

AUDIO_CAPTURE_API void AudioCapture_Destroy(AudioCaptureHandle handle)
{
    if (handle)
        delete static_cast<SystemAudioCapture*>(handle);
}

AUDIO_CAPTURE_API float AudioCapture_GetRMS(const float* data, unsigned int num_samples)
{
    return AudioConverter::GetRMS(data, num_samples);
}

AUDIO_CAPTURE_API float AudioCapture_GetPeak(const float* data, unsigned int num_samples)
{
    return AudioConverter::GetPeak(data, num_samples);
}

AUDIO_CAPTURE_API AudioCaptureFormat AudioCapture_GetDefaultFormat(void)
{
    WAVEFORMATEX wf = SystemAudioCapture::GetDefaultFormat();
    AudioCaptureFormat fmt;
    fmt.sample_rate = wf.nSamplesPerSec;
    fmt.num_channels = wf.nChannels;
    fmt.bits_per_sample = wf.wBitsPerSample;
    return fmt;
}

} // extern "C"
