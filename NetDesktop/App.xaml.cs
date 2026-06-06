using System.IO;
using System.Runtime.InteropServices;
using System.Windows;
using NetDesktop.Services;

namespace NetDesktop;

public partial class App : Application
{
    [DllImport("kernel32.dll")] static extern bool AllocConsole();

    public static SettingsService SettingsService { get; } = new();
    public static Models.SettingsModel Settings { get; private set; } = new();

    protected override void OnStartup(StartupEventArgs e)
    {
        AllocConsole();
        Console.OutputEncoding = System.Text.Encoding.UTF8;

        // 先加载配置，再启动窗口和服务
        Settings = SettingsService.Load();
        Console.WriteLine($"[App] 配置已加载: 引擎={Settings.Engine}, 目标语言={Settings.TargetLanguage}");

        Console.WriteLine("[App] 字幕翻译启动");
        base.OnStartup(e);
    }
}
