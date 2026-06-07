using System.IO;
using System.Windows;

namespace NetDesktop;

public partial class App : Application
{
    private static StreamWriter? _logWriter;

    protected override void OnStartup(StartupEventArgs e)
    {
        var logDir = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "logs");
        Directory.CreateDirectory(logDir);
        var logFile = Path.Combine(logDir, $"{DateTime.Now:yyyyMMdd_HHmmss}.log");
        _logWriter = new StreamWriter(logFile, append: false, encoding: System.Text.Encoding.UTF8) { AutoFlush = true };
        Console.SetOut(_logWriter);

        var modelsDir = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "models");
        Directory.CreateDirectory(modelsDir);
        Console.WriteLine($"[App] 模型目录: {modelsDir}");
        Console.WriteLine($"[App] 日志文件: {logFile}");
        Console.WriteLine("[App] 字幕翻译启动");
        base.OnStartup(e);
    }
}
