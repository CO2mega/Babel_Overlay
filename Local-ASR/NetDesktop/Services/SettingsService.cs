using System.IO;
using System.Text.Json;
using System.Text.Json.Serialization;
using NetDesktop.Models;

namespace NetDesktop.Services;

public class SettingsService
{
    private static readonly string SettingsPath = Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
        "BabelOverlay", "settings.json");

    private static readonly JsonSerializerOptions Options = new()
    {
        WriteIndented = true,
        Converters = { new JsonStringEnumConverter() }
    };

    public SettingsModel Load()
    {
        try
        {
            if (!File.Exists(SettingsPath)) return new SettingsModel();
            var json = File.ReadAllText(SettingsPath);
            return JsonSerializer.Deserialize<SettingsModel>(json, Options) ?? new SettingsModel();
        }
        catch
        {
            return new SettingsModel();
        }
    }

    public void Save(SettingsModel settings)
    {
        Directory.CreateDirectory(Path.GetDirectoryName(SettingsPath)!);
        File.WriteAllText(SettingsPath, JsonSerializer.Serialize(settings, Options));
    }
}
