#!/usr/bin/env python3
"""v1.8: controller screen geometry and OGG buffer producer correction only."""
from pathlib import Path
p=Path("src/evo_ui.c");s=p.read_text()
a=s.index("void evoUiRenderControllers(void)\n{")
b=s.index("void evoUiHandleInputControllers(void)",a)
replacement='''void evoUiRenderControllers(void)
{
    int i;
    char player[24];
    char status[96];

    rmDrawRect(0, 0, 640, 480, evoColBackground);
    thmLoadEvolutionDashboard();
    evoDrawTexture(EVO_DASHBOARD_BG, 0, 0, 640, 480, gDefaultCol);

#ifdef PADEMU
    evoPollControllerStatus();
#endif
    /* XMB background already contains the panel outlines and brand header.
     * Keep every row within the left technical panel (x=30..409). */
    evoXmbTechTitle("STATUS DOS CONTROLES");
    for (i = 0; i < 4; ++i) {
        snprintf(player, sizeof(player), "Player %d", i + 1);
        evoControllerStatus(i, status, sizeof(status));
        evoXmbTechRow(161 + i * 34, player, status,
                      i == evoControllerSelection);
    }
    evoXmbMini(38, 318, 360,
               evoControllerNotice[0] ? evoControllerNotice :
               "X: Configurar  QUADRADO: Reativar portas",
               evoColMuted);

    /* Use the existing right-hand card, not a second overlaid frame. */
    evoXmbTxt(440, 130, 160, "ADAPTADOR", evoColWhite);
    rmDrawLine(441, 153, 600, 153,
               GS_SETREG_RGBA(0x78, 0x92, 0x7B, 0x80));
#ifdef PADEMU
    evoXmbMini(440, 176, 160,
               (evoBtStatus[0] & DS34BT_STATE_USB_CONFIGURED) ?
               "Bluetooth ativo" : "Bluetooth indisponivel",
               evoColMuted);
    if (evoAdapterInfoValid) {
        snprintf(status, sizeof(status), "USB %04X:%04X",
                 evoAdapterInfo.vid, evoAdapterInfo.pid);
        evoXmbMini(440, 204, 160, status, evoColWhite);
    } else {
        evoXmbMini(440, 204, 160, "Sem identificacao", evoColMuted);
    }
#else
    evoXmbMini(440, 176, 160, "PADEMU indisponivel", evoColMuted);
#endif
    evoXmbMini(440, 264, 160, "CIMA / BAIXO", evoColWhite);
    evoXmbMini(440, 287, 160, "Selecionar controle", evoColMuted);
    evoXmbMini(440, 319, 160, "Voltar: Circulo", evoColMuted);
}

'''
s=s[:a]+replacement+s[b:];p.write_text(s)
p=Path("src/sound.c");s=p.read_text()
old='''        while ((wrPtr + partsToRead < BGM_RING_BUFFER_COUNT) && (PollSema(inSema) == inSema))
            partsToRead++;
'''
if s.count(old)!=1:raise SystemExit("BGM producer anchor mismatch")
s=s.replace(old,'''        /* Exactly one inSema slot corresponds to the PCM buffer filled
         * below. Do not consume extra slots without decoding them. */
''',1)
old='''        wrPtr = (wrPtr + partsToRead) % BGM_RING_BUFFER_COUNT;
        for (i = 0; i < partsToRead; i++)
            SignalSema(outSema);
'''
if s.count(old)!=1:raise SystemExit("BGM output slot anchor mismatch")
s=s.replace(old,'''        wrPtr = (wrPtr + 1) % BGM_RING_BUFFER_COUNT;
        SignalSema(outSema);
''',1)
s=s.replace('    int partsToRead, decodeTotal, bitStream, i;','    int decodeTotal, bitStream;',1)
s=s.replace('        partsToRead = 1;\n','',1)
p.write_text(s)
Path("sdceara-edition-v1.8.json").write_text('{"version":"1.8-test","changes":["controller screen geometry","one decoded PCM buffer per output semaphore signal"],"music_keys":"v1.7 preserved","music_panel":"v1.6 preserved"}\n')
print("v1.8 controller layout and PCM producer patch applied")
