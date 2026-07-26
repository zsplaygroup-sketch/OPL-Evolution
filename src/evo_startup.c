/*
 Copyright 2026, OPL Evolution contributors
 Licenced under Academic Free License version 3.0
 Review OpenUsbLd README & LICENSE files for further details.
*/

#include "include/opl.h"
#include "include/evo_startup.h"
#include "include/gui.h"
#include "include/renderman.h"
#include "include/sound.h"
#include "include/textures.h"

#define EVO_STARTUP_IMAGE_PROGRESS 42
#define EVO_STARTUP_TRACK_X        144
#define EVO_STARTUP_TRACK_Y        360
#define EVO_STARTUP_TRACK_WIDTH    356
#define EVO_STARTUP_TRACK_HEIGHT   7
#define EVO_STARTUP_HOLD_FRAMES    12

static GSTEXTURE evoStartupTexture;
static int evoStartupLoaded;
static int evoStartupProgress;

static void evoStartupDrawProgress(void)
{
    int bakedWidth;
    int fillWidth;
    int extensionX;
    int extensionWidth;

    /*
     * The supplied artwork already contains its exact 42% illuminated state.
     * Preserve those source pixels and draw only the completed portion beyond
     * that checkpoint.
     */
    bakedWidth = (EVO_STARTUP_TRACK_WIDTH * EVO_STARTUP_IMAGE_PROGRESS) / 100;
    fillWidth = (EVO_STARTUP_TRACK_WIDTH * evoStartupProgress) / 100;
    if (fillWidth <= bakedWidth)
        return;

    /*
     * Once the bar moves, redraw its completed span as one continuous layer.
     * This covers the baked checkpoint's bright end-cap and prevents a seam
     * where the functional portion begins.
     */
    extensionX = EVO_STARTUP_TRACK_X;
    extensionWidth = fillWidth;

    rmDrawRect(extensionX - 2, EVO_STARTUP_TRACK_Y - 3,
               extensionWidth + 4, EVO_STARTUP_TRACK_HEIGHT + 6,
               GS_SETREG_RGBA(0x00, 0x46, 0xFF, 0x20));
    rmDrawRect(extensionX - 1, EVO_STARTUP_TRACK_Y - 1,
               extensionWidth + 2, EVO_STARTUP_TRACK_HEIGHT + 2,
               GS_SETREG_RGBA(0x08, 0x64, 0xFF, 0x58));
    rmDrawRect(extensionX, EVO_STARTUP_TRACK_Y,
               extensionWidth, EVO_STARTUP_TRACK_HEIGHT,
               GS_SETREG_RGBA(0x28, 0x92, 0xFF, 0x80));
    rmDrawLine(extensionX, EVO_STARTUP_TRACK_Y,
               extensionX + extensionWidth, EVO_STARTUP_TRACK_Y,
               GS_SETREG_RGBA(0x82, 0xD8, 0xFF, 0x80));
}

static void evoStartupRender(void)
{
    if (!evoStartupLoaded)
        return;

    guiStartFrame();
    rmDrawPixmap(&evoStartupTexture, 0, 0, ALIGN_NONE, 640, 480,
                 SCALING_NONE, gDefaultCol);
    evoStartupDrawProgress();
    guiEndFrame();
}

int evoStartupBegin(void)
{
    if (texLoadInternal(&evoStartupTexture, EVO_BOOT_BG) < 0)
        return -1;

    evoStartupTexture.Filter = GS_FILTER_NEAREST;
    evoStartupLoaded = 1;
    evoStartupProgress = EVO_STARTUP_IMAGE_PROGRESS;
    evoStartupRender();
    return 0;
}

void evoStartupAdvance(int progress)
{
    if (!evoStartupLoaded)
        return;
    if (progress < evoStartupProgress)
        progress = evoStartupProgress;
    if (progress > 100)
        progress = 100;

    while (evoStartupProgress < progress) {
        evoStartupProgress++;
        evoStartupRender();
    }
}

void evoStartupFinish(void)
{
    int frame;

    if (!evoStartupLoaded)
        return;

    /*
     * GUI_INIT_DONE is queued only after the preceding device and audio
     * initialization work completes.  Keep the bar below 100% until that real
     * completion signal is processed.
     */
    while (!guiIsInitComplete()) {
        guiExecDeferredOps();
        if (evoStartupProgress < 96)
            evoStartupProgress++;
        evoStartupRender();
    }

    if (gEnableBootSND)
        sfxPlay(SFX_BOOT);
    evoStartupAdvance(100);
    for (frame = 0; frame < EVO_STARTUP_HOLD_FRAMES; frame++)
        evoStartupRender();

    rmUnloadTexture(&evoStartupTexture);
    texFree(&evoStartupTexture);
    evoStartupLoaded = 0;
}
