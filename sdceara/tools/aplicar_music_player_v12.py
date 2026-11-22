#!/usr/bin/env python3
"""Apply SD Ceara Music Player controls after the v1.0 OGG selector patch."""
from pathlib import Path

def patch(path, before, after):
    p=Path(path)
    s=p.read_text()
    n=s.count(before)
    if n!=1: raise SystemExit(f"Music Player patch rejected {path}: anchor count {n}")
    p.write_text(s.replace(before,after,1))

header="include/evo_settings.h"
patch(header,"void evoSettingsDiscard(void);","""void evoSettingsDiscard(void);
/* Player controls act on the existing USB OGG selector and OPL Vorbis BGM. */
void evoMusicPlayerStep(int direction);
void evoMusicPlayerPauseToggle(void);
int evoMusicPlayerIsPaused(void);
int evoMusicPlayerTrackCount(void);
""")
settings="src/evo_settings.c"
patch(settings,"static int evoMusicCount;","static int evoMusicCount;\nstatic int evoMusicPaused;")
patch(settings,"    evoMusicScan();\n    settings.musicTrack = evoMusicSavedIndex();","    evoMusicScan();\n    evoMusicPaused = 0;\n    settings.musicTrack = evoMusicSavedIndex();")
patch(settings,"int evoSettingsSave(void)\n{","""/* Immediate controls: no modal dialogs, no changes to PS2 game backends. */
int evoMusicPlayerTrackCount(void) { return evoMusicCount; }
int evoMusicPlayerIsPaused(void) { return evoMusicPaused; }

void evoMusicPlayerStep(int direction)
{
    char selectedPath[sizeof(gDefaultBGMPath)];
    int total;
    if (evoMusicCount == 0) return;
    total = evoMusicCount + 1;
    settings.musicTrack = (settings.musicTrack + (direction < 0 ? total - 1 : 1)) % total;
    settings.bgm = settings.musicTrack != 0;
    if (isBgmPlaying()) bgmStop();
    selectedPath[0] = '\\0';
    if (settings.musicTrack > 0)
        snprintf(selectedPath, sizeof(selectedPath), "%s%s", evoMusicRoot,
                 evoMusicNames[settings.musicTrack - 1]);
    snprintf(gDefaultBGMPath, sizeof(gDefaultBGMPath), "%s", selectedPath);
    gEnableBGM = settings.bgm;
    evoMusicPaused = 0;
    if (gEnableBGM) bgmStart();
}

void evoMusicPlayerPauseToggle(void)
{
    if (settings.musicTrack <= 0 || evoMusicCount == 0) return;
    if (isBgmPlaying()) {
        bgmStop();
        evoMusicPaused = 1;
    } else {
        /* Resume by restarting the selected track: Vorbis BGM has no seek/resume API. */
        gEnableBGM = 1;
        bgmStart();
        evoMusicPaused = 0;
    }
}

int evoSettingsSave(void)
{""")
patch(settings,"    if (gEnableBGM && !isBgmPlaying())\n        bgmStart();","    if (gEnableBGM && !isBgmPlaying() && !evoMusicPaused)\n        bgmStart();")
ui="src/evo_ui.c"
patch(ui,"        } else if (getKeyOn(KEY_SQUARE)) {\n            const int saved = evoSettingsSave();","""        } else if (getKeyOn(KEY_L1) && evoSettingsGetSelected() == EVO_SETTING_BGM) {
            evoMusicPlayerStep(-1);
            sfxPlay(SFX_CURSOR);
        } else if (getKeyOn(KEY_R1) && evoSettingsGetSelected() == EVO_SETTING_BGM) {
            evoMusicPlayerStep(1);
            sfxPlay(SFX_CURSOR);
        } else if (getKeyOn(KEY_TRIANGLE) && evoSettingsGetSelected() == EVO_SETTING_BGM) {
            evoMusicPlayerPauseToggle();
            sfxPlay(SFX_CONFIRM);
        } else if (getKeyOn(KEY_SQUARE)) {
            const int saved = evoSettingsSave();""")
patch(ui,'    evoDrawText(174, 62, "SETTINGS", evoColCyan);','''    evoDrawText(174, 62, "SETTINGS", evoColCyan);
    if (evoFocus == EVO_FOCUS_CONTENT && evoSettingsGetSelected() == EVO_SETTING_BGM) {
        evoDrawSmallText(174, 346, 340, "MUSIC PLAYER  L1 Previous  R1 Next  TRIANGLE Play/Pause", evoColCyan);
        evoDrawSmallText(174, 363, 340,
                         evoMusicPlayerIsPaused() ? "Paused (Play restarts track)" :
                         evoMusicPlayerTrackCount() ? "USB OGG playlist ready" : "No OGG files detected",
                         evoColWhite);
    }''')
Path("sdceara-edition-v1.2.json").write_text('{"version":"1.2-test","controls":"L1 previous, R1 next, Triangle stop/restart","location":"Settings > Trilha sonora","pause_limitation":"restarts selected OGG from beginning","base":"v1.0 USB OGG","game_loader_unchanged":true}\n')
print("Music Player v1.2 patch applied")
