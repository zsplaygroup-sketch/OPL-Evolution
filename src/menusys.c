/*
  Copyright 2009, Ifcaro & volca
  Licenced under Academic Free License version 3.0
  Review OpenUsbLd README & LICENSE files for further details.
*/

#include "include/opl.h"
#include "include/menusys.h"
#include "include/iosupport.h"
#include "include/renderman.h"
#include "include/fntsys.h"
#include "include/lang.h"
#include "include/themes.h"
#include "include/pad.h"
#include "include/gui.h"
#include "include/guigame.h"
#include "include/system.h"
#include "include/ioman.h"
#include "include/sound.h"
#include <elf-loader.h>
#ifdef OPL_EVO
#include "include/evo_ui.h"
#endif
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>

enum MENU_IDs {
    MENU_SETTINGS = 0,
    MENU_GFX_SETTINGS,
    MENU_AUDIO_SETTINGS,
    MENU_CONTROLLER_SETTINGS,
    MENU_OSD_LANGUAGE_SETTINGS,
    MENU_PARENTAL_LOCK,
    MENU_NET_CONFIG,
    MENU_NET_UPDATE,
    MENU_START_NBD,
#ifdef OPL_EVO
    MENU_EVO_LIBRARY,
    MENU_EVO_ORIGINAL,
    MENU_EVO_CONTROLLERS,
    MENU_EVO_DIAGNOSTICS,
#endif
    MENU_ABOUT,
    MENU_SAVE_CHANGES,
    MENU_EXIT,
    MENU_POWER_OFF
};

enum GAME_MENU_IDs {
    GAME_COMPAT_SETTINGS = 0,
    GAME_CHEAT_SETTINGS,
    GAME_GSM_SETTINGS,
    GAME_VMC_SETTINGS,
#ifdef PADEMU
    GAME_PADEMU_SETTINGS,
    GAME_PADMACRO_SETTINGS,
#endif
    GAME_OSD_LANGUAGE_SETTINGS,
    GAME_SAVE_CHANGES,
    GAME_TEST_CHANGES,
    GAME_REMOVE_CHANGES,
    GAME_RENAME_GAME,
    GAME_DELETE_GAME,
};

// global menu variables
static menu_list_t *menu;
static menu_list_t *selected_item;

static int actionStatus;
static int itemConfigId;
static config_set_t *itemConfig;
static item_list_t *evolutionLaunchSupport;
static config_set_t *evolutionLaunchConfig;
static int evolutionLaunchId = -1;
static int evolutionLaunchOwnsConfig;

static u8 parentalLockCheckEnabled = 1;

// "main menu submenu"
static submenu_list_t *mainMenu;
// active item in the main menu
static submenu_list_t *mainMenuCurrent;

// "game settings submenu"
static submenu_list_t *gameMenu;
// active item in game settings
static submenu_list_t *gameMenuCurrent;

static submenu_list_t *appMenu;
static submenu_list_t *appMenuCurrent;

static s32 menuSemaId;
static s32 menuListSemaId = -1;
static ee_sema_t menuSema;

static void menuRenameGame(submenu_list_t **submenu)
{
    if (!selected_item->item->current) {
        return;
    }

    if (!gEnableWrite)
        return;

    item_list_t *support = selected_item->item->userdata;

    if (support) {
        if (support->itemRename) {
            if (menuCheckParentalLock() == 0) {
                sfxPlay(SFX_MESSAGE);
                int nameLength = support->itemGetNameLength(support, selected_item->item->current->item.id);
                char newName[nameLength];
                strncpy(newName, selected_item->item->current->item.text, nameLength);
                if (guiShowKeyboard(newName, nameLength)) {
                    guiSwitchScreen(GUI_SCREEN_MAIN);
                    submenuDestroy(submenu);

                    // Only rename the file if the name changed; trying to rename a file with a file name that hasn't changed can cause the file
                    // to be deleted on certain file systems.
                    if (strcmp(newName, selected_item->item->current->item.text) != 0) {
                        support->itemRename(support, selected_item->item->current->item.id, newName);
                        ioPutRequest(IO_MENU_UPDATE_DEFFERED, &support->mode);
                    }
                }
            }
        }
    } else
        guiMsgBox("NULL Support object. Please report", 0, NULL);
}

static void menuDeleteGame(submenu_list_t **submenu)
{
    if (!selected_item->item->current)
        return;

    if (!gEnableWrite)
        return;

    item_list_t *support = selected_item->item->userdata;

    if (support) {
        if (support->itemDelete) {
            if (menuCheckParentalLock() == 0) {
                if (guiMsgBox(_l(_STR_DELETE_WARNING), 1, NULL)) {
                    guiSwitchScreen(GUI_SCREEN_MAIN);
                    submenuDestroy(submenu);
                    support->itemDelete(support, selected_item->item->current->item.id);
                    ioPutRequest(IO_MENU_UPDATE_DEFFERED, &support->mode);
                }
            }
        }
    } else
        guiMsgBox("NULL Support object. Please report", 0, NULL);
}

static void _menuLoadConfig()
{
    WaitSema(menuSemaId);
    if (!itemConfig) {
        item_list_t *list = selected_item->item->userdata;
        itemConfig = list->itemGetConfig(list, itemConfigId);
    }
    actionStatus = 0;
    SignalSema(menuSemaId);
}

static void _menuSaveConfig()
{
    int result;

    WaitSema(menuSemaId);
    result = configWrite(itemConfig);
    itemConfigId = -1; // to invalidate cache and force reload
    actionStatus = 0;
    SignalSema(menuSemaId);

    if (!result)
        setErrorMessage(_STR_ERROR_SAVING_SETTINGS);
}

static void _menuRequestConfig()
{
    WaitSema(menuSemaId);
    if (selected_item->item->current != NULL && itemConfigId != selected_item->item->current->item.id) {
        if (itemConfig) {
            configFree(itemConfig);
            itemConfig = NULL;
        }
        item_list_t *list = selected_item->item->userdata;
        if (itemConfigId == -1 || guiInactiveFrames >= list->delay) {
            itemConfigId = selected_item->item->current->item.id;
            ioPutRequest(IO_CUSTOM_SIMPLEACTION, &_menuLoadConfig);
        }
    } else if (itemConfig)
        actionStatus = 0;

    SignalSema(menuSemaId);
}

config_set_t *menuLoadConfig()
{
    actionStatus = 1;
    itemConfigId = -1;
    guiHandleDeferedIO(&actionStatus, _l(_STR_LOADING_SETTINGS), IO_CUSTOM_SIMPLEACTION, &_menuRequestConfig);
    return itemConfig;
}

// we don't want a pop up when transitioning to or refreshing Game Menu gui.
config_set_t *gameMenuLoadConfig(struct UIItem *ui)
{
    actionStatus = 1;
    itemConfigId = -1;
    guiGameHandleDeferedIO(&actionStatus, ui, IO_CUSTOM_SIMPLEACTION, &_menuRequestConfig);
    return itemConfig;
}

void menuSaveConfig()
{
    actionStatus = 1;
    guiHandleDeferedIO(&actionStatus, _l(_STR_SAVING_SETTINGS), IO_CUSTOM_SIMPLEACTION, &_menuSaveConfig);
}

