using System.Diagnostics;

namespace Orquestador.Core.Epe;

/// <summary>
/// Integración EPE = [A DEFINIR] hasta pliego de uso/orquestación cerrado por Langosto.
/// Uso_EPE documenta arnés provisional (CLI/socket); NO adoptar como API de pintado de producto.
/// Este gate solo permite sondeo opcional de presencia/--self-test del BlindRuntime si el operador lo habilita.
/// </summary>
public static class EpeIntegrationGate
{
    public const string Status = "[A DEFINIR]";

    public static string PolicySummary =>
        "NO inventar API de pintado EPE. " +
        "VTable/socket BlindRuntime = [PROVISIONAL] arnés (Uso_EPE). " +
        "Pliego integración orquestador↔EPE: pendiente cierre Langosto. " +
        "Probe opcional: EPE_BLIND_RUNTIME_PATH + ORQ_ALLOW_EPE_HARNESS_PROBE=1 → solo --self-test.";

    public static bool HarnessProbeAllowed()
    {
        var flag = Environment.GetEnvironmentVariable("ORQ_ALLOW_EPE_HARNESS_PROBE");
        return string.Equals(flag, "1", StringComparison.Ordinal)
               || string.Equals(flag, "true", StringComparison.OrdinalIgnoreCase);
    }

    public static string? ResolveBlindRuntimePath()
    {
        var env = Environment.GetEnvironmentVariable("EPE_BLIND_RUNTIME_PATH");
        if (!string.IsNullOrWhiteSpace(env) && File.Exists(env))
            return env;
        return null;
    }

    /// <summary>
    /// Sondeo arnés. No es attach de producto ni contrato de pintado.
    /// </summary>
    public static (bool Ran, string Report) TryHarnessSelfTest(TimeSpan? timeout = null)
    {
        if (!HarnessProbeAllowed())
            return (false, "Probe deshabilitado (ORQ_ALLOW_EPE_HARNESS_PROBE≠1). Integración EPE " + Status);

        var path = ResolveBlindRuntimePath();
        if (path is null)
            return (false, "EPE_BLIND_RUNTIME_PATH no apunta a un ejecutable existente.");

        try
        {
            var psi = new ProcessStartInfo
            {
                FileName = path,
                Arguments = "--self-test",
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false,
                CreateNoWindow = true
            };
            using var p = Process.Start(psi);
            if (p is null) return (false, "No se pudo arrancar BlindRuntime.");
            var t = timeout ?? TimeSpan.FromSeconds(30);
            if (!p.WaitForExit((int)t.TotalMilliseconds))
            {
                try { p.Kill(entireProcessTree: true); } catch { /* ignore */ }
                return (false, "Timeout --self-test.");
            }
            var stdout = p.StandardOutput.ReadToEnd();
            var stderr = p.StandardError.ReadToEnd();
            return (true, $"exit={p.ExitCode}\n{stdout}\n{stderr}".Trim());
        }
        catch (Exception ex)
        {
            return (false, $"Error probe: {ex.Message}");
        }
    }
}
