namespace NetDesktop.Services.Translation;

/// <summary>
/// 翻译引擎统一接口，支持 Google 翻译和 DeepL 切换。
/// </summary>
public interface ITranslationService
{
    /// <summary>
    /// 将文本翻译为目标语言。
    /// </summary>
    /// <param name="text">待翻译的源文本。</param>
    /// <param name="targetLang">目标语言代码（"zh"/"ja"/"ko" 等）。</param>
    /// <param name="ct">取消令牌。</param>
    /// <returns>翻译结果；失败时返回错误描述字符串。</returns>
    Task<string> TranslateAsync(string text, string targetLang, CancellationToken ct = default);
}
