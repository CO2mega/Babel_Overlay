#pragma once

#include <functional>
#include <thread>
#include <array>
#include <vector>
#include <windows.h>
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#include <audioclient.h>
#include <audioclientactivationparams.h>
#include <wrl/implements.h>
#include <wil/com.h>
#include <wil/result.h>
#include "audio_converter.hpp"

using namespace Microsoft::WRL;

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

class SystemAudioCapture {
public:
    // 声道转换模式
    enum ConversionMode {
        PASS_THROUGH,           // 原始音频
        STEREO_TO_MONO_AVG,    // 立体声转单声道（平均）
        STEREO_TO_MONO_WEIGHT, // 立体声转单声道（加权）
        EXTRACT_LEFT,          // 仅左声道
        EXTRACT_RIGHT,         // 仅右声道
    };

    // 音频源类型
    enum AudioSource {
        DESKTOP_OUTPUT,        // 桌面全部音频（扬声器/耳机）
        PROCESS_LOOPBACK,      // 单一应用音频（需指定 PID）
    };

private:
    AudioCallback audio_callback;
    ConversionMode conversion_mode;
    AudioSource audio_source;
    DWORD target_pid;  // 仅在 PROCESS_LOOPBACK 模式时使用

    wil::unique_couninitialize_call couninit{wil::CoInitializeEx()};
    wil::com_ptr<IAudioClient> client;
    wil::com_ptr<IAudioCaptureClient> capture_client;

    WAVEFORMATEX format;
    std::array<wil::unique_event, CaptureEvents::Count> events;
    std::thread capture_thread;

    std::vector<float> convert_buffer;

    // 初始化方法
    void InitClientDesktopOutput();
    void InitClientProcessLoopback();
    void InitClient();
    void InitCapture();
    void ForwardPacket();
    void Capture();
    void CaptureSafe();

public:
    // 捕获桌面全部音频
    SystemAudioCapture(const AudioCallback& callback,
                      const WAVEFORMATEX& format = GetDefaultFormat(),
                      ConversionMode mode = PASS_THROUGH);

    // 捕获单一应用音频
    SystemAudioCapture(DWORD pid, const AudioCallback& callback,
                      const WAVEFORMATEX& format = GetDefaultFormat(),
                      ConversionMode mode = PASS_THROUGH);

    ~SystemAudioCapture();

    WAVEFORMATEX GetFormat() const { return format; }
    ConversionMode GetConversionMode() const { return conversion_mode; }
    AudioSource GetAudioSource() const { return audio_source; }
    DWORD GetTargetPid() const { return target_pid; }

    static WAVEFORMATEX GetDefaultFormat();
};