using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows.Automation;
using NetDesktop.Models;

namespace NetDesktop.Services.Stt;

/// <summary>
/// 通过 UI Automation 与 Windows Live Captions 交互，读取识别文本。
/// </summary>
public class LiveCaptionsSttService : IDisposable
{
    private const string ProcessName = "LiveCaptions";
    private const string CaptionsAutomationId = "CaptionsTextBlock";

    private AutomationElement? _window;
    private AutomationElement? _captionsTextBlock;
    private Thread? _pollThread;
    private volatile bool _running;
    private string _lastPartialText = string.Empty;
    private string _lastFinalText = string.Empty;

    private readonly int _pollingIntervalMs;

    public event Action<SubtitleEntry>? OnPartialResult;
    public event Action<SubtitleEntry>? OnFinalResult;
    public event Action<string>? Error;

    // ── P/Invoke ──
    private const int GWL_EXSTYLE = -20;
    private const int WS_EX_TOOLWINDOW = 0x00000080;
    private const int SW_MINIMIZE = 6;
    private const int SW_RESTORE = 9;

    [DllImport("user32.dll", SetLastError = true)]
    private static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);

    [DllImport("user32.dll", SetLastError = true)]
    private static extern int GetWindowLong(IntPtr hWnd, int nIndex);

    [DllImport("user32.dll", SetLastError = true)]
    private static extern int SetWindowLong(IntPtr hWnd, int nIndex, int dwNewLong);

    [DllImport("user32.dll", SetLastError = true)]
    private static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);

    [DllImport("user32.dll", SetLastError = true)]
    private static extern bool MoveWindow(IntPtr hWnd, int X, int Y, int nWidth, int nHeight, bool bRepaint);

    [DllImport("user32.dll", SetLastError = true)]
    private static extern bool SetForegroundWindow(IntPtr hWnd);

    [StructLayout(LayoutKind.Sequential)]
    private struct RECT
    {
        public int Left, Top, Right, Bottom;
    }

    // ── 句末标点 ──
    private static readonly char[] SentenceEndPunctuation =
    [
        '.', '!', '?',           // English
        '。', '！', '？',         // CJK
    ];

    public LiveCaptionsSttService(SettingsModel settings)
    {
        _pollingIntervalMs = settings.LiveCaptionsPollingIntervalMs;
    }

    public void Initialize()
    {
        Console.WriteLine("[LiveCaptions] 初始化...");

        try
        {
            _window = LaunchLiveCaptions();
            FixWindow(_window);
            HideWindow(_window);
            Console.WriteLine("[LiveCaptions] 窗口已启动并隐藏");

            _running = true;
            _pollThread = new Thread(PollLoop)
            {
                IsBackground = true,
                Name = "LiveCaptions-Poll"
            };
            _pollThread.Start();
            Console.WriteLine("[LiveCaptions] 轮询线程已启动");
        }
        catch (Exception ex)
        {
            var msg = $"Live Captions 启动失败: {ex.Message}";
            Console.WriteLine($"[LiveCaptions] 错误: {msg}");
            Error?.Invoke(msg);
        }
    }

    private AutomationElement LaunchLiveCaptions()
    {
        // 关闭已有进程
        foreach (var p in Process.GetProcessesByName(ProcessName))
        {
            p.Kill();
            p.WaitForExit();
        }

        var process = Process.Start(ProcessName);
        if (process == null)
            throw new Exception("无法启动 LiveCaptions 进程");

        // 等待窗口出现
        AutomationElement? window = null;
        for (int attempt = 0; attempt < 10000; attempt++)
        {
            window = FindWindowByProcessId(process.Id);
            if (window != null && window.Current.ClassName == "LiveCaptionsDesktopWindow")
                break;
            Thread.Sleep(10);
        }

        if (window == null)
            throw new Exception("超时：未找到 LiveCaptions 窗口");

        return window;
    }

    private void FixWindow(AutomationElement window)
    {
        var hWnd = new IntPtr((long)window.Current.NativeWindowHandle);
        GetWindowRect(hWnd, out var rect);
        int width = rect.Right - rect.Left;
        int height = rect.Bottom - rect.Top;

        if (rect.Left < 0 || rect.Top < 0 || width < 100 || height < 100)
            MoveWindow(hWnd, 800, 600, 600, 200, true);
    }

    private void HideWindow(AutomationElement window)
    {
        var hWnd = new IntPtr((long)window.Current.NativeWindowHandle);
        int exStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
        ShowWindow(hWnd, SW_MINIMIZE);
        SetWindowLong(hWnd, GWL_EXSTYLE, exStyle | WS_EX_TOOLWINDOW);
    }

    private void PollLoop()
    {
        while (_running)
        {
            try
            {
                if (_window == null)
                {
                    Thread.Sleep(2000);
                    continue;
                }

                // 检查窗口是否仍然可用
                try
                {
                    _ = _window.Current.Name;
                }
                catch (ElementNotAvailableException)
                {
                    _window = null;
                    _captionsTextBlock = null;
                    continue;
                }

                var rawText = GetCaptions();
                if (string.IsNullOrEmpty(rawText))
                {
                    Thread.Sleep(_pollingIntervalMs);
                    continue;
                }

                var fullText = CleanText(rawText);
                if (string.IsNullOrEmpty(fullText))
                {
                    Thread.Sleep(_pollingIntervalMs);
                    continue;
                }

                // 从全文中提取最后一个句子（最后一个句末标点之后的文本）
                // 参考 Translator.SyncLoop 的逻辑
                int lastEOSIndex;
                if (fullText.Length > 0 && IsSentenceEnd(fullText[^1]))
                    lastEOSIndex = fullText[..^1].LastIndexOfAny(SentenceEndPunctuation);
                else
                    lastEOSIndex = fullText.LastIndexOfAny(SentenceEndPunctuation);

                string latestCaption = lastEOSIndex >= 0
                    ? fullText[(lastEOSIndex + 1)..]
                    : fullText;

                // partial: 正在说的句子（不以标点结尾）
                if (latestCaption.Length > 0 && !IsSentenceEnd(latestCaption[^1]))
                {
                    if (string.CompareOrdinal(_lastPartialText, latestCaption) != 0)
                    {
                        // 如果新 partial 不是旧 partial 的延续，先确认旧的
                        if (!string.IsNullOrEmpty(_lastPartialText) &&
                            !latestCaption.StartsWith(_lastPartialText, StringComparison.Ordinal))
                        {
                            _lastFinalText = _lastPartialText;
                            OnFinalResult?.Invoke(new SubtitleEntry
                            {
                                OriginalText = _lastPartialText,
                                Timestamp = DateTime.Now
                            });
                        }
                        _lastPartialText = latestCaption;
                        OnPartialResult?.Invoke(new SubtitleEntry
                        {
                            OriginalText = latestCaption,
                            Timestamp = DateTime.Now
                        });
                    }
                }

                // final: 完整句子（以标点结尾），跳过纯标点碎片
                if (latestCaption.Length > 1 && IsSentenceEnd(latestCaption[^1]))
                {
                    if (string.CompareOrdinal(_lastFinalText, latestCaption) != 0)
                    {
                        _lastFinalText = latestCaption;
                        _lastPartialText = string.Empty;
                        Console.WriteLine($"[LiveCaptions] final: \"{latestCaption}\"");
                        OnFinalResult?.Invoke(new SubtitleEntry
                        {
                            OriginalText = latestCaption,
                            Timestamp = DateTime.Now
                        });
                    }
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[LiveCaptions] 轮询异常: {ex.Message}");
            }

            Thread.Sleep(_pollingIntervalMs);
        }
    }

    private string GetCaptions()
    {
        _captionsTextBlock ??= FindElementByAutomationId(_window!, CaptionsAutomationId);
        try
        {
            return _captionsTextBlock?.Current.Name ?? string.Empty;
        }
        catch (ElementNotAvailableException)
        {
            _captionsTextBlock = null;
            return string.Empty;
        }
    }

    private static string CleanText(string text)
    {
        // 去除 Live Captions 可能添加的多余换行
        var sb = new StringBuilder(text.Length);
        foreach (var ch in text)
        {
            if (ch == '\n' || ch == '\r')
                sb.Append(' ');
            else
                sb.Append(ch);
        }
        return sb.ToString().Trim();
    }

    private static bool IsSentenceEnd(char c) => Array.IndexOf(SentenceEndPunctuation, c) >= 0;

    private static AutomationElement? FindWindowByProcessId(int processId)
    {
        var condition = new PropertyCondition(AutomationElement.ProcessIdProperty, processId);
        return AutomationElement.RootElement.FindFirst(TreeScope.Children, condition);
    }

    private static AutomationElement? FindElementByAutomationId(AutomationElement parent, string automationId)
    {
        try
        {
            var condition = new PropertyCondition(AutomationElement.AutomationIdProperty, automationId);
            return parent.FindFirst(TreeScope.Descendants, condition);
        }
        catch
        {
            return null;
        }
    }

    /// <summary>
    /// 恢复 Live Captions 窗口并点击其设置按钮，让用户调整语言/麦克风。
    /// </summary>
    public void ShowSettings()
    {
        if (_window == null) return;

        try
        {
            var hWnd = new IntPtr((long)_window.Current.NativeWindowHandle);

            // 恢复窗口显示
            int exStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
            SetWindowLong(hWnd, GWL_EXSTYLE, exStyle & ~WS_EX_TOOLWINDOW);
            ShowWindow(hWnd, SW_RESTORE);
            SetForegroundWindow(hWnd);

            // 点击设置按钮
            var settingsBtn = FindElementByAutomationId(_window, "SettingsButton");
            if (settingsBtn != null)
            {
                var invokePattern = settingsBtn.GetCurrentPattern(InvokePattern.Pattern) as InvokePattern;
                invokePattern?.Invoke();
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"[LiveCaptions] 打开设置失败: {ex.Message}");
        }
    }

    public void Dispose()
    {
        _running = false;
        _pollThread?.Join(2000);

        if (_window != null)
        {
            try
            {
                var hWnd = new IntPtr((long)_window.Current.NativeWindowHandle);
                var process = Process.GetProcessesByName(ProcessName).FirstOrDefault();
                process?.Kill();
            }
            catch { /* 忽略清理异常 */ }
        }

        _window = null;
        _captionsTextBlock = null;
    }
}
