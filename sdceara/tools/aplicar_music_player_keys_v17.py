#!/usr/bin/env python3
"""Restore L1/R1 OGG track switching outside PS1/PS2 library, without other key changes."""
from pathlib import Path
h=Path("include/evo_settings.h")
hs=h.read_text()
anchor="void evoSettingsDiscard(void);"
assert hs.count(anchor)==1
hs=hs.replace(anchor,anchor+"\nvoid evoMusicPlayerStep(int direction);",1)
h.write_text(hs)
p=Path("src/evo_settings.c")
s=p.read_text()
anchor="int evoSettingsSave(void)\n{"
assert s.count(anchor)==1
func="""/* L1/R1: switch active OGG immediately without saving unrelated settings. */
void evoMusicPlayerStep(int direction)
{
    char selectedPath[sizeof(gDefaultBGMPath)];
    int total;
    if (evoMusicCount <= 0 || evoMusicRoot[0] == '\\0')
        return;
    total = evoMusicCount + 1;
    settings.musicTrack = (settings.musicTrack +
                           (direction < 0 ? total - 1 : 1)) % total;
    settings.bgm = settings.musicTrack != 0;
    selectedPath[0] = '\\0';
    if (settings.musicTrack > 0)
        snprintf(selectedPath, sizeof(selectedPath), "%s%s",
                 evoMusicRoot, evoMusicNames[settings.musicTrack - 1]);
    if (isBgmPlaying())
        bgmStop();
    snprintf(gDefaultBGMPath, sizeof(gDefaultBGMPath), "%s", selectedPath);
    gEnableBGM = settings.bgm;
    if (gEnableBGM)
        bgmStart();
}

"""
s=s.replace(anchor,func+anchor,1)
p.write_text(s)
p=Path("src/evo_ui.c")
s=p.read_text()
anchor="    if (getKeyOn(KEY_START)) {\n        evoSection = EVO_HOME;"
assert s.count(anchor)==1
handler="""    /* Keep the library's existing L1/R1 category navigation untouched.
     * Everywhere else, including the selected music card, switch OGG. */
    if (evoSection != EVO_LIBRARY &&
        (getKeyOn(KEY_L1) || getKeyOn(KEY_R1))) {
        evoMusicPlayerStep(getKeyOn(KEY_L1) ? -1 : 1);
        sfxPlay(SFX_CURSOR);
        return;
    }

"""
s=s.replace(anchor,handler+anchor,1)
p.write_text(s)
Path("sdceara-edition-v1.7.json").write_text('{"version":"1.7-test","scope":"L1 R1 music navigation outside library","player_visual":"v1.6 unchanged","triangle_modified":false,"square_modified":false,"confirm_modified":false}\n')
print("v1.7 L1/R1 music controls restored, library input unchanged")
