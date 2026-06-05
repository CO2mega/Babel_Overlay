#include "system_audio_capture.hpp"
#include <cstdio>
#include <mmreg.h>
#include <cmath>

WAVEFORMATEX SystemAudioCapture::GetDefaultFormat()
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

void SystemAudioCapture::InitClientDesktopOutput()
{
    // 获取默认音频渲染设备（扬声器/耳机）
    wil::com_ptr<IMMDeviceEnumerator> device_enumerator;
    THROW_IF_FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL,
                                     __uuidof(IMMDeviceEnumerator), 
                                     (void**)device_enumerator.put_void()));

    wil::com_ptr<IMMDevice> device;
    THROW_IF_FAILED(device_enumerator->GetDefaultAudioEndpoint(eRender, eConsole, device.put()));

    // 直接激活音频客户端
    THROW_IF_FAILED(device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL,
                                     (void**)client.put_void()));

    // 获取混音格式（注意：需要二级指针）
    WAVEFORMATEX* mix_format = nullptr;
    THROW_IF_FAILED(client->GetMixFormat(&mix_format));
    
    // ? 复制格式信息
    format = *mix_format;
    
    // ? 打印详细的格式信息用于调试
    printf("[Desktop] Audio Format Details:\n");
    printf("  Sample Rate: %u Hz\n", format.nSamplesPerSec);
    printf("  Channels: %u\n", format.nChannels);
    printf("  Bits Per Sample: %u\n", format.wBitsPerSample);
    printf("  Format Tag: 0x%04X (", format.wFormatTag);
    
    switch (format.wFormatTag) {
    case WAVE_FORMAT_PCM:
        printf("PCM");
        break;
    case WAVE_FORMAT_IEEE_FLOAT:
        printf("IEEE Float");
        break;
    case WAVE_FORMAT_EXTENSIBLE:
        printf("Extensible");
        // ? 对于 EXTENSIBLE 格式，需要检查子格式
        if (format.cbSize >= sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)) {
            WAVEFORMATEXTENSIBLE* ext_format = (WAVEFORMATEXTENSIBLE*)mix_format;
            printf(" - SubFormat: ");
            
            if (ext_format->SubFormat == KSDATAFORMAT_SUBTYPE_PCM) {
                printf("PCM");
            } else if (ext_format->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT) {
                printf("IEEE Float");
            } else {
                printf("Other");
            }
        }
        break;
    default:
        printf("Unknown");
        break;
    }
    printf(")\n");
    printf("  Block Align: %u bytes\n", format.nBlockAlign);
    printf("  Avg Bytes/Sec: %u\n", format.nAvgBytesPerSec);
    printf("  cbSize: %u\n", format.cbSize);
    
    // ? 关键修复：如果是 EXTENSIBLE 格式，转换为标准格式
    if (format.wFormatTag == WAVE_FORMAT_EXTENSIBLE && format.cbSize > 0) {
        WAVEFORMATEXTENSIBLE* ext_format = (WAVEFORMATEXTENSIBLE*)mix_format;
        
        // 检查子格式并转换
        if (ext_format->SubFormat == KSDATAFORMAT_SUBTYPE_PCM) {
            // 转换为标准 PCM 格式
            format.wFormatTag = WAVE_FORMAT_PCM;
            format.cbSize = 0;
            printf("[Desktop] Converted EXTENSIBLE PCM to standard PCM format\n");
        } else if (ext_format->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT) {
            // 转换为标准 IEEE Float 格式
            format.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
            format.cbSize = 0;
            printf("[Desktop] Converted EXTENSIBLE IEEE_FLOAT to standard IEEE_FLOAT format\n");
        } else {
            // 不支持的子格式，使用默认的 IEEE_FLOAT
            printf("[Desktop] Warning: Unknown subformat, using default IEEE_FLOAT\n");
            format.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
            format.cbSize = 0;
        }
    }
    
    printf("[Desktop] Final format - Tag: 0x%04X, Channels: %u, BitsPerSample: %u\n",
           format.wFormatTag, format.nChannels, format.wBitsPerSample);
    
    // 释放分配的内存
    CoTaskMemFree(mix_format);

    // ? Desktop Loopback 初始化
    // 关键：对于 Desktop Loopback，BufferDuration 和 Periodicity 都必须为 0
    THROW_IF_FAILED(client->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                       AUDCLNT_STREAMFLAGS_LOOPBACK | 
                                       AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
                                       0,      // BufferDuration = 0（Desktop Loopback 必须为 0）
                                       0,      // Periodicity = 0（Desktop Loopback 必须为 0）
                                       &format, 
                                       NULL));

    THROW_IF_FAILED(client->SetEventHandle(events[CaptureEvents::PacketReady].get()));
    
    printf("[Desktop] Audio client initialized successfully\n");
}

