using System.Text;

namespace NetDesktop.Services.Display;

public class LineBuffer
{
    private readonly int _maxLines;
    private readonly List<string> _conversation = new();
    private string[] _lastDisplay = [];
    private const double SimilarityThreshold = 0.6;
    private int _lockedConvPos;

    public event Action<string[]>? LinesChanged;

    public LineBuffer(int maxLines, int activeLines)
    {
        _maxLines = maxLines;
    }

    public void Update(string fullText)
    {
        if (string.IsNullOrWhiteSpace(fullText))
        {
            if (_lastDisplay.Any(l => l != ""))
            {
                _conversation.Clear();
                _lastDisplay = [];
                LinesChanged?.Invoke([]);
            }
            return;
        }

        var newLines = SplitLines(fullText);

        if (_conversation.Count == 0)
        {
            _conversation.AddRange(newLines);
            EmitDisplay();
            return;
        }

        int matchNewIdx = -1;
        int matchConvIdx = -1;

        for (int i = newLines.Length - 1; i >= 0; i--)
        {
            for (int j = _conversation.Count - 1; j >= 0; j--)
            {
                if (IsSimilar(newLines[i], _conversation[j]))
                {
                    matchNewIdx = i;
                    matchConvIdx = j;
                    break;
                }
            }
            if (matchNewIdx >= 0) break;
        }

        if (matchNewIdx >= 0)
        {
            int overlapStart = matchConvIdx - matchNewIdx;

            if (overlapStart < 0)
            {
                _conversation.InsertRange(0, Enumerable.Repeat("", -overlapStart));
                overlapStart = 0;
            }

            _lockedConvPos = overlapStart;

            int neededLen = overlapStart + newLines.Length;
            while (_conversation.Count < neededLen)
                _conversation.Add("");

            for (int i = 0; i < newLines.Length; i++)
                _conversation[overlapStart + i] = newLines[i];

            int removeFrom = overlapStart + newLines.Length;
            if (removeFrom < _conversation.Count)
                _conversation.RemoveRange(removeFrom, _conversation.Count - removeFrom);
        }
        else
        {
            _conversation.AddRange(newLines);
        }

        int maxHistory = _maxLines * 10;
        if (_conversation.Count > maxHistory)
        {
            int removed = _conversation.Count - maxHistory;
            _conversation.RemoveRange(0, removed);
            _lockedConvPos = Math.Max(0, _lockedConvPos - removed);
        }

        EmitDisplay();
    }

    private void EmitDisplay()
    {
        var display = new string[_maxLines];
        Array.Fill(display, "");

        int start = Math.Max(0, _conversation.Count - _maxLines);
        for (int i = 0; i < _maxLines && start + i < _conversation.Count; i++)
            display[i] = _conversation[start + i];

        bool changed = display.Length != _lastDisplay.Length;
        if (!changed)
        {
            for (int i = 0; i < display.Length; i++)
            {
                int convPos = start + i;
                if (convPos < _lockedConvPos)
                    continue;

                if (display[i] != _lastDisplay[i])
                {
                    changed = true;
                    break;
                }
            }
        }

        if (changed)
        {
            _lastDisplay = display;
            LinesChanged?.Invoke(display);
        }
    }

    private static bool IsSimilar(string a, string b)
    {
        if (a.Length == 0 || b.Length == 0) return false;
        if (a == b) return true;

        var freqA = new Dictionary<char, int>();
        var freqB = new Dictionary<char, int>();

        foreach (char c in a)
        {
            freqA.TryGetValue(c, out int n);
            freqA[c] = n + 1;
        }
        foreach (char c in b)
        {
            freqB.TryGetValue(c, out int n);
            freqB[c] = n + 1;
        }

        int common = 0;
        foreach (var kvp in freqA)
        {
            if (freqB.TryGetValue(kvp.Key, out int cb))
                common += Math.Min(kvp.Value, cb);
        }

        return 2.0 * common / (a.Length + b.Length) >= SimilarityThreshold;
    }

    private const string Delimiters = "，,。.!？！?；;：:、";

    private static string[] SplitLines(string text)
    {
        var result = new List<string>();
        var sb = new StringBuilder();

        foreach (char c in text)
        {
            sb.Append(c);
            if (Delimiters.Contains(c) || c == '\n')
            {
                var t = sb.ToString().Trim();
                sb.Clear();
                if (t.Length > 0)
                    result.Add(t);
            }
        }

        var last = sb.ToString().Trim();
        if (last.Length > 0)
            result.Add(last);

        if (result.Count > 1)
            return result.ToArray();

        return ForceWrap(text, 25);
    }

    private static string[] ForceWrap(string text, int maxLen)
    {
        if (text.Length <= maxLen)
            return [text];

        var lines = new List<string>();
        int pos = 0;
        while (pos < text.Length)
        {
            int end = Math.Min(pos + maxLen, text.Length);
            if (end < text.Length)
            {
                int space = text.LastIndexOf(' ', pos, end - pos);
                if (space > pos)
                    end = space + 1;
            }
            lines.Add(text[pos..end].Trim());
            pos = end;
        }
        return lines.ToArray();
    }
}
