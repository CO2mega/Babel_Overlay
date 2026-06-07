namespace NetDesktop.Services.Translation;

/// <summary>
/// DeepL 官方 SDK 翻译实现。支持免费版 / 付费版端点，通过 ServerUrl 切换。
/// </summary>
public class DeepLTranslationService : ITranslationService
{
    private DeepL.Translator? _translator;
    private readonly string _apiKey;
    private readonly string _serverUrl;

    /// <summary>
    /// 创建 DeepL 翻译服务。
    /// </summary>
    /// <param name="apiKey">DeepL API 认证密钥。</param>
    /// <param name="serverUrl">API 端点 URL，默认为免费版。</param>
    public DeepLTranslationService(string apiKey, string serverUrl = "https://api-free.deepl.com")
    {
        _apiKey = apiKey;
        _serverUrl = serverUrl;
    }

    private DeepL.Translator GetTranslator()
    {
        if (_translator == null)
        {
            var options = new DeepL.TranslatorOptions { ServerUrl = _serverUrl };
            _translator = new DeepL.Translator(_apiKey, options);
        }
        return _translator;
    }

    /// <summary>
    /// 使用 DeepL API 翻译文本，支持中/英/日/韩。
    /// </summary>
    /// <param name="text">待翻译的英文源文本。</param>
    /// <param name="targetLang">目标语言代码（"zh"/"en"/"ja"/"ko" 等）。</param>
    /// <param name="ct">取消令牌。</param>
    /// <returns>翻译结果；超时或异常时返回错误描述。</returns>
    public async Task<string> TranslateAsync(string text, string targetLang, CancellationToken ct = default)
    {
        if (string.IsNullOrWhiteSpace(text)) return string.Empty;

        try
        {
            var translator = GetTranslator();
            var targetLangCode = targetLang switch
            {
                "zh" => DeepL.LanguageCode.ChineseSimplified,
                "en" => DeepL.LanguageCode.English,
                "ja" => DeepL.LanguageCode.Japanese,
                "ko" => DeepL.LanguageCode.Korean,
                _ => targetLang
            };

            var result = await translator.TranslateTextAsync(text, DeepL.LanguageCode.English, targetLangCode, cancellationToken: ct);
            return result.Text;
        }
        catch (OperationCanceledException) { return "[翻译超时]"; }
        catch (Exception ex) { return $"[翻译错误: {ex.Message}]"; }
    }
}
