using Orquestador.App;
using Orquestador.Core.Accounting;
using Orquestador.Core.Catalog;
using Orquestador.Core.Epe;
using Orquestador.Core.Session;

var groups = SampleCatalog.Create();
using var session = new LabSessionController();

if (args.Contains("--smoke", StringComparer.OrdinalIgnoreCase))
{
    return RunSmoke(groups, session);
}

while (true)
{
    UiTheme.Clear();
    UiTheme.Banner("ORQUESTADOR DE LABORATORIOS");
    UiTheme.Line($"EPE integración: {EpeIntegrationGate.Status}", UiTheme.Amber);
    UiTheme.Line(EpeIntegrationGate.PolicySummary, UiTheme.Mute);
    Console.WriteLine();

    if (session.HasActive)
    {
        var h = session.Health();
        UiTheme.Line(
            $"ACTIVO: {session.ActiveLab!.Name} · sid={session.SessionId} · health={(h?.Ok == true ? "OK" : "NOK")}",
            h?.Ok == true ? UiTheme.Green : UiTheme.Alert);
    }
    else
    {
        UiTheme.Line("ACTIVO: (ninguno — un lab a la vez)", UiTheme.Mute);
    }

    Console.WriteLine();
    UiTheme.Line("Grupos temáticos:", UiTheme.Cyan);
    for (var i = 0; i < groups.Count; i++)
        UiTheme.Line($"  [{i + 1}] {groups[i].Name}  —  {groups[i].Theme}  ({groups[i].Labs.Count} labs)", UiTheme.White);

    Console.WriteLine();
    UiTheme.Line("  [A] Health del lab activo", UiTheme.CyanDim);
    UiTheme.Line("  [B] Drenar buzón (stub)", UiTheme.CyanDim);
    UiTheme.Line("  [D] Detach lab activo", UiTheme.CyanDim);
    UiTheme.Line("  [E] Probe EPE BlindRuntime --self-test (opcional)", UiTheme.CyanDim);
    UiTheme.Line("  [L] Log sesión", UiTheme.CyanDim);
    UiTheme.Line("  [Q] Salir", UiTheme.CyanDim);
    Console.WriteLine();
    UiTheme.Prompt("grupo # o comando: ");
    var input = (Console.ReadLine() ?? "").Trim();

    if (input.Equals("Q", StringComparison.OrdinalIgnoreCase)) break;
    if (input.Equals("A", StringComparison.OrdinalIgnoreCase)) { ShowHealth(session); continue; }
    if (input.Equals("B", StringComparison.OrdinalIgnoreCase)) { DrainMailbox(session); continue; }
    if (input.Equals("D", StringComparison.OrdinalIgnoreCase)) { Detach(session); continue; }
    if (input.Equals("E", StringComparison.OrdinalIgnoreCase)) { ProbeEpe(); continue; }
    if (input.Equals("L", StringComparison.OrdinalIgnoreCase)) { ShowLog(session); continue; }

    if (!int.TryParse(input, out var gIdx) || gIdx < 1 || gIdx > groups.Count)
    {
        Pause("Entrada no válida.");
        continue;
    }

    ShowGroup(groups[gIdx - 1], session);
}

session.DetachActive();
return 0;

static void ShowGroup(LabGroup group, LabSessionController session)
{
    while (true)
    {
        UiTheme.Clear();
        UiTheme.Banner($"GRUPO · {group.Name}");
        UiTheme.Line(group.Theme, UiTheme.CyanDim);
        Console.WriteLine();

        for (var i = 0; i < group.Labs.Count; i++)
        {
            var lab = group.Labs[i];
            var mark = session.ActiveLab?.Id == lab.Id ? "★" : " ";
            UiTheme.Line(
                $"  {mark}[{i + 1}] {lab.Name}  [{lab.PrimaryRegime}]  · {lab.Modules.Count} módulos",
                UiTheme.White);
            UiTheme.Line($"      {lab.Summary}", UiTheme.Mute);
        }

        Console.WriteLine();
        UiTheme.Line("  [0] Volver al catálogo", UiTheme.CyanDim);
        UiTheme.Prompt("lab #: ");
        var input = (Console.ReadLine() ?? "").Trim();
        if (input == "0" || input.Equals("B", StringComparison.OrdinalIgnoreCase)) return;
        if (!int.TryParse(input, out var lIdx) || lIdx < 1 || lIdx > group.Labs.Count)
        {
            Pause("Entrada no válida.");
            continue;
        }

        ShowLabFocus(group.Labs[lIdx - 1], session);
    }
}

