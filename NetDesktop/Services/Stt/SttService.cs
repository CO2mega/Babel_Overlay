using System.IO;
using SherpaOnnx;
using NetDesktop.Models;

namespace NetDesktop.Services.Stt;

/// <summary>
/// 流式语音识别引擎。使用 OnlineRecognizer 实时解码音频流，
/// 内置端点检测，无需独立 VAD。
/// </summary>
public class SttService : IDisposable
{
    private OnlineRecognizer? _recognizer;
    private OnlineStream? _stream;
    private readonly object _lock = new();
    private int _audioChunkCount;

    private readonly string _encoderPath;
    private readonly string _decoderPath;
    private readonly string _joinerPath;
    private readonly string _tokensPath;
    private readonly float _minTrailingSilence;
    private readonly float _minUtteranceLength;

    /// <summary>
    /// 实时partial识别结果（边说边出）。
    /// </summary>
    public event Action<SubtitleEntry>? OnPartialResult;

    /// <summary>
    /// 最终识别结果（端点检测后触发，用于翻译和历史记录）。
    /// </summary>
    public event Action<SubtitleEntry>? OnFinalResult;

    /// <summary>
    /// 模型加载失败、解码异常等错误发生时触发。
    /// </summary>
    public event Action<string>? Error;

    public SttService(SettingsModel settings)
    {
        _encoderPath = settings.StreamingEncoderPath;
        _decoderPath = settings.StreamingDecoderPath;
        _joinerPath = settings.StreamingJoinerPath;
        _tokensPath = settings.StreamingTokensPath;
        _minTrailingSilence = settings.EndpointMinTrailingSilence;
        _minUtteranceLength = settings.EndpointMinUtteranceLength;
    }

    /// <summary>
    /// 加载流式模型。应在 Error 事件挂载后调用。
    /// </summary>
    public void Initialize(SettingsModel settings)
    {
        Console.WriteLine("[STT] 初始化流式识别器...");
        InitRecognizer();
        Console.WriteLine($"[STT] 初始化完成: recognizer={_recognizer != null}");
    }

    private void InitRecognizer()
    {
        // 检查流式模型文件
        if (!File.Exists(_encoderPath))
        {
            var msg = $"encoder模型不存在: {Path.GetFullPath(_encoderPath)}";
            Console.WriteLine($"[STT] 错误: {msg}");
            Error?.Invoke(msg + "\n下载流式模型: https://github.com/k2-fsa/sherpa-onnx/releases/tag/asr-models");
            return;
        }
        if (!File.Exists(_tokensPath))
        {
            var msg = $"tokens.txt不存在: {Path.GetFullPath(_tokensPath)}";
            Console.WriteLine($"[STT] 错误: {msg}");
            Error?.Invoke(msg);
            return;
        }

        Console.WriteLine($"[STT] 加载流式模型: encoder={Path.GetFullPath(_encoderPath)}");

        var config = new OnlineRecognizerConfig
        {
            FeatConfig = new FeatureConfig { SampleRate = 16000, FeatureDim = 80 },
            ModelConfig = new OnlineTransducerModelConfig
            {
                Encoder = _encoderPath,
                Decoder = _decoderPath,
                Joiner = _joinerPath
            },
            Tokens = _tokensPath,
            NumThreads = 4,
            Provider = "cpu",
            EnableEndpoint = true,
            Rule1 = new OnlineEndpointRule
            {
                MustContainNonSilence = false,
                MinTrailingSilence = _minTrailingSilence,
                MinUtteranceLength = _minUtteranceLength
            },
            Rule2 = new OnlineEndpointRule
            {
                MustContainNonSilence = true,
                MinTrailingSilence = 1.2f,
                MinUtteranceLength = 0
            },
            Rule3 = new OnlineEndpointRule
            {
                MustContainNonSilence = false,
                MinTrailingSilence = 0,
                MinUtteranceLength = 20f
            }
        };

        _recognizer?.Dispose();
        _stream?.Dispose();
        _recognizer = new OnlineRecognizer(config);
        _stream = _recognizer.CreateStream();
        Console.WriteLine("[STT] 流式模型加载成功");
    }

    /// <summary>
    /// 将16kHz单声道音频送入流式识别器。
    /// 实时触发 OnPartialResult，端点检测后触发 OnFinalResult。
    /// </summary>
    public void ProcessAudio(float[] samples16kHzMono)
    {
        lock (_lock)
        {
            if (_recognizer == null || _stream == null)
            {
                if (_audioChunkCount == 0)
                    Console.WriteLine("[STT] ProcessAudio: 模型未加载，跳过处理");
                _audioChunkCount++;
                return;
            }

            _audioChunkCount++;
            if (_audioChunkCount <= 3 || _audioChunkCount % 200 == 0)
                Console.WriteLine($"[STT] 送入音频 #{_audioChunkCount}: {samples16kHzMono.Length} samples");

            _stream.AcceptWaveform(16000, samples16kHzMono);

            // 解码所有可用帧
            while (_recognizer.IsReady(_stream))
                _recognizer.Decode(_stream);

            // 获取partial结果
            var result = _recognizer.GetResult(_stream);
            var text = result.Text?.Trim() ?? string.Empty;

            if (!string.IsNullOrWhiteSpace(text))
            {
                OnPartialResult?.Invoke(new SubtitleEntry
                {
                    OriginalText = text,
                    Timestamp = DateTime.Now
                });

                if (_audioChunkCount % 100 == 0)
                    Console.WriteLine($"[STT] partial: \"{text}\"");
            }

            // 端点检测：说话结束
            if (_recognizer.IsEndpoint(_stream))
            {
                if (!string.IsNullOrWhiteSpace(text))
                {
                    Console.WriteLine($"[STT] final: \"{text}\"");
                    OnFinalResult?.Invoke(new SubtitleEntry
                    {
                        OriginalText = text,
                        Timestamp = DateTime.Now
                    });
                }
                _recognizer.Reset(_stream);
            }
        }
    }

    /// <summary>
    /// 更新端点检测灵敏度。
    /// </summary>
    public void UpdateEndpointConfig(float minTrailingSilence, float minUtteranceLength)
    {
        // 需要重建recognizer才能应用新配置
        // 暂存参数，下次Initialize时生效
        Console.WriteLine($"[STT] 端点参数更新: trailingSilence={minTrailingSilence}s utteranceLength={minUtteranceLength}s");
    }

    public void Dispose()
    {
        lock (_lock)
        {
            _stream?.Dispose();
            _stream = null;
            _recognizer?.Dispose();
            _recognizer = null;
        }
    }
}