static void menuInitMainMenu(void)
{
    if (mainMenu)
        submenuDestroy(&mainMenu);

    // initialize the menu
    submenuAppendItem(&mainMenu, -1, NULL, MENU_SETTINGS, _STR_SETTINGS);
    submenuAppendItem(&mainMenu, -1, NULL, MENU_GFX_SETTINGS, _STR_GFX_SETTINGS);
    submenuAppendItem(&mainMenu, -1, NULL, MENU_AUDIO_SETTINGS, _STR_AUDIO_SETTINGS);
    submenuAppendItem(&mainMenu, -1, NULL, MENU_CONTROLLER_SETTINGS, _STR_CONTROLLER_SETTINGS);
    submenuAppendItem(&mainMenu, -1, NULL, MENU_OSD_LANGUAGE_SETTINGS, _STR_OSD_SETTINGS);
    submenuAppendItem(&mainMenu, -1, NULL, MENU_PARENTAL_LOCK, _STR_PARENLOCKCONFIG);
    submenuAppendItem(&mainMenu, -1, NULL, MENU_NET_CONFIG, _STR_NETCONFIG);
    submenuAppendItem(&mainMenu, -1, NULL, MENU_NET_UPDATE, _STR_NET_UPDATE);
    submenuAppendItem(&mainMenu, -1, NULL, MENU_START_NBD, _STR_STARTNBD);
#ifdef OPL_EVO
    static char evolutionLabel[] = "Enter OPL Evolution";
    static char originalLabel[] = "Original OPL Interface";
    static char controllersLabel[] = "Controllers - Evolution";
    static char diagnosticsLabel[] = "Diagnostics - Evolution";
    submenuAppendItem(&mainMenu, -1, evoUiIsEnabled() ? originalLabel : evolutionLabel,
                      evoUiIsEnabled() ? MENU_EVO_ORIGINAL : MENU_EVO_LIBRARY, -1);
    submenuAppendItem(&mainMenu, -1, controllersLabel, MENU_EVO_CONTROLLERS, -1);
    submenuAppendItem(&mainMenu, -1, diagnosticsLabel, MENU_EVO_DIAGNOSTICS, -1);
#endif
    submenuAppendItem(&mainMenu, -1, NULL, MENU_ABOUT, _STR_ABOUT);
    submenuAppendItem(&mainMenu, -1, NULL, MENU_SAVE_CHANGES, _STR_SAVE_CHANGES);
    submenuAppendItem(&mainMenu, -1, NULL, MENU_EXIT, _STR_EXIT);
    submenuAppendItem(&mainMenu, -1, NULL, MENU_POWER_OFF, _STR_POWEROFF);

    mainMenuCurrent = mainMenu;
}

void menuReinitMainMenu(void)
{
    menuInitMainMenu();
}

void menuInitGameMenu(void)
{
    if (gameMenu)
        submenuDestroy(&gameMenu);

    // initialize the menu
    submenuAppendItem(&gameMenu, -1, NULL, GAME_COMPAT_SETTINGS, _STR_COMPAT_SETTINGS);
    submenuAppendItem(&gameMenu, -1, NULL, GAME_CHEAT_SETTINGS, _STR_CHEAT_SETTINGS);
    submenuAppendItem(&gameMenu, -1, NULL, GAME_GSM_SETTINGS, _STR_GSCONFIG);
    submenuAppendItem(&gameMenu, -1, NULL, GAME_VMC_SETTINGS, _STR_VMC_SCREEN);
#ifdef PADEMU
    submenuAppendItem(&gameMenu, -1, NULL, GAME_PADEMU_SETTINGS, _STR_PADEMUCONFIG);
    submenuAppendItem(&gameMenu, -1, NULL, GAME_PADMACRO_SETTINGS, _STR_PADMACROCONFIG);
#endif
    submenuAppendItem(&gameMenu, -1, NULL, GAME_OSD_LANGUAGE_SETTINGS, _STR_OSD_SETTINGS);
    submenuAppendItem(&gameMenu, -1, NULL, GAME_SAVE_CHANGES, _STR_SAVE_CHANGES);
    submenuAppendItem(&gameMenu, -1, NULL, GAME_TEST_CHANGES, _STR_TEST);
    submenuAppendItem(&gameMenu, -1, NULL, GAME_REMOVE_CHANGES, _STR_REMOVE_ALL_SETTINGS);
    if (gEnableWrite) {
        submenuAppendItem(&gameMenu, -1, NULL, GAME_RENAME_GAME, _STR_RENAME);
        submenuAppendItem(&gameMenu, -1, NULL, GAME_DELETE_GAME, _STR_DELETE);
    }

    gameMenuCurrent = gameMenu;
}

void menuInitAppMenu(void)
{
    if (appMenu)
        submenuDestroy(&appMenu);

    // initialize the menu
    submenuAppendItem(&appMenu, -1, NULL, 0, _STR_RENAME);
    submenuAppendItem(&appMenu, -1, NULL, 1, _STR_DELETE);

    appMenuCurrent = appMenu;
}

// -------------------------------------------------------------------------------------------
// ---------------------------------------- Menu manipulation --------------------------------
// -------------------------------------------------------------------------------------------
void menuInit()
{
    menu = NULL;
    selected_item = NULL;
    itemConfigId = -1;
    itemConfig = NULL;
    mainMenu = NULL;
    mainMenuCurrent = NULL;
    gameMenu = NULL;
    gameMenuCurrent = NULL;
    appMenu = NULL;
    appMenuCurrent = NULL;
    menuInitMainMenu();

    menuSema.init_count = 1;
    menuSema.max_count = 1;
    menuSema.option = 0;
    menuSemaId = CreateSema(&menuSema);
    if (menuListSemaId < 0) {
        menuListSemaId = sbCreateSemaphore();
    }
}

void menuEnd()
{
    // destroy menu
    menu_list_t *cur = menu;

    while (cur) {
        menu_list_t *td = cur;
        cur = cur->next;

        if (td->item)
            submenuDestroy(&(td->item->submenu));

        menuRemoveHints(td->item);

        free(td);
    }

    submenuDestroy(&mainMenu);
    submenuDestroy(&gameMenu);
    submenuDestroy(&appMenu);

    if (itemConfig) {
        configFree(itemConfig);
        itemConfig = NULL;
    }

    DeleteSema(menuSemaId);
    DeleteSema(menuListSemaId);
    menuListSemaId = -1;
}

static menu_list_t *AllocMenuItem(menu_item_t *item)
{
    menu_list_t *it;

    it = malloc(sizeof(menu_list_t));

    it->prev = NULL;
    it->next = NULL;
    it->item = item;

    return it;
}

void menuAppendItem(menu_item_t *item)
{
    assert(item);

    WaitSema(menuListSemaId);

    if (menu == NULL) {
        menu = AllocMenuItem(item);
        selected_item = menu;
    } else {
        menu_list_t *cur = menu;

        // traverse till the end
        while (cur->next)
            cur = cur->next;

        // create new item
        menu_list_t *newitem = AllocMenuItem(item);

        // link
        cur->next = newitem;
        newitem->prev = cur;
    }

    SignalSema(menuListSemaId);
}

static void refreshMenuPosition(void)
{
    // Find the first menu in the list that is visible and set it as the active menu.
    if (menu == NULL)
        return;

    menu_list_t *cur = menu;
    while (cur->item->visible == 0 && cur->next)
        cur = cur->next;

    if (cur->item->visible == 0) {
        // No visible menu was found, just set the current menu to the first one in the list.
        selected_item = menu;
    } else
        selected_item = cur;
}

