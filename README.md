# Babel Overlay

实时屏幕翻译工具，支持两种模式。

## 演示视频

https://pic.pelargonium.top/%E6%BC%94%E7%A4%BA%E8%A7%86%E9%A2%91.mp4

## 版本说明

### Local-ASR
通用，无需系统支持
- 使用本地 ASR（自动语音识别）引擎（Sherpa-ONNX）
- 无需系统字幕支持
- 独立运行，不依赖系统组件
- 需要下载语音模型文件

### Live-Captions
适用于 **Windows 11 22H2 及更新版本**，利用系统内置的实时字幕功能。

- 通过 UI Automation 读取系统 Live Captions 的识别结果
- 更低的资源占用
- 需要 Windows 11 22H2+ 系统支持


## 项目架构

项目采用管道（Pipeline）架构，核心由 `SubtitlePipeline` 统一调度。Pipeline 将音频采集、语音识别、翻译、字幕管理串联成一条完整的数据处理链路，各模块通过事件驱动解耦，便于独立替换和扩展。

### 目录结构

```
NetDesktop/
├── App.xaml / App.xaml.cs          # 应用入口
├── MainWindow.xaml / .cs           # 主界面（WPF + WPF UI）
├── DataModels/
│   ├── SettingsModel.cs            # 配置数据模型
│   └── SubtitleEntry.cs            # 字幕条目数据结构
├── Services/
│   ├── SettingsService.cs          # 配置读写
│   ├── SubtitlePipeline.cs         # 核心管道：串联音频→识别→翻译→显示
│   ├── RollingBuffer.cs            # 滚动文本缓冲
│   ├── Audio/（Local-ASR 专用）
│   │   ├── AudioCaptureService.cs  # 系统音频捕获
│   │   └── AudioResampler.cs       # 音频重采样
│   ├── Stt/
│   │   ├── SttService.cs           # Sherpa-ONNX 流式识别（Local-ASR）
│   │   └── LiveCaptionsSttService.cs # UI Automation 读取字幕（Live-Captions）
│   ├── Translation/
│   │   ├── ITranslationService.cs  # 翻译接口
│   │   ├── GoogleTranslationService.cs    # Google 翻译
│   │   ├── Google2TranslationService.cs   # Google 翻译 （另一个接口）
│   │   ├── DeepLTranslationService.cs     # DeepL 翻译
│   │   ├── OpenAITranslationService.cs    # OpenAI 翻译
│   │   └── ContextualTranslator.cs        # 上下文感知翻译器
│   ├── Subtitle/
│   │   └── SubtitleManager.cs      # 字幕管理（历史、显示）
│   └── Display/
│       └── LineBuffer.cs           # 显示行缓冲
└── models/                         # 其他模型定义
```

### 数据流

整个处理流程由 `SubtitlePipeline` 统一调度。应用启动后，Pipeline 按顺序串联各模块：首先通过 `AudioCaptureService` 捕获系统音频，将原始音频送入 `AudioResampler` 重采样为 16kHz 单声道格式，然后交给 STT 模块进行语音识别。

STT 模块根据版本不同有两种实现：Local-ASR 使用 Sherpa-ONNX 在本地进行流式语音识别，支持 partial（边说边出）和 final（句末确认）两种结果；Live-Captions 则通过 UI Automation 轮询系统字幕窗口，读取识别文本。

识别出的原文进入 `RollingBuffer` 进行滚动缓冲，同时触发 `ContextualTranslator` 进行翻译。翻译支持 Google、DeepL、OpenAI 等多种引擎，可运行时切换。翻译完成后，`SubtitleManager` 将原文和译文配对，最终由 UI 层叠加显示在屏幕上。

### 缓冲逻辑

`RollingBuffer` 负责管理待翻译的原文文本，采用"已确认 + 临时部分"的双层结构：

- `_committed`（已确认）：存储已通过端点检测的完整句子，累积拼接
- `_partial`（临时部分）：存储正在识别中的文本，随 STT 的 partial 结果实时更新

工作流程：STT 产生 partial 结果时调用 `UpdatePartial()`，将临时文本替换为最新识别内容并触发 `ContentChanged` 事件；端点检测确认一句结束时调用 `CommitPartial()`，将临时文本追加到已确认部分，并清空临时缓冲。

缓冲区设有最大长度限制（默认 300 字符），超出时自动裁剪前部内容（保留 60%），防止翻译请求文本过长。裁剪后触发 `ContentChanged`，驱动翻译器重新翻译。

### 翻译逻辑

`ContextualTranslator` 封装了翻译引擎的调度和结果排序：

- 使用递增序号（`_requestSeq` / `_displayedSeq`）确保乱序响应不会覆盖更新的结果。每次翻译请求分配一个序号，返回时检查：只有当响应序号大于已显示序号时才更新 UI，否则丢弃。
- 支持运行时切换翻译引擎（Google / DeepL / OpenAI），无需重启。

翻译触发链路：`RollingBuffer.ContentChanged` → `ContextualTranslator.TranslateRolling()` → `ITranslationService.TranslateAsync()` → `OnTranslationReady` 事件 → `SubtitleManager.UpdateTranslation()` 回写译文。

### 字幕管理

`SubtitleManager` 维护当前字幕和历史记录（最多 100 条）：

- **当前字幕**（`Current`）：正在识别或翻译的条目，partial 结果直接更新文本，final 结果将其推入历史并开启新条目。
- **历史记录**（`History`）：`ObservableCollection`，新条目插入头部，超限时删除尾部。
- 所有变更通过 `CurrentChanged` 和 `HistoryChanged` 事件通知 UI 刷新。

### 翻译引擎支持

- Google Translate
- DeepL
- OpenAI（可自定义模型和 API 地址）

## 使用方法

1. 根据你的系统版本选择对应的文件夹
2. 构建
3. 运行 `NetDesktop.exe`
4. 开始实时翻译
