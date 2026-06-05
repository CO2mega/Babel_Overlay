#pragma once

#include <functional>
#include <thread>
#include <array>
#include <vector>
#include <windows.h>
#include <audiopolicy.h>
#include <audioclient.h>
#include <audioclientactivationparams.h>
#include <mmdeviceapi.h>
#include <wrl/implements.h>
#include <wil/com.h>
#include <wil/result.h>

#include "audio_converter.hpp"

using namespace Microsoft::WRL;

// 音频数据回调接口
using AudioCallback = std::function<void(UINT64 timestamp, float* data, UINT32 num_frames)>;

struct CompletionHandler : public RuntimeClass<RuntimeClassFlags<ClassicCom>, FtmBase,
                                               IActivateAudioInterfaceCompletionHandler> {
    wil::com_ptr<IAudioClient> client;
    HRESULT activate_hr = E_FAIL;
    wil::unique_event event_finished;

    CompletionHandler() { event_finished.create(); }

    STDMETHOD(ActivateCompleted)(IActivateAudioInterfaceAsyncOperation *operation)
    {
        auto set_finished = event_finished.SetEvent_scope_exit();
        RETURN_IF_FAILED(operation->GetActivateResult(&activate_hr, client.put_unknown()));
        return S_OK;
    }
};

namespace CaptureEvents {
enum CaptureEvents {
    PacketReady = 0,
    Shutdown = 1,
    Count = 2,
};
}

class SimpleAudioCapture {
public:
    // 声道转换模式
    enum ConversionMode {
        PASS_THROUGH,           // 原始音频（不转换）
        STEREO_TO_MONO_AVG,    // 立体声转单声道（平均）
        STEREO_TO_MONO_WEIGHT, // 立体声转单声道（加权）
        EXTRACT_LEFT,          // 仅提取左声道
        EXTRACT_RIGHT,         // 仅提取右声道
    };

private:
    DWORD pid;
    AudioCallback audio_callback;
    ConversionMode conversion_mode;

    wil::unique_couninitialize_call couninit{wil::CoInitializeEx()};
    wil::com_ptr<IAudioClient> client;
    wil::com_ptr<IAudioCaptureClient> capture_client;

    WAVEFORMATEX format;
    std::array<wil::unique_event, CaptureEvents::Count> events;
    std::thread capture_thread;

    // 缓冲区（用于声道转换）
    std::vector<float> convert_buffer;

    AUDIOCLIENT_ACTIVATION_PARAMS GetParams();
    PROPVARIANT GetPropvariant(AUDIOCLIENT_ACTIVATION_PARAMS *params);
    void InitClient();
    void InitCapture();
    void ForwardPacket();
    void Capture();
    void CaptureSafe();

public:
    SimpleAudioCapture(DWORD pid, const AudioCallback& callback,
                      const WAVEFORMATEX& format = GetDefaultFormat(),
                      ConversionMode mode = PASS_THROUGH);

    ~SimpleAudioCapture();

    DWORD GetPid() const { return pid; }
    WAVEFORMATEX GetFormat() const { return format; }
    ConversionMode GetConversionMode() const { return conversion_mode; }

    static WAVEFORMATEX GetDefaultFormat();
};