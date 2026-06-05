#include "audio_capture.hpp"
#include <cstdio>

WAVEFORMATEX SimpleAudioCapture::GetDefaultFormat()
{
    WAVEFORMATEX format = {};
    format.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    format.nChannels = 2;
    format.nSamplesPerSec = 48000;
    format.wBitsPerSample = 32;
    format.nBlockAlign = format.nChannels * (format.wBitsPerSample / 8);
    format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
    format.cbSize = 0;
    return format;
}

AUDIOCLIENT_ACTIVATION_PARAMS SimpleAudioCapture::GetParams()
{
    auto mode = PROCESS_LOOPBACK_MODE_INCLUDE_TARGET_PROCESS_TREE;
    return {
        .ActivationType = AUDIOCLIENT_ACTIVATION_TYPE_PROCESS_LOOPBACK,
        .ProcessLoopbackParams = {
            .TargetProcessId = pid,
            .ProcessLoopbackMode = mode,
        },
    };
}

PROPVARIANT SimpleAudioCapture::GetPropvariant(AUDIOCLIENT_ACTIVATION_PARAMS *params)
{
    return {
        .vt = VT_BLOB,
        .blob = {
            .cbSize = sizeof(*params),
            .pBlobData = (BYTE *)params,
        },
    };
}

void SimpleAudioCapture::InitClient()
{
    auto params = GetParams();
    auto propvariant = GetPropvariant(&params);

    wil::com_ptr<IActivateAudioInterfaceAsyncOperation> async_op;
    auto completion_handler = Make<CompletionHandler>();

    THROW_IF_FAILED(ActivateAudioInterfaceAsync(VIRTUAL_AUDIO_DEVICE_PROCESS_LOOPBACK,
                                                __uuidof(IAudioClient), &propvariant,
                                                completion_handler.Get(), &async_op));

    completion_handler->event_finished.wait();
    THROW_IF_FAILED(completion_handler->activate_hr);

    client = completion_handler->client;

    THROW_IF_FAILED(client->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                       AUDCLNT_STREAMFLAGS_LOOPBACK | 
                                       AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
                                       5 * 10000000, 0, &format, NULL));

    THROW_IF_FAILED(client->SetEventHandle(events[CaptureEvents::PacketReady].get()));
}

void SimpleAudioCapture::InitCapture()
{
    InitClient();
    THROW_IF_FAILED(client->GetService(__uuidof(IAudioCaptureClient), 
                                       capture_client.put_void()));
}

void SimpleAudioCapture::ForwardPacket()
{
    UINT32 num_frames = 0;
    THROW_IF_FAILED(capture_client->GetNextPacketSize(&num_frames));

    while (num_frames > 0) {
        BYTE *new_data;
        DWORD flags;
        UINT64 qpc_position;

        THROW_IF_FAILED(capture_client->GetBuffer(&new_data, &num_frames, &flags, NULL,
                                                    &qpc_position));

        if (!(flags & AUDCLNT_BUFFERFLAGS_SILENT)) {
            float* audio_data = reinterpret_cast<float *>(new_data);
            
            switch (conversion_mode) {
            case STEREO_TO_MONO_AVG:
                AudioConverter::StereoToMonoSimple(audio_data, num_frames, convert_buffer);
                audio_callback(qpc_position, convert_buffer.data(), num_frames);
                break;

            case STEREO_TO_MONO_WEIGHT:
                AudioConverter::StereoToMonoWeighted(audio_data, num_frames, 
                                                    format.nChannels, convert_buffer);
                audio_callback(qpc_position, convert_buffer.data(), num_frames);
                break;

            case EXTRACT_LEFT:
                AudioConverter::ExtractChannel(audio_data, num_frames, format.nChannels, 0, convert_buffer);
                audio_callback(qpc_position, convert_buffer.data(), num_frames);
                break;

            case EXTRACT_RIGHT:
                AudioConverter::ExtractChannel(audio_data, num_frames, format.nChannels, 1, convert_buffer);
                audio_callback(qpc_position, convert_buffer.data(), num_frames);
                break;

            case PASS_THROUGH:
            default:
                audio_callback(qpc_position, audio_data, num_frames * format.nChannels);
                break;
            }
        }

        THROW_IF_FAILED(capture_client->ReleaseBuffer(num_frames));
        THROW_IF_FAILED(capture_client->GetNextPacketSize(&num_frames));
    }
}

void SimpleAudioCapture::Capture()
{
    InitCapture();
    THROW_IF_FAILED(client->Start());

    bool shutdown = false;
    while (!shutdown) {
        auto event_id = WaitForMultipleObjects(events.size(), events[0].addressof(), FALSE,
                                               INFINITE);

        switch (event_id) {
        case CaptureEvents::PacketReady:
            ForwardPacket();
            break;

        case CaptureEvents::Shutdown:
            shutdown = true;
            break;

        default:
            shutdown = true;
            break;
        }
    }

    THROW_IF_FAILED(client->Stop());
}

void SimpleAudioCapture::CaptureSafe()
{
    try {
        Capture();
    } catch (wil::ResultException e) {
        fprintf(stderr, "Audio capture error: %s (0x%lx)\n", e.what(), e.GetErrorCode());
    }
}

SimpleAudioCapture::SimpleAudioCapture(DWORD pid, const AudioCallback& callback,
                                       const WAVEFORMATEX& format,
                                       ConversionMode mode)
    : pid(pid), audio_callback(callback), format(format), conversion_mode(mode)
{
    for (auto &event : events)
        event.create();

    capture_thread = std::thread(&SimpleAudioCapture::CaptureSafe, this);
}

SimpleAudioCapture::~SimpleAudioCapture()
{
    events[CaptureEvents::Shutdown].SetEvent();
    if (capture_thread.joinable())
        capture_thread.join();
}