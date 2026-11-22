#!/usr/bin/env python3
"""Show music player sidebar whenever Trilha sonora is highlighted, regardless of focus."""
from pathlib import Path
p=Path("src/evo_ui.c")
s=p.read_text()
old="if (evoFocus == EVO_FOCUS_CONTENT && selected == EVO_SETTING_BGM) {"
new="if (selected == EVO_SETTING_BGM) {"
if s.count(old)!=1: raise SystemExit("Expected exactly one music sidebar focus condition")
s=s.replace(old,new,1)
old2='evoDrawSmallText(529, 94, 98, "MUSIC PLAYER", evoColCyan);'
new2='evoDrawSmallText(529, 94, 98, "PLAYER V1.4", evoColCyan);'
if s.count(old2)!=1: raise SystemExit("Expected exactly one music player sidebar header")
s=s.replace(old2,new2,1)
p.write_text(s)
Path("sdceara-edition-v1.4.json").write_text('{"version":"1.4-test","sidebar":"visible whenever Trilha sonora selected, independent of focus","visual_marker":"PLAYER V1.4","game_loader_unchanged":true}\n')
print("v1.4: music player sidebar focus condition removed and version marker added")
