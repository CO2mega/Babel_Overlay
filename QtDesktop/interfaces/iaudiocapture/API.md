# Audio Capture Library 接口说明

## 概述

`audio_capture.dll` 是一个 Windows 动态链接库，提供系统音频捕获功能，支持两种模式：

| 模式 | 说明 |
|------|------|
| **桌面音频监听** | 捕获所有应用程序的音频输出（扬声器/耳机混音） |
| **单应用音频捕获** | 捕获指定进程（PID）的音频输出 |

音频数据以 **32-bit IEEE Float** 格式通过回调函数实时传递给调用方。

## 构建

### 依赖

- Visual Studio 2022（MSVC 编译器，C++20）
- CMake 3.15+
- [vcpkg](https://github.com/microsoft/vcpkg)（用于安装 WIL 库）
- Windows 10 或更高版本

### 安装依赖

```powershell
vcpkg install wil:x64-windows
```

### 编译

```powershell
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=<vcpkg-root>/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```

构建产物：

| 文件 | 说明 |
|------|------|
| `build/Release/audio_capture.dll` | 动态库 |
| `build/Release/audio_capture.lib` | 导入库（链接时使用） |
| `build/Release/audio_capture_example.exe` | 示例控制台程序 |

### 集成到你的项目

**CMake 方式：**

```cmake
target_link_libraries(your_target PRIVATE
    /path/to/audio_capture.lib
)
```

**手动配置：**
- 将 `audio_capture_api.h` 加入 include 路径
- 链接 `audio_capture.lib`（导入库）
- 运行时确保 `audio_capture.dll` 在可执行文件目录或 PATH 中

---

## 数据类型

### AudioCaptureHandle

```c
typedef void* AudioCaptureHandle;
```

不透明句柄，由 `DesktopCapture_Create` 或 `ProcessCapture_Create` 返回，用于后续销毁。

### AudioCaptureCallback

```c
typedef void (*AudioCaptureCallback)(
    unsigned long long timestamp,  // QPC 时间戳
    float* data,                   // 音频数据（32-bit float 交错格式）
    unsigned int num_frames        // 采样帧数（非样本数）
);
```

音频数据回调函数原型。**注意：回调在音频采集线程中执行，请不要在回调中执行耗时操作。**

- `timestamp`：QPC（QueryPerformanceCounter）时间戳，单位为性能计数器 ticks
- `data`：指向音频数据的指针。多声道时为交错格式 `[L0,R0,L1,R1,...]`，单声道时为 `[M0,M1,...]`。数据仅在回调期间有效，如需保留请自行拷贝。
- `num_frames`：数据帧数。对于直通模式，总样本数 = `num_frames × 声道数`；对于转单声道模式，总样本数 = `num_frames`

### AudioConversionMode

```c
typedef enum {
    AUDIO_CAPTURE_PASS_THROUGH        = 0,  // 原始音频（不转换）
    AUDIO_CAPTURE_STEREO_TO_MONO_AVG  = 1,  // 立体声 → 单声道（平均）
    AUDIO_CAPTURE_STEREO_TO_MONO_WEIGHT = 2, // 立体声 → 单声道（ITU-R BS.775 加权）
    AUDIO_CAPTURE_EXTRACT_LEFT        = 3,  // 仅提取左声道
    AUDIO_CAPTURE_EXTRACT_RIGHT       = 4   // 仅提取右声道
} AudioConversionMode;
```

声道转换模式枚举。选择直通模式时，回调中的 `data` 保持原始声道数；选择其他模式时，回调中的 `data` 为单声道数据。

### AudioCaptureFormat

```c
typedef struct {
    unsigned int sample_rate;       // 采样率（Hz），如 48000
    unsigned short num_channels;    // 声道数，如 2
    unsigned short bits_per_sample; // 位深度，如 32
} AudioCaptureFormat;
```

音频格式描述结构体。

> **注意**：桌面音频监听模式下，实际格式以系统混音格式为准，传入的 format 仅作参考。单应用模式下，format 参数将被实际使用。

---

## API 函数

### DesktopCapture_Create

```c
AudioCaptureHandle DesktopCapture_Create(
    AudioCaptureFormat format,
    AudioConversionMode mode,
    AudioCaptureCallback callback
);
```

创建桌面音频捕获实例（捕获所有应用程序音频输出）。

**参数：**

| 参数 | 类型 | 说明 |
|------|------|------|
| `format` | `AudioCaptureFormat` | 期望的音频格式（桌面模式会被系统混音格式覆盖） |
| `mode` | `AudioConversionMode` | 声道转换模式 |
| `callback` | `AudioCaptureCallback` | 音频数据回调函数（不可为 NULL） |

**返回值：**
- 成功：返回有效的 `AudioCaptureHandle`
- 失败：返回 `NULL`（通常是权限不足或音频设备问题）

**示例：**
```c
AudioCaptureFormat fmt = AudioCapture_GetDefaultFormat();
AudioCaptureHandle handle = DesktopCapture_Create(fmt, AUDIO_CAPTURE_PASS_THROUGH, MyCallback);
if (!handle) {
    fprintf(stderr, "创建桌面音频捕获失败\n");
}
```

---

### ProcessCapture_Create

```c
AudioCaptureHandle ProcessCapture_Create(
    unsigned long target_pid,
    AudioCaptureFormat format,
    AudioConversionMode mode,
    AudioCaptureCallback callback
);
```

创建单应用音频捕获实例（捕获指定进程的音频输出）。

**参数：**

| 参数 | 类型 | 说明 |
|------|------|------|
| `target_pid` | `unsigned long` | 目标进程 ID（PID） |
| `format` | `AudioCaptureFormat` | 音频格式 |
| `mode` | `AudioConversionMode` | 声道转换模式 |
| `callback` | `AudioCaptureCallback` | 音频数据回调函数（不可为 NULL） |

**返回值：**
- 成功：返回有效的 `AudioCaptureHandle`
- 失败：返回 `NULL`（通常是进程不存在、无音频输出或权限不足）

**示例：**
```c
AudioCaptureFormat fmt = AudioCapture_GetDefaultFormat();
AudioCaptureHandle handle = ProcessCapture_Create(12345, fmt, AUDIO_CAPTURE_STEREO_TO_MONO_AVG, MyCallback);
if (!handle) {
    fprintf(stderr, "创建进程音频捕获失败\n");
}
```

---

### AudioCapture_Destroy

```c
void AudioCapture_Destroy(AudioCaptureHandle handle);
```

销毁音频捕获实例，停止捕获并释放资源。无论通过 `DesktopCapture_Create` 还是 `ProcessCapture_Create` 创建的句柄，均使用此函数销毁。

**参数：**

| 参数 | 类型 | 说明 |
|------|------|------|
| `handle` | `AudioCaptureHandle` | 要销毁的捕获句柄（可为 NULL，此时函数不做任何操作） |

---

### AudioCapture_GetDefaultFormat

```c
AudioCaptureFormat AudioCapture_GetDefaultFormat(void);
```

获取默认音频格式（48000 Hz, 2 声道, 32-bit float）。

**返回值：** 默认的 `AudioCaptureFormat` 结构体。

---

### AudioCapture_GetRMS

```c
float AudioCapture_GetRMS(const float* data, unsigned int num_samples);
```

计算音频数据的 RMS（均方根）值，可用于音量监控。

**参数：**

| 参数 | 类型 | 说明 |
|------|------|------|
| `data` | `const float*` | 音频样本数据 |
| `num_samples` | `unsigned int` | 样本数量 |

**返回值：** RMS 值（0.0 ~ 1.0 范围），若 `num_samples == 0` 则返回 0。

---

### AudioCapture_GetPeak

```c
float AudioCapture_GetPeak(const float* data, unsigned int num_samples);
```

获取音频数据的峰值幅度。

**参数：**

| 参数 | 类型 | 说明 |
|------|------|------|
| `data` | `const float*` | 音频样本数据 |
| `num_samples` | `unsigned int` | 样本数量 |

**返回值：** 峰值幅度（0.0 ~ 1.0 范围），若 `num_samples == 0` 则返回 0。

---

## 使用示例

### C 语言示例：捕获桌面全部音频

```c
#include <stdio.h>
#include <windows.h>
#include "audio_capture_api.h"

static volatile int g_running = 1;

BOOL WINAPI CtrlHandler(DWORD type) {
    if (type == CTRL_C_EVENT) g_running = 0;
    return TRUE;
}

void OnAudio(unsigned long long ts, float* data, unsigned int frames) {
    float rms = AudioCapture_GetRMS(data, frames);
    printf("[%llu] frames=%u rms=%.4f\n", ts, frames, rms);
}

int main() {
    SetConsoleCtrlHandler(CtrlHandler, TRUE);

    AudioCaptureFormat fmt = AudioCapture_GetDefaultFormat();
    AudioCaptureHandle handle = DesktopCapture_Create(fmt, AUDIO_CAPTURE_PASS_THROUGH, OnAudio);

    if (!handle) {
        fprintf(stderr, "Failed to start desktop capture\n");
        return 1;
    }

    printf("Capturing... Press Ctrl+C to stop\n");
    while (g_running) Sleep(100);

    AudioCapture_Destroy(handle);
    return 0;
}
```

### C 语言示例：捕获指定进程音频

```c
#include <stdio.h>
#include <windows.h>
#include "audio_capture_api.h"

static volatile int g_running = 1;

BOOL WINAPI CtrlHandler(DWORD type) {
    if (type == CTRL_C_EVENT) g_running = 0;
    return TRUE;
}

void OnAudio(unsigned long long ts, float* data, unsigned int frames) {
    // 数据已转为单声道
    float peak = AudioCapture_GetPeak(data, frames);
    printf("peak=%.4f\n", peak);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <PID>\n", argv[0]);
        return 1;
    }

    SetConsoleCtrlHandler(CtrlHandler, TRUE);

    unsigned long pid = strtoul(argv[1], NULL, 10);
    AudioCaptureFormat fmt = AudioCapture_GetDefaultFormat();

    AudioCaptureHandle handle = ProcessCapture_Create(
        pid, fmt, AUDIO_CAPTURE_STEREO_TO_MONO_AVG, OnAudio);

    if (!handle) {
        fprintf(stderr, "Failed to capture PID %lu\n", pid);
        return 1;
    }

    printf("Capturing PID %lu... Press Ctrl+C to stop\n", pid);
    while (g_running) Sleep(100);

    AudioCapture_Destroy(handle);
    return 0;
}
```

### C++ 示例：捕获桌面音频并保存为 WAV

```cpp
#include "audio_capture_api.h"
#include <windows.h>
#include <cstdio>
#include <fstream>
#include <vector>

static volatile bool g_running = true;
static std::vector<float> g_buffer;

BOOL WINAPI CtrlHandler(DWORD type) {
    if (type == CTRL_C_EVENT) g_running = false;
    return TRUE;
}

void OnAudio(unsigned long long ts, float* data, unsigned int frames) {
    // 将数据复制到全局缓冲区（实际使用中建议写文件或推送到队列）
    g_buffer.insert(g_buffer.end(), data, data + frames);
}

int main() {
    SetConsoleCtrlHandler(CtrlHandler, TRUE);

    AudioCaptureFormat fmt = AudioCapture_GetDefaultFormat();
    AudioCaptureHandle handle = DesktopCapture_Create(
        fmt, AUDIO_CAPTURE_STEREO_TO_MONO_AVG, OnAudio);

    if (!handle) {
        fprintf(stderr, "Failed to create capture\n");
        return 1;
    }

    printf("Recording... Press Ctrl+C to stop\n");
    while (g_running) Sleep(100);

    AudioCapture_Destroy(handle);

    printf("Captured %zu samples\n", g_buffer.size());
    return 0;
}
```

---

## 注意事项

1. **线程安全**：回调函数在独立的音频采集线程中执行，不建议在回调中执行耗时操作（如 I/O、锁竞争等）。如需处理数据，建议将数据拷贝到队列后异步处理。

2. **数据生命周期**：回调中的 `data` 指针仅在回调期间有效。回调返回后指针可能失效，请勿保存指针供后续使用。

3. **COM 初始化**：如果你的应用程序已调用 `CoInitializeEx`（使用 `COINIT_MULTITHREADED` 或 `COINIT_APARTMENTTHREADED`），库会自动处理嵌套 COM 调用。

4. **管理员权限**：部分系统配置下，捕获进程音频可能需要以管理员权限运行。

5. **格式覆盖**：桌面音频监听模式（`DesktopCapture_Create`）下，传入的 `AudioCaptureFormat` 会被系统实际混音格式覆盖。可通过回调中的第二参数判断实际声道数（直通模式下 `data` 长度 = `num_frames × 实际声道数`）。

6. **进程退出**：使用 `ProcessCapture_Create` 捕获的进程退出后，音频流会自动停止，但句柄仍需要调用 `AudioCapture_Destroy` 销毁。

7. **多实例**：可以同时创建多个捕获实例（例如同时捕获桌面和单个进程），每个实例有独立的线程和回调。

8. **动态库运行时**：分发程序时，必须将 `audio_capture.dll` 与你的可执行文件放在同一目录，或将 DLL 所在路径加入系统 PATH。
