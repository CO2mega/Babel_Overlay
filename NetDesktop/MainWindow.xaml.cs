using System.ComponentModel;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Input;
using System.Windows.Media;
using NetDesktop.Models;
using NetDesktop.Services;
using NetDesktop.Services.Display;

namespace NetDesktop;

public partial class MainWindow : Window
{
    private SubtitlePipeline? _pipeline;
    private readonly SettingsService _settingsService = new();
    private SettingsModel _settings = new();
    private bool _isLoading;
    private readonly LineBuffer _originalLines = new(2, 2);
    private readonly LineBuffer _translationLines = new(2, 2);

    public MainWindow()
    {
        InitializeComponent();
    }

    private void Window_Loaded(object sender, RoutedEventArgs e)
    {
        _isLoading = true;

        _settings = _settingsService.Load();
        LoadSettingsToUI();
        ApplyAppearance();
        SetupSliderLabels();

        _pipeline = new SubtitlePipeline(_settings);
        _pipeline.OriginalBuffer.ContentChanged += text => _originalLines.Update(text);
        _pipeline.TranslationBuffer.ContentChanged += text => _translationLines.Update(text);
        _pipeline.Manager.HistoryChanged += OnHistoryEntryAdded;
        _pipeline.Error += msg => Dispatcher.Invoke(() => StatusText.Text = msg);

        _originalLines.LinesChanged += OnOriginalLinesChanged;
        _translationLines.LinesChanged += OnTranslationLinesChanged;

        HistoryList.ItemsSource = _pipeline.Manager.History;
        BindingOperations.EnableCollectionSynchronization(_pipeline.Manager.History, new object());

        _pipeline.Start();

        StatusText.Text = "字幕翻译已启动";
        _isLoading = false;
    }

    private void Window_Closing(object? sender, CancelEventArgs e)
    {
        _pipeline?.Dispose();
    }

    private void Border_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
    {
        if (e.ClickCount == 2)
            WindowState = WindowState == WindowState.Maximized ? WindowState.Normal : WindowState.Maximized;
        else
            DragMove();
    }

    private void CloseBtn_Click(object sender, RoutedEventArgs e)
    {
        Close();
    }

    private void NavItemClick(object sender, RoutedEventArgs e)
    {
        if (sender is not Wpf.Ui.Controls.NavigationViewItem item) return;
        var tag = item.Tag?.ToString();
        PageOverlay.Visibility = tag == "0" ? Visibility.Visible : Visibility.Collapsed;
        PageHistory.Visibility = tag == "1" ? Visibility.Visible : Visibility.Collapsed;
        PageSettings.Visibility = tag == "2" ? Visibility.Visible : Visibility.Collapsed;
    }

