using System.IO;
using SherpaOnnx;
using NetDesktop.Models;

namespace NetDesktop.Services.Stt;

/// <summary>
/// Silero VAD 语音活动检测 + SenseVoice 离线语音识别引擎。
/// VAD 根据静音切分语音段，每段送入 SenseVoice 解码出文本。
/// </summary>
public class SttService : IDisposable
{
    private VoiceActivityDetector? _vad;
    private OfflineRecognizer? _recognizer;
    private readonly object _vadLock = new();

    /// <summary>
    /// 识别到一段语音文本后触发，参数为包含原文、时间戳和时长的 SubtitleEntry。
    /// </summary>
    public event Action<SubtitleEntry>? OnSpeechRecognized;

    /// <summary>
    /// 模型加载失败、解码异常等错误发生时触发。
    /// </summary>
    public event Action<string>? Error;

    private float _currentMinSilence;
    private float _currentThreshold;
    private readonly string _modelPath;
    private readonly string _tokensPath;
    private int _audioChunkCount;
    private int _segmentCount;

    /// <summary>
    /// 创建 STT 服务实例。构造后须调用 <see cref="Initialize"/> 加载模型。
    /// </summary>
    /// <param name="settings">应用配置，提供模型路径和 VAD 参数。</param>
    public SttService(SettingsModel settings)
    {
        _modelPath = settings.SenseVoiceModelPath;
        _tokensPath = settings.SenseVoiceTokensPath;
        _currentMinSilence = settings.MinSilenceDuration;
        _currentThreshold = settings.VadThreshold;
    }

    /// <summary>
    /// VAD 是否正在检测到语音活动（用于 UI 状态指示灯轮询）。
    /// </summary>
    public bool IsSpeechActive
    {
        get { lock (_vadLock) { return _vad?.IsSpeechDetected() ?? false; } }
    }

    /// <summary>
    /// 加载 Silero VAD 和 SenseVoice 模型。应在 Error 事件挂载后调用。
    /// </summary>
    /// <param name="settings">应用配置。</param>
    public void Initialize(SettingsModel settings)
    {
        Console.WriteLine("[STT] 初始化...");
        InitVad(settings);
        InitRecognizer();
    }
/// <summary>
/// 初始化VAD模型
/// </summary>
/// <param name="settings"></param>
    private void InitVad(SettingsModel settings)
    {
        var path = settings.SileroVadModelPath;
        Console.WriteLine($"[VAD] 模型路径: {Path.GetFullPath(path)}");
        if (!File.Exists(path))
        {
            var msg = $"VAD模型不存在: {path}";
            Console.WriteLine($"[VAD] 错误: {msg}");
            Error?.Invoke(msg + "\n下载: https://github.com/k2-fsa/sherpa-onnx/releases/download/asr-models/silero_vad.onnx");
            return;
        }

        Console.WriteLine($"[VAD] 加载模型 threshold={settings.VadThreshold} minSilence={settings.MinSilenceDuration}s");
        var vadConfig = new VadModelConfig
        {
            SileroVad = new SileroVadModelConfig
            {
                Model = path,
                Threshold = settings.VadThreshold,
                MinSilenceDuration = settings.MinSilenceDuration,
                MinSpeechDuration = settings.MinSpeechDuration,
                WindowSize = 512,
                MaxSpeechDuration = 6f   // 最长6s一段，提升实时性
            },
            SampleRate = 16000,
            NumThreads = 2
        };

        lock (_vadLock)
        {
            _vad?.Dispose();
            _vad = new VoiceActivityDetector(vadConfig, 60f);
        }
        Console.WriteLine("[VAD] 加载成功");
    }
/// <summary>
/// 初始化SenseVoice模型
/// </summary>
    private void InitRecognizer()
    {
        Console.WriteLine($"[STT] SenseVoice模型: {Path.GetFullPath(_modelPath)}");
        if (!File.Exists(_modelPath))
        {
            var msg = $"SenseVoice模型不存在: {_modelPath}";
            Console.WriteLine($"[STT] 错误: {msg}");
            Error?.Invoke(msg + "\n下载: https://huggingface.co/csukuangfj/sherpa-onnx-sense-voice-zh-en-ja-ko-yue-2024-07-17");
            return;
        }
        if (!File.Exists(_tokensPath))
        {
            var msg = $"tokens.txt不存在: {_tokensPath}";
            Console.WriteLine($"[STT] 错误: {msg}");
            Error?.Invoke(msg);
            return;
        }

        Console.WriteLine("[STT] 加载SenseVoice模型中...");
        var config = new OfflineRecognizerConfig
        {
            FeatConfig = new FeatureConfig { SampleRate = 16000, FeatureDim = 80 },
            ModelConfig = new OfflineModelConfig
            {
                SenseVoice = new OfflineSenseVoiceModelConfig
                {
                    Model = _modelPath,
                    Language = "en",
                    UseInverseTextNormalization = 1
                },
                Tokens = _tokensPath,
                NumThreads = 4,
                Provider = "cpu"
            },
            DecodingMethod = "greedy_search"
        };

        _recognizer?.Dispose();
        _recognizer = new OfflineRecognizer(config);
        Console.WriteLine("[STT] SenseVoice加载成功");
    }

