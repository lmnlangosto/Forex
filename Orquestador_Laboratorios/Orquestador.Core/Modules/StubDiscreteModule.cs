using Orquestador.Core.Accounting;
using Orquestador.Core.Telemetry;

namespace Orquestador.Core.Modules;

/// <summary>Módulo Discreto mentira: ráfagas; una ráfaga puede fallar sin matar el orquestador.</summary>
public sealed class StubDiscreteModule : IAccountingModule
{
    private readonly BoundMailbox _mailbox = new(32);
    private readonly object _state = new();
    private bool _attached;
    private bool _session;
    private string _sessionId = "";
    private ulong _bursts;
    private ulong _failed;
    private int _lastError;
    private int _burstCounter;

    public StubDiscreteModule(string id, string name, string version = "0.0.1-stub")
    {
        Identity = new ModIdentity(id, name, version, ModuleRegime.Discreto);
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
            return 0;
        }
    }

    public int SessionEnd()
    {
        lock (_state)
        {
            _session = false;
            return 0;
        }
    }

    public ModHealth Health()
    {
        lock (_state)
        {
            return new ModHealth(
                Ok: _attached && _lastError == 0,
                Frames: _bursts,
                Presents: _failed,
                TelemetryPushed: _mailbox.Pushed,
                TelemetryDropped: _mailbox.Dropped,
                LastError: _lastError,
                Detail: $"bursts={_bursts} failed={_failed}");
        }
    }

    public bool TelemetryTryPop(out TelemetryRecord record) => _mailbox.TryPop(out record);

    public int ContinuousTick() => -11;

    public int DiscreteBurst(string requestId)
    {
        lock (_state)
        {
            if (!_attached || !_session) return -5;
            _burstCounter++;
            _bursts++;
            // Cada 3ª ráfaga falla de forma controlada (criterio aceptación ENCARGO §10.5)
            if (_burstCounter % 3 == 0)
            {
                _failed++;
                _lastError = 42;
                _mailbox.PushDropOldest(_sessionId, kind: 3, a: 42, b: _burstCounter);
                return -42;
            }

            _lastError = 0;
            _mailbox.PushDropOldest(_sessionId, kind: 4, a: _burstCounter, b: requestId.GetHashCode());
            return 0;
        }
    }
}
