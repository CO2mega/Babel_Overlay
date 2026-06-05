using NetDesktop.Models;

namespace NetDesktop.Services.Translation;

/// <summary>
/// 带滑动窗口上下文的翻译器。维护最近 N 句作为上下文发送给翻译引擎，
/// 提升跨句连贯性，若 STT 快速修正则合并。
/// </summary>
public class ContextualTranslator
{
    private readonly List<string> _window = new();
    private readonly int _maxWindowSize;
    private ITranslationService _engine;
    private readonly string _targetLang;
    private readonly SemaphoreSlim _translateLock = new(1, 1);
    private CancellationTokenSource? _debounceCts;

    /// <summary>
    /// 翻译完成后触发，参数为 (原文, 译文)。
    /// </summary>
    public event Action<string, string>? OnTranslationReady;

    /// <summary>
    /// 创建上下文翻译器。
    /// </summary>
    /// <param name="engine">底层翻译引擎（Google / DeepL）。</param>
    /// <param name="targetLang">目标语言代码。</param>
    /// <param name="windowSize">滑动窗口大小，越大上下文越丰富但延迟越高。默认 6 句。</param>
    public ContextualTranslator(ITranslationService engine, string targetLang, int windowSize = 6)
    {
        _engine = engine;
        _targetLang = targetLang;
        _maxWindowSize = windowSize;
    }

    /// <summary>
    /// 运行时切换翻译引擎
    /// </summary>
    /// <param name="newEngine">新的翻译引擎实例。</param>
    public void SwitchEngine(ITranslationService newEngine)
    {
        _engine = newEngine;
    }

    /// <summary>
    /// 将新句子加入滑动窗口并异步翻译。200ms 防抖避免重复请求。
    /// </summary>
    /// <param name="sentence">STT 识别出的新句子。</param>
    /// <param name="isCorrection">若为 true 则替换窗口末尾句子而非追加（用于 STT 修正）。</param>
    public async Task TranslateNewSentence(string sentence, bool isCorrection = false)
    {
        if (string.IsNullOrWhiteSpace(sentence)) return;
        
        _debounceCts?.Cancel();
        _debounceCts = new CancellationTokenSource();
        var ct = _debounceCts.Token;

        try
        {
            await Task.Delay(200, ct);
        }
        catch (OperationCanceledException)
        {
            return;
        }

        if (isCorrection && _window.Count > 0)
            _window[^1] = sentence;
        else
            _window.Add(sentence);

        while (_window.Count > _maxWindowSize)
            _window.RemoveAt(0);

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
    /// 从多句合并翻译结果中提取最后一句的译文。
    /// </summary>
    /// <param name="fullTranslation">完整翻译。</param>
    /// <param name="sentenceCount">窗口内句子数。</param>
    /// <returns>最后一句的译文。</returns>
    private static string ExtractLastSentence(string fullTranslation, int sentenceCount)
    {
        if (sentenceCount <= 1) return fullTranslation;
        var parts = fullTranslation.Split('\n', StringSplitOptions.RemoveEmptyEntries);
        return parts.Length > 0 ? parts[^1].Trim() : fullTranslation;
    }
}
