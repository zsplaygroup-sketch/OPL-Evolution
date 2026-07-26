/*
 Copyright 2026, OPL Evolution contributors
 Licenced under Academic Free License version 3.0
 Review OpenUsbLd README & LICENSE files for further details.
*/

#include "include/evo_settings.h"
#include "include/opl.h"
#include "include/config.h"
#include "include/pad.h"

#include <stdio.h>

typedef struct {
    int artwork;
    int widescreen;
    int notifications;
    int sfx;
    int bgm;
    int autosort;
    int autorefresh;
    int confirmButton;
    int videoMode;
} evo_settings_state_t;

static const char *const settingNames[EVO_SETTING_COUNT] = {
    "Cover artwork",
    "Widescreen layout",
    "Notifications",
    "Interface sounds",
    "Background music",
    "Automatic sorting",
    "Automatic refresh",
    "Confirm button",
    "Video mode",
};

static evo_settings_state_t settings;
static int settingsSelection;

static int evoSettingsToggle(int value)
{
    return value ? 0 : 1;
}

void evoSettingsBegin(void)
{
    settings.artwork = gEnableArt;
    settings.widescreen = gWideScreen;
    settings.notifications = gEnableNotifications;
    settings.sfx = gEnableSFX;
    settings.bgm = gEnableBGM;
    settings.autosort = gAutosort;
    settings.autorefresh = gAutoRefresh;
    settings.confirmButton = gSelectButton;
    settings.videoMode = gVMode;
    settingsSelection = 0;
}

int evoSettingsGetCount(void)
{
    return EVO_SETTING_COUNT;
}

int evoSettingsGetSelected(void)
{
    return settingsSelection;
}

void evoSettingsSetSelected(int id)
{
    if (id < 0)
        id = 0;
    else if (id >= EVO_SETTING_COUNT)
        id = EVO_SETTING_COUNT - 1;
    settingsSelection = id;
}

void evoSettingsMove(int direction)
{
    if (direction < 0)
        settingsSelection = settingsSelection > 0 ?
                                settingsSelection - 1 :
                                EVO_SETTING_COUNT - 1;
    else
        settingsSelection = (settingsSelection + 1) % EVO_SETTING_COUNT;
}

void evoSettingsChange(int direction)
{
    switch (settingsSelection) {
        case EVO_SETTING_ARTWORK:
            settings.artwork = evoSettingsToggle(settings.artwork);
            break;
        case EVO_SETTING_WIDESCREEN:
            settings.widescreen = evoSettingsToggle(settings.widescreen);
            break;
        case EVO_SETTING_NOTIFICATIONS:
            settings.notifications = evoSettingsToggle(settings.notifications);
            break;
        case EVO_SETTING_SFX:
            settings.sfx = evoSettingsToggle(settings.sfx);
            break;
        case EVO_SETTING_BGM:
            settings.bgm = evoSettingsToggle(settings.bgm);
            break;
        case EVO_SETTING_AUTOSORT:
            settings.autosort = evoSettingsToggle(settings.autosort);
            break;
        case EVO_SETTING_AUTOREFRESH:
            settings.autorefresh = evoSettingsToggle(settings.autorefresh);
            break;
        case EVO_SETTING_CONFIRM_BUTTON:
            settings.confirmButton =
                settings.confirmButton == KEY_CROSS ? KEY_CIRCLE : KEY_CROSS;
            break;
        case EVO_SETTING_VIDEO_MODE:
            settings.videoMode += direction < 0 ? -1 : 1;
            if (settings.videoMode < 0)
                settings.videoMode = 3;
            else if (settings.videoMode > 3)
                settings.videoMode = 0;
            break;
    }
}

const char *evoSettingsGetName(int id)
{
    if (id < 0 || id >= EVO_SETTING_COUNT)
        return "";
    return settingNames[id];
}

void evoSettingsGetValue(int id, char *value, size_t valueSize)
{
    int enabled = 0;

    if (value == NULL || valueSize == 0)
        return;

    switch (id) {
        case EVO_SETTING_ARTWORK:
            enabled = settings.artwork;
            break;
        case EVO_SETTING_WIDESCREEN:
            enabled = settings.widescreen;
            break;
        case EVO_SETTING_NOTIFICATIONS:
            enabled = settings.notifications;
            break;
        case EVO_SETTING_SFX:
            enabled = settings.sfx;
            break;
        case EVO_SETTING_BGM:
            enabled = settings.bgm;
            break;
        case EVO_SETTING_AUTOSORT:
            enabled = settings.autosort;
            break;
        case EVO_SETTING_AUTOREFRESH:
            enabled = settings.autorefresh;
            break;
        case EVO_SETTING_CONFIRM_BUTTON:
            snprintf(value, valueSize, "%s",
                     settings.confirmButton == KEY_CROSS ? "Cross" : "Circle");
            return;
        case EVO_SETTING_VIDEO_MODE:
            snprintf(value, valueSize, "%s",
                     settings.videoMode == 1 ? "PAL" :
                     settings.videoMode == 2 ? "NTSC" :
                     settings.videoMode == 3 ? "480p" : "Auto");
            return;
        default:
            value[0] = '\0';
            return;
    }

    snprintf(value, valueSize, "%s", enabled ? "On" : "Off");
}

int evoSettingsSave(void)
{
    gEnableArt = settings.artwork;
    gWideScreen = settings.widescreen;
    gEnableNotifications = settings.notifications;
    gEnableSFX = settings.sfx;
    gEnableBGM = settings.bgm;
    gAutosort = settings.autosort;
    gAutoRefresh = settings.autorefresh;
    gSelectButton = settings.confirmButton;
    gVMode = settings.videoMode;

    /*
     * Reuse OPL's proven config serializer and storage discovery only.  No
     * original dialog or menu object is involved when showUI is zero.
     */
    return saveConfig(CONFIG_OPL, 0);
}

void evoSettingsDiscard(void)
{
    evoSettingsBegin();
}
