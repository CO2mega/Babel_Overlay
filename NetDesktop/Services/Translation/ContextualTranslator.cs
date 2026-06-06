using NetDesktop.Models;

namespace NetDesktop.Services.Translation;

/// <summary>
/// 翻译器。支持两种翻译模式：
/// - TranslateNewSentence: 用于 final 结果，新到的 final 会取消前一个
/// - TranslateThrottled: 500ms 节流，用于 partial 结果（低延迟）
/// </summary>
public class ContextualTranslator
{
    private readonly List<string> _window = new();
    private readonly int _maxWindowSize;
    private ITranslationService _engine;
    private readonly string _targetLang;
    private readonly SemaphoreSlim _translateLock = new(1, 1);
    private CancellationTokenSource? _throttleCts;
    private CancellationTokenSource? _finalCts;
    private readonly int _throttleDelayMs;
    private readonly int _maxRetry;
    private readonly int _retryDelayMs;

    /// <summary>
    /// 翻译完成后触发，参数为 (原文, 译文)。
    /// </summary>
    public event Action<string, string>? OnTranslationReady;

    public ContextualTranslator(ITranslationService engine, string targetLang, int windowSize = 6,
        int throttleDelayMs = 500, int maxRetry = 2, int retryDelayMs = 500)
    {
        _engine = engine;
        _targetLang = targetLang;
        _maxWindowSize = windowSize;
        _throttleDelayMs = throttleDelayMs;
        _maxRetry = maxRetry;
        _retryDelayMs = retryDelayMs;
    }

    /// <summary>
    /// 运行时切换翻译引擎。
    /// </summary>
    public void SwitchEngine(ITranslationService newEngine)
    {
        _engine = newEngine;
    }

    /// <summary>
    /// 带上下文窗口的翻译。用于 final 结果，翻译质量高。
    /// 新的 final 会取消前一个正在执行的翻译。
    /// </summary>
    public async Task TranslateNewSentence(string sentence, bool isCorrection = false)
    {
        if (string.IsNullOrWhiteSpace(sentence)) return;

        // 取消前一个 final 翻译
        _finalCts?.Cancel();
        _finalCts = new CancellationTokenSource();
        var ct = _finalCts.Token;

        if (isCorrection && _window.Count > 0)
            _window[^1] = sentence;
        else
            _window.Add(sentence);

        while (_window.Count > _maxWindowSize)
            _window.RemoveAt(0);

        await _translateLock.WaitAsync(ct);
        try
        {
            string translation = sentence;
            for (int attempt = 0; attempt <= _maxRetry; attempt++)
            {
                translation = await _engine.TranslateAsync(sentence, _targetLang, ct);
                if (!translation.StartsWith("[翻译"))
                {
                    OnTranslationReady?.Invoke(sentence, translation);
                    return;
                }
                if (attempt < _maxRetry)
                    await Task.Delay(_retryDelayMs, ct);
            }
            OnTranslationReady?.Invoke(sentence, translation);
        }
        catch (OperationCanceledException) { }
        catch (Exception ex)
        {
            OnTranslationReady?.Invoke(sentence, $"[翻译错误: {ex.Message}]");
        }
        finally
        {
            _translateLock.Release();
        }
    }

    /// <summary>
    /// 节流翻译。用于 partial 结果，500ms 内有新请求则取消前一个，无上下文窗口。
    /// </summary>
    public async Task TranslateThrottled(string sentence)
    {
        if (string.IsNullOrWhiteSpace(sentence)) return;

        _throttleCts?.Cancel();
        _throttleCts = new CancellationTokenSource();
        var ct = _throttleCts.Token;

        try
        {
            await Task.Delay(_throttleDelayMs, ct);
        }
        catch (OperationCanceledException)
        {
            return;
        }

        await _translateLock.WaitAsync(ct);
        try
        {
            string translation = sentence;
            for (int attempt = 0; attempt <= _maxRetry; attempt++)
            {
                translation = await _engine.TranslateAsync(sentence, _targetLang, ct);
                if (!translation.StartsWith("[翻译"))
                {
                    OnTranslationReady?.Invoke(sentence, translation);
                    return;
                }
                if (attempt < _maxRetry)
                    await Task.Delay(_retryDelayMs, ct);
            }
            OnTranslationReady?.Invoke(sentence, translation);
        }
        catch (OperationCanceledException) { }
        catch (Exception ex)
        {
            OnTranslationReady?.Invoke(sentence, $"[翻译错误: {ex.Message}]");
        }
        finally
        {
            _translateLock.Release();
        }
    }

}