    /// <summary>
    /// 将已处理的16kHz 单声道浮点采样送入 VAD + STT 管道。
    /// VAD 检测到完整语音段后触发 OnSpeechRecognized
    /// </summary>
    /// <param name="samples16kHzMono">16kHz 单声道浮点采样数组。</param>
    public void ProcessAudio(float[] samples16kHzMono)
    {
        lock (_vadLock)
        {
            if (_vad == null || _recognizer == null) return;

            _audioChunkCount++;
            if (_audioChunkCount <= 3 || _audioChunkCount % 200 == 0)
                Console.WriteLine($"[VAD] 送入音频 #{_audioChunkCount}: {samples16kHzMono.Length} samples, speech={_vad.IsSpeechDetected()}");

            _vad.AcceptWaveform(samples16kHzMono);

            while (!_vad.IsEmpty())
            {
                var segment = _vad.Front();
                _segmentCount++;
                Console.WriteLine($"[VAD] 检测到语音段 #{_segmentCount}: {segment.Samples.Length} samples ({segment.Samples.Length / 16000.0:F2}s)");

                if (segment.Samples.Length > 0)
                {
                    try
                    {
                        var stream = _recognizer!.CreateStream();
                        stream.AcceptWaveform(16000, segment.Samples);
                        _recognizer.Decode(stream);
                        var text = stream.Result.Text?.Trim() ?? string.Empty;
                        Console.WriteLine($"[STT] 识别结果: \"{text}\"");

                        if (!string.IsNullOrWhiteSpace(text))
                        {
                            OnSpeechRecognized?.Invoke(new SubtitleEntry
                            {
                                OriginalText = text,
                                Timestamp = DateTime.Now,
                                Duration = TimeSpan.FromSeconds(segment.Samples.Length / 16000.0)
                            });
                        }
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine($"[STT] 解码异常: {ex.Message}");
                        Error?.Invoke($"STT解码错误: {ex.Message}");
                    }
                }
                _vad.Pop();
            }
        }
    }
    

    /// <summary>
    /// 使用当前设置重建 VAD 实例，立即生效。用于实时调整参数。
    /// </summary>
    /// <param name="settings">应用配置。</param>
    public void RebuildVad(SettingsModel settings)
    {
        Console.WriteLine($"[VAD] 重建 minSilence={settings.MinSilenceDuration}s threshold={settings.VadThreshold}");
        InitVad(settings);
    }

    /// <summary>
    /// 释放 VAD 和 SenseVoice 识别器资源。
    /// </summary>
    public void Dispose()
    {
        lock (_vadLock) { _vad?.Dispose(); _vad = null; }
        _recognizer?.Dispose();
        _recognizer = null;
    }
}
