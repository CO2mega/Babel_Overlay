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
    public string BackgroundColor { get; set; } = "#CC0D0D1A";
    public double FontSize { get; set; } = 17;
    public string FontWeight { get; set; } = "SemiBold";

    // Streaming STT — Endpoint detection
    public float EndpointMinTrailingSilence { get; set; } = 1.2f;
    public float EndpointMinUtteranceLength { get; set; } = 2.0f;

    // Translation
    public TranslationEngine Engine { get; set; } = TranslationEngine.Google;
    public string GoogleApiKey { get; set; } = string.Empty;
    public string DeepLApiKey { get; set; } = string.Empty;
    public string DeepLServerUrl { get; set; } = "https://api-free.deepl.com";
    public string TargetLanguage { get; set; } = "zh";
    public int ContextWindowSize { get; set; } = 6;

    // Model paths (streaming zipformer)
    public string StreamingEncoderPath { get; set; } = "models/encoder.onnx";
    public string StreamingDecoderPath { get; set; } = "models/decoder.onnx";
    public string StreamingJoinerPath { get; set; } = "models/joiner.onnx";
    public string StreamingTokensPath { get; set; } = "models/tokens.txt";
}