void submenuRebuildCache(submenu_list_t *submenu)
{
    while (submenu) {
        if (submenu->item.cache_id)
            free(submenu->item.cache_id);
        if (submenu->item.cache_uid)
            free(submenu->item.cache_uid);

        int size = gTheme->gameCacheCount * sizeof(int);
        submenu->item.cache_id = malloc(size);
        memset(submenu->item.cache_id, -1, size);
        submenu->item.cache_uid = malloc(size);
        memset(submenu->item.cache_uid, -1, size);

        submenu = submenu->next;
    }
}

static submenu_list_t *submenuAllocItem(int icon_id, char *text, int id, int text_id)
{
    submenu_list_t *it = (submenu_list_t *)malloc(sizeof(submenu_list_t));

    it->prev = NULL;
    it->next = NULL;
    it->item.icon_id = icon_id;
    it->item.text = text;
    it->item.text_id = text_id;
    it->item.id = id;
    it->item.cache_id = NULL;
    it->item.cache_uid = NULL;
    submenuRebuildCache(it);

    return it;
}

submenu_list_t *submenuAppendItem(submenu_list_t **submenu, int icon_id, char *text, int id, int text_id)
{
    if (*submenu == NULL) {
        *submenu = submenuAllocItem(icon_id, text, id, text_id);
        return *submenu;
    }

    submenu_list_t *cur = *submenu;

    // traverse till the end
    while (cur->next)
        cur = cur->next;

    // create new item
    submenu_list_t *newitem = submenuAllocItem(icon_id, text, id, text_id);

    // link
    cur->next = newitem;
    newitem->prev = cur;

    return newitem;
}

static void submenuDestroyItem(submenu_list_t *submenu)
{
    free(submenu->item.cache_id);
    free(submenu->item.cache_uid);

    free(submenu);
}

void submenuRemoveItem(submenu_list_t **submenu, int id)
{
    submenu_list_t *cur = *submenu;
    submenu_list_t *prev = NULL;

    while (cur) {
        if (cur->item.id == id) {
            submenu_list_t *next = cur->next;

            if (prev)
                prev->next = cur->next;

            if (*submenu == cur)
                *submenu = next;

            submenuDestroyItem(cur);

            cur = next;
        } else {
            prev = cur;
            cur = cur->next;
        }
    }
}

void submenuDestroy(submenu_list_t **submenu)
{
    // destroy sub menu
    submenu_list_t *cur = *submenu;

    while (cur) {
        submenu_list_t *td = cur;
        cur = cur->next;

        submenuDestroyItem(td);
    }

    *submenu = NULL;
}

void menuAddHint(menu_item_t *menu, int text_id, int icon_id)
{
    // allocate a new hint item
    menu_hint_item_t *hint = malloc(sizeof(menu_hint_item_t));

    hint->text_id = text_id;
    hint->icon_id = icon_id;
    hint->next = NULL;

    if (menu->hints) {
        menu_hint_item_t *top = menu->hints;

        // rewind to end
        for (; top->next; top = top->next)
            ;

        top->next = hint;
    } else {
        menu->hints = hint;
    }
}

void menuRemoveHints(menu_item_t *menu)
{
    while (menu->hints) {
        menu_hint_item_t *hint = menu->hints;
        menu->hints = hint->next;
        free(hint);
    }
}

char *menuItemGetText(menu_item_t *it)
{
    if (it->text_id >= 0)
        return _l(it->text_id);
    else
        return it->text;
}

char *submenuItemGetText(submenu_item_t *it)
{
    if (it->text_id >= 0)
        return _l(it->text_id);
    else
        return it->text;
}

static void swap(submenu_list_t *a, submenu_list_t *b)
{
    submenu_list_t *pa, *nb;
    pa = a->prev;
    nb = b->next;

    a->next = nb;
    b->prev = pa;
    b->next = a;
    a->prev = b;

    if (pa)
        pa->next = b;

    if (nb)
        nb->prev = a;
}

// Sorts the given submenu by comparing the on-screen titles
void submenuSort(submenu_list_t **submenu)
{
    // a simple bubblesort
    // *submenu = mergeSort(*submenu);
    submenu_list_t *head;
    int sorted = 0;

    if ((submenu == NULL) || (*submenu == NULL) || ((*submenu)->next == NULL))
        return;

    head = *submenu;

    while (!sorted) {
        sorted = 1;

        submenu_list_t *tip = head;

        while (tip->next) {
            submenu_list_t *nxt = tip->next;

            char *txt1 = submenuItemGetText(&tip->item);
            char *txt2 = submenuItemGetText(&nxt->item);

            int cmp = strcasecmp(txt1, txt2);

            if (cmp > 0) {
                swap(tip, nxt);

                if (tip == head)
                    head = nxt;

                sorted = 0;
            } else {
                tip = tip->next;
            }
        }
    }

    *submenu = head;
}

static void menuNextH()
{
    struct menu_list *next = selected_item->next;
    while (next != NULL && next->item->visible == 0)
        next = next->next;

    // If we found a valid menu transition to it.
    if (next != NULL) {
        selected_item = next;
        itemConfigId = -1;
        sfxPlay(SFX_CURSOR);
    }
}

static void menuPrevH()
{
    struct menu_list *prev = selected_item->prev;
    while (prev != NULL && prev->item->visible == 0)
        prev = prev->prev;

    if (prev != NULL) {
        selected_item = prev;
        itemConfigId = -1;
        sfxPlay(SFX_CURSOR);
    }
}

static void menuFirstPage()
{
    submenu_list_t *cur = selected_item->item->current;
    if (cur) {
        if (cur->prev) {
            sfxPlay(SFX_CURSOR);
        }

        selected_item->item->current = selected_item->item->submenu;
        selected_item->item->pagestart = selected_item->item->current;
    }
}

static void menuLastPage()
{
    submenu_list_t *cur = selected_item->item->current;
    if (cur) {
        if (cur->next) {
            sfxPlay(SFX_CURSOR);
        }
        while (cur->next)
            cur = cur->next; // go to end

        selected_item->item->current = cur;

        int itms = ((items_list_t *)gTheme->itemsList->extended)->displayedItems;
        while (--itms && cur->prev) // and move back to have a full page
            cur = cur->prev;

        selected_item->item->pagestart = cur;
    }
}

static void menuNextV()
{
    submenu_list_t *cur = selected_item->item->current;

    if (cur && cur->next) {
        selected_item->item->current = cur->next;
        sfxPlay(SFX_CURSOR);

        // if the current item is beyond the page start, move the page start one page down
        cur = selected_item->item->pagestart;
        int itms = ((items_list_t *)gTheme->itemsList->extended)->displayedItems + 1;
        while (--itms && cur)
            if (selected_item->item->current == cur)
                return;
            else
                cur = cur->next;

        selected_item->item->pagestart = selected_item->item->current;
    } else { // wrap to start
        menuFirstPage();
    }
}

static void menuPrevV()
{
    submenu_list_t *cur = selected_item->item->current;

    if (cur && cur->prev) {
        selected_item->item->current = cur->prev;
        sfxPlay(SFX_CURSOR);

        // if the current item is on the page start, move the page start one page up
        if (selected_item->item->pagestart == cur) {
            int itms = ((items_list_t *)gTheme->itemsList->extended)->displayedItems + 1; // +1 because the selection will move as well
            while (--itms && selected_item->item->pagestart->prev)
                selected_item->item->pagestart = selected_item->item->pagestart->prev;
        }
    } else { // wrap to end
        menuLastPage();
    }
}

