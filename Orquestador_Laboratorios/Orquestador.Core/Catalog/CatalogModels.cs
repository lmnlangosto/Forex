using Orquestador.Core.Accounting;

namespace Orquestador.Core.Catalog;

public sealed record LabModuleInfo(
    string Id,
    string Name,
    string Role);

public sealed record LabInfo(
    string Id,
    string Name,
    string Summary,
    ModuleRegime PrimaryRegime,
    IReadOnlyList<LabModuleInfo> Modules);

public sealed record LabGroup(
    string Id,
    string Name,
    string Theme,
    IReadOnlyList<LabInfo> Labs);

public static class SampleCatalog
{
    /// <summary>Datos DEMO / no dominio. Placeholders hasta análisis por lab.</summary>
    public static IReadOnlyList<LabGroup> Create() =>
    [
        new LabGroup(
            "grp-motor",
            "Motor Gráfico",
            "Pintado / canal / embellecedores",
            [
                new LabInfo(
                    "lab-mg-epe",
                    "Lab Motor Gráfico (EPE)",
                    "Ensayo motor ciego + docs. Anfitrión ≠ Lab. DEMO.",
                    ModuleRegime.Continuo,
                    [
                        new LabModuleInfo("mod-epe-core", "EPE Core (observación)", "Continuo · health/buzón"),
                        new LabModuleInfo("mod-evidencias", "Registrador evidencias D", "Refs fuera hot path"),
                        new LabModuleInfo("mod-doc6-rack", "Rack Doc_6 (placeholder)", "Pendiente contrato flags")
                    ]),
                new LabInfo(
                    "lab-mg-canal",
                    "Lab Canal Doc_5 (stub)",
                    "Inspección canal — sin escribir MMF como pintor. DEMO.",
                    ModuleRegime.Continuo,
                    [
                        new LabModuleInfo("mod-canal-view", "Vista canal", "Solo lectura / N/A"),
                        new LabModuleInfo("mod-layout-check", "Layout version", "Fail-Closed layout")
                    ])
            ]),
        new LabGroup(
            "grp-decision",
            "Decisión (placeholders)",
            "Bayes / Fuzzy — sin dominio real en v0",
            [
                new LabInfo(
                    "lab-bayes",
                    "Lab Bayes (mentira)",
                    "Módulo Discreto stub. DEMO / no dominio.",
                    ModuleRegime.Discreto,
                    [
                        new LabModuleInfo("mod-bayes-burst", "Ráfaga Bayes", "Discreto"),
                        new LabModuleInfo("mod-bayes-hist", "Histórico ráfagas", "Registro B stub")
                    ]),
                new LabInfo(
                    "lab-fuzzy",
                    "Lab Fuzzy (mentira)",
                    "Módulo Discreto stub. DEMO / no dominio.",
                    ModuleRegime.Discreto,
                    [
                        new LabModuleInfo("mod-fuzzy-burst", "Ráfaga Fuzzy", "Discreto")
                    ])
            ]),
        new LabGroup(
            "grp-captura",
            "Captura / tiempos (placeholders)",
            "Fuera de MT5 en v0",
            [
                new LabInfo(
                    "lab-tiempos",
                    "Lab Tiempos (stub)",
                    "Placeholder. DEMO.",
                    ModuleRegime.Discreto,
                    [
                        new LabModuleInfo("mod-clock", "Reloj ensayo", "Discreto")
                    ])
            ])
    ];
}
