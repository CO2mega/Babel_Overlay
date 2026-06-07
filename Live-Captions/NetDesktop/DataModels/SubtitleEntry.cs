namespace NetDesktop.Models;

public class SubtitleEntry
{
    public string OriginalText { get; set; } = string.Empty;
    public string TranslatedText { get; set; } = string.Empty;
    public DateTime Timestamp { get; set; } = DateTime.Now;
    public TimeSpan Duration { get; set; }
    public int WordCount => OriginalText.Split(' ', StringSplitOptions.RemoveEmptyEntries).Length;
    public bool IsConfirmed { get; set; }
}
