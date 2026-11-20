#!/usr/bin/env python3
"""SD CEARA GAMES Evolution XMB v0.4
Patches a clean OPL Evolution checkout (evo-development at revision 47119b7).
Installs the previous v0.2 interface and a native-backed APPS browser.
Refuses upstream drift; never overwrites working ELF or game files.
"""
from pathlib import Path
import argparse, hashlib, json, shutil, sys
from aplicar_no_opl_evolution import build_patch, replace

ROOT = Path(__file__).resolve().parents[1]
VERSION = 'SD CEARA GAMES Evolution XMB v0.4 — APPS'
FILES = ['src/evo_ui.c', 'src/evo_startup.c', 'include/evo_ui.h', 'src/evo_boot.c', 'src/opl.c']
ART = ['gfx/evo_boot.png', 'gfx/evo_dashboard.png']


def patch_ui(ui):
    ui = replace(ui, '#include "include/evo_engine.h"\n',
                 '#include "include/evo_engine.h"\n#include "include/appsupport.h"\n', 'incluir appsupport')
    ui = replace(ui, '    EVO_LAUNCH_PS1\n};',
                 '    EVO_LAUNCH_PS1,\n    EVO_LAUNCH_APP\n};', 'pedido de lancamento app')
    ui = replace(ui, 'static int evoPs1Selection;',
                 'static int evoPs1Selection;\nstatic int evoAppSelection;', 'selecionador app')

    # All app enumeration and launch calls go through OPL's native app support.
    # This keeps USB, legacy conf_apps.cfg and title.cfg paths consistent.
    adapter = r'''/* SD Ceará: browse and launch using the existing OPL application backend. */
static item_list_t *evoAppsGetSupport(void)
{
    item_list_t *apps = appGetObject(1);
    return apps != NULL && apps->enabled ? apps : NULL;
}

static int evoAppsCount(void)
{
    item_list_t *apps = evoAppsGetSupport();
    int count = apps && apps->itemGetCount ? apps->itemGetCount(apps) : 0;
    if (count <= 0)
        evoAppSelection = 0;
    else if (evoAppSelection >= count)
        evoAppSelection = count - 1;
    return count;
}

static const char *evoAppsName(int id)
{
    item_list_t *apps = evoAppsGetSupport();
    int count = evoAppsCount();
    if (!apps || !apps->itemGetName || id < 0 || id >= count)
        return "";
    return apps->itemGetName(apps, id);
}

static const char *evoAppsElf(int id)
{
    item_list_t *apps = evoAppsGetSupport();
    int count = evoAppsCount();
    if (!apps || !apps->itemGetStartup || id < 0 || id >= count)
        return "";
    return apps->itemGetStartup(apps, id);
}

static void evoAppsRefresh(void)
{
    item_list_t *apps = evoAppsGetSupport();
    if (apps)
        ioPutRequest(IO_MENU_UPDATE_DEFFERED, &apps->mode);
}

static int evoAppsLaunchSelected(void)
{
    item_list_t *apps = evoAppsGetSupport();
    config_set_t *config;
    int count = evoAppsCount();
    if (!apps || !apps->itemGetConfig || !apps->itemLaunch ||
        count <= 0 || evoAppSelection >= count)
        return -1;
    config = apps->itemGetConfig(apps, evoAppSelection);
    if (!config)
        return -2;
    /* Native itemLaunch deinitializes the frontend and loads the ELF. */
    apps->itemLaunch(apps, evoAppSelection, config);
    /* Mirror upstream itemExecSelect: it delegates config lifetime to
     * the native launcher after deinitialization. */
    return -3;
}

static void evoAppsBeginLaunch(void)
{
    if (evoAppsCount() <= 0) {
        snprintf(evoLaunchNotice, sizeof(evoLaunchNotice), "Nenhum ELF disponivel");
        evoLaunchInProgress = 1;
        evoLaunchRequest = EVO_LAUNCH_NONE;
        return;
    }
    snprintf(evoLaunchNotice, sizeof(evoLaunchNotice), "Abrindo aplicativo...");
    evoLaunchInProgress = 1;
    evoLaunchRequest = EVO_LAUNCH_APP;
}

'''
    ui = replace(ui, 'static void evoSetLibraryFilter(int filter)\n',
                 adapter + 'static void evoSetLibraryFilter(int filter)\n', 'adaptador de apps')

    listdraw = r'''static void evoDrawAppsList(int x, int y, int width, int rowHeight, int rows)
{
    int count = evoAppsCount();
    int first, row;
    if (count <= 0) {
        evoDrawFittedText(EVO_MAIN_FONT, x, y + 6, width, 22,
                          "Sem aplicativos. Pressione Quadrado para atualizar.", evoColMuted);
        evoDrawSmallText(x, y + 34, width, "USB:/APPS ou conf_apps.cfg", evoColMuted);
        return;
    }
    first = evoAppSelection - rows / 2;
    if (first < 0) first = 0;
    if (first + rows > count) first = count > rows ? count - rows : 0;
    for (row = 0; row < rows && first + row < count; ++row) {
        int id = first + row;
        int rowY = y + row * rowHeight;
        int selected = id == evoAppSelection;
        if (selected) {
            rmDrawRect(x - 6, rowY - 3, width + 12, rowHeight,
                       evoColPanelStrong);
            evoDrawOutline(x - 6, rowY - 3, width + 12, rowHeight, evoColCyan);
        } else {
            rmDrawLine(x, rowY + rowHeight - 4, x + width,
                       rowY + rowHeight - 4, evoColBlue);
        }
        evoDrawFittedText(EVO_MAIN_FONT, x + 3, rowY + 6, width - 6,
                          rowHeight - 8, evoAppsName(id),
                          selected ? evoColWhite : evoColMuted);
    }
}

'''
    ui = replace(ui, 'static void evoDrawSelectionPanel(void)\n',
                 listdraw + 'static void evoDrawSelectionPanel(void)\n', 'renderizador apps')
    ui = replace(ui, r'''    } else {
        snprintf(source, sizeof(source), "Evolution");
        snprintf(title, sizeof(title), "Homebrew scanner pending");
        startup[0] = '\0';
        position = 0;
        total = 0;
    }

    if (evoFocus == EVO_FOCUS_CONTENT)''',
    r'''    } else if (evoLibraryFilter == EVO_LIBRARY_APPS) {
        total = evoAppsCount();
        position = total > 0 ? evoAppSelection + 1 : 0;
        snprintf(source, sizeof(source), "APPS / ELF");
        snprintf(title, sizeof(title), "%s",
                 total > 0 ? evoAppsName(evoAppSelection) : "Nenhum aplicativo");
        snprintf(startup, sizeof(startup), "%s",
                 total > 0 ? evoAppsElf(evoAppSelection) : "--");
    } else {
        snprintf(source, sizeof(source), "Evolution");
        snprintf(title, sizeof(title), "Sem aplicativo");
        startup[0] = '\0';
        position = 0;
        total = 0;
    }

    if (evoFocus == EVO_FOCUS_CONTENT)''', 'dados app no painel')
    ui = replace(ui, '''    if (!(evoLibraryFilter == EVO_LIBRARY_PS2 &&
          evoEngineDrawSelectedCover(421, 71, 88, 166)))
        fntRenderString(EVO_MAIN_FONT, 464, 143, ALIGN_CENTER, 84, 24, "NO ART", evoColMuted);''',
    '''    if (evoLibraryFilter == EVO_LIBRARY_APPS) {
        fntRenderString(EVO_MAIN_FONT, 464, 143, ALIGN_CENTER, 84, 24,
                        "ELF", evoColCyan);
    } else if (!(evoLibraryFilter == EVO_LIBRARY_PS2 &&
                 evoEngineDrawSelectedCover(421, 71, 88, 166))) {
        fntRenderString(EVO_MAIN_FONT, 464, 143, ALIGN_CENTER, 84, 24,
                        "SEM CAPA", evoColMuted);
    }''', 'painel placeholder app')
    # v0.2 transforms the user-facing strings here; never revert them.
    ui = replace(ui, '''    if (evoLibraryFilter == EVO_LIBRARY_APPS) {
        evoDrawText(174, 118, "Scanner de aplicativos indisponivel.", evoColMuted);
        evoDrawText(174, 148, "Use o OPL funcional para aplicativos.", evoColMuted);
    } else if (evoLibraryFilter == EVO_LIBRARY_PS2) {''',
    '''    if (evoLibraryFilter == EVO_LIBRARY_APPS) {
        evoDrawAppsList(174, 112, 211, 38, 6);
    } else if (evoLibraryFilter == EVO_LIBRARY_PS2) {''', 'aba apps lista')
    ui = replace(ui, '''        } else {
            evoLaunchRequest = EVO_LAUNCH_NONE;
            evoBootPlay(EVO_BOOT_PS1, evoPs1Device,''',
    '''        } else if (request == EVO_LAUNCH_APP) {
            if (ioHasPendingRequests()) {
                snprintf(evoLaunchNotice, sizeof(evoLaunchNotice),
                         "Finalizando leitura de dispositivos...");
                return;
            }
            evoLaunchRequest = EVO_LAUNCH_NONE;
            thmUnloadEvolutionDashboard();
            result = evoAppsLaunchSelected();
        } else {
            evoLaunchRequest = EVO_LAUNCH_NONE;
            evoBootPlay(EVO_BOOT_PS1, evoPs1Device,''', 'executar app com launcher original')
    ui = replace(ui, '''        if (request != EVO_LAUNCH_PS1 && result < 0) {
            evoLaunchInProgress = 1;
            switch (result) {''',
    '''        if (request != EVO_LAUNCH_PS1 && result < 0) {
            evoLaunchInProgress = 1;
            if (request == EVO_LAUNCH_APP) {
                snprintf(evoLaunchNotice, sizeof(evoLaunchNotice),
                         "Falha ao abrir ELF (%d)", result);
                return;
            }
            switch (result) {''', 'erro aplicativo com detalhe')
    # Square opens Options only outside interactive lists; inside library it refreshes.
    ui = replace(ui, "    if (getKeyOn(KEY_SQUARE) &&\n        !(evoSection == EVO_SETTINGS && evoFocus == EVO_FOCUS_CONTENT)) {",
                 "    if (getKeyOn(KEY_SQUARE) &&\n        !(evoFocus == EVO_FOCUS_CONTENT &&\n          (evoSection == EVO_SETTINGS || evoSection == EVO_LIBRARY))) {",
                 "habilitar refresh com Quadrado")
    ui = replace(ui, '''        } else if (getKeyOn(gSelectButton)) {
            snprintf(evoLaunchNotice, sizeof(evoLaunchNotice),
                     "Evolution homebrew scanner is not ready");
            evoLaunchInProgress = 1;
            evoLaunchRequest = EVO_LAUNCH_NONE;
        }
    } else if (evoSection == EVO_MEMORY_CARDS) {''',
    '''        } else if (evoLibraryFilter == EVO_LIBRARY_APPS) {
            int count = evoAppsCount();
            if (getKeyOn(gSelectButton)) {
                sfxPlay(SFX_CONFIRM);
                evoAppsBeginLaunch();
            } else if (getKeyOn(KEY_UP) && count > 0) {
                evoAppSelection = evoAppSelection > 0 ? evoAppSelection - 1 : count - 1;
                sfxPlay(SFX_CURSOR);
            } else if (getKeyOn(KEY_DOWN) && count > 0) {
                evoAppSelection = (evoAppSelection + 1) % count;
                sfxPlay(SFX_CURSOR);
            } else if (getKeyOn(KEY_L2) && count > 0) {
                evoAppSelection = evoAppSelection >= 6 ? evoAppSelection - 6 : 0;
                sfxPlay(SFX_CURSOR);
            } else if (getKeyOn(KEY_R2) && count > 0) {
                evoAppSelection = evoAppSelection + 6 < count ? evoAppSelection + 6 : count - 1;
                sfxPlay(SFX_CURSOR);
            } else if (getKeyOn(KEY_SQUARE)) {
                evoAppsRefresh();
                sfxPlay(SFX_CONFIRM);
            }
        }
    } else if (evoSection == EVO_MEMORY_CARDS) {''', 'controles navegacao APPS')
    # Reserve 211 px for the library headline, including PS1 mode.
    # A long title must not run into the cover-art panel on a 4:3 CRT.
    ui = replace(ui,
                 '    evoDrawText(174, 62, "BIBLIOTECA DE JOGOS", evoColCyan);',
                 '    evoDrawFittedText(EVO_MAIN_FONT, 174, 62, 211, 22, '
                 '"BIBLIOTECA DE JOGOS", evoColCyan);',
                 'cabecalhos da biblioteca confinados', count=2)
    return ui


