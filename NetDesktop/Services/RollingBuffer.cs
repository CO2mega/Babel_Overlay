using System.Text;

namespace NetDesktop.Services;

public class RollingBuffer
{
    private readonly StringBuilder _committed = new();
    private string _partial = string.Empty;
    private readonly int _maxLength;

    public event Action<string>? ContentChanged;

    public string FullContent
    {
        get
        {
            if (string.IsNullOrEmpty(_partial))
                return _committed.ToString();
            return _committed.ToString() + _partial;
        }
    }

    public RollingBuffer(int maxLength = 300)
    {
        _maxLength = maxLength;
    }

    public void UpdatePartial(string text)
    {
        if (_partial == text) return;
        _partial = text;
        ContentChanged?.Invoke(FullContent);
    }

    public void CommitPartial()
    {
        if (string.IsNullOrWhiteSpace(_partial)) return;
        _committed.Append(_partial);
        Trim();
        _partial = string.Empty;
        ContentChanged?.Invoke(FullContent);
    }

    public void Replace(string text)
    {
        _committed.Clear();
        _committed.Append(text);
        Trim();
        _partial = string.Empty;
        ContentChanged?.Invoke(FullContent);
    }

    public void Clear()
    {
        _committed.Clear();
        _partial = string.Empty;
        ContentChanged?.Invoke(FullContent);
    }

    private void Trim()
    {
        if (_committed.Length > _maxLength)
        {
            var target = (int)(_maxLength * 0.6);
            _committed.Remove(0, _committed.Length - target);
        }
    }
}
