using NetDesktop.Models;

namespace NetDesktop.Services.Stt;

/// <summary>
/// 自适应 VAD 灵敏度控制器。在 Auto 模式下根据最近 N 句的平均词数
/// 动态调整 MinSilenceDuration：长难句多则缩短静音阈值以细分片段，
/// 碎片化则延长以合并片段。每 5 句评估一次，避免频繁抖动。
/// </summary>
public class AdaptiveVadController
{
    private readonly SttService _sttService;
    private readonly SettingsModel _settings;
    private readonly Queue<int> _recentWordCounts = new();
    private const int WindowSize = 5;
    private const float MinSilenceLower = 0.2f;
    private const float MinSilenceUpper = 1.2f;
    private const float StepSize = 0.1f;
    private const int HighWordCountThreshold = 20;
    private const int LowWordCountThreshold = 5;

    /// <summary>
    /// 创建自适应 VAD 控制器。
    /// </summary>
    /// <param name="sttService">绑定的 STT 服务，用于重建 VAD 实例。</param>
    /// <param name="settings">应用配置，MinSilenceDuration 将被动态修改。</param>
    public AdaptiveVadController(SttService sttService, SettingsModel settings)
    {
        _sttService = sttService;
        _settings = settings;
    }

    /// <summary>
    /// 每次识别到新句子时调用，更新滑动窗口统计并决定是否调整 VAD 参数。
    /// 仅在 Auto（自适应）模式下生效。
    /// </summary>
    /// <param name="entry">刚识别完成的字幕条目。</param>
    public void OnNewSentence(SubtitleEntry entry)
    {
        if (_settings.VadMode != VadPreset.Auto) return;

        _recentWordCounts.Enqueue(entry.WordCount);
        while (_recentWordCounts.Count > WindowSize)
            _recentWordCounts.Dequeue();

        if (_recentWordCounts.Count < WindowSize) return;

        var avgWords = _recentWordCounts.Average();
        var currentSilence = _settings.MinSilenceDuration;

        if (avgWords > HighWordCountThreshold)
        {
            _settings.MinSilenceDuration = Math.Max(MinSilenceLower, currentSilence - StepSize);
            _sttService.RebuildVad(_settings);
        }
        else if (avgWords < LowWordCountThreshold)
        {
            _settings.MinSilenceDuration = Math.Min(MinSilenceUpper, currentSilence + StepSize);
            _sttService.RebuildVad(_settings);
        }
    }

    /// <summary>
    /// 清空滑动窗口历史，重新开始统计。用于 VAD 模式切换时重置。
    /// </summary>
    public void Reset()
    {
        _recentWordCounts.Clear();
    }
}
