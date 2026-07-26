/*
 Copyright 2026, OPL Evolution contributors
 Licenced under Academic Free License version 3.0
 Review OpenUsbLd README & LICENSE files for further details.
*/

#include "include/opl.h"
#include "include/evo_ps1_art.h"
#include "include/renderman.h"
#include "include/textures.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#define EVO_PS1_ART_KEY_MAX 128
#define EVO_PS1_ART_PATH_MAX 256

static GSTEXTURE evoPs1Cover;
static char evoPs1CoverKey[EVO_PS1_ART_KEY_MAX];
static int evoPs1CoverLoaded;

static void evoPs1ArtRelease(void)
{
    if (evoPs1Cover.Mem != NULL) {
        rmUnloadTexture(&evoPs1Cover);
    }
    if (evoPs1Cover.Mem != NULL || evoPs1Cover.Clut != NULL) {
        texFree(&evoPs1Cover);
    }
    evoPs1CoverLoaded = 0;
}

void evoPs1ArtReset(void)
{
    evoPs1ArtRelease();
    evoPs1CoverKey[0] = '\0';
}

static void evoPs1ArtBasename(const char *vcdName, char *result, size_t resultSize)
{
    size_t length;

    snprintf(result, resultSize, "%s", vcdName ? vcdName : "");
    length = strlen(result);
    if (length > 4 && strcasecmp(result + length - 4, ".VCD") == 0)
        result[length - 4] = '\0';
}

static int evoPs1ArtSerial(const char *name, char *serial, size_t serialSize)
{
    int i;

    if (name == NULL || strlen(name) < 10 || serialSize < 12)
        return 0;
    for (i = 0; i < 4; i++) {
        if (!isalpha((unsigned char)name[i]))
            return 0;
    }
    if (name[4] != '_' && name[4] != '-')
        return 0;
    for (i = 5; i < 8; i++) {
        if (!isdigit((unsigned char)name[i]))
            return 0;
    }

    if (name[8] == '.') {
        if (!isdigit((unsigned char)name[9]) ||
            !isdigit((unsigned char)name[10]))
            return 0;
        snprintf(serial, serialSize, "%c%c%c%c_%c%c%c.%c%c",
                 toupper((unsigned char)name[0]),
                 toupper((unsigned char)name[1]),
                 toupper((unsigned char)name[2]),
                 toupper((unsigned char)name[3]),
                 name[5], name[6], name[7], name[9], name[10]);
    } else {
        if (!isdigit((unsigned char)name[8]) ||
            !isdigit((unsigned char)name[9]))
            return 0;
        snprintf(serial, serialSize, "%c%c%c%c_%c%c%c.%c%c",
                 toupper((unsigned char)name[0]),
                 toupper((unsigned char)name[1]),
                 toupper((unsigned char)name[2]),
                 toupper((unsigned char)name[3]),
                 name[5], name[6], name[7], name[8], name[9]);
    }

    return 1;
}

static int evoPs1ArtTryPath(const char *pathWithoutExtension)
{
    return texDiscoverLoad(&evoPs1Cover, pathWithoutExtension, -1) >= 0 &&
           evoPs1Cover.Mem != NULL;
}

static int evoPs1ArtLoad(int device, const char *vcdName)
{
    char baseName[EVO_PS1_ART_KEY_MAX];
    char serial[16];
    char path[EVO_PS1_ART_PATH_MAX];

    evoPs1ArtBasename(vcdName, baseName, sizeof(baseName));
    if (device < 0 || baseName[0] == '\0')
        return 0;

    snprintf(path, sizeof(path), "mass%d:/ART/%s_COV", device, baseName);
    if (evoPs1ArtTryPath(path))
        return 1;

    snprintf(path, sizeof(path), "mass%d:/POPS/ART/%s_COV", device, baseName);
    if (evoPs1ArtTryPath(path))
        return 1;

    if (evoPs1ArtSerial(baseName, serial, sizeof(serial))) {
        snprintf(path, sizeof(path), "mass%d:/ART/%s_COV", device, serial);
        if (evoPs1ArtTryPath(path))
            return 1;
        snprintf(path, sizeof(path), "mass%d:/POPS/ART/%s_COV", device, serial);
        if (evoPs1ArtTryPath(path))
            return 1;
    }

    /* Compatibility with older POPSLoader sidecar-cover layouts. */
    snprintf(path, sizeof(path), "mass%d:/POPS/%s", device, baseName);
    return evoPs1ArtTryPath(path);
}

static int evoPs1ArtEnsure(int device, const char *vcdName)
{
    char key[EVO_PS1_ART_KEY_MAX];

    snprintf(key, sizeof(key), "%d:%s", device, vcdName ? vcdName : "");
    if (strcmp(key, evoPs1CoverKey) == 0)
        return evoPs1CoverLoaded;

    evoPs1ArtRelease();
    snprintf(evoPs1CoverKey, sizeof(evoPs1CoverKey), "%s", key);
    evoPs1CoverLoaded = evoPs1ArtLoad(device, vcdName);
    return evoPs1CoverLoaded;
}

int evoPs1ArtDraw(int device, const char *vcdName,
                  int x, int y, int width, int height)
{
    int drawWidth;
    int drawHeight;
    int finalWidth;

    if (!gEnableArt || !evoPs1ArtEnsure(device, vcdName))
        return 0;

    drawWidth = width;
    drawHeight = height;
    if (evoPs1Cover.Width > 0 && evoPs1Cover.Height > 0) {
        if (evoPs1Cover.Width * height > evoPs1Cover.Height * width)
            drawHeight = (width * evoPs1Cover.Height) / evoPs1Cover.Width;
        else
            drawWidth = (height * evoPs1Cover.Width) / evoPs1Cover.Height;
    }

    /*
     * SCALING_RATIO narrows the rendered width in anamorphic 16:9 mode.
     * Center using that final width, otherwise the art appears left-aligned
     * inside its frame even though its source-space rectangle is centered.
     */
    finalWidth = rmWideScale(drawWidth);
    rmDrawPixmap(&evoPs1Cover,
                 x + (width - finalWidth) / 2,
                 y + (height - drawHeight) / 2,
                 ALIGN_NONE, drawWidth, drawHeight,
                 SCALING_RATIO, gDefaultCol);
    return 1;
}