static void ShowLabFocus(LabInfo lab, LabSessionController session)
{
    while (true)
    {
        UiTheme.Clear();
        UiTheme.Banner($"LAB · {lab.Name}");
        UiTheme.Line($"Id: {lab.Id} · Régimen primario: {lab.PrimaryRegime}", UiTheme.CyanDim);
        UiTheme.Line(lab.Summary, UiTheme.White);
        Console.WriteLine();
        UiTheme.Line("Módulos del laboratorio (clic conceptual → foco completo):", UiTheme.Cyan);
        foreach (var m in lab.Modules)
            UiTheme.Line($"  · [{m.Id}] {m.Name}  —  {m.Role}", UiTheme.White);

        Console.WriteLine();
        var isActive = session.ActiveLab?.Id == lab.Id;
        if (isActive)
        {
            var h = session.Health();
            UiTheme.Line($"SESIÓN ACTIVA sid={session.SessionId}", UiTheme.Green);
            if (h is not null)
                UiTheme.Line(
                    $"health ok={h.Ok} frames={h.Frames} presents={h.Presents} pushed={h.TelemetryPushed} dropped={h.TelemetryDropped} err={h.LastError} · {h.Detail}",
                    h.Ok ? UiTheme.Green : UiTheme.Alert);
        }
        else if (session.HasActive)
        {
            UiTheme.Line($"Otro lab activo ({session.ActiveLab!.Id}). Detach antes de attach.", UiTheme.Amber);
        }

        Console.WriteLine();
        UiTheme.Line("  [1] Attach este lab (único)", UiTheme.CyanDim);
        UiTheme.Line("  [2] Detach si es el activo", UiTheme.CyanDim);
        UiTheme.Line("  [3] Health", UiTheme.CyanDim);
        UiTheme.Line("  [4] Drenar buzón", UiTheme.CyanDim);
        if (lab.PrimaryRegime == ModuleRegime.Discreto)
            UiTheme.Line("  [5] Ráfaga Discreta (stub)", UiTheme.CyanDim);
        UiTheme.Line("  [0] Volver al grupo", UiTheme.CyanDim);
        UiTheme.Prompt("acción: ");
        var input = (Console.ReadLine() ?? "").Trim();

        switch (input)
        {
            case "0":
                return;
            case "1":
            {
                var (ok, msg) = session.AttachLab(lab);
                Pause(msg, ok);
                break;
            }
            case "2":
            {
                if (!isActive) { Pause("Este lab no está activo."); break; }
                var (ok, msg) = session.DetachActive();
                Pause(msg, ok);
                break;
            }
            case "3":
                ShowHealth(session);
                break;
            case "4":
                DrainMailbox(session);
                break;
            case "5" when lab.PrimaryRegime == ModuleRegime.Discreto:
            {
                if (!isActive) { Pause("Attach primero."); break; }
                var (ok, msg) = session.DiscreteBurst($"R-{DateTime.UtcNow:HHmmss}");
                Pause(msg, ok);
                break;
            }
            default:
                Pause("Entrada no válida.");
                break;
        }
    }
}

static void ShowHealth(LabSessionController session)
{
    UiTheme.Clear();
    UiTheme.Banner("HEALTH");
    var h = session.Health();
    if (h is null)
        UiTheme.Line("Sin lab activo.", UiTheme.Amber);
    else
    {
        UiTheme.Line($"Lab: {session.ActiveLab!.Name}", UiTheme.White);
        UiTheme.Line($"ok={h.Ok}", h.Ok ? UiTheme.Green : UiTheme.Alert);
        UiTheme.Line($"frames={h.Frames} presents={h.Presents}", UiTheme.White);
        UiTheme.Line($"telemetry pushed={h.TelemetryPushed} dropped={h.TelemetryDropped}", UiTheme.White);
        UiTheme.Line($"last_error={h.LastError} detail={h.Detail}", UiTheme.CyanDim);
    }
    Pause();
}

