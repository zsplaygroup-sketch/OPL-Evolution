#!/usr/bin/env python3
"""Define o caminho IGR padrao sem alterar a combinacao de botoes."""
from pathlib import Path

p = Path("src/opl.c")
s = p.read_text(encoding="utf-8")
default = 'mc0:/APPS/OPNPS2LD.ELF'

old_init = "    gExitPath[0] = '\\0';"
new_init = '    snprintf(gExitPath, sizeof(gExitPath), "%s", "' + default + '");'
if s.count(old_init) != 1:
    raise SystemExit("ERRO: inicializacao de gExitPath inesperada")
s = s.replace(old_init, new_init, 1)

old_load = "configGetStrCopy(configOPL, CONFIG_OPL_EXIT_PATH, gExitPath, sizeof(gExitPath));"
if s.count(old_load) != 2:
    raise SystemExit("ERRO: leituras de ExitPath inesperadas")
new_load = old_load + """
            if (gExitPath[0] == '\\0')
                snprintf(gExitPath, sizeof(gExitPath), "%s", "mc0:/APPS/OPNPS2LD.ELF");"""
s = s.replace(old_load, new_load)
p.write_text(s, encoding="utf-8")
print("IGR padrao configurado para " + default + "; combinacao original preservada")
