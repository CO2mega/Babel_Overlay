namespace NetDesktop.Models;

public enum TranslationEngine
{
    Google,
    Google2,
    DeepL
}

public class SettingsModel
{
    // UI
    public double Opacity { get; set; } = 0.85;
    public string SubtitleColor { get; set; } = "#FFFFFF";
    public string TranslationColor { get; set; } = "#FFD700";
    public string BackgroundColor { get; set; } = "#0D0D1A";
    public double FontSize { get; set; } = 17;
    public string FontWeight { get; set; } = "SemiBold";

    // Live Captions
    public int LiveCaptionsPollingIntervalMs { get; set; } = 30;

    // Translation polling
    public int TranslationPollingIntervalMs { get; set; } = 800;

    // Translation
    public TranslationEngine Engine { get; set; } = TranslationEngine.Google;
    public string GoogleApiKey { get; set; } = string.Empty;
    public string DeepLApiKey { get; set; } = string.Empty;
    public string DeepLServerUrl { get; set; } = "https://api-free.deepl.com";
    public string TargetLanguage { get; set; } = "zh";
    public int ContextWindowSize { get; set; } = 6;

}