    private void CbEngine_SelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (_isLoading) return;
        var idx = CbEngine.SelectedIndex;
        GoogleKeyPanel.Visibility = idx == 0 ? Visibility.Visible : Visibility.Collapsed;
        DeepLPanel.Visibility = idx == 2 ? Visibility.Visible : Visibility.Collapsed;
        OpenAIPanel.Visibility = idx == 3 ? Visibility.Visible : Visibility.Collapsed;
    }

    
    private void ColorBox_TextChanged(object sender, TextChangedEventArgs e)
    {
        if (sender is not System.Windows.Controls.TextBox tb) return;
        var rect = tb.Name switch
        {
            "TbSubtitleColor" => RectSubtitleColor,
            "TbTranslationColor" => RectTranslationColor,
            "TbBackgroundColor" => RectBackgroundColor,
            _ => null
        };
        if (rect == null) return;
        try
        {
            var color = (Color)ColorConverter.ConvertFromString(tb.Text);
            rect.Fill = new SolidColorBrush(color);
        }
        catch { }
    }

    private void SaveSettings_Click(object sender, RoutedEventArgs e)
    {
        _settings.Engine = CbEngine.SelectedIndex switch
        {
            1 => TranslationEngine.Google2,
            2 => TranslationEngine.DeepL,
            3 => TranslationEngine.OpenAI,
            _ => TranslationEngine.Google
        };
        _settings.GoogleApiKey = TbGoogleKey.Text;
        _settings.DeepLApiKey = TbDeepLKey.Text;
        _settings.DeepLServerUrl = TbDeepLUrl.Text;
        _settings.TargetLanguage = TbTargetLang.Text;
        _settings.OpenAIApiUrl = TbOpenAIUrl.Text;
        _settings.OpenAIApiKey = TbOpenAIKey.Text;
        _settings.OpenAIModel = TbOpenAIModel.Text;

        _settings.Opacity = SlOpacity.Value;
        _settings.FontSize = SlFontSize.Value;
        _settings.FontWeight = ((ComboBoxItem)CbFontWeight.SelectedItem).Content.ToString()!;
        _settings.SubtitleColor = TbSubtitleColor.Text;
        _settings.TranslationColor = TbTranslationColor.Text;
        _settings.BackgroundColor = TbBackgroundColor.Text;

        _settingsService.Save(_settings);
        ApplyAppearance();
        _pipeline?.SwitchTranslationEngine(_settings.Engine, _settings.DeepLApiKey,
            _settings.DeepLServerUrl, _settings.GoogleApiKey);

        StatusText.Text = "设置已保存";
    }

    private void OnOriginalLinesChanged(string[] lines)
    {
        Dispatcher.Invoke(() => OriginalText.Text = string.Join("\n", lines));
    }

    private void OnTranslationLinesChanged(string[] lines)
    {
        Dispatcher.Invoke(() => TranslatedText.Text = string.Join("\n", lines));
    }

    private void OnHistoryEntryAdded(SubtitleEntry entry)
    {
    }

    private void LoadSettingsToUI()
    {
        CbEngine.SelectedIndex = _settings.Engine switch
        {
            TranslationEngine.Google => 0,
            TranslationEngine.Google2 => 1,
            TranslationEngine.DeepL => 2,
            TranslationEngine.OpenAI => 3,
            _ => 0
        };
        GoogleKeyPanel.Visibility = _settings.Engine == TranslationEngine.Google
            ? Visibility.Visible : Visibility.Collapsed;
        DeepLPanel.Visibility = _settings.Engine == TranslationEngine.DeepL
            ? Visibility.Visible : Visibility.Collapsed;
        OpenAIPanel.Visibility = _settings.Engine == TranslationEngine.OpenAI
            ? Visibility.Visible : Visibility.Collapsed;

        TbGoogleKey.Text = _settings.GoogleApiKey;
        TbDeepLKey.Text = _settings.DeepLApiKey;
        TbDeepLUrl.Text = _settings.DeepLServerUrl;
        TbTargetLang.Text = _settings.TargetLanguage;
        TbOpenAIUrl.Text = _settings.OpenAIApiUrl;
        TbOpenAIKey.Text = _settings.OpenAIApiKey;
        TbOpenAIModel.Text = _settings.OpenAIModel;

        SlOpacity.Value = _settings.Opacity;
        LblOpacity.Text = $"{_settings.Opacity:F1}";

        SlFontSize.Value = _settings.FontSize;
        LblFontSize.Text = $"{_settings.FontSize:F0}";

        CbFontWeight.SelectedIndex = _settings.FontWeight switch
        {
            "Normal" => 0, "Medium" => 1, "SemiBold" => 2, "Bold" => 3, _ => 2
        };

        TbSubtitleColor.Text = _settings.SubtitleColor;
        TbTranslationColor.Text = _settings.TranslationColor;
        TbBackgroundColor.Text = _settings.BackgroundColor;

        UpdateColorPreviews();
    }

    private void SetupSliderLabels()
    {
        SlOpacity.ValueChanged += (_, _) => LblOpacity.Text = $"{SlOpacity.Value:F1}";
        SlFontSize.ValueChanged += (_, _) => LblFontSize.Text = $"{SlFontSize.Value:F0}";
    }

    private void ApplyAppearance()
    {
        var bg = ParseColor(_settings.BackgroundColor);
        RootBorder.Background = new SolidColorBrush(bg);

        var subColor = ParseColor(_settings.SubtitleColor);
        OriginalText.Foreground = new SolidColorBrush(subColor);

        var transColor = ParseColor(_settings.TranslationColor);
        TranslatedText.Foreground = new SolidColorBrush(transColor);

        OriginalText.FontSize = _settings.FontSize;
        TranslatedText.FontSize = Math.Max(10, _settings.FontSize - 3);

        OriginalText.FontWeight = FontWeightFromString(_settings.FontWeight);
    }

    private void UpdateColorPreviews()
    {
        RectSubtitleColor.Fill = new SolidColorBrush(ParseColor(_settings.SubtitleColor));
        RectTranslationColor.Fill = new SolidColorBrush(ParseColor(_settings.TranslationColor));
        RectBackgroundColor.Fill = new SolidColorBrush(ParseColor(_settings.BackgroundColor));
    }

    private static Color ParseColor(string hex)
    {
        try { return (Color)ColorConverter.ConvertFromString(hex); }
        catch { return Colors.White; }
    }

    private static FontWeight FontWeightFromString(string weight)
    {
        return weight switch
        {
            "Normal" => FontWeights.Normal,
            "Medium" => FontWeights.Medium,
            "SemiBold" => FontWeights.SemiBold,
            "Bold" => FontWeights.Bold,
            _ => FontWeights.SemiBold
        };
    }
    private void LiveCaptionsSettings_Click(object sender, RoutedEventArgs e)
    {
        _pipeline?.ShowLiveCaptionsSettings();
    }
}
