#!/usr/bin/env python3
"""Draw OGG player in existing XMB settings right-side information pane."""
from pathlib import Path
p=Path("src/evo_ui.c")
s=p.read_text()
old='''    evoDrawSmallText(529, 92, 98, "CONTROLS", evoColCyan);
    evoDrawSmallText(529, 116, 98, "X / Left / Right", evoColWhite);
    evoDrawSmallText(529, 140, 98, "Change value", evoColMuted);
    evoDrawSmallText(529, 180, 98, "Square", evoColWhite);
    evoDrawSmallText(529, 204, 98, "Save changes", evoColMuted);
    evoDrawSmallText(529, 244, 98, "Circle", evoColWhite);
    evoDrawSmallText(529, 268, 98, "Discard / Back", evoColMuted);'''
new='''    if (evoFocus == EVO_FOCUS_CONTENT && selected == EVO_SETTING_BGM) {
        /* Reuse the established 529..627 sidebar; no new screen or overlay. */
        char song[32];
        evoSettingsGetValue(EVO_SETTING_BGM, song, sizeof(song));
        rmDrawRect(523, 80, 110, 232, evoColPanelStrong);
        evoDrawOutline(523, 80, 110, 232, evoColCyan);
        evoDrawSmallText(529, 94, 98, "MUSIC PLAYER", evoColCyan);
        evoDrawSmallText(529, 119, 98, "SDCEARA/MUSIC", evoColMuted);
        evoDrawSmallText(529, 148, 98, "NOW PLAYING", evoColCyan);
        evoDrawSmallText(529, 170, 98, song, evoColWhite);
        evoDrawSmallText(529, 197, 98,
                         evoMusicPlayerIsPaused() ? "STOPPED" :
                         evoMusicPlayerTrackCount() == 0 ? "NO OGG FOUND" :
                         isBgmPlaying() ? "PLAYING" : "READY", evoColGreen);
        evoDrawSmallText(529, 229, 98, "L1  <  |  >  R1", evoColWhite);
        evoDrawSmallText(529, 252, 98, "TRIANGLE", evoColCyan);
        evoDrawSmallText(529, 272, 98, "Stop / Restart", evoColMuted);
        evoDrawSmallText(529, 296, 98, "SQUARE Save", evoColMuted);
    } else {
        evoDrawSmallText(529, 92, 98, "CONTROLS", evoColCyan);
        evoDrawSmallText(529, 116, 98, "X / Left / Right", evoColWhite);
        evoDrawSmallText(529, 140, 98, "Change value", evoColMuted);
        evoDrawSmallText(529, 180, 98, "Square", evoColWhite);
        evoDrawSmallText(529, 204, 98, "Save changes", evoColMuted);
        evoDrawSmallText(529, 244, 98, "Circle", evoColWhite);
        evoDrawSmallText(529, 268, 98, "Discard / Back", evoColMuted);
    }'''
if s.count(old)!=1: raise SystemExit("Sidebar patch rejected: original controls panel not found exactly once")
s=s.replace(old,new)
old2='''    if (evoFocus == EVO_FOCUS_CONTENT && evoSettingsGetSelected() == EVO_SETTING_BGM) {
        evoDrawSmallText(174, 346, 340, "MUSIC PLAYER  L1 Previous  R1 Next  TRIANGLE Play/Pause", evoColCyan);
        evoDrawSmallText(174, 363, 340,
                         evoMusicPlayerIsPaused() ? "Paused (Play restarts track)" :
                         evoMusicPlayerTrackCount() ? "USB OGG playlist ready" : "No OGG files detected",
                         evoColWhite);
    }
'''
if s.count(old2)!=1: raise SystemExit("Old footer player instructions not found")
s=s.replace(old2,"")
p.write_text(s)
Path("sdceara-edition-v1.3.json").write_text('{"version":"1.3-test","player_location":"existing settings right information panel","selection":"Trilha sonora","buttons":"L1 previous, R1 next, Triangle stop/restart","game_loader_unchanged":true}\n')
print("v1.3: Music Player integrated into existing XMB right sidebar")
