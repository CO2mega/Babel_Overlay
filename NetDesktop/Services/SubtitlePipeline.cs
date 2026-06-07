using NetDesktop.Models;
using NetDesktop.Services.Stt;
using NetDesktop.Services.Translation;
using NetDesktop.Services.Subtitle;

namespace NetDesktop.Services;

public class SubtitlePipeline : IDisposable
{
    private LiveCaptionsSttService? _stt;
    private ContextualTranslator? _translator;
    private readonly SettingsModel _settings;
    private readonly object _lock = new();
    private bool _running;

    public SubtitleManager Manager { get; } = new SubtitleManager();

    public RollingBuffer OriginalBuffer { get; } = new();
    public RollingBuffer TranslationBuffer { get; } = new();

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
                TranslationEngine.OpenAI => new OpenAITranslationService(_settings.OpenAIApiKey, _settings.OpenAIApiUrl, _settings.OpenAIModel),
                _ => new GoogleTranslationService(_settings.GoogleApiKey)
            };

            _translator = new ContextualTranslator(engine, _settings.TargetLanguage);
            _translator.OnTranslationReady += (original, translated) =>
            {
                TranslationBuffer.Replace(translated);
                Manager.UpdateTranslation(original, translated);
            };

            _stt = new LiveCaptionsSttService(_settings);

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
            _stt.Initialize();
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
            TranslationEngine.OpenAI => new OpenAITranslationService(
                apiKey ?? _settings.OpenAIApiKey,
                serverUrl ?? _settings.OpenAIApiUrl,
                _settings.OpenAIModel),
            _ => new GoogleTranslationService(googleApiKey ?? _settings.GoogleApiKey)
        };
        _translator?.SwitchEngine(newEngine);
        _settings.Engine = engine;
        if (apiKey != null)
        {
            if (engine == TranslationEngine.DeepL) _settings.DeepLApiKey = apiKey;
            else if (engine == TranslationEngine.OpenAI) _settings.OpenAIApiKey = apiKey;
        }
        if (serverUrl != null)
        {
            if (engine == TranslationEngine.DeepL) _settings.DeepLServerUrl = serverUrl;
            else if (engine == TranslationEngine.OpenAI) _settings.OpenAIApiUrl = serverUrl;
        }
        if (googleApiKey != null) _settings.GoogleApiKey = googleApiKey;
    }

    public void ShowLiveCaptionsSettings()
    {
        _stt?.ShowSettings();
    }

    public void Stop()
    {
        lock (_lock)
        {
            if (!_running) return;
            _running = false;
        }
        _stt?.Dispose();
        OriginalBuffer.Clear();
        TranslationBuffer.Clear();
        Manager.Clear();
    }

    public void Dispose()
    {
        Stop();
        _stt?.Dispose();
    }
}