void SystemAudioCapture::InitClientProcessLoopback()
{
    // 进程 Loopback 模式（捕获单一应用）
    AUDIOCLIENT_ACTIVATION_PARAMS params = {
        .ActivationType = AUDIOCLIENT_ACTIVATION_TYPE_PROCESS_LOOPBACK,
        .ProcessLoopbackParams = {
            .TargetProcessId = target_pid,
            .ProcessLoopbackMode = PROCESS_LOOPBACK_MODE_INCLUDE_TARGET_PROCESS_TREE,
        },
    };

    PROPVARIANT prop_variant = {};
    prop_variant.vt = VT_BLOB;
    prop_variant.blob.cbSize = sizeof(params);
    prop_variant.blob.pBlobData = (BYTE *)&params;

    wil::com_ptr<IActivateAudioInterfaceAsyncOperation> async_op;
    auto completion_handler = Make<CompletionHandler>();

    THROW_IF_FAILED(ActivateAudioInterfaceAsync(VIRTUAL_AUDIO_DEVICE_PROCESS_LOOPBACK,
                                                __uuidof(IAudioClient), &prop_variant,
                                                completion_handler.Get(), &async_op));

    completion_handler->event_finished.wait();
    THROW_IF_FAILED(completion_handler->activate_hr);

    client = completion_handler->client;

    printf("[Process %lu] Audio Format: %u Hz, %u channels, %u bits\n", 
           target_pid, format.nSamplesPerSec, format.nChannels, format.wBitsPerSample);

    // ? Process Loopback 初始化
    THROW_IF_FAILED(client->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                       AUDCLNT_STREAMFLAGS_LOOPBACK | 
                                       AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
                                       0,      // BufferDuration = 0
                                       0,      // Periodicity = 0
                                       &format, 
                                       NULL));

    THROW_IF_FAILED(client->SetEventHandle(events[CaptureEvents::PacketReady].get()));
    
    printf("[Process %lu] Audio client initialized successfully\n", target_pid);
}

void SystemAudioCapture::InitClient()
{
    if (audio_source == DESKTOP_OUTPUT) {
        InitClientDesktopOutput();
    } else {
        InitClientProcessLoopback();
    }
}

void SystemAudioCapture::InitCapture()
{
    InitClient();
    THROW_IF_FAILED(client->GetService(__uuidof(IAudioCaptureClient), 
                                       capture_client.put_void()));
    
    // 获取实际的缓冲区大小
    UINT32 buffer_frame_count = 0;
    THROW_IF_FAILED(client->GetBufferSize(&buffer_frame_count));
    printf("[Audio] Buffer frame count: %u frames (%u ms)\n", 
           buffer_frame_count, 
           (buffer_frame_count * 1000) / format.nSamplesPerSec);
}

// ? 从 int16 转换为 float
static void ConvertInt16ToFloat(const int16_t* src, float* dst, UINT32 num_samples)
{
    const float scale = 1.0f / 32768.0f;
    for (UINT32 i = 0; i < num_samples; i++) {
        dst[i] = static_cast<float>(src[i]) * scale;
    }
}

// ? 从 int32 转换为 float
static void ConvertInt32ToFloat(const int32_t* src, float* dst, UINT32 num_samples)
{
    const float scale = 1.0f / 2147483648.0f;
    for (UINT32 i = 0; i < num_samples; i++) {
        dst[i] = static_cast<float>(src[i]) * scale;
    }
}

