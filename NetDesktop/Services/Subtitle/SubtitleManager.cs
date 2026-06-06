using System.Collections.ObjectModel;
using NetDesktop.Models;

namespace NetDesktop.Services.Subtitle;

/// <summary>
/// 字幕管理器。维护当前字幕和历史记录，处理 STT 修正并回写
/// </summary>
public class SubtitleManager
{
    /// <summary>
    /// 字幕列表（从新到旧
    /// </summary>
    public ObservableCollection<SubtitleEntry> History { get; } = new();

    /// <summary>
    /// 当前正在显示的字幕条目。新语音段到来后替换，STT 修正时更新。
    /// </summary>
    public SubtitleEntry? Current { get; private set; }

    /// <summary>
    /// 当前字幕发生变化时触发（新识别或翻译更新），参数为更新后的 SubtitleEntry。
    /// </summary>
    public event Action<SubtitleEntry>? CurrentChanged;

    /// <summary>
    /// 有新条目加入历史列表时触发。
    /// </summary>
    public event Action<SubtitleEntry>? HistoryChanged;

    private const int MaxHistory = 100;
    private const double SimilarityThreshold = 0.6;
    private DateTime _lastUpdateTime = DateTime.MinValue;

    /// <summary>
    /// 实时partial识别结果。更新当前字幕文本，不推入历史。
    /// </summary>
    public void OnPartialRecognition(SubtitleEntry entry)
    {
        if (Current == null || Current.IsConfirmed)
        {
            Current = entry;
        }
        else
        {
            Current.OriginalText = entry.OriginalText;
            Current.Timestamp = entry.Timestamp;
        }
        _lastUpdateTime = DateTime.Now;
        CurrentChanged?.Invoke(Current);
    }

    /// <summary>
    /// 最终识别结果。将当前条目更新为 final 文本后推入历史，设置新条目为当前。
    /// </summary>
    public void OnFinalRecognition(SubtitleEntry entry)
    {
        if (Current != null && !Current.IsConfirmed)
        {
            // 用 final 的完整文本更新当前条目后再推入历史
            Current.OriginalText = entry.OriginalText;
            Current.Timestamp = entry.Timestamp;
            Current.IsConfirmed = true;
            AddToHistory(Current);
        }
        Current = entry;
        _lastUpdateTime = DateTime.Now;
        CurrentChanged?.Invoke(Current);
    }

    /// <summary>
    /// 兼容旧接口：新语音识别结果到达时调用。
    /// </summary>
    public void OnNewRecognition(SubtitleEntry entry)
    {
        OnFinalRecognition(entry);
    }

    // 最近一次请求翻译的条目（用于对象引用匹配）
    private SubtitleEntry? _lastTranslationTarget;

    /// <summary>
    /// 标记当前条目为翻译目标（在请求翻译前调用）。
    /// </summary>
    public void MarkTranslationTarget(SubtitleEntry entry)
    {
        _lastTranslationTarget = entry;
    }

    /// <summary>
    /// 翻译完成后回写译文。优先对象引用匹配，其次文本匹配。
    /// </summary>
    public void UpdateTranslation(string originalText, string translatedText)
    {
        // 优先对象引用匹配
        if (_lastTranslationTarget != null)
        {
            _lastTranslationTarget.TranslatedText = translatedText;
            if (Current == _lastTranslationTarget)
                CurrentChanged?.Invoke(Current);
            _lastTranslationTarget = null;
            return;
        }

        // 回退：文本匹配
        if (Current?.OriginalText == originalText)
        {
            Current.TranslatedText = translatedText;
            CurrentChanged?.Invoke(Current);
            return;
        }

        foreach (var entry in History)
        {
            if (entry.OriginalText == originalText)
            {
                entry.TranslatedText = translatedText;
                break;
            }
        }
    }

    private void AddToHistory(SubtitleEntry entry)
    {
        History.Insert(0, entry);
        while (History.Count > MaxHistory)
            History.RemoveAt(History.Count - 1);
        HistoryChanged?.Invoke(entry);
    }

    /// <summary>
    /// 检查新文本是否与最后翻译过的文本过于相似（Live Captions 修正），若是则复用上次译文。
    /// 返回 true 表示应跳过翻译请求。
    /// </summary>
    public bool TryReuseLastTranslation(string newText, out string reusedTranslation)
    {
        reusedTranslation = string.Empty;

        // 找最近一条有译文的历史条目
        SubtitleEntry? lastTranslated = null;
        foreach (var entry in History)
        {
            if (!string.IsNullOrEmpty(entry.TranslatedText) && !entry.TranslatedText.StartsWith("[翻译"))
            {
                lastTranslated = entry;
                break;
            }
        }

        if (lastTranslated == null) return false;

        if (Similarity(lastTranslated.OriginalText, newText) > SimilarityThreshold)
        {
            reusedTranslation = lastTranslated.TranslatedText;
            return true;
        }
        return false;
    }

    /// <summary>
    /// Levenshtein 距离相似度（与参考项目 TextUtil.Similarity 一致）。
    /// </summary>
    private static double Similarity(string text1, string text2)
    {
        if (text1.StartsWith(text2) || text2.StartsWith(text1))
            return 1.0;
        int distance = LevenshteinDistance(text1, text2);
        int maxLen = Math.Max(text1.Length, text2.Length);
        return maxLen == 0 ? 1.0 : 1.0 - (double)distance / maxLen;
    }

    private static int LevenshteinDistance(string text1, string text2)
    {
        if (string.IsNullOrEmpty(text1)) return string.IsNullOrEmpty(text2) ? 0 : text2.Length;
        if (string.IsNullOrEmpty(text2)) return text1.Length;

        if (text1.Length > text2.Length)
            (text2, text1) = (text1, text2);

        int len1 = text1.Length;
        int len2 = text2.Length;
        int[] prev = new int[len1 + 1];
        int[] curr = new int[len1 + 1];

        for (int i = 0; i <= len1; i++) prev[i] = i;
        for (int j = 1; j <= len2; j++)
        {
            curr[0] = j;
            for (int i = 1; i <= len1; i++)
            {
                int cost = text1[i - 1] == text2[j - 1] ? 0 : 1;
                curr[i] = Math.Min(Math.Min(curr[i - 1] + 1, prev[i] + 1), prev[i - 1] + cost);
            }
            (curr, prev) = (prev, curr);
        }
        return prev[len1];
    }

    /// <summary>
    /// 停止时调用，将当前未确认的字幕推入历史并清空。
    /// </summary>
    public void Clear()
    {
        if (Current != null)
        {
            Current.IsConfirmed = true;
            AddToHistory(Current);
        }
        Current = null;
        CurrentChanged?.Invoke(new SubtitleEntry());
    }
}
