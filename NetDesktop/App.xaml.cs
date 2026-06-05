using System.Runtime.InteropServices;
using System.Windows;

namespace NetDesktop;

public partial class App : Application
{
    [DllImport("kernel32.dll")] static extern bool AllocConsole();

    protected override void OnStartup(StartupEventArgs e)
    {
        AllocConsole();
        Console.OutputEncoding = System.Text.Encoding.UTF8;
        Console.WriteLine("[App] 字幕翻译启动");
        base.OnStartup(e);
    }
}
