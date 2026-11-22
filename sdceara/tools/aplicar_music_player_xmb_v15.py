#!/usr/bin/env python3
"""Show-only music sidebar in the actual SD Ceara XMB settings renderer.

No input bindings, audio playback, music selection, or game loader changes.
"""
from pathlib import Path
p = Path("src/evo_ui.c")
src = p.read_text()
start = src.find("static void evoXmbTechSettings(void)")
end = src.find("static void evoXmbTechMemory(void)", start)
if start < 0 or end < 0:
    raise SystemExit("XMB renderer not found: refusing to patch the fallback UI")
part = src[start:end]
anchor = "    evoXmbTechHelp(1);\n"
if part.count(anchor) != 1:
    raise SystemExit("Expected exactly one XMB settings help anchor")
panel = '''    /* SD Ceara music display in the REAL XMB technical settings panel.
     * Existing controls and playback are intentionally left untouched. */
    if (evoSettingsGetSelected() == EVO_SETTING_BGM) {
        char trackLabel[64];
        evoSettingsGetValue(EVO_SETTING_BGM, trackLabel, sizeof(trackLabel));
        rmDrawRect(421, 126, 205, 192,
                   GS_SETREG_RGBA(0x0B, 0x24, 0x21, 0x70));
        evoDrawOutline(421, 126, 205, 192, evoColCyan);
        evoXmbTxt(432, 147, 181, "MUSIC PLAYER V1.5", evoColCyan);
        evoXmbMini(432, 177, 181, "USB / SDCEARA / MUSIC", evoColMuted);
        evoXmbMini(432, 207, 181, "TRILHA SELECIONADA", evoColCyan);
        evoXmbTxt(432, 230, 181, trackLabel, evoColWhite);
        evoXmbMini(432, 269, 181,
                   isBgmPlaying() ? "REPRODUZINDO" : "PARADO", evoColWhite);
        evoXmbMini(432, 294, 181, "L1 ANTERIOR  R1 PROXIMA", evoColMuted);
    }
'''
part = part.replace(anchor, panel + anchor, 1)
src = src[:start] + part + src[end:]
p.write_text(src)
Path("sdceara-edition-v1.5.json").write_text('{"version":"1.5-test","scope":"XMB actual evoXmbTechSettings renderer only","marker":"MUSIC PLAYER V1.5","input_modified":false,"audio_modified":false,"game_loader_modified":false}\n')
print("v1.5 sidebar injected in actual evoXmbTechSettings renderer")
