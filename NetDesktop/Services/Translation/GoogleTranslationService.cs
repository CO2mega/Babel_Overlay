using System.Net.Http;
using System.Text.Json;

namespace NetDesktop.Services.Translation;

/// <summary>
/// Google 字典扩展 API 翻译实现。无需注册 API Key，通过伪装 Chrome 扩展请求头调用。
/// 注意：该接口可能随时被 Google 限制或关闭。
/// </summary>
public class GoogleTranslationService : ITranslationService
{
    private const string FallbackApiKey = "AIzaSyA6EEtrDCfBkHV8uU2lgGY-N383ZgAOo7Y";
    private readonly HttpClient _client;
    private readonly string _apiKey;

    public GoogleTranslationService(string? userKey = null, int timeoutSeconds = 10)
    {
        _apiKey = string.IsNullOrWhiteSpace(userKey) ? FallbackApiKey : userKey;
        _client = new HttpClient { Timeout = TimeSpan.FromSeconds(timeoutSeconds) };
    }

    /// <summary>
    /// 使用 Google 字典扩展 API 翻译文本。
    /// </summary>
    /// <param name="text">待翻译的源文本。</param>
    /// <param name="targetLang">目标语言代码（"zh"/"ja"/"ko" 等）。</param>
    /// <param name="ct">取消令牌。</param>
    /// <returns>翻译结果文本；超时或网络错误时返回错误描述。</returns>
    public async Task<string> TranslateAsync(string text, string targetLang, CancellationToken ct = default)
    {
        if (string.IsNullOrWhiteSpace(text)) return string.Empty;

        var encodedText = Uri.EscapeDataString(text);
        var url = $"https://dictionaryextension-pa.googleapis.com/v1/dictionaryExtensionData?" +
                  $"language={targetLang}&key={_apiKey}&term={encodedText}&strategy=2";

        var request = new HttpRequestMessage(HttpMethod.Get, url);
        request.Headers.Add("x-referer", "chrome-extension://mgijmajocgfcbeboacabfgobmjgjcoja");

        try
        {
            var response = await _client.SendAsync(request, ct);
            if (!response.IsSuccessStatusCode)
                return $"[翻译失败: {response.StatusCode}]";

            var body = await response.Content.ReadAsStringAsync(ct);
            using var doc = JsonDocument.Parse(body);
            var root = doc.RootElement;

            if (root.TryGetProperty("translateResponse", out var tr) &&
                tr.TryGetProperty("translateText", out var tt))
            {
                return tt.GetString() ?? text;
            }

            return text;
        }
        catch (OperationCanceledException)
        {
            return "[翻译超时]";
        }
        catch (Exception ex)
        {
            return $"[翻译错误: {ex.Message}]";
        }
    }
}
