/*
  Copyright 2009, Ifcaro & volca
  Licenced under Academic Free License version 3.0
  Review OpenUsbLd README & LICENSE files for further details.

  OPL Evolution standalone frontend engine.
*/

#include "include/evo_engine.h"
#include "include/opl.h"
#include "include/iosupport.h"
#include "include/config.h"
#include "include/ioman.h"
#include "include/themes.h"

#include <stdlib.h>
#include <string.h>

static item_list_t *evoPs2Support;
static int evoPs2Selection;
static int *evoCoverCacheId;
static int *evoCoverCacheUid;
static int evoCoverCacheCount;

static int evoEngineSupportCount(item_list_t *support)
{
    if (!support || !support->enabled || !support->itemGetCount)
        return 0;

    return support->itemGetCount(support);
}

static void evoEngineResetCoverCache(void)
{
    if (evoCoverCacheId && evoCoverCacheCount > 0)
        memset(evoCoverCacheId, -1, evoCoverCacheCount * sizeof(int));
    if (evoCoverCacheUid && evoCoverCacheCount > 0)
        memset(evoCoverCacheUid, -1, evoCoverCacheCount * sizeof(int));
}

static int evoEngineEnsureCoverCache(void)
{
    int count;

    if (!gTheme || gTheme->gameCacheCount <= 0)
        return 0;

    count = gTheme->gameCacheCount;
    if (count == evoCoverCacheCount && evoCoverCacheId && evoCoverCacheUid)
        return 1;

    free(evoCoverCacheId);
    free(evoCoverCacheUid);
    evoCoverCacheId = malloc(count * sizeof(int));
    evoCoverCacheUid = malloc(count * sizeof(int));
    if (!evoCoverCacheId || !evoCoverCacheUid) {
        free(evoCoverCacheId);
        free(evoCoverCacheUid);
        evoCoverCacheId = NULL;
        evoCoverCacheUid = NULL;
        evoCoverCacheCount = 0;
        return 0;
    }

    evoCoverCacheCount = count;
    evoEngineResetCoverCache();
    return 1;
}

void evoEngineInit(void)
{
    evoPs2Support = NULL;
    evoPs2Selection = 0;
    evoCoverCacheId = NULL;
    evoCoverCacheUid = NULL;
    evoCoverCacheCount = 0;
    evoEngineSync();
}

void evoEngineSync(void)
{
    item_list_t *support;
    int mode;
    int count;

    count = evoEngineSupportCount(evoPs2Support);
    if (count > 0) {
        if (evoPs2Selection >= count)
            evoPs2Selection = count - 1;
        return;
    }

    evoPs2Support = NULL;
    evoPs2Selection = 0;

    /*
     * Prefer directly attached BDM sources, then internal HDD and SMB.
     * These are engine objects only; no original-menu selection is involved.
     */
    for (mode = BDM_MODE; mode <= BDM_MODE4; mode++) {
        support = oplGetSupport(mode);
        if (evoEngineSupportCount(support) > 0) {
            evoPs2Support = support;
            evoEngineResetCoverCache();
            return;
        }
    }

    support = oplGetSupport(HDD_MODE);
    if (evoEngineSupportCount(support) > 0) {
        evoPs2Support = support;
        evoEngineResetCoverCache();
        return;
    }

    support = oplGetSupport(ETH_MODE);
    if (evoEngineSupportCount(support) > 0) {
        evoPs2Support = support;
        evoEngineResetCoverCache();
    }
}

int evoEngineHasPs2(void)
{
    evoEngineSync();
    return evoEngineSupportCount(evoPs2Support) > 0;
}

int evoEngineGetCount(void)
{
    evoEngineSync();
    return evoEngineSupportCount(evoPs2Support);
}

int evoEngineGetSelected(void)
{
    return evoPs2Selection;
}

const char *evoEngineGetName(int id)
{
    int count = evoEngineGetCount();

    if (!evoPs2Support || !evoPs2Support->itemGetName ||
        id < 0 || id >= count)
        return "";

    return evoPs2Support->itemGetName(evoPs2Support, id);
}

const char *evoEngineGetStartup(int id)
{
    int count = evoEngineGetCount();

    if (!evoPs2Support || !evoPs2Support->itemGetStartup ||
        id < 0 || id >= count)
        return "";

    return evoPs2Support->itemGetStartup(evoPs2Support, id);
}

