#!/usr/bin/env python3
"""Corrige comparacao do cache de jogos ISO/ZSO sem alterar os leitores."""
from pathlib import Path
p=Path("src/supportbase.c")
s=p.read_text(encoding="utf-8")
old="strncmp(cache->games[i].name, game->gameinfo.name, ISO_GAME_NAME_MAX + 1) == 0 && strncmp(cache->games[i].extension, game->gameinfo.extension, ISO_GAME_EXTENSION_MAX + 1) == 0"
new="strncmp(cache->games[j].name, game->gameinfo.name, ISO_GAME_NAME_MAX + 1) == 0 && strncmp(cache->games[j].extension, game->gameinfo.extension, ISO_GAME_EXTENSION_MAX + 1) == 0"
if s.count(old)!=1: raise SystemExit("ERRO: comparacao de cache inesperada")
p.write_text(s.replace(old,new,1),encoding="utf-8")
print("Cache ISO/ZSO: indice de busca corrigido")