static void DrainMailbox(LabSessionController session)
{
    UiTheme.Clear();
    UiTheme.Banner("BUZÓN (drain baja prioridad)");
    var bag = new List<TelemetryRecord>();
    var n = session.DrainMailbox(32, bag);
    if (n == 0)
        UiTheme.Line("Vacío o sin lab activo.", UiTheme.Mute);
    else
    {
        foreach (var r in bag)
            UiTheme.Line($"  seq={r.Seq} kind={r.Kind} a={r.A} b={r.B} sid={r.SessionId}", UiTheme.White);
        UiTheme.Line($"Drenados: {n}", UiTheme.Green);
    }
    Pause();
}

static void Detach(LabSessionController session)
{
    var (ok, msg) = session.DetachActive();
    Pause(msg, ok);
}

static void ProbeEpe()
{
    UiTheme.Clear();
    UiTheme.Banner("EPE · probe arnés (NO API pintado)");
    UiTheme.Line(EpeIntegrationGate.PolicySummary, UiTheme.Mute);
    Console.WriteLine();
    var (ran, report) = EpeIntegrationGate.TryHarnessSelfTest();
    UiTheme.Line(ran ? "Probe ejecutado:" : "Probe no ejecutado:", ran ? UiTheme.Green : UiTheme.Amber);
    UiTheme.Line(report, UiTheme.White);
    Pause();
}

static void ShowLog(LabSessionController session)
{
    UiTheme.Clear();
    UiTheme.Banner("LOG SESIÓN");
    foreach (var line in session.SnapshotLog())
        UiTheme.Line(line, UiTheme.White);
    if (session.SnapshotLog().Count == 0)
        UiTheme.Line("(vacío)", UiTheme.Mute);
    Pause();
}

static void Pause(string? msg = null, bool ok = true)
{
    if (!string.IsNullOrWhiteSpace(msg))
        UiTheme.Line(msg, ok ? UiTheme.Green : UiTheme.Alert);
    Console.WriteLine();
    UiTheme.Prompt("Enter…");
    Console.ReadLine();
}

static int RunSmoke(IReadOnlyList<LabGroup> groups, LabSessionController session)
{
    Console.WriteLine("SMOKE Orquestador v0");
    Console.WriteLine($"grupos={groups.Count} epe={EpeIntegrationGate.Status}");
    var continuo = groups.SelectMany(g => g.Labs).First(l => l.PrimaryRegime == ModuleRegime.Continuo);
    var discreto = groups.SelectMany(g => g.Labs).First(l => l.PrimaryRegime == ModuleRegime.Discreto);

    var (ok1, msg1) = session.AttachLab(continuo);
    Console.WriteLine($"attach continuo: ok={ok1} {msg1}");
    if (!ok1) return 1;
    Thread.Sleep(200);
    var h = session.Health()!;
    Console.WriteLine($"health: ok={h.Ok} frames={h.Frames} pushed={h.TelemetryPushed} dropped={h.TelemetryDropped}");
    var bag = new List<TelemetryRecord>();
    var n = session.DrainMailbox(16, bag);
    Console.WriteLine($"buzon drain={n}");

    var (okBlock, msgBlock) = session.AttachLab(discreto);
    Console.WriteLine($"attach 2do (debe fallar): ok={okBlock} {msgBlock}");
    if (okBlock) return 2;

    session.DetachActive();
    var (ok2, msg2) = session.AttachLab(discreto);
    Console.WriteLine($"attach discreto: ok={ok2} {msg2}");
    if (!ok2) return 3;
    var burst = session.DiscreteBurst("smoke-1");
    Console.WriteLine($"burst1: {burst.Message}");
    burst = session.DiscreteBurst("smoke-2");
    Console.WriteLine($"burst2: {burst.Message}");
    burst = session.DiscreteBurst("smoke-3");
    Console.WriteLine($"burst3 (falla controlada esperada): {burst.Message}");
    session.DetachActive();
    Console.WriteLine("SMOKE PASS");
    return 0;
}
