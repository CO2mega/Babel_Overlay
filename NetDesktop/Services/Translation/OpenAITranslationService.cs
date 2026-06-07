using System.Net.Http;
using System.Net.Http.Headers;
using System.Text;
using System.Text.Json;

namespace NetDesktop.Services.Translation;

public class OpenAITranslationService : ITranslationService
{
    private readonly HttpClient _client;
    private readonly string _model;
    private readonly string _baseUrl;

    public OpenAITranslationService(string apiKey, string baseUrl, string model)
    {
        _baseUrl = baseUrl.TrimEnd('/');
        _model = model;
        _client = new HttpClient { Timeout = TimeSpan.FromSeconds(15) };
        _client.DefaultRequestHeaders.Authorization =
            new AuthenticationHeaderValue("Bearer", apiKey);
    }

    public async Task<string> TranslateAsync(string text, string targetLang, CancellationToken ct = default)
    {
        if (string.IsNullOrWhiteSpace(text)) return string.Empty;

        var body = new
        {
            model = _model,
            messages = new object[]
            {
                new { role = "system", content = $"Translate the following text to {targetLang}. Output only the translation, no explanations." },
                new { role = "user", content = text }
            }
        };

        var json = JsonSerializer.Serialize(body);
        var content = new StringContent(json, Encoding.UTF8, "application/json");

        try
        {
            var response = await _client.PostAsync($"{_baseUrl}/v1/chat/completions", content, ct);
            if (!response.IsSuccessStatusCode)
                return $"[翻译失败: {response.StatusCode}]";

            var responseBody = await response.Content.ReadAsStringAsync(ct);
            using var doc = JsonDocument.Parse(responseBody);
            var root = doc.RootElement;

            if (root.TryGetProperty("choices", out var choices) &&
                choices.GetArrayLength() > 0 &&
                choices[0].TryGetProperty("message", out var message) &&
                message.TryGetProperty("content", out var msgContent))
            {
                return msgContent.GetString()?.Trim() ?? text;
            }

            return "[翻译失败: 响应格式异常]";
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