def patch_opl(opl):
    # Enable native Apps in standalone frontend, regardless of legacy config.
    opl = replace(opl, '''    if (gBDMStartMode != START_MODE_AUTO)
        gBDMStartMode = START_MODE_AUTO;
    bdmEnumerateDevices();
#endif''',
    '''    if (gBDMStartMode != START_MODE_AUTO)
        gBDMStartMode = START_MODE_AUTO;
    /* Standalone Evolution owns its APPS tab; do not change saved OPL config. */
    if (gAPPStartMode != START_MODE_AUTO)
        gAPPStartMode = START_MODE_AUTO;
    bdmEnumerateDevices();
#endif''', 'ligar apps apenas no runtime standalone')
    opl = replace(opl, '#include <unistd.h>\n',
                  '#include <unistd.h>\n#include <strings.h>\n', 'strcasecmp extensao ELF')
    # Native scanApps already handles subfolders/title.cfg. Extend only loose
    # ELF files in the root APPS dir; all go through the same appScanCallback.
    opl = replace(opl, '''            snprintf(dir, sizeof(dir), "%s/%s", appsPath, pdirent->d_name);
            if (pdirent->d_type != DT_DIR)
                continue;

            snprintf(path, sizeof(path), "%s/%s", dir, APP_TITLE_CONFIG_FILE);''',
    '''            snprintf(dir, sizeof(dir), "%s/%s", appsPath, pdirent->d_name);
#ifdef OPL_EVO_STANDALONE
            if (pdirent->d_type != DT_DIR) {
                /* Simple drop-in mode: APPS/*.ELF (USB/HDD/SMB only).
                 * Original folders with title.cfg remain the preferred mode.
                 * No arbitrary executable or subfolder recursion occurs. */
                size_t nameLen = strlen(pdirent->d_name);
                if (!exception && nameLen > 4 && nameLen <= APP_BOOT_MAX &&
                    strcasecmp(pdirent->d_name + nameLen - 4, ".elf") == 0) {
                    char name[APP_BOOT_MAX + 1];
                    appConfig = configAlloc(0, NULL, appsPath);
                    if (appConfig != NULL) {
                        snprintf(name, sizeof(name), "%s", pdirent->d_name);
                        name[nameLen - 4] = '\\0';
                        configSetStr(appConfig, APP_CONFIG_TITLE, name);
                        configSetStr(appConfig, APP_CONFIG_BOOT, pdirent->d_name);
                        ret = callback(appsPath, appConfig, arg);
                        configFree(appConfig);
                        if (ret == 0)
                            count++;
                        else if (ret < 0)
                            break;
                    }
                }
                continue;
            }
#else
            if (pdirent->d_type != DT_DIR)
                continue;
#endif

            snprintf(path, sizeof(path), "%s/%s", dir, APP_TITLE_CONFIG_FILE);''', 'arquivos ELF soltos na pasta APPS')
    # Check the code written above has a C NUL literal, not backslash+NUL.
    return opl


