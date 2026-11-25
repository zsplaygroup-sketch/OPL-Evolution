#!/usr/bin/env python3
"""Teste ISO+ZSO: nao confundir entradas antigas de cache e validar formato real."""
from pathlib import Path
p=Path("src/supportbase.c")
s=p.read_text()
old='if (strcmp(filename, isoname) == 0) {'
new='if (strcasecmp(filename, isoname) == 0 &&\n            cache->games[i].format == GAME_FORMAT_ISO) {'
if s.count(old)!=1: raise SystemExit("cache lookup anchor mismatch")
s=s.replace(old,new,1)
old='    int size = strlen(name);\n    if (strcasecmp(&name[size - 4], ".iso") == 0 || strcasecmp(&name[size - 4], ".zso") == 0) {'
new='    int size = strlen(name);\n    if (size < 5) return 0;\n    if (strcasecmp(&name[size - 4], ".iso") == 0 || strcasecmp(&name[size - 4], ".zso") == 0) {'
if s.count(old)!=1: raise SystemExit("extension anchor mismatch")
s=s.replace(old,new,1)
p.write_text(s)
print("ISO/ZSO: cache case insensitive and extension guard")
