namespace NetDesktop.Models;

public enum VadPreset
{
    Realtime,
    Accurate,
    Auto,
    Custom
}

public enum TranslationEngine
{
    Google,
    DeepL
}

public class SettingsModel
{
    // UI
    public double Opacity { get; set; } = 0.85;
    public string SubtitleColor { get; set; } = "#FFFFFF";
    public string TranslationColor { get; set; } = "#FFD700";

    // VAD
    public VadPreset VadMode { get; set; } = VadPreset.Auto;
    public float MinSilenceDuration { get; set; } = 0.5f;
    public float VadThreshold { get; set; } = 0.5f;
    public float MinSpeechDuration { get; set; } = 0.25f;

    // Translation
    public TranslationEngine Engine { get; set; } = TranslationEngine.Google;
    public string DeepLApiKey { get; set; } = string.Empty;
    public string DeepLServerUrl { get; set; } = "https://api-free.deepl.com"; // free tier default
    public string TargetLanguage { get; set; } = "zh";
    public int ContextWindowSize { get; set; } = 6;

    // Model paths
    public string SileroVadModelPath { get; set; } = "models/silero_vad.onnx";
    public string SenseVoiceModelPath { get; set; } = "models/sense-voice/model.onnx";
    public string SenseVoiceTokensPath { get; set; } = "models/sense-voice/tokens.txt";
}
