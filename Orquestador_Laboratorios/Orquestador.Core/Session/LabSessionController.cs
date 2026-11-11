using Orquestador.Core.Accounting;
using Orquestador.Core.Catalog;
using Orquestador.Core.Modules;

namespace Orquestador.Core.Session;

/// <summary>
/// Orquestador v0: UN laboratorio a la vez (attach/detach + health + buzón).
/// Continuo/Discreto lo gestiona el lab; el orquestador no impone régimen.
/// </summary>
public sealed class LabSessionController : IDisposable
{
    private readonly object _gate = new();
    private IAccountingModule? _active;
    private LabInfo? _activeLab;
    private string? _sessionId;
    private readonly List<string> _log = new();

    public LabInfo? ActiveLab
    {
        get { lock (_gate) return _activeLab; }
    }

    public string? SessionId
    {
        get { lock (_gate) return _sessionId; }
    }

    public bool HasActive
    {
        get { lock (_gate) return _active is not null; }
    }

    public IReadOnlyList<string> SnapshotLog()
    {
        lock (_gate) return _log.ToArray();
    }

    public (bool Ok, string Message) AttachLab(LabInfo lab)
    {
        lock (_gate)
        {
            if (_active is not null)
                return (false, $"Ya hay un lab activo: {_activeLab!.Id}. Detach primero. (norma: un lab a la vez)");

            var module = CreateStubFor(lab);
            var sid = $"S-{DateTime.UtcNow:yyyyMMddHHmmss}-{lab.Id}";
            var rc = module.Attach(sid);
            if (rc != 0)
                return (false, $"attach rc={rc}");

            rc = module.SessionStart();
            if (rc != 0)
            {
                module.Detach();
                (module as IDisposable)?.Dispose();
                return (false, $"session_start rc={rc}");
            }

            _active = module;
            _activeLab = lab;
            _sessionId = sid;
            Log($"ATTACH+SESSION {lab.Id} sid={sid} regime={module.Identity.Regime}");
            return (true, $"Activo: {lab.Name} ({sid})");
        }
    }

    public (bool Ok, string Message) DetachActive()
    {
        lock (_gate)
        {
            if (_active is null)
                return (false, "Ningún lab activo.");

            var id = _activeLab!.Id;
            _active.SessionEnd();
            _active.Detach();
            (_active as IDisposable)?.Dispose();
            _active = null;
            _activeLab = null;
            _sessionId = null;
            Log($"DETACH {id}");
            return (true, $"Detach OK: {id}");
        }
    }

    public ModHealth? Health()
    {
        lock (_gate) return _active?.Health();
    }

    public int DrainMailbox(int max, List<TelemetryRecord> into)
    {
        lock (_gate)
        {
            if (_active is null) return 0;
            var n = 0;
            while (n < max && _active.TelemetryTryPop(out var rec))
            {
                into.Add(rec);
                n++;
            }
            return n;
        }
    }

    public (bool Ok, string Message) DiscreteBurst(string requestId)
    {
        lock (_gate)
        {
            if (_active is null) return (false, "Sin lab activo.");
            if (_active.Identity.Regime != ModuleRegime.Discreto)
                return (false, "Lab activo no es Discreto.");
            var rc = _active.DiscreteBurst(requestId);
            Log($"BURST {requestId} rc={rc}");
            return rc == 0
                ? (true, $"Ráfaga OK rc=0")
                : (true, $"Ráfaga falló controlado rc={rc} (orquestador vivo)");
        }
    }

    private static IAccountingModule CreateStubFor(LabInfo lab) =>
        lab.PrimaryRegime switch
        {
            ModuleRegime.Continuo => new StubContinuousModule(lab.Id, lab.Name),
            ModuleRegime.Discreto => new StubDiscreteModule(lab.Id, lab.Name),
            _ => new StubDiscreteModule(lab.Id, lab.Name)
        };

    private void Log(string line) => _log.Add($"{DateTime.Now:HH:mm:ss} {line}");

    public void Dispose() => DetachActive();
}