static void menuNextPage()
{
    submenu_list_t *cur = selected_item->item->pagestart;

    if (cur && cur->next) {
        int itms = ((items_list_t *)gTheme->itemsList->extended)->displayedItems + 1;
        sfxPlay(SFX_CURSOR);

        while (--itms && cur->next)
            cur = cur->next;

        selected_item->item->current = cur;
        selected_item->item->pagestart = selected_item->item->current;
    } else { // wrap to start
        menuFirstPage();
    }
}

static void menuPrevPage()
{
    submenu_list_t *cur = selected_item->item->pagestart;

    if (cur && cur->prev) {
        int itms = ((items_list_t *)gTheme->itemsList->extended)->displayedItems + 1;
        sfxPlay(SFX_CURSOR);

        while (--itms && cur->prev)
            cur = cur->prev;

        selected_item->item->current = cur;
        selected_item->item->pagestart = selected_item->item->current;
    } else { // wrap to end
        menuLastPage();
    }
}

void menuSetSelectedItem(menu_item_t *item)
{
    menu_list_t *itm = menu;

    while (itm) {
        if (itm->item == item) {
            selected_item = itm;
            return;
        }

        itm = itm->next;
    }
}

void menuRenderMenu()
{
    guiDrawBGPlasma();

    if (!mainMenu)
        return;

    // draw the animated menu
    if (!mainMenuCurrent)
        mainMenuCurrent = mainMenu;

    submenu_list_t *it = mainMenu;

    // calculate the number of items
    int count = 0;
    int sitem = 0;
    for (; it; count++, it = it->next) {
        if (it == mainMenuCurrent)
            sitem = count;
    }

    int spacing = 25;
    int y = (gTheme->usedHeight >> 1) - (spacing * (count >> 1));
    int cp = 0; // current position
    for (it = mainMenu; it; it = it->next, cp++) {
        // render, advance
        fntRenderString(gTheme->fonts[0], 320, y, ALIGN_CENTER, 0, 0, submenuItemGetText(&it->item), (cp == sitem) ? gTheme->selTextColor : gTheme->textColor);
        y += spacing;
        if (it->next && it->next->item.id == MENU_ABOUT)
            y += spacing / 2;
    }

    // hints
    guiDrawSubMenuHints();
}

int menuSetParentalLockCheckState(int enabled)
{
    int wasEnabled;

    wasEnabled = parentalLockCheckEnabled;
    parentalLockCheckEnabled = enabled ? 1 : 0;

    return wasEnabled;
}

int menuCheckParentalLock(void)
{
    const char *parentalLockPassword;
    char password[CONFIG_KEY_VALUE_LEN];
    int result;

    result = 0; // Default to unlocked.
    if (parentalLockCheckEnabled) {
        config_set_t *configOPL = configGetByType(CONFIG_OPL);

        // Prompt for password, only if one was set.
        if (configGetStr(configOPL, CONFIG_OPL_PARENTAL_LOCK_PWD, &parentalLockPassword) && (parentalLockPassword[0] != '\0')) {
            password[0] = '\0';
            if (diaShowKeyb(password, CONFIG_KEY_VALUE_LEN, 1, _l(_STR_PARENLOCK_ENTER_PASSWORD_TITLE))) {
                if (strncmp(parentalLockPassword, password, CONFIG_KEY_VALUE_LEN) == 0) {
                    result = 0;
                    parentalLockCheckEnabled = 0; // Stop asking for the password.
                } else if (strncmp(OPL_PARENTAL_LOCK_MASTER_PASS, password, CONFIG_KEY_VALUE_LEN) == 0) {
                    guiMsgBox(_l(_STR_PARENLOCK_DISABLE_WARNING), 0, NULL);

                    configRemoveKey(configOPL, CONFIG_OPL_PARENTAL_LOCK_PWD);
                    saveConfig(CONFIG_OPL, 1);

                    result = 0;
                    parentalLockCheckEnabled = 0; // Stop asking for the password.
                } else {
                    guiMsgBox(_l(_STR_PARENLOCK_PASSWORD_INCORRECT), 0, NULL);
                    result = EACCES;
                }
            } else // User aborted.
                result = EACCES;
        }
    }

    return result;
}

void menuHandleInputMenu()
{
    if (!mainMenu)
        return;

    if (!mainMenuCurrent)
        mainMenuCurrent = mainMenu;

    if (getKey(KEY_UP)) {
        sfxPlay(SFX_CURSOR);
        if (mainMenuCurrent->prev)
            mainMenuCurrent = mainMenuCurrent->prev;
        else // rewind to the last item
            while (mainMenuCurrent->next)
                mainMenuCurrent = mainMenuCurrent->next;
    }

    if (getKey(KEY_DOWN)) {
        sfxPlay(SFX_CURSOR);
        if (mainMenuCurrent->next)
            mainMenuCurrent = mainMenuCurrent->next;
        else
            mainMenuCurrent = mainMenu;
    }

    if (getKeyOn(gSelectButton)) {
        // execute the item via looking at the id of it
        int id = mainMenuCurrent->item.id;

        sfxPlay(SFX_CONFIRM);

        if (id == MENU_SETTINGS) {
            if (menuCheckParentalLock() == 0)
                guiShowConfig();
        } else if (id == MENU_GFX_SETTINGS) {
            if (menuCheckParentalLock() == 0)
                guiShowUIConfig();
        } else if (id == MENU_AUDIO_SETTINGS) {
            if (menuCheckParentalLock() == 0)
                guiShowAudioConfig();
        } else if (id == MENU_CONTROLLER_SETTINGS) {
            if (menuCheckParentalLock() == 0)
                guiShowControllerConfig();
        } else if (id == MENU_OSD_LANGUAGE_SETTINGS) {
            if (menuCheckParentalLock() == 0)
                guiGameShowOSDLanguageConfig(1);
        } else if (id == MENU_PARENTAL_LOCK) {
            if (menuCheckParentalLock() == 0)
                guiShowParentalLockConfig();
        } else if (id == MENU_NET_CONFIG) {
            if (menuCheckParentalLock() == 0)
                guiShowNetConfig();
        } else if (id == MENU_NET_UPDATE) {
            if (menuCheckParentalLock() == 0)
                guiShowNetCompatUpdate();
        } else if (id == MENU_START_NBD) {
            if (menuCheckParentalLock() == 0)
                handleLwnbdSrv();
#ifdef OPL_EVO
        } else if (id == MENU_EVO_LIBRARY) {
            evoUiSetEnabled(1);
            guiSwitchScreen(GUI_SCREEN_MAIN);
            refreshMenuPosition();
        } else if (id == MENU_EVO_ORIGINAL) {
            evoUiSetEnabled(0);
            guiSwitchScreen(GUI_SCREEN_MAIN);
            refreshMenuPosition();
        } else if (id == MENU_EVO_CONTROLLERS) {
            guiSwitchScreen(GUI_SCREEN_EVO_CONTROLLERS);
        } else if (id == MENU_EVO_DIAGNOSTICS) {
            guiSwitchScreen(GUI_SCREEN_EVO_DIAGNOSTICS);
#endif
        } else if (id == MENU_ABOUT) {
            guiShowAbout();
        } else if (id == MENU_SAVE_CHANGES) {
            if (menuCheckParentalLock() == 0) {
                guiGameSaveOSDLanguageGlobalConfig(configGetByType(CONFIG_GAME));
#ifdef PADEMU
                guiGameSavePadEmuGlobalConfig(configGetByType(CONFIG_GAME));
                guiGameSavePadMacroGlobalConfig(configGetByType(CONFIG_GAME));
#endif
                saveConfig(CONFIG_OPL | CONFIG_NETWORK | CONFIG_GAME, 1);
                menuSetParentalLockCheckState(1); // Re-enable parental lock check.
            }
        } else if (id == MENU_EXIT) {
            if (guiMsgBox(_l(_STR_CONFIRMATION_EXIT), 1, NULL))
                sysExecExit();
        } else if (id == MENU_POWER_OFF) {
            if (guiMsgBox(_l(_STR_CONFIRMATION_POFF), 1, NULL))
                sysPowerOff();
        }

        // so the exit press wont propagate twice
        readPads();
    }

    if (getKeyOn(KEY_START) || getKeyOn(gSelectButton == KEY_CIRCLE ? KEY_CROSS : KEY_CIRCLE)) {
        // Check if there is anything to show the user, at all.
        if (gAPPStartMode || gETHStartMode || gBDMStartMode || gHDDStartMode) {
            guiSwitchScreen(GUI_SCREEN_MAIN);
            refreshMenuPosition();
        }
    }
}

