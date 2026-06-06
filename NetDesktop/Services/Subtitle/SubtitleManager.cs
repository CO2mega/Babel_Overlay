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

    /// <summary>
    /// 翻译完成后回写译文。优先匹配当前字幕，其次在历史中查找。
    /// </summary>
    /// <param name="originalText">原文（用于匹配目标条目）。</param>
    /// <param name="translatedText">翻译结果。</param>
    public void UpdateTranslation(string originalText, string translatedText)
    {
        if (Current?.OriginalText == originalText)
        {
            Current.TranslatedText = translatedText;
            CurrentChanged?.Invoke(Current);
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
