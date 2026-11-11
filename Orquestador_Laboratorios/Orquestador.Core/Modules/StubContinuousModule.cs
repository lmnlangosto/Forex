using Orquestador.Core.Accounting;
using Orquestador.Core.Telemetry;

namespace Orquestador.Core.Modules;

/// <summary>Módulo Continuo mentira: hilo propio + buzón bound drop-oldest.</summary>
public sealed class StubContinuousModule : IAccountingModule, IDisposable
{
    private readonly BoundMailbox _mailbox;
    private readonly object _state = new();
    private CancellationTokenSource? _cts;
    private Task? _loop;
    private bool _attached;
    private bool _session;
    private string _sessionId = "";
    private ulong _frames;
    private ulong _presents;
    private int _lastError;

    public StubContinuousModule(string id, string name, string version = "0.0.1-stub", int mailboxCapacity = 64)
    {
        Identity = new ModIdentity(id, name, version, ModuleRegime.Continuo);
        _mailbox = new BoundMailbox(mailboxCapacity);
    }

    public ModIdentity Identity { get; }

    public int Attach(string sessionId)
    {
        lock (_state)
        {
            if (_attached) return -1;
            _sessionId = sessionId;
            _attached = true;
            _lastError = 0;
            return 0;
        }
    }

    public int Detach()
    {
        SessionEnd();
        lock (_state)
        {
            if (!_attached) return -2;
            _attached = false;
            _sessionId = "";
            return 0;
        }
    }

    public int SessionStart()
    {
        lock (_state)
        {
            if (!_attached) return -3;
            if (_session) return -4;
            _session = true;
            _frames = 0;
            _presents = 0;
            _cts = new CancellationTokenSource();
            _loop = Task.Run(() => RunLoop(_cts.Token));
            return 0;
        }
    }

    public int SessionEnd()
    {
        CancellationTokenSource? cts;
        Task? loop;
        lock (_state)
        {
            if (!_session) return 0;
            _session = false;
            cts = _cts;
            loop = _loop;
            _cts = null;
            _loop = null;
        }

        try { cts?.Cancel(); } catch { /* ignore */ }
        try { loop?.Wait(TimeSpan.FromSeconds(2)); } catch { /* ignore */ }
        cts?.Dispose();
        return 0;
    }

    public ModHealth Health()
    {
        lock (_state)
        {
            return new ModHealth(
                Ok: _attached && _session && _lastError == 0,
                Frames: _frames,
                Presents: _presents,
                TelemetryPushed: _mailbox.Pushed,
                TelemetryDropped: _mailbox.Dropped,
                LastError: _lastError,
                Detail: _session ? "runtime-stub" : (_attached ? "attached" : "idle"));
        }
    }

    public bool TelemetryTryPop(out TelemetryRecord record) => _mailbox.TryPop(out record);

    public int ContinuousTick()
    {
        // El Continuo posee su bucle; tick externo es no-op en este stub.
        return _session ? 0 : -5;
    }

    public int DiscreteBurst(string requestId) => -10; // Continuo no implementa ráfaga

    private void RunLoop(CancellationToken ct)
    {
        while (!ct.IsCancellationRequested)
        {
            ulong frames;
            ulong presents;
            string sid;
            lock (_state)
            {
                if (!_session) break;
                _frames++;
                _presents++;
                frames = _frames;
                presents = _presents;
                sid = _sessionId;
            }

            // kind=1 frame sample (DEMO)
            _mailbox.PushDropOldest(sid, kind: 1, a: (long)frames, b: (long)presents);
            Thread.Sleep(50); // ~20 Hz stub; no dominio
        }
    }

    public void Dispose() => SessionEnd();
}