static void menuRenderElements(theme_element_t *elem)
{
    // selected_item can't be NULL here as we only allow to switch to "Main" rendering when there is at least one device activated
    _menuRequestConfig();

    WaitSema(menuSemaId);

    while (elem) {
        if (elem->drawElem)
            elem->drawElem(selected_item, selected_item->item->current, itemConfig, elem);

        elem = elem->next;
    }
    SignalSema(menuSemaId);
}

void menuRenderMain(void)
{
    item_list_t *list = selected_item->item->userdata;

    if (list->mode == APP_MODE) {
        menuRenderElements(gTheme->appsMainElems.first);
        gTheme->itemsList = gTheme->appsItemsList;
    } else {
        menuRenderElements(gTheme->mainElems.first);
        gTheme->itemsList = gTheme->gamesItemsList;
    }
}

void menuHandleInputMain()
{
    if (getKey(KEY_LEFT)) {
        menuPrevH();
    } else if (getKey(KEY_RIGHT)) {
        menuNextH();
    } else if (getKey(KEY_UP)) {
        menuPrevV();
    } else if (getKey(KEY_DOWN)) {
        menuNextV();
    } else if (getKeyOn(KEY_CROSS)) {
        selected_item->item->execCross(selected_item->item);
    } else if (getKeyOn(KEY_TRIANGLE)) {
        selected_item->item->execTriangle(selected_item->item);
    } else if (getKeyOn(KEY_CIRCLE)) {
        selected_item->item->execCircle(selected_item->item);
    } else if (getKeyOn(KEY_SQUARE)) {
        selected_item->item->execSquare(selected_item->item);
    } else if (getKeyOn(KEY_START)) {
        // reinit main menu - show/hide items valid in the active context
        menuInitMainMenu();
        guiSwitchScreen(GUI_SCREEN_MENU);
    } else if (getKeyOn(KEY_SELECT)) {
        selected_item->item->refresh(selected_item->item);
    } else if (getKey(KEY_L1)) {
        menuPrevPage();
    } else if (getKey(KEY_R1)) {
        menuNextPage();
    } else if (getKeyOn(KEY_L2)) { // home
        menuFirstPage();
    } else if (getKeyOn(KEY_R2)) { // end
        menuLastPage();
    }

    // Last Played Auto Start
    if (RemainSecs < 0) {
        DisableCron = 1; // Disable Counter
        if (gSelectButton == KEY_CIRCLE)
            selected_item->item->execCircle(selected_item->item);
        else
            selected_item->item->execCross(selected_item->item);
    }
}

void menuGetSelectionSummary(char *source, size_t sourceSize, char *title, size_t titleSize)
{
    const char *sourceText = "No active source";
    const char *titleText = "No item selected";

    if (selected_item && selected_item->item) {
        sourceText = menuItemGetText(selected_item->item);
        if (selected_item->item->current)
            titleText = submenuItemGetText(&selected_item->item->current->item);
    }

    if (source && sourceSize > 0)
        snprintf(source, sourceSize, "%s", sourceText);
    if (title && titleSize > 0)
        snprintf(title, titleSize, "%s", titleText);
}

#ifdef OPL_EVO
void menuEvolutionRenderList(int x, int y, int width, int rowHeight, int rows)
{
    submenu_list_t *it;
    int before;
    int row;

    if (!selected_item || !selected_item->item || !selected_item->item->current) {
        fntRenderString(FNT_DEFAULT, x, y, ALIGN_NONE, width, rowHeight,
                        "No items available", gTheme->textColor);
        return;
    }

    WaitSema(menuSemaId);
    it = selected_item->item->current;
    before = rows / 2;
    while (before-- > 0 && it->prev)
        it = it->prev;

    for (row = 0; row < rows && it; row++, it = it->next) {
        const int rowY = y + row * rowHeight;
        const int selected = it == selected_item->item->current;
        const int hasThumbnail = rowHeight >= 38;
        const int thumbnailWidth = rowHeight >= 41 ? 45 : 39;
        const int textX = hasThumbnail ? x + thumbnailWidth + 7 : x;
        const int textWidth = hasThumbnail ? width - thumbnailWidth - 7 : width;

        if (selected) {
            rmDrawRect(x - 6, rowY - 3, width + 12, rowHeight, GS_SETREG_RGBA(0x02, 0x0C, 0x1C, 0x80));
            rmDrawLine(x - 6, rowY - 3, x + width + 6, rowY - 3, GS_SETREG_RGBA(0x00, 0x94, 0xFF, 0x80));
            rmDrawLine(x - 6, rowY + rowHeight - 3, x + width + 6, rowY + rowHeight - 3, GS_SETREG_RGBA(0x00, 0x94, 0xFF, 0x80));
            rmDrawLine(x - 6, rowY - 3, x - 6, rowY + rowHeight - 3, GS_SETREG_RGBA(0x00, 0x94, 0xFF, 0x80));
            rmDrawLine(x + width + 6, rowY - 3, x + width + 6, rowY + rowHeight - 3, GS_SETREG_RGBA(0x00, 0x94, 0xFF, 0x80));
        }

        if (hasThumbnail) {
            rmDrawRect(x, rowY, thumbnailWidth, rowHeight - 6, GS_SETREG_RGBA(0x02, 0x0C, 0x1C, 0x80));
            thmDrawEvolutionCover(selected_item, it, x, rowY, thumbnailWidth, rowHeight - 6);
        }

        fntRenderString(FNT_DEFAULT, textX, rowY + (hasThumbnail ? 7 : 0), ALIGN_NONE, textWidth, rowHeight,
                        submenuItemGetText(&it->item),
                        selected ? GS_SETREG_RGBA(0xD6, 0xE0, 0xEF, 0x80) : GS_SETREG_RGBA(0x67, 0x7A, 0x98, 0x80));
    }
    SignalSema(menuSemaId);
}

