using System.Net.Http;
using System.Text.Json;

namespace NetDesktop.Services.Translation;

/// <summary>
/// Google 翻译免费接口（clients5），无需 API Key。
/// </summary>
public class Google2TranslationService : ITranslationService
{
    private readonly HttpClient _client;

    public Google2TranslationService(int timeoutSeconds = 10)
    {
        _client = new HttpClient { Timeout = TimeSpan.FromSeconds(timeoutSeconds) };
    }

    public async Task<string> TranslateAsync(string text, string targetLang, CancellationToken ct = default)
    {
        if (string.IsNullOrWhiteSpace(text)) return string.Empty;

        var encodedText = Uri.EscapeDataString(text);
        var url = $"https://clients5.google.com/translate_a/t?" +
                  $"client=dict-chrome-ex&sl=auto&" +
                  $"tl={targetLang}&" +
                  $"q={encodedText}";

        try
        {
            var response = await _client.GetAsync(url, ct);
            if (!response.IsSuccessStatusCode)
                return $"[翻译失败: {response.StatusCode}]";

            var body = await response.Content.ReadAsStringAsync(ct);
            var result = JsonSerializer.Deserialize<List<List<string>>>(body);
            return result?[0]?[0] ?? text;
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
