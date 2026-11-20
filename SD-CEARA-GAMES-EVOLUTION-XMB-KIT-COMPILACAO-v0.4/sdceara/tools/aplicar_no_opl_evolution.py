#!/usr/bin/env python3
"""Apply SD Ceará Games XMB v0.2 visual customization to a CLEAN local clone
of officialjuicedesigns/OPL-Evolution, branch evo-development.

No changes are made to the PS2 game-loading engine or backend modules.
Refuses partially compatible sources instead of guessing.
"""
from pathlib import Path
import argparse, hashlib, json, shutil, sys

ROOT = Path(__file__).resolve().parents[1]
EDITION = 'SD CEARA GAMES - OPL Evolution XMB v0.2'

def replace(src, old, new, name, count=1):
    actual=src.count(old)
    if actual != count:
        raise ValueError(f'{name}: esperado {count} trecho(s), encontrado {actual}. ' 
                         'A base pode ter mudado; nao sobrescrever automaticamente.')
    return src.replace(old,new)

def build_patch(ui, start, header, engine_boot):
    ui=replace(ui, "#include <stdlib.h>\n#include <strings.h>",
                   "#include <stdlib.h>\n#include <string.h>\n#include <strings.h>", "biblioteca string")
    ui=replace(ui,
        '''static const char *const evoSectionLabels[EVO_SECTION_COUNT] = {
    "Home",
    "Library",
    "Controllers",
    "Memory",
    "Network",
    "Settings",
    "Diagnostics",
};''',
        '''static const char *const evoSectionLabels[EVO_SECTION_COUNT] = {
    "Inicio",
    "Jogos",
    "Controles",
    "Memoria",
    "Rede",
    "Ajustes",
    "Sistema",
};''','menu principal')
    ui=replace(ui,
        '''static const char *const evoLibraryFilterLabels[EVO_LIBRARY_FILTER_COUNT] = {
    "PS2",
    "PS1",
    "HOMEBREW",
};''',
        '''static const char *const evoLibraryFilterLabels[EVO_LIBRARY_FILTER_COUNT] = {
    "PS2",
    "PS1",
    "APPS",
};''','filtros da biblioteca')
    palette={
        'static const u64 evoColBackground = GS_SETREG_RGBA(0x00, 0x02, 0x07, 0x80);':
         'static const u64 evoColBackground = GS_SETREG_RGBA(0x04, 0x0D, 0x0B, 0x80);',
        'static const u64 evoColPanelStrong = GS_SETREG_RGBA(0x02, 0x0C, 0x1C, 0x80);':
         'static const u64 evoColPanelStrong = GS_SETREG_RGBA(0x08, 0x1A, 0x14, 0x80);',
        'static const u64 evoColCyan = GS_SETREG_RGBA(0x00, 0x94, 0xFF, 0x80);':
         'static const u64 evoColCyan = GS_SETREG_RGBA(0x06, 0xCA, 0x71, 0x80);',
        'static const u64 evoColBlue = GS_SETREG_RGBA(0x00, 0x3D, 0x78, 0x80);':
         'static const u64 evoColBlue = GS_SETREG_RGBA(0x3A, 0x6D, 0x4A, 0x80);',
        'static const u64 evoColWhite = GS_SETREG_RGBA(0xD6, 0xE0, 0xEF, 0x80);':
         'static const u64 evoColWhite = GS_SETREG_RGBA(0xDF, 0xEA, 0xE5, 0x80);',
        'static const u64 evoColMuted = GS_SETREG_RGBA(0x88, 0x9A, 0xB8, 0x80);':
         'static const u64 evoColMuted = GS_SETREG_RGBA(0x86, 0x9A, 0x90, 0x80);',
        'static const u64 evoColGreen = GS_SETREG_RGBA(0x3A, 0xD8, 0x45, 0x80);':
         'static const u64 evoColGreen = GS_SETREG_RGBA(0x06, 0xCA, 0x71, 0x80);',
    }
    for old,new in palette.items():
        ui=replace(ui,old,new, 'cor nativa: '+old.split(' ')[3])
    ui=replace(ui,
        '''static void evoDrawText(int x, int y, const char *text, u64 color)
{
    fntRenderString(EVO_MAIN_FONT, x, y, ALIGN_NONE, 0, 0, text, color);
}

static void evoDrawSmallText''',
        '''static void evoDrawText(int x, int y, const char *text, u64 color)
{
    fntRenderString(EVO_MAIN_FONT, x, y, ALIGN_NONE, 0, 0, text, color);
}

/* Truncation with explicit pixel budget prevents titles crossing into art. */
static void evoDrawFittedText(int font, int x, int y, int width, int height,
                              const char *text, u64 color)
{
    char visible[160];
    size_t count;
    const char *suffix = "...";

    if (text == NULL)
        text = "";
    snprintf(visible, sizeof(visible), "%s", text);
    if (fntCalcDimensions(font, visible) > width) {
        count = strlen(visible);
        while (count > 0) {
            visible[--count] = '\\0';
            if (fntCalcDimensions(font, visible) +
                    fntCalcDimensions(font, suffix) <= width)
                break;
        }
        if (count + 3 < sizeof(visible))
            strcat(visible, suffix);
    }
    fntRenderString(font, x, y, ALIGN_NONE, width, height, visible, color);
}

static void evoDrawSmallText''','funcao ajuste de texto')
    ui=replace(ui,
        "    fntRenderString(EVO_MAIN_FONT, x, y, ALIGN_NONE, width, 20, text, color);",
        "    evoDrawFittedText(EVO_MAIN_FONT, x, y, width, 20, text, color);",
        "labels de configuracao")
    ui=replace(ui,
        '''    fntRenderString(font, x, y, ALIGN_NONE, width, 18, text, color);
}''',
        '''    evoDrawFittedText(font, x, y, width, 18, text, color);
}''','textos pequenos')
    ui=replace(ui,
        '''        fntRenderString(EVO_MAIN_FONT, x + 3, rowY + 6, ALIGN_NONE,
                        width - 6, rowHeight - 8,
                        name && name[0] ? name : "Untitled PS2 game",
                        isSelected ? evoColWhite : evoColMuted);''',
        '''        evoDrawFittedText(EVO_MAIN_FONT, x + 3, rowY + 6,
                          width - 6, rowHeight - 8,
                          name && name[0] ? name : "Jogo PS2 sem nome",
                          isSelected ? evoColWhite : evoColMuted);''','linha da lista de jogos')
    ui=replace(ui,
        '''        fntRenderString(EVO_MAIN_FONT, 174, y, ALIGN_NONE, 208, 24,
                        evoPs1Games[game], game == evoPs1Selection ? evoColWhite : evoColMuted);''',
        '''        evoDrawFittedText(EVO_MAIN_FONT, 174, y, 208, 24,
                          evoPs1Games[game], game == evoPs1Selection ? evoColWhite : evoColMuted);''',
        'navegacao PS1')
    # Existing Evolution cover paint extends 8px into the 529px title column.
    ui=replace(ui, 'evoEngineDrawSelectedCover(421, 71, 116, 166)',
                     'evoEngineDrawSelectedCover(421, 71, 88, 166)',
                     'limite fisico da capa',count=2)
    substitutes={
        '"UNIFIED LIBRARY"':'"BIBLIOTECA DE JOGOS"',
        '"RECENT GAMES"':'"JOGOS RECENTES"',
        '"Waiting for a PS2 storage source..."':'"Procurando jogos no USB..."',
        '"Press X to open the selected title."':'"X: iniciar o jogo selecionado"',
        '"Evolution homebrew scanner is not ready yet."':'"Scanner de aplicativos indisponivel."',
        '"No original OPL Apps menu is used."':'"Use o OPL funcional para aplicativos."',
        '"TITLE"':'"JOGO"',
        '"SOURCE"':'"ORIGEM"',
        '"ITEM"':'"POSICAO"',
    }
    for old,new in substitutes.items():
        # Some labels repeat across Home / Library / PS1 / other views; intentional.
        count=ui.count(old)
        if not count: raise ValueError('nao encontrado texto: '+old)
        ui=ui.replace(old,new)
    header=replace(header, '#define OPL_EVO_APP_NAME      "OPL Evolution"',
        '#define OPL_EVO_APP_NAME      "SD CEARA GAMES - Evolution XMB"', 'identidade do executavel')
    header=replace(header, '#define OPL_EVO_APP_ID        "OPL-EVO"',
        '#define OPL_EVO_APP_ID        "SDCEARA-EVO"', 'identificador da edicao')
    # Maintain upstream version so diagnostics stays honest about original engine.
    # Barra do Evolution mantida; ao concluir, exibir marca por ~4 segundos.
    start=replace(start, '#include "include/textures.h"\n',
                  '#include "include/textures.h"\n\n#include <delaythread.h>\n',
                  'inclusao do temporizador')
    start=replace(start, '#define EVO_STARTUP_HOLD_FRAMES    12',
                  '#define EVO_STARTUP_HOLD_FRAMES    200',
                  'tempo de apreciacao')
    start=replace(start,
                  '    for (frame = 0; frame < EVO_STARTUP_HOLD_FRAMES; frame++)\n        evoStartupRender();',
                  '    for (frame = 0; frame < EVO_STARTUP_HOLD_FRAMES; frame++) {\n        evoStartupRender();\n        DelayThread(20000); /* aprox. 4s apos 100% */\n    }',
                  'pausa apos inicializacao')
    start=replace(start, 'GS_SETREG_RGBA(0x00, 0x46, 0xFF, 0x20)',
                        'GS_SETREG_RGBA(0x00, 0x66, 0x33, 0x20)', 'brilho barra 1')
    start=replace(start, 'GS_SETREG_RGBA(0x08, 0x64, 0xFF, 0x58)',
                        'GS_SETREG_RGBA(0x04, 0x88, 0x4D, 0x58)', 'brilho barra 2')
    start=replace(start, 'GS_SETREG_RGBA(0x28, 0x92, 0xFF, 0x80)',
                        'GS_SETREG_RGBA(0x06, 0xCA, 0x71, 0x80)', 'cor barra')
    start=replace(start, 'GS_SETREG_RGBA(0x82, 0xD8, 0xFF, 0x80)',
                        'GS_SETREG_RGBA(0xEB, 0xBA, 0x49, 0x80)', 'contorno barra')
    engine_boot=replace(engine_boot, 'GS_SETREG_RGBA(0x00, 0x54, 0xA8, 0x80)',
                        'GS_SETREG_RGBA(0x06, 0x64, 0x3A, 0x80)', 'fallback lancamento')
    return ui, start, header, engine_boot

