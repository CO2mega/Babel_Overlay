#pragma once

#include <windows.h>
#include <vector>
#include <algorithm>

class AudioConverter {
public:
    // 声道配置
    enum ChannelLayout {
        MONO = 1,
        STEREO = 2,
        STEREO_51 = 6,
        STEREO_71 = 8,
    };

    // 将多声道音频转换为单声道（求平均）
    // input: 输入音频数据，格式为 [L1, R1, L2, R2, ...]
    // num_frames: 样本帧数（每帧多个声道）
    // input_channels: 输入声道数
    // output: 输出单声道数据
    static void StereoToMono(const float* input, UINT32 num_frames, 
                            UINT16 input_channels, std::vector<float>& output)
    {
        output.clear();
        output.reserve(num_frames);

        for (UINT32 i = 0; i < num_frames; i++) {
            float sample = 0.0f;
            
            // 平均所有声道
            for (UINT16 ch = 0; ch < input_channels; ch++) {
                sample += input[i * input_channels + ch];
            }
            
            output.push_back(sample / input_channels);
        }
    }

    // 多声道转单声道（带权重）
    // 更逼真的混音效果（使用标准权重）
    static void StereoToMonoWeighted(const float* input, UINT32 num_frames,
                                   UINT16 input_channels, std::vector<float>& output)
    {
        output.clear();
        output.reserve(num_frames);

        // ITU-R BS.775 权重（6.1 声道系统）
        const float weights[] = {0.7f, 0.7f, 1.0f, 1.0f, 0.5f, 0.5f, 0.5f, 0.5f};
        float total_weight = 0.0f;

        for (UINT16 i = 0; i < input_channels && i < 8; i++) {
            total_weight += weights[i];
        }

        for (UINT32 i = 0; i < num_frames; i++) {
            float sample = 0.0f;

            for (UINT16 ch = 0; ch < input_channels && ch < 8; ch++) {
                sample += input[i * input_channels + ch] * weights[ch];
            }

            output.push_back(sample / total_weight);
        }
    }

    // 立体声转单声道（简单方法：左 + 右 / 2）
    // 最简洁的双声道合并
    static void StereoToMonoSimple(const float* input, UINT32 num_frames,
                                  std::vector<float>& output)
    {
        output.clear();
        output.reserve(num_frames);

        for (UINT32 i = 0; i < num_frames; i++) {
            float left = input[i * 2];
            float right = input[i * 2 + 1];
            output.push_back((left + right) * 0.5f);
        }
    }

    // 提取单个声道
    static void ExtractChannel(const float* input, UINT32 num_frames,
                              UINT16 input_channels, UINT16 channel_index,
                              std::vector<float>& output)
    {
        output.clear();
        output.reserve(num_frames);

        if (channel_index >= input_channels)
            return;

        for (UINT32 i = 0; i < num_frames; i++) {
            output.push_back(input[i * input_channels + channel_index]);
        }
    }

    // 获取音量RMS（用于监控音量）
    static float GetRMS(const float* data, UINT32 num_samples)
    {
        if (num_samples == 0) return 0.0f;

        float sum = 0.0f;
        for (UINT32 i = 0; i < num_samples; i++) {
            sum += data[i] * data[i];
        }

        return sqrt(sum / num_samples);
    }

    // 获取峰值
    static float GetPeak(const float* data, UINT32 num_samples)
    {
        if (num_samples == 0) return 0.0f;

        float peak = 0.0f;
        for (UINT32 i = 0; i < num_samples; i++) {
            float abs_val = abs(data[i]);
            if (abs_val > peak)
                peak = abs_val;
        }

        return peak;
    }
};