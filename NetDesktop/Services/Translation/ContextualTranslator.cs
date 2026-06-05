using NetDesktop.Models;

namespace NetDesktop.Services.Translation;

/// <summary>
/// 带滑动窗口上下文的翻译器。维护最近 N 句作为上下文发送给翻译引擎，
/// 提升跨句连贯性。支持两种翻译模式：
/// - TranslateNewSentence: 带上下文窗口，用于 final 结果（高质量）
/// - TranslateThrottled: 无上下文，500ms 节流，用于 partial 结果（低延迟）
/// </summary>
public class ContextualTranslator
{
    private readonly List<string> _window = new();
    private readonly int _maxWindowSize;
    private ITranslationService _engine;
    private readonly string _targetLang;
    private readonly SemaphoreSlim _translateLock = new(1, 1);
    private CancellationTokenSource? _throttleCts;

    /// <summary>
    /// 翻译完成后触发，参数为 (原文, 译文)。
    /// </summary>
    public event Action<string, string>? OnTranslationReady;

    public ContextualTranslator(ITranslationService engine, string targetLang, int windowSize = 6)
    {
        _engine = engine;
        _targetLang = targetLang;
        _maxWindowSize = windowSize;
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
    /// </summary>
    public async Task TranslateNewSentence(string sentence, bool isCorrection = false)
    {
        if (string.IsNullOrWhiteSpace(sentence)) return;

        if (isCorrection && _window.Count > 0)
            _window[^1] = sentence;
        else
            _window.Add(sentence);

        while (_window.Count > _maxWindowSize)
            _window.RemoveAt(0);

        var ct = _throttleCts?.Token ?? CancellationToken.None;
        await _translateLock.WaitAsync(ct);
        try
        {
            var contextText = string.Join(" \n", _window);
            var translation = await _engine.TranslateAsync(contextText, _targetLang, ct);
            var translatedSentence = ExtractLastSentence(translation, _window.Count);
            OnTranslationReady?.Invoke(sentence, translatedSentence);
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
            await Task.Delay(500, ct);
        }
        catch (OperationCanceledException)
        {
            return;
        }

        await _translateLock.WaitAsync(ct);
        try
        {
            var translation = await _engine.TranslateAsync(sentence, _targetLang, ct);
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

    private static string ExtractLastSentence(string fullTranslation, int sentenceCount)
    {
        if (sentenceCount <= 1) return fullTranslation;
        var parts = fullTranslation.Split('\n', StringSplitOptions.RemoveEmptyEntries);
        return parts.Length > 0 ? parts[^1].Trim() : fullTranslation;
    }
}
