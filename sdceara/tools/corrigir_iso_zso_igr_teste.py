#!/usr/bin/env python3
"""Teste isolado: cache ISO/ZSO e caminho IGR na inicializacao do jogo."""
from pathlib import Path
p=Path("src/supportbase.c")
s=p.read_text(encoding="utf-8")
old="cache->games[i].name, game->gameinfo.name"
new="cache->games[j].name, game->gameinfo.name"
old2="cache->games[i].extension, game->gameinfo.extension"
new2="cache->games[j].extension, game->gameinfo.extension"
if s.count(old)!=1 or s.count(old2)!=1: raise SystemExit("Cache inesperado; nao modificar")
s=s.replace(old,new,1).replace(old2,new2,1)
p.write_text(s,encoding="utf-8")
p=Path("src/system.c")
s=p.read_text(encoding="utf-8")
old='    if (gExitPath[0] == \'\\0\')\n        strncpy(gExitPath, "Browser", sizeof(gExitPath));'
new='    /* SD Ceara FunTuna: sempre usar o ELF comprovadamente instalado. */\n    snprintf(gExitPath, sizeof(gExitPath), "%s", "mc0:/APPS/OPNPS2LD.ELF");'
if s.count(old)!=1: raise SystemExit("IGR inesperado; nao modificar")
p.write_text(s.replace(old,new,1),encoding="utf-8")
print("Patch de teste aplicado: cache ISO/ZSO e caminho IGR no launch")
