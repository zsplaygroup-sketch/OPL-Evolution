#!/usr/bin/env python3
"""v1.6: music card replaces (never overlays) the existing right-side help card."""
from pathlib import Path
p=Path("src/evo_ui.c")
s=p.read_text()
start=s.index("static void evoXmbTechSettings(void)")
end=s.index("static void evoXmbTechMemory(void)",start)
part=s[start:end]
a=part.find("    /* SD Ceara music display in the REAL XMB technical settings panel.")
b=part.find("    evoXmbTechHelp(1);",a)
if a<0 or b<0: raise SystemExit("v1.5 sidebar patch not found in real XMB settings")
replacement='''    /* One existing right-hand card, with mutually exclusive contents.
     * Coordinates are INSIDE x=424..619; do not draw a second rectangle. */
    if (evoSettingsGetSelected() == EVO_SETTING_BGM) {
        char trackLabel[64];
        evoSettingsGetValue(EVO_SETTING_BGM, trackLabel, sizeof(trackLabel));
        evoXmbTxt(440, 130, 160, "MUSIC PLAYER", evoColWhite);
        rmDrawLine(441, 153, 600, 153,
                   GS_SETREG_RGBA(0x78, 0x92, 0x7B, 0x80));
        evoXmbMini(440, 174, 160, "SDCEARA / MUSIC", evoColMuted);
        evoXmbMini(440, 202, 160, "TRILHA SONORA", evoColCyan);
        evoXmbTxt(440, 226, 160, trackLabel, evoColWhite);
        evoXmbMini(440, 263, 160,
                   isBgmPlaying() ? "REPRODUZINDO" : "PARADO", evoColWhite);
        evoXmbMini(440, 295, 160, "L1 ANTERIOR", evoColMuted);
        evoXmbMini(440, 318, 160, "R1 PROXIMA", evoColMuted);
    } else {
        evoXmbTechHelp(1);
    }
'''
part=part[:a]+replacement+part[b+len("    evoXmbTechHelp(1);\n"):]
s=s[:start]+part+s[end:]
p.write_text(s)
Path("sdceara-edition-v1.6.json").write_text('{"version":"1.6-test","panel":"replaces right-hand help contents only when Trilha sonora selected","within_existing_bounds":true,"input_modified":false,"audio_modified":false,"game_loader_modified":false}\n')
print("v1.6: sidebar contents exclusive and constrained to original right card")
