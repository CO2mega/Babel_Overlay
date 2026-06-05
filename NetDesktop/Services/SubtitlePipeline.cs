using NetDesktop.Models;
using NetDesktop.Services.Audio;
using NetDesktop.Services.Stt;
using NetDesktop.Services.Translation;
using NetDesktop.Services.Subtitle;

namespace NetDesktop.Services;

/// <summary>
/// 全流程管线：WASAPI音频捕获，重采样，VAD+STT，滑动窗口翻译，字幕管理器。
/// 调用方在构造后、Start() 前订阅 Manager 的事件。
/// </summary>
public class SubtitlePipeline : IDisposable
{
    private AudioCaptureService? _capture;
    private AudioResampler? _resampler;
    private SttService? _stt;
    private AdaptiveVadController? _adaptiveVad;
    private ContextualTranslator? _translator;
    private readonly SettingsModel _settings;
    private readonly object _lock = new();
    private bool _running;

    /// <summary>
    /// 字幕管理器，构造时即创建，供调用方在 Start() 前订阅事件。
    /// </summary>
    public SubtitleManager Manager { get; } = new SubtitleManager();

    /// <summary>
    /// STT 服务实例（Start() 后可访问，用于参数调整）。
    /// </summary>
    public SttService? Stt => _stt;

    /// <summary>
    /// VAD 是否正在检测到语音活动，供 UI 轮询显示状态。
    /// </summary>
    public bool IsSpeechActive => _stt?.IsSpeechActive ?? false;

    /// <summary>
    /// 管道运行过程中的错误（模型加载、音频捕获、STT 解码等）。
    /// </summary>
    public event Action<string>? Error;

    /// <summary>
    /// 创建管道实例。调用 <see cref="Start"/> 启动全部服务。
    /// </summary>
    /// <param name="settings">应用配置（VAD 参数、翻译引擎、模型路径等）。</param>
    public SubtitlePipeline(SettingsModel settings)
    {
        _settings = settings;
    }

    /// <summary>
    /// 启动完整管道：初始化翻译引擎 → 加载 STT 模型 → 启动 WASAPI 捕获。
    /// 幂等：多次调用无效。
    /// </summary>
    public void Start()
    {
        lock (_lock)
        {
            if (_running) return;
            _running = true;
        }

        try
        {
            ITranslationService engine = _settings.Engine switch
            {
                TranslationEngine.DeepL => new DeepLTranslationService(_settings.DeepLApiKey, _settings.DeepLServerUrl),
                _ => new GoogleTranslationService(_settings.GoogleApiKey)
            };

            _translator = new ContextualTranslator(engine, _settings.TargetLanguage, _settings.ContextWindowSize);
            _translator.OnTranslationReady += (original, translated) =>
                Manager.UpdateTranslation(original, translated);

            _stt = new SttService(_settings);
            _stt.OnSpeechRecognized += entry =>
            {
                Manager.OnNewRecognition(entry);
                _adaptiveVad?.OnNewSentence(entry);
                _ = _translator.TranslateNewSentence(entry.OriginalText);
            };
            _stt.Error += msg => Error?.Invoke(msg);
            _stt.Initialize(_settings);

            _adaptiveVad = new AdaptiveVadController(_stt, _settings);
            _resampler = new AudioResampler();

            _capture = new AudioCaptureService();
            _capture.AudioDataAvailable += (samples, channels) =>
            {
                var fmt = _capture.CaptureFormat;
                var resampled = _resampler.Resample(samples, channels, fmt.SampleRate);
                _stt?.ProcessAudio(resampled);
            };
            _capture.Error += msg => Error?.Invoke(msg);
            _capture.Start();
        }
        catch (Exception ex)
        {
            Error?.Invoke($"管道启动失败: {ex.Message}");
            _running = false;
        }
    }

    /// <summary>
    /// 运行时切换翻译引擎。
    /// </summary>
    /// <param name="engine">目标引擎。</param>
    /// <param name="apiKey">DeepL API Key（仅 DeepL 需要）。</param>
    /// <param name="serverUrl">DeepL API 端点（仅 DeepL 需要）。</param>
    public void SwitchTranslationEngine(TranslationEngine engine, string? apiKey = null, string? serverUrl = null, string? googleApiKey = null)
    {
        ITranslationService newEngine = engine switch
        {
            TranslationEngine.DeepL => new DeepLTranslationService(
                apiKey ?? _settings.DeepLApiKey,
                serverUrl ?? _settings.DeepLServerUrl),
            _ => new GoogleTranslationService(googleApiKey ?? _settings.GoogleApiKey)
        };
        _translator?.SwitchEngine(newEngine);
        _settings.Engine = engine;
        if (apiKey != null) _settings.DeepLApiKey = apiKey;
        if (serverUrl != null) _settings.DeepLServerUrl = serverUrl;
        if (googleApiKey != null) _settings.GoogleApiKey = googleApiKey;
    }

    /// <summary>
    /// 切换 VAD 预设模式，自动设置对应的 MinSilenceDuration 并立即重建 VAD。
    /// </summary>
    /// <param name="preset">预设：Realtime(0.3s) / Accurate(0.8s) / Auto / Custom。</param>
    public void UpdateVadMode(VadPreset preset)
    {
        _settings.VadMode = preset;
        if (preset == VadPreset.Realtime) _settings.MinSilenceDuration = 0.3f;
        else if (preset == VadPreset.Accurate) _settings.MinSilenceDuration = 0.8f;
        _stt?.RebuildVad(_settings);
    }

    /// <summary>
    /// 停止音频捕获，将当前字幕推入历史。
    /// </summary>
    public void Stop()
    {
        lock (_lock)
        {
            if (!_running) return;
            _running = false;
        }
        _capture?.Stop();
        Manager.Clear();
    }

    /// <summary>
    /// 停止管道并释放所有资源（WASAPI、STT 模型）。
    /// </summary>
    public void Dispose()
    {
        Stop();
        _capture?.Dispose();
        _stt?.Dispose();
    }
}
