#!/usr/bin/env python3
"""Validation of anchored v0.3 transformations using source excerpts.
This does NOT replace a clean PS2DEV compile or real-console tests.
"""
from pathlib import Path
import sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from aplicar_v04 import patch_ui, patch_opl

ui='''#include "include/evo_engine.h"
enum evo_launch_request {
    EVO_LAUNCH_NONE = 0,
    EVO_LAUNCH_PS2_PREPARE,
    EVO_LAUNCH_PS2_COMMIT,
    EVO_LAUNCH_PS1
};
static int evoPs1Selection;
static void evoSetLibraryFilter(int filter)
static void evoDrawSelectionPanel(void)
'''
# Target fragments copied from the inspected upstream revision after v0.2 transformations.
ui += '''    } else {
        snprintf(source, sizeof(source), "Evolution");
        snprintf(title, sizeof(title), "Homebrew scanner pending");
        startup[0] = '\\0';
        position = 0;
        total = 0;
    }

    if (evoFocus == EVO_FOCUS_CONTENT)
'''
ui += '''    if (!(evoLibraryFilter == EVO_LIBRARY_PS2 &&
          evoEngineDrawSelectedCover(421, 71, 88, 166)))
        fntRenderString(EVO_MAIN_FONT, 464, 143, ALIGN_CENTER, 84, 24, "NO ART", evoColMuted);
'''
ui += '''    if (evoLibraryFilter == EVO_LIBRARY_APPS) {
        evoDrawText(174, 118, "Scanner de aplicativos indisponivel.", evoColMuted);
        evoDrawText(174, 148, "Use o OPL funcional para aplicativos.", evoColMuted);
    } else if (evoLibraryFilter == EVO_LIBRARY_PS2) {
'''
ui += '''    evoDrawText(174, 62, "BIBLIOTECA DE JOGOS", evoColCyan);
    evoDrawText(174, 62, "BIBLIOTECA DE JOGOS", evoColCyan);
        } else {
            evoLaunchRequest = EVO_LAUNCH_NONE;
            evoBootPlay(EVO_BOOT_PS1, evoPs1Device,
'''
ui += '''        if (request != EVO_LAUNCH_PS1 && result < 0) {
            evoLaunchInProgress = 1;
            switch (result) {
'''
ui += '''    if (getKeyOn(KEY_SQUARE) &&
        !(evoSection == EVO_SETTINGS && evoFocus == EVO_FOCUS_CONTENT)) {
'''
ui += '''        } else if (getKeyOn(gSelectButton)) {
            snprintf(evoLaunchNotice, sizeof(evoLaunchNotice),
                     "Evolution homebrew scanner is not ready");
            evoLaunchInProgress = 1;
            evoLaunchRequest = EVO_LAUNCH_NONE;
        }
    } else if (evoSection == EVO_MEMORY_CARDS) {
'''
patched=patch_ui(ui)
assert patched.count('EVO_LAUNCH_APP') >= 4
assert 'evoAppsLaunchSelected()' in patched
assert patched.count('evoDrawFittedText(EVO_MAIN_FONT, 174, 62, 211, 22,') == 2
assert 'apps->itemLaunch(apps, evoAppSelection, config);' in patched
assert 'evoDrawAppsList(174, 112, 211, 38, 6)' in patched
assert 'evoAppsRefresh();' in patched
assert 'evoDrawFittedText(EVO_MAIN_FONT, x + 3' in patched
assert 'Evolution homebrew scanner is not ready' not in patched
assert 'evoSection == EVO_SETTINGS || evoSection == EVO_LIBRARY' in patched

opl='''#include <unistd.h>
    if (gBDMStartMode != START_MODE_AUTO)
        gBDMStartMode = START_MODE_AUTO;
    bdmEnumerateDevices();
#endif
            snprintf(dir, sizeof(dir), "%s/%s", appsPath, pdirent->d_name);
            if (pdirent->d_type != DT_DIR)
                continue;

            snprintf(path, sizeof(path), "%s/%s", dir, APP_TITLE_CONFIG_FILE);'''
op=patch_opl(opl)
assert 'gAPPStartMode = START_MODE_AUTO' in op
assert 'strcasecmp(pdirent->d_name + nameLen - 4, ".elf") == 0' in op
assert "name[nameLen - 4] = '\\0';" in op
assert '\x00' not in op
assert 'configSetStr(appConfig, APP_CONFIG_TITLE, name);' in op
assert 'configSetStr(appConfig, APP_CONFIG_BOOT, pdirent->d_name);' in op
assert 'if (!exception && nameLen > 4 && nameLen <= APP_BOOT_MAX' in op

for image_name in ('evo_boot.png','evo_dashboard.png'):
    assert (ROOT / 'assets/gfx' / image_name).is_file(),image_name
print('PASS: ancoras C de v0.3, modulo APPS nativo, extensao .ELF, controles de selecao, PNGs 640x480')
print('NAO TESTADO: compilacao PS2DEV, boot no PS2, descoberta USB/HDD/SMB real, execucao de ELF no console.')
