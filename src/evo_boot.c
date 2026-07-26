/*
 Copyright 2026, OPL Evolution contributors
 Licenced under Academic Free License version 3.0

 Launch transition adapted from Irfanlesnar/PS2-Launcher:
 https://github.com/Irfanlesnar/PS2-Launcher
 Review OpenUsbLd README & LICENSE files for further details.
*/

#include "include/opl.h"
#include "include/evo_boot.h"
#include "include/evo_engine.h"
#include "include/evo_ps1_art.h"
#include "include/gui.h"
#include "include/renderman.h"
#include "include/sound.h"
#include "include/textures.h"
#include "include/themes.h"

#include <delaythread.h>

#define EVO_BOOT_ZOOM_FRAMES    35
#define EVO_BOOT_LOADING_FRAMES 60

static void evoBootDrawZoomFrame(enum evo_boot_platform platform,
                                 int ps1Device, const char *ps1VcdName,
                                 int frame, int screenWidth, int screenHeight)
{
    const float t = (float)frame / (float)EVO_BOOT_ZOOM_FRAMES;
    const float inverse = 1.0f - t;
    const float ease = 1.0f - inverse * inverse * inverse;
    const float startX = 412.0f;
    const float startY = 71.0f;
    const float startWidth = 105.0f;
    const float startHeight = 166.0f;
    const int currentX = (int)(startX * (1.0f - ease));
    const int currentY = (int)(startY * (1.0f - ease));
    const int currentWidth =
        (int)(startWidth + ease * ((float)screenWidth - startWidth));
    const int currentHeight =
        (int)(startHeight + ease * ((float)screenHeight - startHeight));
    float fade = t * 2.0f;
    int blackAlpha;
    int drewCover = 0;

    if (fade > 1.0f)
        fade = 1.0f;
    blackAlpha = (int)(fade * 0x80);

    guiStartFrame();
    rmDrawRect(0, 0, screenWidth, screenHeight,
               GS_SETREG_RGBA(0x00, 0x02, 0x07, 0x80));

    if (platform == EVO_BOOT_PS2)
        drewCover = evoEngineDrawSelectedCover(currentX, currentY,
                                               currentWidth, currentHeight);
    else
        drewCover = evoPs1ArtDraw(ps1Device, ps1VcdName,
                                  currentX, currentY,
                                  currentWidth, currentHeight);

    if (!drewCover)
        rmDrawRect(currentX, currentY, currentWidth, currentHeight,
                   GS_SETREG_RGBA(0x00, 0x54, 0xA8, 0x80));

    rmDrawRect(0, 0, screenWidth, screenHeight,
               GS_SETREG_RGBA(0, 0, 0, blackAlpha));
    guiEndFrame();
}

static void evoBootDrawLoadingFrame(int frame, int screenWidth, int screenHeight)
{
    GSTEXTURE *loader = NULL;
    int alpha = frame < 45 ? 0x80 : (0x80 * (EVO_BOOT_LOADING_FRAMES - 1 - frame)) / 14;

    if (gTheme != NULL && gTheme->loadingIconCount > 0)
        loader = thmGetTexture(LOAD0_ICON +
                               ((unsigned int)frame >> 1) % gTheme->loadingIconCount);

    guiStartFrame();
    rmDrawRect(0, 0, screenWidth, screenHeight,
               GS_SETREG_RGBA(0, 0, 0, 0x80));
    if (loader != NULL && loader->Mem != NULL && alpha > 0)
        rmDrawPixmap(loader, screenWidth - 34, screenHeight - 34,
                     ALIGN_NONE, 14, 14, SCALING_RATIO,
                     GS_SETREG_RGBA(0xFF, 0xFF, 0xFF, alpha));
    guiEndFrame();
}

void evoBootPlay(enum evo_boot_platform platform, int ps1Device,
                 const char *ps1VcdName)
{
    int frame;
    int screenWidth;
    int screenHeight;

    rmGetScreenExtents(&screenWidth, &screenHeight);
    sfxPlay(SFX_TRANSITION);

    for (frame = 0; frame <= EVO_BOOT_ZOOM_FRAMES; frame++)
        evoBootDrawZoomFrame(platform, ps1Device, ps1VcdName,
                             frame, screenWidth, screenHeight);

    for (frame = 0; frame < EVO_BOOT_LOADING_FRAMES; frame++) {
        evoBootDrawLoadingFrame(frame, screenWidth, screenHeight);
        DelayThread(16000);
    }
}