def make_edit(raw):
    ui, bootstart, h, launch = build_patch(raw['src/evo_ui.c'], raw['src/evo_startup.c'], raw['include/evo_ui.h'], raw['src/evo_boot.c'])
    return {
        'src/evo_ui.c': patch_ui(ui),
        'src/evo_startup.c': bootstart,
        'include/evo_ui.h': h,
        'src/evo_boot.c': launch,
        'src/opl.c': patch_opl(raw['src/opl.c']),
    }


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    ap = argparse.ArgumentParser(description=VERSION)
    ap.add_argument('repo', type=Path, help='copia LIMPA do repositorio OPL-Evolution')
    ap.add_argument('--verificar', action='store_true', help='apenas testar a aplicacao; nao escrever')
    args = ap.parse_args()
    repo = args.repo.expanduser().resolve()
    for rel in FILES + ART + ['Makefile', 'src/appsupport.c']:
        if not (repo / rel).is_file():
            sys.exit('ERRO: nao encontrei '+str(repo / rel))
    raw = {p: (repo/p).read_text(encoding='utf-8') for p in FILES}
    try:
        patched = make_edit(raw)
    except ValueError as exc:
        sys.exit('VERSAO INCOMPATIVEL — nenhuma alteracao: '+str(exc))
    if args.verificar:
        print('COMPATIVEL: todas as substituicoes foram verificadas; nenhuma alteracao feita.')
        return
    backup = repo / '.sdceara-backup-v0.4'
    if backup.exists():
        sys.exit('ERRO: backup existente; nao sobrepor: '+str(backup))
    backup.mkdir()
    for p in FILES + ART:
        to = backup/p
        to.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(repo/p, to)
    try:
        for p, content in patched.items():
            (repo/p).write_text(content, encoding='utf-8')
        for p in ART:
            shutil.copy2(ROOT/'assets'/p, repo/p)
        report = {'edicao': VERSION,
                  'origem_recomendada': '47119b7128f347eb046c52a48dee36a9559b2d1b',
                  'alterados': {p: digest(repo/p) for p in FILES+ART},
                  'motor_de_jogos_modificado': False,
                  'observacao': 'src/opl.c altera somente inicializacao runtime APPS e varredura ELF; nao muda o game loader'}
        (repo/'sdceara-edition-v0.4.json').write_text(json.dumps(report, indent=2, ensure_ascii=False), encoding='utf-8')
    except Exception:
        for p in FILES+ART:
            shutil.copy2(backup/p, repo/p)
        raise
    print('APLICADO:', VERSION)
    print('Arquivos:', ', '.join(FILES+ART))
    print('OBS: compile com PS2DEV via make clean && make evolution. ELF nao compilado aqui.')

if __name__ == '__main__':
    main()
