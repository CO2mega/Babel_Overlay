using NetDesktop.Models;
using NetDesktop.Services.Stt;
using NetDesktop.Services.Translation;
using NetDesktop.Services.Subtitle;

namespace NetDesktop.Services;

/// <summary>
/// 全流程管线：Windows Live Captions 语音识别，定时轮询翻译，字幕管理器。
/// </summary>
public class SubtitlePipeline : IDisposable
{
    private LiveCaptionsSttService? _stt;
    private ContextualTranslator? _translator;
    private Thread? _translationPollThread;
    private readonly SettingsModel _settings;
    private readonly object _lock = new();
    private bool _running;

    /// <summary>
    /// 字幕管理器，构造时即创建，供调用方在 Start() 前订阅事件。
    /// </summary>
    public SubtitleManager Manager { get; } = new SubtitleManager();

    /// <summary>
    /// 管道运行过程中的错误。
    /// </summary>
    public event Action<string>? Error;

    public SubtitlePipeline(SettingsModel settings)
    {
        _settings = settings;
    }

    /// <summary>
    /// 启动完整管道：翻译引擎 → Live Captions STT → 翻译轮询线程。
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
            // 翻译引擎
            ITranslationService engine = _settings.Engine switch
            {
                TranslationEngine.DeepL => new DeepLTranslationService(_settings.DeepLApiKey, _settings.DeepLServerUrl),
                TranslationEngine.Google2 => new Google2TranslationService(),
                _ => new GoogleTranslationService(_settings.GoogleApiKey)
            };

            _translator = new ContextualTranslator(engine, _settings.TargetLanguage, _settings.ContextWindowSize);
            _translator.OnTranslationReady += (original, translated) =>
                Manager.UpdateTranslation(original, translated);

            // Live Captions STT
            _stt = new LiveCaptionsSttService(_settings);

            // partial → 仅更新原文显示
            _stt.OnPartialResult += entry =>
            {
                Manager.OnPartialRecognition(entry);
            };

            // final → 确认字幕 + 带上下文翻译
            _stt.OnFinalResult += entry =>
            {
                Manager.OnFinalRecognition(entry);
                _ = _translator.TranslateNewSentence(entry.OriginalText);
            };

            _stt.Error += msg => Error?.Invoke(msg);
            _stt.Initialize();

            // 翻译轮询线程：定时检查当前字幕并翻译
            _translationPollThread = new Thread(TranslationPollLoop)
            {
                IsBackground = true,
                Name = "Translation-Poll"
            };
            _translationPollThread.Start();
        }
        catch (Exception ex)
        {
            Error?.Invoke($"管道启动失败: {ex.Message}");
            _running = false;
        }
    }

    /// <summary>
    /// 定时轮询：每 N ms 检查当前字幕是否有未翻译的内容，有则请求翻译。
    /// </summary>
    private void TranslationPollLoop()
    {
        string lastTranslatedText = string.Empty;
        var interval = _settings.TranslationPollingIntervalMs;

        while (_running)
        {
            Thread.Sleep(interval);

            try
            {
                var current = Manager.Current;
                if (current == null || string.IsNullOrWhiteSpace(current.OriginalText))
                    continue;

                // 已翻译过相同的文本则跳过
                if (string.CompareOrdinal(lastTranslatedText, current.OriginalText) == 0)
                    continue;

                // 如果已有翻译（由 final 事件驱动的翻译完成），跳过轮询
                if (!string.IsNullOrEmpty(current.TranslatedText))
                {
                    lastTranslatedText = current.OriginalText;
                    continue;
                }

                lastTranslatedText = current.OriginalText;
                _ = _translator?.TranslateThrottled(current.OriginalText);
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[TranslationPoll] 异常: {ex.Message}");
            }
        }
    }

    /// <summary>
    /// 运行时切换翻译引擎。
    /// </summary>
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

    /// <summary>
    /// 恢复 Live Captions 窗口并打开其设置面板。
    /// </summary>
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
        Manager.Clear();
    }

    public void Dispose()
    {
        Stop();
        _stt?.Dispose();
    }
}