const char *evoEngineGetSourceName(void)
{
    if (!evoPs2Support)
        return "No PS2 source";

    if (evoPs2Support->mode >= BDM_MODE &&
        evoPs2Support->mode <= BDM_MODE4)
        return "USB / BDM";
    if (evoPs2Support->mode == HDD_MODE)
        return "Internal HDD";
    if (evoPs2Support->mode == ETH_MODE)
        return "SMB Network";

    return "PS2";
}

void evoEngineMove(int direction)
{
    int count = evoEngineGetCount();

    if (count <= 0)
        return;

    if (direction < 0)
        evoPs2Selection = evoPs2Selection > 0 ?
                              evoPs2Selection - 1 :
                              count - 1;
    else
        evoPs2Selection = (evoPs2Selection + 1) % count;

    evoEngineResetCoverCache();
}

void evoEngineMovePage(int direction, int pageSize)
{
    int i;

    if (pageSize < 1)
        pageSize = 1;
    for (i = 0; i < pageSize; i++)
        evoEngineMove(direction);
}

void evoEngineRefresh(void)
{
    if (evoPs2Support && evoPs2Support->enabled)
        ioPutRequest(IO_MENU_UPDATE_DEFFERED, &evoPs2Support->mode);
}

int evoEngineDrawSelectedCover(int x, int y, int width, int height)
{
    if (!evoEngineHasPs2() || !evoEngineEnsureCoverCache())
        return 0;

    return thmDrawEvolutionItemCover(evoPs2Support, evoPs2Selection,
                                     evoCoverCacheId, evoCoverCacheUid,
                                     x, y, width, height);
}

int evoEngineLoadSelectedVMC(char *slot1, size_t slot1Size,
                             char *slot2, size_t slot2Size)
{
    config_set_t *configSet;

    if (slot1 && slot1Size)
        slot1[0] = '\0';
    if (slot2 && slot2Size)
        slot2[0] = '\0';

    evoEngineSync();
    if (!evoPs2Support || !evoPs2Support->itemGetConfig ||
        evoPs2Selection < 0 ||
        evoPs2Selection >= evoEngineSupportCount(evoPs2Support))
        return 0;

    configSet = evoPs2Support->itemGetConfig(evoPs2Support, evoPs2Selection);
    if (!configSet)
        return 0;

    if (slot1 && slot1Size)
        configGetVMC(configSet, slot1, slot1Size, 0);
    if (slot2 && slot2Size)
        configGetVMC(configSet, slot2, slot2Size, 1);
    configFree(configSet);
    return 1;
}

int evoEngineSaveSelectedVMC(const char *slot1, const char *slot2)
{
    config_set_t *configSet;
    int result;

    evoEngineSync();
    if (!evoPs2Support || !evoPs2Support->itemGetConfig ||
        evoPs2Selection < 0 ||
        evoPs2Selection >= evoEngineSupportCount(evoPs2Support))
        return 0;

    configSet = evoPs2Support->itemGetConfig(evoPs2Support, evoPs2Selection);
    if (!configSet)
        return 0;

    configSetVMC(configSet, slot1 ? slot1 : "", 0);
    configSetVMC(configSet, slot2 ? slot2 : "", 1);
    result = configWrite(configSet);
    configFree(configSet);
    return result;
}

int evoEngineCheckSelectedVMC(char *name, int createSize)
{
    evoEngineSync();
    if (!evoPs2Support || !evoPs2Support->itemCheckVMC || !name || !name[0])
        return -1;

    return evoPs2Support->itemCheckVMC(evoPs2Support, name, createSize);
}

int evoEngineLaunchSelected(void)
{
    config_set_t *configSet;
    int count;

    evoEngineSync();
    count = evoEngineSupportCount(evoPs2Support);
    if (!evoPs2Support || evoPs2Selection < 0 ||
        evoPs2Selection >= count)
        return -1;
    if (!evoPs2Support->itemGetConfig || !evoPs2Support->itemLaunch)
        return -2;

    configSet = evoPs2Support->itemGetConfig(evoPs2Support, evoPs2Selection);
    if (!configSet)
        return -3;

    evoPs2Support->itemLaunch(evoPs2Support, evoPs2Selection, configSet);

    /* A successful PS2 launch transfers control and never returns. */
    configFree(configSet);
    return -4;
}
