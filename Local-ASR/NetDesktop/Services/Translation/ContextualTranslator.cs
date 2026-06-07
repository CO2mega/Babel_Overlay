using System.Threading;

namespace NetDesktop.Services.Translation;

public class ContextualTranslator
{
    private ITranslationService _engine;
    private readonly string _targetLang;
    private long _requestSeq;
    private long _displayedSeq;
    private readonly object _deliveryLock = new();

    public event Action<string, string>? OnTranslationReady;

    public ContextualTranslator(ITranslationService engine, string targetLang)
    {
        _engine = engine;
        _targetLang = targetLang;
    }

    public void SwitchEngine(ITranslationService newEngine)
    {
        _engine = newEngine;
    }

    public async Task TranslateRolling(string fullText)
    {
        if (string.IsNullOrWhiteSpace(fullText)) return;

        var seq = Interlocked.Increment(ref _requestSeq);

        try
        {
            var translation = await _engine.TranslateAsync(fullText, _targetLang);
            TryDeliver(seq, fullText, translation);
        }
        catch (Exception ex)
        {
            TryDeliver(seq, fullText, $"[翻译错误: {ex.Message}]");
        }
    }

    private void TryDeliver(long seq, string original, string translation)
    {
        lock (_deliveryLock)
        {
            if (seq > _displayedSeq)
            {
                _displayedSeq = seq;
                OnTranslationReady?.Invoke(original, translation);
            }
        }
    }
}
