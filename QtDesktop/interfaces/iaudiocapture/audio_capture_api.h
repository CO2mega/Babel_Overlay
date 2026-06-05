#ifndef AUDIO_CAPTURE_API_H
#define AUDIO_CAPTURE_API_H

#ifdef AUDIO_CAPTURE_EXPORTS
#define AUDIO_CAPTURE_API __declspec(dllexport)
#else
#define AUDIO_CAPTURE_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void* AudioCaptureHandle;

typedef void (*AudioCaptureCallback)(unsigned long long timestamp, float* data, unsigned int num_frames);

typedef enum {
    AUDIO_CAPTURE_PASS_THROUGH = 0,
    AUDIO_CAPTURE_STEREO_TO_MONO_AVG = 1,
    AUDIO_CAPTURE_STEREO_TO_MONO_WEIGHT = 2,
    AUDIO_CAPTURE_EXTRACT_LEFT = 3,
    AUDIO_CAPTURE_EXTRACT_RIGHT = 4
} AudioConversionMode;

typedef struct {
    unsigned int sample_rate;
    unsigned short num_channels;
    unsigned short bits_per_sample;
} AudioCaptureFormat;

AUDIO_CAPTURE_API AudioCaptureHandle DesktopCapture_Create(
    AudioCaptureFormat format,
    AudioConversionMode mode,
    AudioCaptureCallback callback);

AUDIO_CAPTURE_API AudioCaptureHandle ProcessCapture_Create(
    unsigned long target_pid,
    AudioCaptureFormat format,
    AudioConversionMode mode,
    AudioCaptureCallback callback);

AUDIO_CAPTURE_API void AudioCapture_Destroy(AudioCaptureHandle handle);

AUDIO_CAPTURE_API float AudioCapture_GetRMS(const float* data, unsigned int num_samples);

AUDIO_CAPTURE_API float AudioCapture_GetPeak(const float* data, unsigned int num_samples);

AUDIO_CAPTURE_API AudioCaptureFormat AudioCapture_GetDefaultFormat(void);

#ifdef __cplusplus
}
#endif

#endif // AUDIO_CAPTURE_API_H
