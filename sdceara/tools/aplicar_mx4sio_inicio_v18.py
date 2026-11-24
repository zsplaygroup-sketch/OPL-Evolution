#!/usr/bin/env python3
"""Patch minimal para OPL Evolution XMB v1.8: controle MX4SIO na aba Inicio.
Uso: python3 aplicar_mx4sio_inicio_v18.py .
Executar apos as personalizacoes existentes e antes de 'make evolution'.
"""
from pathlib import Path
import sys
root = Path(sys.argv[1] if len(sys.argv)>1 else '.')
p = root / 'src/evo_ui.c'
s = p.read_text(encoding='utf-8')
if 'MX4SIO: %s' in s and 'evoMx4sioNotice' in s:
    print('Patch MX4SIO ja aplicado.'); sys.exit(0)

def replace_once(old, new, label):
    global s
    n=s.count(old)
    if n!=1: raise RuntimeError(f'{label}: esperado 1 trecho, encontrado {n}. Nenhuma gravacao realizada.')
    s=s.replace(old,new,1)

replace_once('static char evoSettingsNotice[96];',
'''static char evoSettingsNotice[96];
static char evoMx4sioNotice[96];''','estado do aviso')
replace_once('    evoDrawOutline(408, 264, 219, 58, evoColBlue);\n    evoDrawSmallText(417, 274, 201, "Open Library for every platform.", evoColMuted);',
'''    evoDrawOutline(408, 264, 219, 74, evoColBlue);
    snprintf(countText, sizeof(countText), "MX4SIO: %s", gEnableMX4SIO ? "ON" : "OFF");
    evoDrawSmallText(417, 270, 201, countText, gEnableMX4SIO ? evoColGreen : evoColWhite);
    evoDrawSmallText(417, 289, 201, "Confirm: toggle | Square: save", evoColMuted);
    evoDrawSmallText(417, 308, 201, evoMx4sioNotice, evoColMuted);''', 'cartao MX4SIO na aba Inicio')
replace_once('''        case EVO_HOME:
            evoSection = EVO_LIBRARY;
            evoSetLibraryFilter(evoLibraryFilter);
            evoFocus = EVO_FOCUS_CONTENT;
            break;''',
'''        case EVO_HOME:
            evoFocus = EVO_FOCUS_CONTENT;
            break;''','abrir aba Inicio')
replace_once('''    if (evoSection == EVO_LIBRARY) {
        if (getKeyOn(KEY_L1)) {''',
'''    if (evoSection == EVO_HOME) {
        if (getKeyOn(gSelectButton)) {
            gEnableMX4SIO = !gEnableMX4SIO;
            if (gEnableMX4SIO && gBDMStartMode == 0)
                gBDMStartMode = START_MODE_AUTO;
            snprintf(evoMx4sioNotice, sizeof(evoMx4sioNotice),
                     "Changed. Save and restart OPL.");
            sfxPlay(SFX_CONFIRM);
        } else if (getKeyOn(KEY_SQUARE)) {
            const int saved = saveConfig(CONFIG_OPL, 0);
            snprintf(evoMx4sioNotice, sizeof(evoMx4sioNotice), "%s",
                     saved ? "Saved. Restart OPL to apply." : "Save failed. Check memory card.");
            sfxPlay(saved ? SFX_CONFIRM : SFX_CANCEL);
        } else if (getKeyOn(evoCancelButton())) {
            evoFocus = EVO_FOCUS_NAV;
            sfxPlay(SFX_CANCEL);
        }
    } else if (evoSection == EVO_LIBRARY) {
        if (getKeyOn(KEY_L1)) {''','controles da aba Inicio')
p.write_text(s, encoding='utf-8')
print('OK: src/evo_ui.c atualizado; MX4SIO ON/OFF na aba Inicio, salvar com Quadrado.')