int menuEvolutionRenderCover(int x, int y, int width, int height)
{
    int result = 0;

    if (!selected_item || !selected_item->item || !selected_item->item->current)
        return 0;

    WaitSema(menuSemaId);
    result = thmDrawEvolutionCover(selected_item, selected_item->item->current, x, y, width, height);
    SignalSema(menuSemaId);
    return result;
}

int menuEvolutionGetDetails(char *startup, size_t startupSize, int *position, int *total)
{
    submenu_list_t *it;
    item_list_t *list;
    int index = 0;
    int count = 0;

    if (startup && startupSize > 0)
        startup[0] = '\0';
    if (position)
        *position = 0;
    if (total)
        *total = 0;

    if (!selected_item || !selected_item->item || !selected_item->item->current)
        return 0;

    list = (item_list_t *)selected_item->item->userdata;
    if (startup && startupSize > 0 && list && list->itemGetStartup) {
        const char *value = list->itemGetStartup(list, selected_item->item->current->item.id);
        if (value)
            snprintf(startup, startupSize, "%s", value);
    }

    it = selected_item->item->submenu;
    while (it) {
        count++;
        if (it == selected_item->item->current)
            index = count;
        it = it->next;
    }

    if (position)
        *position = index;
    if (total)
        *total = count;
    return 1;
}

void menuEvolutionMoveItem(int direction)
{
    submenu_list_t *cur;

    if (!selected_item || !selected_item->item || !selected_item->item->current)
        return;

    cur = selected_item->item->current;
    if (direction < 0) {
        if (cur->prev) {
            selected_item->item->current = cur->prev;
        } else {
            while (cur->next)
                cur = cur->next;
            selected_item->item->current = cur;
        }
    } else {
        selected_item->item->current = cur->next ? cur->next : selected_item->item->submenu;
    }
    selected_item->item->pagestart = selected_item->item->current;
    itemConfigId = -1;
    sfxPlay(SFX_CURSOR);
}

void menuEvolutionMoveSource(int direction)
{
    menu_list_t *it;
    item_list_t *currentList;
    int wantApps;

    if (!selected_item || !selected_item->item)
        return;

    currentList = (item_list_t *)selected_item->item->userdata;
    wantApps = currentList && currentList->mode == APP_MODE;
    it = direction < 0 ? selected_item->prev : selected_item->next;

    while (it) {
        item_list_t *list = (item_list_t *)it->item->userdata;
        const int isApps = list && list->mode == APP_MODE;

        if (it->item->visible && isApps == wantApps) {
            selected_item = it;
            itemConfigId = -1;
            sfxPlay(SFX_CURSOR);
            return;
        }
        it = direction < 0 ? it->prev : it->next;
    }
}

void menuEvolutionMovePage(int direction)
{
    int count = 6;

    while (count-- > 0)
        menuEvolutionMoveItem(direction);
}

void menuEvolutionActivate(void)
{
    if (!selected_item || !selected_item->item || !selected_item->item->current)
        return;

    if (gSelectButton == KEY_CIRCLE)
        selected_item->item->execCircle(selected_item->item);
    else
        selected_item->item->execCross(selected_item->item);
}

void menuEvolutionPrimeConfig(void)
{
    if (selected_item && selected_item->item && selected_item->item->current)
        _menuRequestConfig();
}

int menuEvolutionLaunchDirect(void)
{
    item_list_t *support;
    config_set_t *configSet;
    int id;
    int ownsConfig = 0;

    if (!selected_item || !selected_item->item || !selected_item->item->current)
        return -1;

    support = (item_list_t *)selected_item->item->userdata;
    if (!support || !support->enabled || !support->itemGetConfig || !support->itemLaunch)
        return -2;

    id = selected_item->item->current->item.id;
    configSet = NULL;

    /*
     * Match original OPL's normal path: use the configuration that its menu
     * preload routine populated while the title was selected.  Only fall back
     * to a direct read if Confirm arrives before that preload completes.
     */
    WaitSema(menuSemaId);
    if (itemConfigId == id && itemConfig != NULL)
        configSet = itemConfig;
    SignalSema(menuSemaId);

    if (!configSet) {
        configSet = support->itemGetConfig(support, id);
        ownsConfig = 1;
    }
    if (!configSet)
        return -3;

    support->itemLaunch(support, id, configSet);

    /* Successful PS2 launches do not return. */
    if (ownsConfig)
        configFree(configSet);
    return -4;
}

void menuEvolutionCancelPreparedLaunch(void)
{
    if (evolutionLaunchOwnsConfig && evolutionLaunchConfig)
        configFree(evolutionLaunchConfig);

    evolutionLaunchSupport = NULL;
    evolutionLaunchConfig = NULL;
    evolutionLaunchId = -1;
    evolutionLaunchOwnsConfig = 0;
}

int menuEvolutionPrepareLaunch(void)
{
    item_list_t *support;
    config_set_t *configSet = NULL;
    int id;
    int ownsConfig = 0;

    menuEvolutionCancelPreparedLaunch();

    if (!selected_item || !selected_item->item || !selected_item->item->current)
        return -1;

    support = (item_list_t *)selected_item->item->userdata;
    if (!support || !support->enabled || !support->itemGetConfig || !support->itemLaunch)
        return -2;

    id = selected_item->item->current->item.id;
    WaitSema(menuSemaId);
    if (itemConfigId == id && itemConfig != NULL)
        configSet = itemConfig;
    SignalSema(menuSemaId);

    if (!configSet) {
        configSet = support->itemGetConfig(support, id);
        ownsConfig = 1;
    }
    if (!configSet)
        return -3;

    evolutionLaunchSupport = support;
    evolutionLaunchConfig = configSet;
    evolutionLaunchId = id;
    evolutionLaunchOwnsConfig = ownsConfig;
    return 0;
}

int menuEvolutionCommitLaunch(void)
{
    if (!evolutionLaunchSupport || !evolutionLaunchConfig || evolutionLaunchId < 0)
        return -1;

    evolutionLaunchSupport->itemLaunch(evolutionLaunchSupport,
                                       evolutionLaunchId,
                                       evolutionLaunchConfig);

    /* Successful PS2 launches do not return. */
    menuEvolutionCancelPreparedLaunch();
    return -2;
}

static int evolutionTryBootPath(char *resolved, size_t resolvedSize,
                                const char *candidate)
{
    int fd;

    fd = open(candidate, O_RDONLY);
    if (fd < 0)
        return 0;

    close(fd);
    snprintf(resolved, resolvedSize, "%s", candidate);
    return 1;
}

