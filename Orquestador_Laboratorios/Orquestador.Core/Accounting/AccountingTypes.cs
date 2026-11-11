namespace Orquestador.Core.Accounting;

/// <summary>
/// ABI de cuenta (attach/session/health/buzón). Stub v0 alineado a ENCARGO.
/// NO es contrato de pintado EPE. Integración EPE = [A DEFINIR] hasta pliego cerrado.
/// VTable C (mod_accounting_abi.h) permanece [PROVISIONAL] según Uso_EPE.
/// </summary>
public enum ModuleRegime : byte
{
    Unknown = 0,
    Continuo = 1,
    Discreto = 2
}

public sealed record ModIdentity(
    string Id,
    string DisplayName,
    string Version,
    ModuleRegime Regime);

public sealed record ModHealth(
    bool Ok,
    ulong Frames,
    ulong Presents,
    ulong TelemetryPushed,
    ulong TelemetryDropped,
    int LastError,
    string Detail);

public sealed record TelemetryRecord(
    ulong Seq,
    string SessionId,
    long TimestampUtcTicks,
    int Kind,
    long A,
    long B);

public interface IAccountingModule
{
    ModIdentity Identity { get; }
    int Attach(string sessionId);
    int Detach();
    int SessionStart();
    int SessionEnd();
    ModHealth Health();
    bool TelemetryTryPop(out TelemetryRecord record);
    /// <summary>Continuo: tick propio del módulo; el orquestador NO impone ritmo a todos.</summary>
    int ContinuousTick();
    /// <summary>Discreto: ráfaga request/response.</summary>
    int DiscreteBurst(string requestId);
}
