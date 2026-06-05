using NAudio.CoreAudioApi;
using NAudio.Wave;

namespace NetDesktop.Services.Audio;

/// <summary>
/// 通过 WASAPI Loopback 捕获系统音频输出，将 PCM 数据以 float[] 形式推送。
/// </summary>
public class AudioCaptureService : IDisposable
{
    private WasapiLoopbackCapture? _capture;
    private int _callCount;

    /// <summary>
    /// 音频数据到达时触发，参数为浮点采样数组和声道数。
    /// </summary>
    public event Action<float[], int>? AudioDataAvailable;
    
    public event Action<string>? Error;

    /// <summary>
    /// 当前捕获设备的音频格式（采样率、声道、位深）。
    /// </summary>
    public WaveFormat CaptureFormat => _capture?.WaveFormat ?? new WaveFormat();

    /// <summary>
    /// 启动 WASAPI Loopback 捕获，开始监听系统音频。
    /// </summary>
    public void Start()
    {
        try
        {
            _capture = new WasapiLoopbackCapture();
            var fmt = _capture.WaveFormat;
            Console.WriteLine($"[Audio] 捕获设备格式: {fmt.SampleRate}Hz {fmt.Channels}ch {fmt.BitsPerSample}bit encoding={fmt.Encoding}");
            _capture.DataAvailable += OnDataAvailable;
            _capture.RecordingStopped += (_, e) =>
            {
                Console.WriteLine($"[Audio] 录音停止: {e.Exception?.Message ?? "正常"}");
                if (e.Exception != null) Error?.Invoke($"录音停止异常: {e.Exception.Message}");
            };
            _capture.StartRecording();
            Console.WriteLine("[Audio] WASAPI loopback 已启动，等待系统音频...");
        }
        catch (Exception ex)
        {
            Console.WriteLine($"[Audio] 启动失败: {ex.Message}");
            Error?.Invoke($"音频捕获启动失败: {ex.Message}");
        }
    }

    private void OnDataAvailable(object? sender, WaveInEventArgs e)
    {
        if (e.BytesRecorded == 0) return;

        _callCount++;
        if (_callCount <= 3 || _callCount % 100 == 0)
            Console.WriteLine($"[Audio] 收到数据 #{_callCount}: {e.BytesRecorded} bytes");

        var format = _capture!.WaveFormat;
        int floatCount = e.BytesRecorded / 4;
        var samples = new float[floatCount];
        Buffer.BlockCopy(e.Buffer, 0, samples, 0, e.BytesRecorded);

        AudioDataAvailable?.Invoke(samples, format.Channels);
    }

    public void Stop()
    {
        _capture?.StopRecording();
    }

    public void Dispose()
    {
        Stop();
        _capture?.Dispose();
        _capture = null;
    }
}