void SystemAudioCapture::ForwardPacket()
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
            // ? 根据实际格式处理数据
            if (format.wFormatTag == WAVE_FORMAT_IEEE_FLOAT && format.wBitsPerSample == 32) {
                // 直接处理 float32 数据
                float* audio_data = reinterpret_cast<float*>(new_data);
                
                switch (conversion_mode) {
                case STEREO_TO_MONO_AVG:
                    if (format.nChannels >= 2) {
                        AudioConverter::StereoToMonoSimple(audio_data, num_frames, convert_buffer);
                        audio_callback(qpc_position, convert_buffer.data(), num_frames);
                    } else {
                        audio_callback(qpc_position, audio_data, num_frames);
                    }
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
                    if (format.nChannels >= 2) {
                        AudioConverter::ExtractChannel(audio_data, num_frames, format.nChannels, 1, convert_buffer);
                        audio_callback(qpc_position, convert_buffer.data(), num_frames);
                    } else {
                        AudioConverter::ExtractChannel(audio_data, num_frames, format.nChannels, 0, convert_buffer);
                        audio_callback(qpc_position, convert_buffer.data(), num_frames);
                    }
                    break;

                case PASSTHROUGH:
                default:
                    audio_callback(qpc_position, audio_data, num_frames * format.nChannels);
                    break;
                }
            } else if (format.wFormatTag == WAVE_FORMAT_PCM && format.wBitsPerSample == 16) {
                // ? 处理 int16 数据：先转换为 float
                int16_t* int_data = reinterpret_cast<int16_t*>(new_data);
                UINT32 num_samples = num_frames * format.nChannels;
                
                convert_buffer.resize(num_samples);
                ConvertInt16ToFloat(int_data, convert_buffer.data(), num_samples);
                
                // 现在用 float 数据处理
                switch (conversion_mode) {
                case STEREO_TO_MONO_AVG:
                    if (format.nChannels >= 2) {
                        std::vector<float> mono_buffer;
                        AudioConverter::StereoToMonoSimple(convert_buffer.data(), num_frames, mono_buffer);
                        audio_callback(qpc_position, mono_buffer.data(), num_frames);
                    } else {
                        audio_callback(qpc_position, convert_buffer.data(), num_frames);
                    }
                    break;

                default:
                    audio_callback(qpc_position, convert_buffer.data(), num_samples);
                    break;
                }
            } else if (format.wFormatTag == WAVE_FORMAT_PCM && format.wBitsPerSample == 32) {
                // ? 处理 int32 数据
                int32_t* int_data = reinterpret_cast<int32_t*>(new_data);
                UINT32 num_samples = num_frames * format.nChannels;
                
                convert_buffer.resize(num_samples);
                ConvertInt32ToFloat(int_data, convert_buffer.data(), num_samples);
                
                audio_callback(qpc_position, convert_buffer.data(), num_samples);
            }
        }

        // 处理音频缓冲区标志
        if (flags & AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY) {
            fprintf(stderr, "[Warning] Data discontinuity detected\n");
        }
        if (flags & AUDCLNT_BUFFERFLAGS_TIMESTAMP_ERROR) {
            fprintf(stderr, "[Warning] Timestamp error detected\n");
        }

        THROW_IF_FAILED(capture_client->ReleaseBuffer(num_frames));
        THROW_IF_FAILED(capture_client->GetNextPacketSize(&num_frames));
    }
}

void SystemAudioCapture::Capture()
{
    InitCapture();
    THROW_IF_FAILED(client->Start());
    
    printf("[Audio] Capture started\n");

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
            fprintf(stderr, "[Error] Invalid event ID: %lu\n", event_id);
            shutdown = true;
            break;
        }
    }

    THROW_IF_FAILED(client->Stop());
    printf("[Audio] Capture stopped\n");
}

void SystemAudioCapture::CaptureSafe()
{
    try {
        Capture();
    } catch (wil::ResultException e) {
        fprintf(stderr, "Audio capture error: %s (0x%lx)\n", e.what(), e.GetErrorCode());
    }
}

// 构造函数 1：桌面全部音频
SystemAudioCapture::SystemAudioCapture(const AudioCallback& callback,
                                       const WAVEFORMATEX& format,
                                       ConversionMode mode)
    : audio_callback(callback), format(format), conversion_mode(mode),
      audio_source(DESKTOP_OUTPUT), target_pid(0)
{
    for (auto &event : events)
        event.create();

    capture_thread = std::thread(&SystemAudioCapture::CaptureSafe, this);
}

// 构造函数 2：单一应用音频
SystemAudioCapture::SystemAudioCapture(DWORD pid, const AudioCallback& callback,
                                       const WAVEFORMATEX& format,
                                       ConversionMode mode)
    : audio_callback(callback), format(format), conversion_mode(mode),
      audio_source(PROCESS_LOOPBACK), target_pid(pid)
{
    for (auto &event : events)
        event.create();

    capture_thread = std::thread(&SystemAudioCapture::CaptureSafe, this);
}

SystemAudioCapture::~SystemAudioCapture()
{
    events[CaptureEvents::Shutdown].SetEvent();
    if (capture_thread.joinable())
        capture_thread.join();
}