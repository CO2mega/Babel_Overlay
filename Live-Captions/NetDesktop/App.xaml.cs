using System.IO;
using System.Windows;
using NetDesktop.Services;

namespace NetDesktop;

public partial class App : Application
{
    private static StreamWriter? _logWriter;

    public static SettingsService SettingsService { get; } = new();
    public static Models.SettingsModel Settings { get; private set; } = new();

    protected override void OnStartup(StartupEventArgs e)
    {
        var logDir = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "logs");
        Directory.CreateDirectory(logDir);
        var logFile = Path.Combine(logDir, $"{DateTime.Now:yyyyMMdd_HHmmss}.log");
        _logWriter = new StreamWriter(logFile, append: false, encoding: System.Text.Encoding.UTF8) { AutoFlush = true };
        Console.SetOut(_logWriter);

        Settings = SettingsService.Load();
        Console.WriteLine($"[App] 配置已加载: 引擎={Settings.Engine}, 目标语言={Settings.TargetLanguage}");
        Console.WriteLine($"[App] 日志文件: {logFile}");
        Console.WriteLine("[App] 字幕翻译启动");
        base.OnStartup(e);
    }
}