static int evolutionResolveBootPath(char *resolved, size_t resolvedSize)
{
    char candidate[256];
    const char *path;
    const char *suffix;
    const char *baseName;
    int unit;

    if (!resolved || resolvedSize == 0 || gEvolutionBootPath[0] == '\0')
        return 0;

    path = gEvolutionBootPath;

    /*
     * wLaunchELF and OPL may report the first USB device as mass:/ or mass?:/.
     * Resolve that alias while the BDM modules are still mounted.  The ELF
     * loader is then given the concrete unit that was proven readable.
     */
    if (strncmp(path, "mass:", 5) == 0 ||
        strncmp(path, "mass?:", 6) == 0) {
        suffix = strchr(path, ':');
        if (suffix) {
            suffix++;
            for (unit = 0; unit <= 4; unit++) {
                snprintf(candidate, sizeof(candidate), "mass%d:%s",
                         unit, suffix);
                if (evolutionTryBootPath(resolved, resolvedSize, candidate))
                    return 1;
            }
        }
    } else if (evolutionTryBootPath(resolved, resolvedSize, path)) {
        return 1;
    }

    /*
     * Some ELF launchers pass only a filename.  The test build is deployed in
     * the USB root, so search each mounted mass unit for that basename.
     */
    baseName = strrchr(path, '/');
    baseName = baseName ? baseName + 1 : path;
    if (baseName[0] != '\0') {
        for (unit = 0; unit <= 4; unit++) {
            snprintf(candidate, sizeof(candidate), "mass%d:/%s",
                     unit, baseName);
            if (evolutionTryBootPath(resolved, resolvedSize, candidate))
                return 1;
        }
    }

    return 0;
}

int menuEvolutionLaunchTrampoline(void)
{
    char bootPath[256];
    char gameName[192];
    char startup[32];
    char media[16];
    char format[16];
    char partition[2] = "";
    char *argv[4];
    int mode;

    if (!evolutionLaunchSupport || !evolutionLaunchConfig || evolutionLaunchId < 0)
        return -1;
    if (evolutionLaunchSupport->mode < BDM_MODE ||
        evolutionLaunchSupport->mode > BDM_MODE4)
        return -2;
    if (!evolutionResolveBootPath(bootPath, sizeof(bootPath)))
        return -3;

    if (!configGetStrCopy(evolutionLaunchConfig, CONFIG_ITEM_NAME,
                          gameName, sizeof(gameName)) ||
        !configGetStrCopy(evolutionLaunchConfig, CONFIG_ITEM_STARTUP,
                          startup, sizeof(startup)) ||
        !configGetStrCopy(evolutionLaunchConfig, CONFIG_ITEM_MEDIA,
                          media, sizeof(media)) ||
        !configGetStrCopy(evolutionLaunchConfig, CONFIG_ITEM_FORMAT,
                          format, sizeof(format)))
        return -4;

    if (strcasecmp(format, "ISO") == 0)
        strncat(gameName, ".iso", sizeof(gameName) - strlen(gameName) - 1);
    else if (strcasecmp(format, "ZSO") == 0)
        strncat(gameName, ".zso", sizeof(gameName) - strlen(gameName) - 1);
    else
        return -5; /* OPL's clean BDM auto-launch does not support UL games. */

    mode = oplPath2Mode(bootPath);
    if (mode < BDM_MODE || mode > BDM_MODE4)
        mode = evolutionLaunchSupport->mode;
    argv[0] = gameName;
    argv[1] = startup;
    argv[2] = media;
    argv[3] = "bdm";

    /*
     * Load the same Evolution ELF again from its original device.  Its
     * built-in "bdm" argv path performs miniInit and launches the selected
     * game without constructing the GUI, so dashboard resources and menu
     * state cannot contaminate the loader.
     */
    deinit(UNMOUNT_EXCEPTION, mode);
    LoadELFFromFileWithPartition(bootPath, partition, 4, argv);
    return -6;
}

int menuEvolutionLaunchNative(void)
{
    if (!selected_item || !selected_item->item ||
        !selected_item->item->current)
        return -1;

    /*
     * Discard only Evolution's retained launch reference.  The normal OPL
     * selection callback then owns configuration loading and calls the live
     * BDM support object's itemLaunch routine exactly as original OPL does.
     */
    menuEvolutionCancelPreparedLaunch();

    if (gSelectButton == KEY_CIRCLE)
        selected_item->item->execCircle(selected_item->item);
    else
        selected_item->item->execCross(selected_item->item);

    /* A successful OPL launch transfers control and never returns. */
    return -6;
}

void menuEvolutionContext(void)
{
    if (selected_item && selected_item->item && selected_item->item->current)
        selected_item->item->execTriangle(selected_item->item);
}

void menuEvolutionSecondary(void)
{
    if (selected_item && selected_item->item && selected_item->item->current)
        selected_item->item->execSquare(selected_item->item);
}

void menuEvolutionRefresh(void)
{
    if (selected_item && selected_item->item && selected_item->item->refresh)
        selected_item->item->refresh(selected_item->item);
}

int menuEvolutionSelectCategory(int apps)
{
    menu_list_t *it = menu;
    menu_list_t *fallback = NULL;

    while (it) {
        item_list_t *list = (item_list_t *)it->item->userdata;
        const int isApps = list && list->mode == APP_MODE;

        if (it->item->visible && isApps == (apps ? 1 : 0)) {
            if (!fallback)
                fallback = it;

            /*
             * BDM devices initialize asynchronously.  Prefer a source whose
             * game submenu is already populated instead of permanently
             * attaching Evolution to the first visible but empty source.
             */
            if (it->item->current || it->item->submenu) {
                selected_item = it;
                itemConfigId = -1;
                return 1;
            }
        }
        it = it->next;
    }

    if (fallback) {
        selected_item = fallback;
        itemConfigId = -1;
        return 1;
    }

    return 0;
}

int menuEvolutionEnsureCategory(int apps)
{
    item_list_t *list;
    const int wantApps = apps ? 1 : 0;

    if (selected_item && selected_item->item) {
        list = (item_list_t *)selected_item->item->userdata;
        if (list && ((list->mode == APP_MODE) == wantApps) &&
            (selected_item->item->current || selected_item->item->submenu))
            return 1;
    }

    return menuEvolutionSelectCategory(apps);
}
#endif

void menuRenderInfo(void)
{
    item_list_t *list = selected_item->item->userdata;

    if (list->mode == APP_MODE) {
        menuRenderElements(gTheme->appsInfoElems.first);
        gTheme->itemsList = gTheme->appsItemsList;
    } else {
        menuRenderElements(gTheme->infoElems.first);
        gTheme->itemsList = gTheme->gamesItemsList;
    }
}

void menuHandleInputInfo()
{
    if (getKeyOn(KEY_CROSS)) {
        if (gSelectButton == KEY_CIRCLE)
            guiSwitchScreen(GUI_SCREEN_MAIN);
        else
            selected_item->item->execCross(selected_item->item);
    } else if (getKey(KEY_UP)) {
        menuPrevV();
    } else if (getKey(KEY_DOWN)) {
        menuNextV();
    } else if (getKeyOn(KEY_CIRCLE)) {
        if (gSelectButton == KEY_CROSS)
            guiSwitchScreen(GUI_SCREEN_MAIN);
        else
            selected_item->item->execCircle(selected_item->item);
    } else if (getKey(KEY_L1)) {
        menuPrevPage();
    } else if (getKey(KEY_R1)) {
        menuNextPage();
    } else if (getKeyOn(KEY_L2)) {
        menuFirstPage();
    } else if (getKeyOn(KEY_R2)) {
        menuLastPage();
    }
}

