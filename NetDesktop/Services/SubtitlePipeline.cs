using NetDesktop.Models;
using NetDesktop.Services.Audio;
using NetDesktop.Services.Stt;
using NetDesktop.Services.Translation;
using NetDesktop.Services.Subtitle;

namespace NetDesktop.Services;

public class SubtitlePipeline : IDisposable
{
    private AudioCaptureService? _capture;
    private AudioResampler? _resampler;
    private SttService? _stt;
    private ContextualTranslator? _translator;
    private readonly SettingsModel _settings;
    private readonly object _lock = new();
    private bool _running;

    public SubtitleManager Manager { get; } = new SubtitleManager();

    public RollingBuffer OriginalBuffer { get; } = new();
    public RollingBuffer TranslationBuffer { get; } = new();

    public SttService? Stt => _stt;

    public event Action<string>? Error;

    public SubtitlePipeline(SettingsModel settings)
    {
        _settings = settings;
    }

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
                TranslationEngine.Google2 => new Google2TranslationService(),
                _ => new GoogleTranslationService(_settings.GoogleApiKey)
            };

            _translator = new ContextualTranslator(engine, _settings.TargetLanguage);
            _translator.OnTranslationReady += (original, translated) =>
                TranslationBuffer.Replace(translated);

            _stt = new SttService(_settings);

            _stt.OnPartialResult += entry =>
                OriginalBuffer.UpdatePartial(entry.OriginalText);

            _stt.OnFinalResult += entry =>
            {
                OriginalBuffer.CommitPartial();
                Manager.OnFinalRecognition(entry);
            };

            OriginalBuffer.ContentChanged += fullText =>
                _ = _translator.TranslateRolling(fullText);

            _stt.Error += msg => Error?.Invoke(msg);
            _stt.Initialize(_settings);

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

    public void SwitchTranslationEngine(TranslationEngine engine, string? apiKey = null, string? serverUrl = null, string? googleApiKey = null)
    {
        ITranslationService newEngine = engine switch
        {
            TranslationEngine.DeepL => new DeepLTranslationService(
                apiKey ?? _settings.DeepLApiKey,
                serverUrl ?? _settings.DeepLServerUrl),
            TranslationEngine.Google2 => new Google2TranslationService(),
            _ => new GoogleTranslationService(googleApiKey ?? _settings.GoogleApiKey)
        };
        _translator?.SwitchEngine(newEngine);
        _settings.Engine = engine;
        if (apiKey != null) _settings.DeepLApiKey = apiKey;
        if (serverUrl != null) _settings.DeepLServerUrl = serverUrl;
        if (googleApiKey != null) _settings.GoogleApiKey = googleApiKey;
    }

    public void Stop()
    {
        lock (_lock)
        {
            if (!_running) return;
            _running = false;
        }
        _capture?.Stop();
        OriginalBuffer.Clear();
        TranslationBuffer.Clear();
        Manager.Clear();
    }

    public void Dispose()
    {
        Stop();
        _capture?.Dispose();
        _stt?.Dispose();
    }
}
