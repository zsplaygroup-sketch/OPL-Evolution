#!/usr/bin/env python3
"""Fit the SELECT diagnostics page to the v1.8 XMB dashboard."""
from pathlib import Path
import re

source = Path("src/evo_ui.c")
original = source.read_text(encoding="utf-8")
updated, count = re.subn(
    r"void evoUiRenderDiagnostics\(void\)\n\{.*?\n\}(?=\n\nvoid evoUiHandleInputDiagnostics)",
    lambda _: """void evoUiRenderDiagnostics(void)
{
    char text[96];
    int nativeWidth;
    int nativeHeight;

    rmDrawRect(0, 0, 640, 480, evoColBackground);
    thmLoadEvolutionDashboard();
    evoDrawTexture(EVO_DASHBOARD_BG, 0, 0, 640, 480, gDefaultCol);
    evoDrawTitle("DIAGNOSTICO");
#ifdef PADEMU
    evoPollControllerStatus();
#endif

    evoXmbTechTitle("DIAGNOSTICO");
    evoXmbMini(38, 153, 362, "SD CEARA GAMES | OPL Evolution v1.8", evoColWhite);
    snprintf(text, sizeof(text), "ROM: %s", evoRomVersion());
    evoXmbMini(38, 176, 362, text, evoColWhite);
    snprintf(text, sizeof(text), "Memoria do console: %d MiB",
             (int)(GetMemorySize() / (1024 * 1024)));
    evoXmbMini(38, 199, 362, text, evoColWhite);

    rmGetScreenExtentsNative(&nativeWidth, &nativeHeight);
    snprintf(text, sizeof(text), "Video: modo %d | %dx%d | H-sync %u kHz",
             gVMode, nativeWidth, nativeHeight, rmGetHsync());
    evoXmbMini(38, 222, 362, text, evoColWhite);
    snprintf(text, sizeof(text), "Rede: %s", evoNetworkStateName(gNetworkStartup));
    evoXmbMini(38, 245, 362, text, evoColWhite);
    evoControllerStatus(0, text, sizeof(text));
    evoXmbMini(38, 268, 75, "Controle 1", evoColCyan);
    evoXmbMini(122, 268, 278, text, evoColWhite);
    evoXmbMini(38, 291, 362, gEnableArt ? "Capas: ativadas" : "Capas: desativadas", evoColWhite);
    evoXmbMini(38, 322, 362, "PS1/ELF: sinal 240p depende", evoColMuted);
    evoXmbMini(38, 339, 362, "do adaptador HDMI.", evoColMuted);

    evoXmbTxt(440, 116, 170, "DISPOSITIVOS", evoColWhite);
    rmDrawLine(440, 140, 610, 140, evoColMuted);
    snprintf(text, sizeof(text), "BDM: %s", evoStartModeName(gBDMStartMode));
    evoXmbMini(440, 153, 170, text, evoColWhite);
    snprintf(text, sizeof(text), "HDD: %s", evoStartModeName(gHDDStartMode));
    evoXmbMini(440, 176, 170, text, evoColWhite);
    snprintf(text, sizeof(text), "SMB: %s", evoStartModeName(gETHStartMode));
    evoXmbMini(440, 199, 170, text, evoColWhite);
    snprintf(text, sizeof(text), "Apps: %s", evoStartModeName(gAPPStartMode));
    evoXmbMini(440, 222, 170, text, evoColWhite);
    evoXmbMini(440, 268, 170, gEnableMX4SIO ? "MX4SIO: ligado" : "MX4SIO: desligado", evoColWhite);
    evoXmbMini(440, 291, 170, gEnableILK ? "iLink: ligado" : "iLink: desligado", evoColWhite);

    /* Cover the dashboard tiles baked into the image on this read-only screen. */
    rmDrawRect(0, 380, 640, 100, evoColBackground);
    evoDrawStatusStrip(evoCancelButton() == KEY_CROSS ? "X: Voltar" : "Circulo: Voltar");
}""",
    original,
    flags=re.S,
)
if count != 1:
    raise SystemExit("Expected one v1.8 diagnostics renderer")
source.write_text(updated, encoding="utf-8")
