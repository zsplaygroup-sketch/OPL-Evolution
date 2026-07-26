#ifndef __EVO_SETTINGS_H
#define __EVO_SETTINGS_H

#include <stddef.h>

enum evo_setting_id {
    EVO_SETTING_ARTWORK = 0,
    EVO_SETTING_WIDESCREEN,
    EVO_SETTING_NOTIFICATIONS,
    EVO_SETTING_SFX,
    EVO_SETTING_BGM,
    EVO_SETTING_AUTOSORT,
    EVO_SETTING_AUTOREFRESH,
    EVO_SETTING_CONFIRM_BUTTON,
    EVO_SETTING_VIDEO_MODE,
    EVO_SETTING_COUNT
};

void evoSettingsBegin(void);
int evoSettingsGetCount(void);
int evoSettingsGetSelected(void);
void evoSettingsSetSelected(int id);
void evoSettingsMove(int direction);
void evoSettingsChange(int direction);
const char *evoSettingsGetName(int id);
void evoSettingsGetValue(int id, char *value, size_t valueSize);
int evoSettingsSave(void);
void evoSettingsDiscard(void);

#endif
