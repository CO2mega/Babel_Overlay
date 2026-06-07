namespace NetDesktop.Services.Audio;

/// <summary>
/// 音频重采样器。将输入的浮点音频转换为 16kHz 单声道格式，供 STT 管道使用。
/// 支持任意采样率和声道数，使用线性插值降采样。
/// </summary>
public class AudioResampler
{
    private const int TargetSampleRate = 16000;
    private int _callCount;
    
    /// <param name="input">输入浮点采样数据。</param>
    /// <param name="inputChannels">输入声道数（2=立体声自动混合为单声道）。</param>
    /// <param name="inputSampleRate">输入采样率。</param>
    /// <returns>16kHz 单声道浮点采样数组。</returns>
    public float[] Resample(float[] input, int inputChannels, int inputSampleRate)
    {
        float[] mono;
        if (inputChannels == 2)
        {
            int monoLength = input.Length / 2;
            mono = new float[monoLength];
            for (int i = 0; i < monoLength; i++)
                mono[i] = (input[i * 2] + input[i * 2 + 1]) * 0.5f;
        }
        else
        {
            mono = input;
        }

        float[] output;
        if (inputSampleRate == TargetSampleRate)
        {
            output = mono;
        }
        else
        {
            //线性插值降采样
            double ratio = (double)TargetSampleRate / inputSampleRate;
            int outputLength = (int)(mono.Length * ratio);
            output = new float[outputLength];
            for (int i = 0; i < outputLength; i++)
            {
                double srcPos = i / ratio;
                int srcIdx = (int)srcPos;
                double frac = srcPos - srcIdx;
                output[i] = (srcIdx + 1 < mono.Length)
                    ? (float)(mono[srcIdx] * (1 - frac) + mono[srcIdx + 1] * frac)
                    : srcIdx < mono.Length ? mono[srcIdx] : 0f;
            }
        }

        _callCount++;
        if (_callCount <= 3 || _callCount % 200 == 0)
            Console.WriteLine($"[Resample] #{_callCount}: {inputSampleRate}Hz {inputChannels}ch {input.Length}samples → 16kHz mono {output.Length}samples");

        return output;
    }
}

