namespace Orquestador.App;

/// <summary>Paleta Escáner Autónomo (N-UI-01) vía ANSI — familia visual del proyecto principal.</summary>
internal static class UiTheme
{
    // #00B4FF cian · #6EB5FF meta · #00FF7A ok · #FF7070 alerta · #FFB347 ámbar · blanco/negro
    public const string Reset = "\x1b[0m";
    public const string Bold = "\x1b[1m";
    public const string Cyan = "\x1b[38;2;0;180;255m";
    public const string CyanDim = "\x1b[38;2;110;181;255m";
    public const string White = "\x1b[38;2;255;255;255m";
    public const string Green = "\x1b[38;2;0;255;122m";
    public const string Alert = "\x1b[38;2;255;112;112m";
    public const string Amber = "\x1b[38;2;255;179;71m";
    public const string Mute = "\x1b[38;2;80;88;104m";

    public static void Clear()
    {
        Console.Clear();
        Console.Write("\x1b[40m"); // fondo negro
    }

    public static void Banner(string title)
    {
        Console.WriteLine();
        Console.WriteLine($"{Cyan}{Bold}════════════════════════════════════════{Reset}");
        Console.WriteLine($"{Cyan}{Bold}  {title}{Reset}");
        Console.WriteLine($"{CyanDim}  DEMO / no dominio · Orquestador Labs v0{Reset}");
        Console.WriteLine($"{Cyan}{Bold}════════════════════════════════════════{Reset}");
        Console.WriteLine();
    }

    public static void Line(string text, string color = White) =>
        Console.WriteLine($"{color}{text}{Reset}");

    public static void Prompt(string text) =>
        Console.Write($"{Cyan}>{Reset} {White}{text}{Reset}");
}