def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()

def main():
    ap=argparse.ArgumentParser(description=EDITION)
    ap.add_argument('repositorio',type=Path,help='caminho do clone local OPL-Evolution')
    ap.add_argument('--verificar',action='store_true',help='somente verificar compatibilidade, nao alterar arquivos')
    args=ap.parse_args()
    repo=args.repositorio.expanduser().resolve()
    sources=['src/evo_ui.c','src/evo_startup.c','include/evo_ui.h','src/evo_boot.c']
    for rel in sources+['Makefile','gfx/evo_dashboard.png','gfx/evo_boot.png']:
        if not (repo/rel).is_file():
            sys.exit(f'ERRO: nao encontrado {repo/rel}. Verifique o clone oficial.')
    raw=[(repo/rel).read_text(encoding='utf-8') for rel in sources]
    try:
        patched=build_patch(*raw)
    except ValueError as exc:
        sys.exit('ERRO DE COMPATIBILIDADE: '+str(exc))
    if args.verificar:
        print('COMPATIVEL: todos os trechos esperados foram encontrados; nenhuma alteracao realizada.')
        return
    backup=repo/'.sdceara-backup-v0.2'
    if backup.exists():
        sys.exit(f'ERRO: ja existe backup {backup}; abortando para evitar sobregravacao.')
    backup.mkdir()
    paths=sources+['gfx/evo_dashboard.png','gfx/evo_boot.png']
    for rel in paths:
        dst=backup/rel;dst.parent.mkdir(parents=True,exist_ok=True)
        shutil.copy2(repo/rel,dst)
    for rel, data in zip(sources,patched):
        (repo/rel).write_text(data,encoding='utf-8')
    for fname in ['evo_dashboard.png','evo_boot.png']:
        shutil.copy2(ROOT/'assets'/'gfx'/fname,repo/'gfx'/fname)
    report={'edicao':EDITION,'arquivos':{p:sha(repo/p) for p in paths},
            'arquivos_engine_alterados':False,'backup':str(backup)}
    (repo/'sdceara-edition-v0.2.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print('APLICADO:',EDITION)
    print('Arquivos alterados:',', '.join(paths))
    print('NENHUM ARQUIVO DO MOTOR DE JOGOS FOI MODIFICADO.')
    print('Proximo passo: make evolution, no ambiente PS2DEV configurado.')

if __name__=='__main__': main()
