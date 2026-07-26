#ifndef __EVO_ENGINE_H
#define __EVO_ENGINE_H

#include <stddef.h>

void evoEngineInit(void);
void evoEngineSync(void);

int evoEngineHasPs2(void);
int evoEngineGetCount(void);
int evoEngineGetSelected(void);
const char *evoEngineGetName(int id);
const char *evoEngineGetStartup(int id);
const char *evoEngineGetSourceName(void);

void evoEngineMove(int direction);
void evoEngineMovePage(int direction, int pageSize);
void evoEngineRefresh(void);

int evoEngineDrawSelectedCover(int x, int y, int width, int height);
int evoEngineLoadSelectedVMC(char *slot1, size_t slot1Size,
                             char *slot2, size_t slot2Size);
int evoEngineSaveSelectedVMC(const char *slot1, const char *slot2);
int evoEngineCheckSelectedVMC(char *name, int createSize);
int evoEngineLaunchSelected(void);

#endif