void menuRenderGameMenu()
{
    guiDrawBGPlasma();

    if (!gameMenu)
        return;

    // If the device menu that has the selected game suddenly goes invisible (device was removed), switch
    // back to the game list menu.
    if (selected_item->item->visible == 0) {
        guiSwitchScreen(GUI_SCREEN_MAIN);
        return;
    }

    // If we enter the game settings menu and there's no selected item bail out. I'm not entirely sure how we get into
    // this state but it seems to happen on some consoles when transitioning from the game settings menu back to the game
    // list menu.
    if (selected_item->item->current == NULL)
        return;

    // draw the animated menu
    if (!gameMenuCurrent)
        gameMenuCurrent = gameMenu;

    submenu_list_t *it = gameMenu;

    // calculate the number of items
    int count = 0;
    int sitem = 0;
    for (; it; count++, it = it->next) {
        if (it == gameMenuCurrent)
            sitem = count;
    }

    int spacing = 25;
    int y = (gTheme->usedHeight >> 1) - (spacing * (count >> 1));
    int cp = 0; // current position

    // game title
    fntRenderString(gTheme->fonts[0], 320, 20, ALIGN_CENTER, 0, 0, selected_item->item->current->item.text, gTheme->selTextColor);

    // config source
    char *cfgSource = gameConfigSource();
    fntRenderString(gTheme->fonts[0], 320, 40, ALIGN_CENTER, 0, 0, cfgSource, gTheme->textColor);

    // settings list
    for (it = gameMenu; it; it = it->next, cp++) {
        // render, advance
        fntRenderString(gTheme->fonts[0], 320, y, ALIGN_CENTER, 0, 0, submenuItemGetText(&it->item), (cp == sitem) ? gTheme->selTextColor : gTheme->textColor);
        y += spacing;
        if (cp == (GAME_SAVE_CHANGES - 1) || cp == (GAME_REMOVE_CHANGES - 1))
            y += spacing / 2;
    }

    // hints
    guiDrawSubMenuHints();
}

void menuHandleInputGameMenu()
{
    if (!gameMenu)
        return;

    if (!gameMenuCurrent)
        gameMenuCurrent = gameMenu;

    if (getKey(KEY_UP)) {
        sfxPlay(SFX_CURSOR);
        if (gameMenuCurrent->prev)
            gameMenuCurrent = gameMenuCurrent->prev;
        else // rewind to the last item
            while (gameMenuCurrent->next)
                gameMenuCurrent = gameMenuCurrent->next;
    }

    if (getKey(KEY_DOWN)) {
        sfxPlay(SFX_CURSOR);
        if (gameMenuCurrent->next)
            gameMenuCurrent = gameMenuCurrent->next;
        else
            gameMenuCurrent = gameMenu;
    }

    if (getKeyOn(gSelectButton)) {
        // execute the item via looking at the id of it
        int menuID = gameMenuCurrent->item.id;

        sfxPlay(SFX_CONFIRM);

        if (menuID == GAME_COMPAT_SETTINGS) {
            guiGameShowCompatConfig(selected_item->item->current->item.id, selected_item->item->userdata, itemConfig);
        } else if (menuID == GAME_CHEAT_SETTINGS) {
            guiGameShowCheatConfig();
        } else if (menuID == GAME_GSM_SETTINGS) {
            guiGameShowGSConfig();
        } else if (menuID == GAME_VMC_SETTINGS) {
            guiGameShowVMCMenu(selected_item->item->current->item.id, selected_item->item->userdata);
#ifdef PADEMU
        } else if (menuID == GAME_PADEMU_SETTINGS) {
            guiGameShowPadEmuConfig(0);
        } else if (menuID == GAME_PADMACRO_SETTINGS) {
            guiGameShowPadMacroConfig(0);
#endif
        } else if (menuID == GAME_OSD_LANGUAGE_SETTINGS) {
            guiGameShowOSDLanguageConfig(0);
        } else if (menuID == GAME_SAVE_CHANGES) {
            if (guiGameSaveConfig(itemConfig, selected_item->item->userdata))
                configSetInt(itemConfig, CONFIG_ITEM_CONFIGSOURCE, CONFIG_SOURCE_USER);
            menuSaveConfig();
            saveConfig(CONFIG_GAME, 0);
            guiMsgBox(_l(_STR_GAME_SETTINGS_SAVED), 0, NULL);
            guiGameLoadConfig(selected_item->item->userdata, gameMenuLoadConfig(NULL));
        } else if (menuID == GAME_TEST_CHANGES) {
            guiGameTestSettings(selected_item->item->current->item.id, selected_item->item->userdata, itemConfig);
        } else if (menuID == GAME_REMOVE_CHANGES) {
            if (guiGameShowRemoveSettings(itemConfig, configGetByType(CONFIG_GAME))) {
                guiGameLoadConfig(selected_item->item->userdata, gameMenuLoadConfig(NULL));
            }
        } else if (menuID == GAME_RENAME_GAME) {
            menuRenameGame(&gameMenu);
        } else if (menuID == GAME_DELETE_GAME) {
            menuDeleteGame(&gameMenu);
        }
        // so the exit press wont propagate twice
        readPads();
    }

    if (getKeyOn(KEY_START) || getKeyOn(gSelectButton == KEY_CIRCLE ? KEY_CROSS : KEY_CIRCLE)) {
        guiSwitchScreen(GUI_SCREEN_MAIN);
    }
}

void menuRenderAppMenu()
{
    guiDrawBGPlasma();

    if (!appMenu)
        return;

    // draw the animated menu
    if (!appMenuCurrent)
        appMenuCurrent = appMenu;

    submenu_list_t *it = appMenu;

    // calculate the number of items
    int count = 0;
    int sitem = 0;
    for (; it; count++, it = it->next) {
        if (it == appMenuCurrent)
            sitem = count;
    }

    int spacing = 25;
    int y = (gTheme->usedHeight >> 1) - (spacing * (count >> 1));
    int cp = 0; // current position

    // app title
    fntRenderString(gTheme->fonts[0], 320, 20, ALIGN_CENTER, 0, 0, selected_item->item->current->item.text, gTheme->selTextColor);

    for (it = appMenu; it; it = it->next, cp++) {
        // render, advance
        fntRenderString(gTheme->fonts[0], 320, y, ALIGN_CENTER, 0, 0, submenuItemGetText(&it->item), (cp == sitem) ? gTheme->selTextColor : gTheme->textColor);
        y += spacing;
    }

    // hints
    guiDrawSubMenuHints();
}

void menuHandleInputAppMenu()
{
    if (!appMenu)
        return;

    if (!appMenuCurrent)
        appMenuCurrent = appMenu;

    if (getKey(KEY_UP)) {
        sfxPlay(SFX_CURSOR);
        if (appMenuCurrent->prev)
            appMenuCurrent = appMenuCurrent->prev;
        else // rewind to the last item
            while (appMenuCurrent->next)
                appMenuCurrent = appMenuCurrent->next;
    }

    if (getKey(KEY_DOWN)) {
        sfxPlay(SFX_CURSOR);
        if (appMenuCurrent->next)
            appMenuCurrent = appMenuCurrent->next;
        else
            appMenuCurrent = appMenu;
    }

    if (getKeyOn(gSelectButton)) {
        // execute the item via looking at the id of it
        int menuID = appMenuCurrent->item.id;

        sfxPlay(SFX_CONFIRM);

        if (menuID == 0) {
            menuRenameGame(&appMenu);
        } else if (menuID == 1) {
            menuDeleteGame(&appMenu);
        }
        // so the exit press wont propagate twice
        readPads();
    }

    if (getKeyOn(KEY_START) || getKeyOn(gSelectButton == KEY_CIRCLE ? KEY_CROSS : KEY_CIRCLE)) {
        guiSwitchScreen(GUI_SCREEN_MAIN);
    }
}
