using Orquestador.Core.Accounting;

namespace Orquestador.Core.Telemetry;

/// <summary>
/// Buzón bound + drop-oldest (Anexo telemetría Continuo del ENCARGO).
/// Consumo solo fuera del hot path del módulo.
/// </summary>
public sealed class BoundMailbox
{
    private readonly Queue<TelemetryRecord> _queue = new();
    private readonly object _gate = new();
    private readonly int _capacity;
    private ulong _seq;
    private ulong _pushed;
    private ulong _dropped;

    public BoundMailbox(int capacity = 64)
    {
        if (capacity < 2)
            throw new ArgumentOutOfRangeException(nameof(capacity), "capacidad mínima 2");
        _capacity = capacity;
    }

    public int Capacity => _capacity;

    public ulong Pushed
    {
        get { lock (_gate) return _pushed; }
    }

    public ulong Dropped
    {
        get { lock (_gate) return _dropped; }
    }

    public int Count
    {
        get { lock (_gate) return _queue.Count; }
    }

    public void PushDropOldest(string sessionId, int kind, long a, long b)
    {
        lock (_gate)
        {
            while (_queue.Count >= _capacity)
            {
                _queue.Dequeue();
                _dropped++;
            }

            _seq++;
            _queue.Enqueue(new TelemetryRecord(
                _seq,
                sessionId,
                DateTime.UtcNow.Ticks,
                kind,
                a,
                b));
            _pushed++;
        }
    }

    public bool TryPop(out TelemetryRecord record)
    {
        lock (_gate)
        {
            if (_queue.Count == 0)
            {
                record = null!;
                return false;
            }

            record = _queue.Dequeue();
            return true;
        }
    }
}
