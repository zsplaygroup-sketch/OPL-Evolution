/*
 Copyright 2013, Ifcaro & jimmikaelkael
 Copyright 2026, OPL Evolution contributors (standalone frontend)
 Licenced under Academic Free License version 3.0
 Review OpenUsbLd README & LICENSE files for further details.
*/

#include "include/opl.h"
#include "include/evo_boot.h"
#include "include/evo_engine.h"
#include "include/evo_ps1_art.h"
#include "include/evo_settings.h"
#include "include/evo_ui.h"
#include "include/config.h"
#include "include/fntsys.h"
#include "include/gui.h"
#include "include/ioman.h"
#include "include/pad.h"
#include "include/renderman.h"
#include "include/sound.h"
#include "include/system.h"
#include "include/themes.h"
#include "include/textures.h"
#include "include/vmc_groups.h"

#include <dirent.h>
#include <fcntl.h>
#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>
#include <kernel.h>
#include <loadfile.h>
#include <stdlib.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>

#ifdef PADEMU
#include <libds34bt.h>
#include <libds34usb.h>
#endif

enum evo_section {
    EVO_HOME = 0,
    EVO_LIBRARY,
    EVO_CONTROLLERS,
    EVO_MEMORY_CARDS,
    EVO_NETWORK,
    EVO_SETTINGS,
    EVO_DIAGNOSTICS,
    EVO_SECTION_COUNT
};

enum evo_library_filter {
    EVO_LIBRARY_PS2 = 0,
    EVO_LIBRARY_PS1,
    EVO_LIBRARY_APPS,
    EVO_LIBRARY_FILTER_COUNT
};

enum evo_focus {
    EVO_FOCUS_NAV = 0,
    EVO_FOCUS_CONTENT
};

enum evo_launch_request {
    EVO_LAUNCH_NONE = 0,
    EVO_LAUNCH_PS2_PREPARE,
    EVO_LAUNCH_PS2_COMMIT,
    EVO_LAUNCH_PS1
};

enum evo_overlay {
    EVO_OVERLAY_NONE = 0,
    EVO_OVERLAY_OPTIONS,
    EVO_OVERLAY_SYSTEM
};

enum evo_option_action {
    EVO_OPTION_EVOLUTION = 0,
    EVO_OPTION_DISPLAY,
    EVO_OPTION_AUDIO,
    EVO_OPTION_CONTROLLER,
    EVO_OPTION_NETWORK,
    EVO_OPTION_DIAGNOSTICS,
    EVO_OPTION_BACK,
    EVO_OPTION_COUNT
};

enum evo_system_action {
    EVO_SYSTEM_RESUME = 0,
    EVO_SYSTEM_OPTIONS,
    EVO_SYSTEM_SAVE,
    EVO_SYSTEM_EXIT,
    EVO_SYSTEM_POWER_OFF,
    EVO_SYSTEM_COUNT
};

enum evo_vmc_mode {
    EVO_VMC_PHYSICAL = 0,
    EVO_VMC_GENERIC,
    EVO_VMC_PER_GAME,
    EVO_VMC_MODE_COUNT
};

enum evo_vmc_row {
    EVO_VMC_ROW_SLOT1 = 0,
    EVO_VMC_ROW_SLOT2,
    EVO_VMC_ROW_SIZE,
    EVO_VMC_ROW_CREATE1,
    EVO_VMC_ROW_CREATE2,
    EVO_VMC_ROW_SAVE,
    EVO_VMC_ROW_COUNT
};

typedef struct {
    int status;
    int error;
    int progress;
    char message[64];
} evo_vmc_status_t;

static const char *const evoSectionLabels[EVO_SECTION_COUNT] = {
    "Home",
    "Library",
    "Controllers",
    "Memory",
    "Network",
    "Settings",
    "Diagnostics",
};

static const char *const evoLibraryFilterLabels[EVO_LIBRARY_FILTER_COUNT] = {
    "PS2",
    "PS1",
    "HOMEBREW",
};

static const char *const evoOptionLabels[EVO_OPTION_COUNT] = {
    "Evolution settings",
    "Display settings",
    "Audio settings",
    "Controller status",
    "Network settings",
    "Diagnostics",
    "Back",
};

static const char *const evoSystemLabels[EVO_SYSTEM_COUNT] = {
    "Resume",
    "Options",
    "Save settings",
    "Exit to PS2 Browser",
    "Power off console",
};

// Evolution owns the foreground application for the lifetime of this ELF.
static int evoUiEnabled = 0;
static int evoSection = EVO_LIBRARY;
static int evoFocus = EVO_FOCUS_NAV;
static int evoLibraryFilter = EVO_LIBRARY_PS2;
static int evoControllerSelection = 0;
static char evoControllerNotice[96];
static int evoPs1Device = -1;
static char evoPs1PopstarterPath[256];
static char evoPs1Notice[96];
static char evoSettingsNotice[96];
static int evoVmcSelection;
static int evoVmcMode[2];
static int evoVmcSizeIndex;
static int evoVmcBusy;
static int evoVmcBusySlot;
static char evoVmcName[2][32];
static char evoVmcNotice[96];
static evo_vmc_status_t evoVmcStatus;
#define EVO_PS1_MAX_GAMES 128
#define EVO_PS1_NAME_MAX  96
#define EVO_PS1_VISIBLE   6
static char evoPs1Games[EVO_PS1_MAX_GAMES][EVO_PS1_NAME_MAX];
static int evoPs1GameCount;
static int evoPs1Selection;
static int evoLaunchInProgress;
static int evoLaunchRequest;
static char evoLaunchNotice[64];
static int evoOverlay;
static int evoOverlaySelection;
static char evoOverlayNotice[64];
static int evoMemoryCardType[2];
static int evoMemoryCardFree[2];
static int evoMemoryCardFormatted[2];
static clock_t evoMemoryCardPollTime;
static int evoMainFont = FNT_ERROR;
static int evoSmallFont = FNT_ERROR;

#define EVO_MAIN_FONT (evoMainFont == FNT_ERROR ? FNT_DEFAULT : evoMainFont)

#ifdef PADEMU
static int evoBtStatus[2];
static int evoUsbStatus[2];
static clock_t evoControllerPollTime;
static hci_information_t evoAdapterInfo;
static int evoAdapterInfoValid;
#endif

static const u64 evoColBackground = GS_SETREG_RGBA(0x00, 0x02, 0x07, 0x80);
static const u64 evoColPanelStrong = GS_SETREG_RGBA(0x02, 0x0C, 0x1C, 0x80);
static const u64 evoColCyan = GS_SETREG_RGBA(0x00, 0x94, 0xFF, 0x80);
static const u64 evoColBlue = GS_SETREG_RGBA(0x00, 0x3D, 0x78, 0x80);
static const u64 evoColWhite = GS_SETREG_RGBA(0xD6, 0xE0, 0xEF, 0x80);
static const u64 evoColMuted = GS_SETREG_RGBA(0x88, 0x9A, 0xB8, 0x80);
static const u64 evoColGreen = GS_SETREG_RGBA(0x3A, 0xD8, 0x45, 0x80);

static int evoComparePs1Games(const void *left, const void *right)
{
    return strcasecmp((const char *)left, (const char *)right);
}

static int evoFindPs1Backend(void)
{
    int device;

    evoPs1Device = -1;
    evoPs1PopstarterPath[0] = '\0';

    for (device = 0; device <= 4; device++) {
        int popstarterFd;

        snprintf(evoPs1PopstarterPath, sizeof(evoPs1PopstarterPath),
                 "mass%d:/POPS/POPSTARTER.ELF", device);
        popstarterFd = open(evoPs1PopstarterPath, O_RDONLY);
        if (popstarterFd >= 0) {
            close(popstarterFd);
            evoPs1Device = device;
            return 1;
        }
    }

    evoPs1PopstarterPath[0] = '\0';
    snprintf(evoPs1Notice, sizeof(evoPs1Notice), "POPSTARTER.ELF was not found in a USB POPS folder.");
    return 0;
}

static void evoScanPs1Games(void)
{
    char folder[32];
    DIR *dir;
    struct dirent *entry;

    evoPs1GameCount = 0;
    evoPs1Selection = 0;
    if (evoPs1Device < 0)
        return;

    snprintf(folder, sizeof(folder), "mass%d:/POPS", evoPs1Device);
    dir = opendir(folder);
    if (dir == NULL) {
        snprintf(evoPs1Notice, sizeof(evoPs1Notice), "The USB POPS folder could not be opened.");
        return;
    }

    while (evoPs1GameCount < EVO_PS1_MAX_GAMES && (entry = readdir(dir)) != NULL) {
        const size_t length = strlen(entry->d_name);

        if (length > 4 && strcasecmp(entry->d_name + length - 4, ".VCD") == 0) {
            snprintf(evoPs1Games[evoPs1GameCount], EVO_PS1_NAME_MAX, "%s", entry->d_name);
            evoPs1GameCount++;
        }
    }
    closedir(dir);

    if (evoPs1GameCount > 1)
        qsort(evoPs1Games, evoPs1GameCount, sizeof(evoPs1Games[0]), evoComparePs1Games);

    if (evoPs1GameCount > 0)
        snprintf(evoPs1Notice, sizeof(evoPs1Notice), "%d PS1 game%s detected on USB device %d.",
                 evoPs1GameCount, evoPs1GameCount == 1 ? "" : "s", evoPs1Device);
    else
        snprintf(evoPs1Notice, sizeof(evoPs1Notice), "No .VCD games were found in mass%d:/POPS.", evoPs1Device);
}

static void evoRescanPs1(void)
{
    evoPs1ArtReset();
    if (evoFindPs1Backend())
        evoScanPs1Games();
    else {
        evoPs1GameCount = 0;
        evoPs1Selection = 0;
    }
}

static void evoSetLibraryFilter(int filter)
{
    if (filter < 0)
        filter = EVO_LIBRARY_FILTER_COUNT - 1;
    else if (filter >= EVO_LIBRARY_FILTER_COUNT)
        filter = 0;

    evoLibraryFilter = filter;
}

/*
 * POPStarter's USB handoff must preserve the active IOP and BDM stack.
 * This mirrors the no-IOP-reset ExecPS2 route used by POPSLoader BETA-5,
 * using the standard PS2SDK loadfile and kernel APIs.
 */
static int evoExecElfNoIopReset(const char *path, int argc, char *argv[])
{
    t_ExecData elf;
    int result;

    SifInitRpc(0);
    SifLoadFileInit();
    result = SifLoadElf(path, &elf);
    SifLoadFileExit();

    if (result != 0 || elf.epc == 0)
        return -1;

    ExecPS2((void *)elf.epc, (void *)elf.gp, argc, argv);
    return -1;
}

static void evoLaunchPs1Game(void)
{
    char gameName[EVO_PS1_NAME_MAX];
    char selector[EVO_PS1_NAME_MAX + 32];
    char *argv[1];
    int mode;

    if (evoPs1GameCount <= 0 || evoPs1Selection < 0 || evoPs1Selection >= evoPs1GameCount) {
        sfxPlay(SFX_CANCEL);
        return;
    }

    snprintf(gameName, sizeof(gameName), "%s", evoPs1Games[evoPs1Selection]);
    gameName[strlen(gameName) - 4] = '\0';
    snprintf(selector, sizeof(selector), "mass:/POPS/XX.%s.ELF", gameName);
    argv[0] = selector;

    mode = oplPath2Mode(evoPs1PopstarterPath);
    if (mode < 0)
        mode = APP_MODE;

    deinit(UNMOUNT_EXCEPTION, mode);
    evoExecElfNoIopReset(evoPs1PopstarterPath, 1, argv);
}

static void evoLaunchSelectedPs2(void)
{
    /*
     * Keep Evolution as the active screen owner while handing the selected
     * item directly to OPL's proven support backend.  Do not destroy or
     * invalidate the dashboard here: Confirm is processed while the texture
     * can still be referenced by the active GS frame.  The backend's normal
     * launch/deinit path owns graphics shutdown.  A successful itemLaunch does
     * not return.  If configuration loading or launch fails, the next
     * Evolution frame restores the dashboard.
     */
    evoLaunchInProgress = 1;
    evoLaunchRequest = EVO_LAUNCH_PS2_PREPARE;
    snprintf(evoLaunchNotice, sizeof(evoLaunchNotice), "Finishing background artwork I/O...");
}

static void evoLaunchSelectedPs1(void)
{
    evoLaunchInProgress = 1;
    evoLaunchRequest = EVO_LAUNCH_PS1;
    snprintf(evoLaunchNotice, sizeof(evoLaunchNotice), "Preparing selected PS1 title...");
}

static const char *evoNetworkStateName(int state)
{
    switch (state) {
        case 0:
            return "Ready";
        case ERROR_ETH_NOT_STARTED:
            return "Not started";
        case ERROR_ETH_MODULE_NETIF_FAILURE:
            return "Network module failed";
        case ERROR_ETH_MODULE_SMBMAN_FAILURE:
            return "SMB module failed";
        case ERROR_ETH_SMB_CONN:
            return "SMB connection failed";
        case ERROR_ETH_SMB_LOGON:
            return "SMB logon failed";
        case ERROR_ETH_SMB_ECHO:
            return "SMB echo failed";
        case ERROR_ETH_SMB_OPENSHARE:
            return "SMB share open failed";
        case ERROR_ETH_SMB_LISTSHARES:
            return "SMB share listing failed";
        case ERROR_ETH_SMB_LISTGAMES:
            return "SMB game listing failed";
        case ERROR_ETH_LINK_FAIL:
            return "Ethernet link failed";
        case ERROR_ETH_DHCP_FAIL:
            return "DHCP failed";
        default:
            return state > 0 && state < ERROR_ETH_NOT_STARTED ? "Starting" : "Unknown error";
    }
}

static const char *evoRomVersion(void)
{
    static char romVersion[16];
    static int initialized;
    int fd;
    int length;
    int i;

    if (initialized)
        return romVersion[0] ? romVersion : "Unavailable";

    initialized = 1;
    fd = open("rom0:ROMVER", O_RDONLY);
    if (fd < 0)
        return "Unavailable";

    length = read(fd, romVersion, sizeof(romVersion) - 1);
    close(fd);
    if (length <= 0) {
        romVersion[0] = '\0';
        return "Unavailable";
    }

    romVersion[length] = '\0';
    for (i = 0; i < length; i++) {
        if (romVersion[i] == '\r' || romVersion[i] == '\n') {
            romVersion[i] = '\0';
            break;
        }
    }

    return romVersion;
}

#ifdef PADEMU
static void evoPollControllerStatus(void)
{
    const clock_t now = clock();
    int i;

    if (evoControllerPollTime != 0 &&
        (now - evoControllerPollTime) < CLOCKS_PER_SEC / 4)
        return;

    evoControllerPollTime = now;
    for (i = 0; i < 2; i++) {
        evoBtStatus[i] = ds34bt_get_status(i);
        evoUsbStatus[i] = ds34usb_get_status(i);
    }

    if (evoBtStatus[0] & DS34BT_STATE_USB_CONFIGURED) {
        if (!evoAdapterInfoValid)
            evoAdapterInfoValid = ds34bt_get_version(&evoAdapterInfo);
    } else {
        evoAdapterInfoValid = 0;
    }
}
#endif

static void evoControllerStatus(int player, char *text, size_t size)
{
    int port;
    int slot;
    int nativeStatus;

#ifdef PADEMU
    const int btStatus = player < 2 ? evoBtStatus[player] : 0;
    const int usbStatus = player < 2 ? evoUsbStatus[player] : 0;

    if (btStatus & DS34BT_STATE_RUNNING) {
        snprintf(text, size, "Bluetooth pad active");
        return;
    }
    if (usbStatus & DS34USB_STATE_RUNNING) {
        snprintf(text, size, "USB pad active");
        return;
    }
    if (btStatus & DS34BT_STATE_CONNECTED) {
        snprintf(text, size, "Bluetooth connected; not running");
        return;
    }
    if (usbStatus & DS34USB_STATE_CONNECTED) {
        snprintf(text, size, "USB connected; not running");
        return;
    }
#endif

    nativeStatus = padGetConnectionStatus(player, &port, &slot);
    if (nativeStatus > 0)
        snprintf(text, size, "Native pad P%d / S%d", port + 1, slot + 1);
    else if (nativeStatus == 0)
        snprintf(text, size, "Native pad disconnected");
    else
        snprintf(text, size, "No initialized controller");
}

static int evoControllerStatusCompact(int player, char *text, size_t size)
{
    int port;
    int slot;
    int nativeStatus;

#ifdef PADEMU
    const int btStatus = player < 2 ? evoBtStatus[player] : 0;
    const int usbStatus = player < 2 ? evoUsbStatus[player] : 0;

    if (btStatus & DS34BT_STATE_RUNNING) {
        snprintf(text, size, "(Bluetooth)");
        return 1;
    }
    if (usbStatus & DS34USB_STATE_RUNNING) {
        snprintf(text, size, "(USB)");
        return 1;
    }
    if ((btStatus & DS34BT_STATE_CONNECTED) || (usbStatus & DS34USB_STATE_CONNECTED)) {
        snprintf(text, size, "(Connecting)");
        return 0;
    }
#endif

    nativeStatus = padGetConnectionStatus(player, &port, &slot);
    if (nativeStatus > 0) {
        snprintf(text, size, "(Wired)");
        return 1;
    }

    snprintf(text, size, "(Offline)");
    return 0;
}

static void evoPollMemoryCardStatus(void)
{
    const clock_t now = clock();
    int port;

    if (evoMemoryCardPollTime != 0 &&
        (now - evoMemoryCardPollTime) < CLOCKS_PER_SEC * 5)
        return;

    evoMemoryCardPollTime = now;
    for (port = 0; port < 2; port++) {
        int result;

        evoMemoryCardType[port] = MC_TYPE_NONE;
        evoMemoryCardFree[port] = 0;
        evoMemoryCardFormatted[port] = 0;
        mcGetInfo(port, 0, &evoMemoryCardType[port],
                  &evoMemoryCardFree[port], &evoMemoryCardFormatted[port]);
        mcSync(0, NULL, &result);
        if (result < 0) {
            evoMemoryCardType[port] = MC_TYPE_NONE;
            evoMemoryCardFree[port] = 0;
            evoMemoryCardFormatted[port] = 0;
        }
    }
}

static void evoMemoryCardStatusCompact(int port, char *text, size_t size)
{
    if (evoMemoryCardType[port] == MC_TYPE_PS2 &&
        evoMemoryCardFormatted[port] == MC_FORMATTED) {
        const int freeTenths = (evoMemoryCardFree[port] * 10) / 1024;
        if (freeTenths > 0)
            snprintf(text, size, "%d.%d MB free",
                     freeTenths / 10, freeTenths % 10);
        else
            snprintf(text, size, "Ready");
    } else if (evoMemoryCardType[port] != MC_TYPE_NONE) {
        snprintf(text, size, "Unformatted");
    } else {
        snprintf(text, size, "No card");
    }
}

static void evoDrawText(int x, int y, const char *text, u64 color)
{
    fntRenderString(EVO_MAIN_FONT, x, y, ALIGN_NONE, 0, 0, text, color);
}

static void evoDrawSmallText(int x, int y, int width, const char *text, u64 color)
{
    const int font = evoSmallFont == FNT_ERROR ? EVO_MAIN_FONT : evoSmallFont;
    fntRenderString(font, x, y, ALIGN_NONE, width, 18, text, color);
}

static void evoDrawClippedText(int x, int y, int width, const char *text, u64 color)
{
    fntRenderString(EVO_MAIN_FONT, x, y, ALIGN_NONE, width, 20, text, color);
}

static int evoDrawTexture(int textureId, int x, int y, int width, int height, u64 color)
{
    GSTEXTURE *texture = thmGetTexture(textureId);

    if (texture != NULL && texture->Mem != NULL) {
        /*
         * This helper is used only for the full-screen Evolution dashboard.
         * Its geometry is already authored in the same 640x480 coordinate
         * space as the live panels. Applying SCALING_RATIO in widescreen mode
         * compressed the background to 75% while the panels remained full
         * width, producing the visibly offset layout.
         */
        rmDrawPixmap(texture, x, y, ALIGN_NONE, width, height, SCALING_NONE, color);
        return 1;
    }
    return 0;
}

static void evoDrawOutline(int x, int y, int width, int height, u64 color)
{
    rmDrawLine(x, y, x + width, y, color);
    rmDrawLine(x, y + height, x + width, y + height, color);
    rmDrawLine(x, y, x, y + height, color);
    rmDrawLine(x + width, y, x + width, y + height, color);
}

static void evoDrawControllerIcon(int x, int y, u64 color)
{
    evoDrawOutline(x + 3, y + 3, 11, 7, color);
    rmDrawLine(x + 3, y + 5, x + 1, y + 13, color);
    rmDrawLine(x + 14, y + 5, x + 16, y + 13, color);
    rmDrawLine(x + 1, y + 13, x + 5, y + 9, color);
    rmDrawLine(x + 16, y + 13, x + 12, y + 9, color);
    rmDrawLine(x + 6, y + 5, x + 6, y + 9, color);
    rmDrawLine(x + 4, y + 7, x + 8, y + 7, color);
    rmDrawRect(x + 11, y + 6, 2, 2, color);
}

static void evoDrawMemoryCardIcon(int x, int y, u64 color)
{
    evoDrawOutline(x + 2, y + 1, 12, 15, color);
    rmDrawLine(x + 5, y + 1, x + 5, y + 5, color);
    rmDrawLine(x + 11, y + 1, x + 11, y + 5, color);
    rmDrawLine(x + 5, y + 5, x + 11, y + 5, color);
    rmDrawLine(x + 5, y + 12, x + 11, y + 12, color);
}

static void evoDrawStorageIcon(int x, int y, u64 color)
{
    evoDrawOutline(x + 1, y + 1, 15, 15, color);
    evoDrawOutline(x + 4, y + 4, 9, 5, color);
    rmDrawRect(x + 12, y + 12, 2, 2, color);
    rmDrawLine(x + 4, y + 13, x + 9, y + 13, color);
}

static void evoDrawUsbIcon(int x, int y, u64 color)
{
    rmDrawLine(x + 7, y + 15, x + 7, y + 3, color);
    rmDrawLine(x + 7, y + 7, x + 3, y + 4, color);
    rmDrawLine(x + 3, y + 4, x + 3, y + 1, color);
    rmDrawLine(x + 7, y + 10, x + 11, y + 7, color);
    rmDrawLine(x + 11, y + 7, x + 11, y + 4, color);
    rmDrawRect(x + 1, y, 4, 2, color);
    rmDrawRect(x + 9, y + 2, 4, 3, color);
    rmDrawRect(x + 5, y + 14, 5, 3, color);
}

static void evoDrawNetworkIcon(int x, int y, u64 color)
{
    evoDrawOutline(x + 5, y, 6, 5, color);
    evoDrawOutline(x, y + 11, 6, 5, color);
    evoDrawOutline(x + 10, y + 11, 6, 5, color);
    rmDrawLine(x + 8, y + 5, x + 8, y + 8, color);
    rmDrawLine(x + 3, y + 8, x + 13, y + 8, color);
    rmDrawLine(x + 3, y + 8, x + 3, y + 11, color);
    rmDrawLine(x + 13, y + 8, x + 13, y + 11, color);
}

static const char *evoStartModeName(int mode)
{
    switch (mode) {
        case START_MODE_MANUAL:
            return "Manual";
        case START_MODE_AUTO:
            return "Auto";
        default:
            return "Off";
    }
}

static int evoCancelButton(void)
{
    return gSelectButton == KEY_CIRCLE ? KEY_CROSS : KEY_CIRCLE;
}

static void evoDrawTitle(const char *section)
{
    char clockText[32];
    char dateText[32];
    time_t now;
    struct tm *clock;
    int width;
    int height;

    (void)section;
    rmGetScreenExtents(&width, &height);
    (void)height;

    now = time(NULL);
    clock = localtime(&now);
    if (clock != NULL) {
        snprintf(clockText, sizeof(clockText), "%02d:%02d:%02d",
                 clock->tm_hour, clock->tm_min, clock->tm_sec);
        fntRenderString(EVO_MAIN_FONT, width - 108, 5, ALIGN_RIGHT, 96, 22, clockText, evoColCyan);
        if (clock->tm_year + 1900 >= 2024) {
            snprintf(dateText, sizeof(dateText), "%02d/%02d/%04d",
                     clock->tm_mon + 1, clock->tm_mday, clock->tm_year + 1900);
            fntRenderString(EVO_MAIN_FONT, width - 108, 25, ALIGN_RIGHT, 96, 22, dateText, evoColMuted);
        }
    }
}

static void evoDrawStatusStrip(const char *message)
{
    int width;
    int height;

    rmGetScreenExtents(&width, &height);
    rmDrawRect(0, height - 30, width, 30, evoColPanelStrong);
    rmDrawLine(0, height - 30, width, height - 30, evoColBlue);
    fntRenderString(EVO_MAIN_FONT, 12, height - 23, ALIGN_NONE, width - 24, 22, message, evoColMuted);
}

static void evoDrawNavigation(void)
{
    const int top = 84 + evoSection * 36;
    const u64 color = evoFocus == EVO_FOCUS_NAV ? evoColCyan : evoColBlue;

    evoDrawOutline(13, top, 133, 31, color);
    if (evoFocus == EVO_FOCUS_NAV)
        evoDrawOutline(14, top + 1, 131, 29, evoColBlue);
}

static void evoDrawEngineList(int x, int y, int width, int rowHeight, int rows)
{
    int count = evoEngineGetCount();
    int selected = evoEngineGetSelected();
    int first;
    int row;

    if (count <= 0) {
        evoDrawText(x, y + 8, "Waiting for a PS2 storage source...", evoColMuted);
        return;
    }

    first = selected - rows / 2;
    if (first < 0)
        first = 0;
    if (first + rows > count)
        first = count > rows ? count - rows : 0;

    for (row = 0; row < rows && first + row < count; row++) {
        const int id = first + row;
        const int rowY = y + row * rowHeight;
        const int isSelected = id == selected;
        const char *name = evoEngineGetName(id);

        if (isSelected) {
            rmDrawRect(x - 6, rowY - 3, width + 12, rowHeight,
                       GS_SETREG_RGBA(0x02, 0x0C, 0x1C, 0x80));
            evoDrawOutline(x - 6, rowY - 3, width + 12, rowHeight,
                           evoColCyan);
        } else {
            rmDrawLine(x, rowY + rowHeight - 4, x + width,
                       rowY + rowHeight - 4, evoColBlue);
        }

        fntRenderString(EVO_MAIN_FONT, x + 3, rowY + 6, ALIGN_NONE,
                        width - 6, rowHeight - 8,
                        name && name[0] ? name : "Untitled PS2 game",
                        isSelected ? evoColWhite : evoColMuted);
    }
}

static void evoDrawSelectionPanel(void)
{
    char source[40];
    char title[96];
    char startup[32];
    char countText[40];
    int position;
    int total;

    if (evoLibraryFilter == EVO_LIBRARY_PS2) {
        const int selected = evoEngineGetSelected();
        snprintf(source, sizeof(source), "%s", evoEngineGetSourceName());
        snprintf(title, sizeof(title), "%s", evoEngineGetName(selected));
        snprintf(startup, sizeof(startup), "%s",
                 evoEngineGetStartup(selected));
        total = evoEngineGetCount();
        position = total > 0 ? selected + 1 : 0;
    } else {
        snprintf(source, sizeof(source), "Evolution");
        snprintf(title, sizeof(title), "Homebrew scanner pending");
        startup[0] = '\0';
        position = 0;
        total = 0;
    }

    if (evoFocus == EVO_FOCUS_CONTENT)
        rmDrawLine(400, 52, 400, 345, evoColCyan);
    evoDrawOutline(417, 67, 95, 174, evoColBlue);
    if (!(evoLibraryFilter == EVO_LIBRARY_PS2 &&
          evoEngineDrawSelectedCover(421, 71, 116, 166)))
        fntRenderString(EVO_MAIN_FONT, 464, 143, ALIGN_CENTER, 84, 24, "NO ART", evoColMuted);

    snprintf(countText, sizeof(countText), "%d of %d", position, total);
    evoDrawSmallText(529, 67, 98, "TITLE", evoColCyan);
    evoDrawSmallText(529, 84, 98, title, evoColWhite);
    evoDrawSmallText(529, 116, 98, "SOURCE", evoColCyan);
    evoDrawSmallText(529, 133, 98, source, evoColWhite);
    evoDrawSmallText(529, 165, 98, "ID", evoColCyan);
    evoDrawSmallText(529, 182, 98,
                    startup[0] ? startup : "--", evoColWhite);
    evoDrawSmallText(529, 214, 98, "ITEM", evoColCyan);
    evoDrawSmallText(529, 231, 98, countText, evoColMuted);
    evoDrawOutline(408, 264, 219, 58, evoColBlue);
    evoDrawSmallText(417, 274, 201, "Press X to open the selected title.", evoColMuted);
}

static void evoDrawLibrary(void)
{
    int filter;
    int tabX = 174;

    if (evoFocus == EVO_FOCUS_CONTENT)
        rmDrawLine(161, 52, 399, 52, evoColCyan);
    evoDrawText(174, 62, "UNIFIED LIBRARY", evoColCyan);
    for (filter = 0; filter < EVO_LIBRARY_FILTER_COUNT; filter++) {
        const int width = filter == EVO_LIBRARY_APPS ? 86 : 42;
        if (filter == evoLibraryFilter) {
            evoDrawOutline(tabX - 4, 83, width, 23, evoColCyan);
        }
        evoDrawText(tabX, 86, evoLibraryFilterLabels[filter],
                    filter == evoLibraryFilter ? evoColWhite : evoColMuted);
        tabX += width + 5;
    }

    if (evoLibraryFilter == EVO_LIBRARY_APPS) {
        evoDrawText(174, 118, "Evolution homebrew scanner is not ready yet.", evoColMuted);
        evoDrawText(174, 148, "No original OPL Apps menu is used.", evoColMuted);
    } else if (evoLibraryFilter == EVO_LIBRARY_PS2) {
        evoDrawEngineList(174, 112, 211, 38, 6);
    }
    evoDrawSelectionPanel();
}

static void evoDrawSystemCards(void)
{
    char controller1[64];
    char controller2[64];
    char memoryCard1[48];
    char memoryCard2[48];
    char storage[64];
    char network[64];
    int controller1Connected;
    int controller2Connected;

#ifdef PADEMU
    evoPollControllerStatus();
#endif
    evoPollMemoryCardStatus();
    controller1Connected = evoControllerStatusCompact(0, controller1, sizeof(controller1));
    controller2Connected = evoControllerStatusCompact(1, controller2, sizeof(controller2));
    evoMemoryCardStatusCompact(0, memoryCard1, sizeof(memoryCard1));
    evoMemoryCardStatusCompact(1, memoryCard2, sizeof(memoryCard2));
    snprintf(network, sizeof(network), "%s", evoNetworkStateName(gNetworkStartup));

    evoDrawControllerIcon(27, 372, evoColMuted);
    evoDrawControllerIcon(27, 400, evoColMuted);
    evoDrawSmallText(48, 365, 78, "DUALSHOCK 2", evoColWhite);
    evoDrawSmallText(48, 378, 78, controller1, evoColMuted);
    evoDrawSmallText(48, 393, 78, "DUALSHOCK 2", evoColWhite);
    evoDrawSmallText(48, 406, 78, controller2, evoColMuted);
    if (controller1Connected)
        rmDrawRect(132, 377, 18, 4, evoColGreen);
    if (controller2Connected)
        rmDrawRect(132, 405, 18, 4, evoColGreen);

    evoDrawMemoryCardIcon(174, 372, evoColMuted);
    evoDrawSmallText(193, 365, 50, "SLOT 1", evoColWhite);
    evoDrawSmallText(193, 381, 50, memoryCard1, evoColMuted);
    evoDrawMemoryCardIcon(245, 372, evoColMuted);
    evoDrawSmallText(264, 365, 46, "SLOT 2", evoColWhite);
    evoDrawSmallText(264, 381, 46, memoryCard2, evoColMuted);
    if (evoMemoryCardType[0] == MC_TYPE_PS2 &&
        evoMemoryCardFormatted[0] == MC_FORMATTED)
        rmDrawRect(193, 407, 44, 3, evoColGreen);
    if (evoMemoryCardType[1] == MC_TYPE_PS2 &&
        evoMemoryCardFormatted[1] == MC_FORMATTED)
        rmDrawRect(264, 407, 40, 3, evoColGreen);

    evoDrawStorageIcon(329, 369, evoColMuted);
    evoDrawSmallText(349, 366, 63, "HDD", evoColWhite);
    evoDrawSmallText(349, 382, 63,
                     gHDDStartMode == START_MODE_AUTO ? "Auto" :
                     gHDDStartMode == START_MODE_MANUAL ? "Manual" : "Off",
                     evoColMuted);
    evoDrawUsbIcon(415, 368, evoColMuted);
    evoDrawSmallText(435, 366, 59, "USB", evoColWhite);
    evoDrawSmallText(435, 382, 59,
                     gBDMStartMode == START_MODE_AUTO ? "Auto" :
                     gBDMStartMode == START_MODE_MANUAL ? "Manual" : "Off",
                     evoColMuted);
    snprintf(storage, sizeof(storage), "PS1: %d title%s", evoPs1GameCount,
             evoPs1GameCount == 1 ? "" : "s");
    evoDrawSmallText(349, 405, 142, storage, evoColMuted);

    evoDrawNetworkIcon(515, 372, evoColMuted);
    evoDrawSmallText(537, 368, 79, network,
                     gNetworkStartup == 0 ? evoColGreen : evoColWhite);
    if (gNetworkStartup == 0) {
        char address[32];
        snprintf(address, sizeof(address), "%d.%d.%d.%d",
                 ps2_ip[0], ps2_ip[1], ps2_ip[2], ps2_ip[3]);
        evoDrawSmallText(537, 386, 79, address, evoColMuted);
    } else {
        evoDrawSmallText(537, 386, 79,
                         ps2_ip_use_dhcp ? "DHCP" : "Static IP", evoColMuted);
    }
    evoDrawSmallText(537, 404, 79,
                     gETHStartMode == START_MODE_AUTO ? "SMB Auto" :
                     gETHStartMode == START_MODE_MANUAL ? "SMB Manual" : "SMB Off",
                     evoColMuted);
}

static void evoDrawHome(void)
{
    char source[40];
    char title[96];
    char startup[32];
    char countText[40];
    int position;
    int total;

    const int selected = evoEngineGetSelected();
    snprintf(source, sizeof(source), "%s", evoEngineGetSourceName());
    snprintf(title, sizeof(title), "%s", evoEngineGetName(selected));
    snprintf(startup, sizeof(startup), "%s",
             evoEngineGetStartup(selected));
    total = evoEngineGetCount();
    position = total > 0 ? selected + 1 : 0;

    evoDrawText(174, 62, "RECENT GAMES", evoColCyan);
    snprintf(countText, sizeof(countText), "%d / %d", position, total);
    fntRenderString(EVO_MAIN_FONT, 316, 62, ALIGN_RIGHT, 70, 22, countText, evoColMuted);
    evoDrawEngineList(174, 88, 211, 41, 6);

    evoDrawOutline(417, 67, 95, 174, evoColBlue);
    if (!evoEngineDrawSelectedCover(421, 71, 116, 166))
        fntRenderString(EVO_MAIN_FONT, 464, 143, ALIGN_CENTER, 84, 22, "NO ART", evoColMuted);

    evoDrawSmallText(529, 67, 98, "TITLE", evoColCyan);
    evoDrawSmallText(529, 84, 98, title, evoColWhite);
    evoDrawSmallText(529, 116, 98, "GAME ID", evoColCyan);
    evoDrawSmallText(529, 133, 98,
                    startup[0] ? startup : "--", evoColWhite);
    evoDrawSmallText(529, 165, 98, "PLATFORM", evoColCyan);
    evoDrawSmallText(529, 182, 98, "PS2", evoColWhite);
    evoDrawSmallText(529, 214, 98, "SOURCE", evoColCyan);
    evoDrawSmallText(529, 231, 98, source, evoColWhite);
    evoDrawOutline(408, 264, 219, 58, evoColBlue);
    evoDrawSmallText(417, 274, 201, "Open Library for every platform.", evoColMuted);
}

static int evoVmcSize(void)
{
    static const int sizes[] = {8, 16, 32, 64};
    return sizes[evoVmcSizeIndex];
}

static void evoVmcBuildName(int slot)
{
    const char *startup;
    const char *group;

    if (evoVmcMode[slot] == EVO_VMC_PHYSICAL) {
        evoVmcName[slot][0] = '\0';
    } else if (evoVmcMode[slot] == EVO_VMC_GENERIC) {
        snprintf(evoVmcName[slot], sizeof(evoVmcName[slot]),
                 "generic_%d", slot);
    } else if (evoVmcName[slot][0] == '\0' ||
               strcmp(evoVmcName[slot], "generic_0") == 0 ||
               strcmp(evoVmcName[slot], "generic_1") == 0) {
        startup = evoEngineGetStartup(evoEngineGetSelected());
        group = getGroupIdForTitleId(startup);
        snprintf(evoVmcName[slot], sizeof(evoVmcName[slot]),
                 "%.28s_%d", group && group[0] ? group : "game", slot);
    }
}

static void evoVmcBegin(void)
{
    int slot;

    evoVmcSelection = EVO_VMC_ROW_SLOT1;
    evoVmcSizeIndex = 0;
    evoVmcBusy = 0;
    evoVmcBusySlot = -1;
    evoVmcNotice[0] = '\0';
    memset(&evoVmcStatus, 0, sizeof(evoVmcStatus));

    if (!evoEngineLoadSelectedVMC(evoVmcName[0],
                                  sizeof(evoVmcName[0]),
                                  evoVmcName[1],
                                  sizeof(evoVmcName[1]))) {
        snprintf(evoVmcNotice, sizeof(evoVmcNotice),
                 "Select a PS2 game before configuring VMCs.");
        return;
    }

    for (slot = 0; slot < 2; slot++) {
        if (evoVmcName[slot][0] == '\0')
            evoVmcMode[slot] = EVO_VMC_PHYSICAL;
        else if (strcmp(evoVmcName[slot], "generic_0") == 0 ||
                 strcmp(evoVmcName[slot], "generic_1") == 0)
            evoVmcMode[slot] = EVO_VMC_GENERIC;
        else
            evoVmcMode[slot] = EVO_VMC_PER_GAME;
    }
}

static void evoVmcPoll(void)
{
    int result;

    if (!evoVmcBusy)
        return;

    result = fileXioDevctl("genvmc:", 0xC0DE0003, NULL, 0,
                           &evoVmcStatus, sizeof(evoVmcStatus));
    if (result != 0)
        return;

    evoVmcStatus.message[sizeof(evoVmcStatus.message) - 1] = '\0';
    if (evoVmcStatus.status == 0) {
        evoVmcBusy = 0;
        snprintf(evoVmcNotice, sizeof(evoVmcNotice),
                 evoVmcStatus.error == 0 ?
                     "Slot %d VMC is ready." :
                     "Slot %d VMC creation failed (%d).",
                 evoVmcBusySlot + 1, evoVmcStatus.error);
        sfxPlay(evoVmcStatus.error == 0 ? SFX_CONFIRM : SFX_CANCEL);
    }
}

static void evoVmcCreate(int slot)
{
    int currentSize;

    if (evoVmcBusy)
        return;
    if (!gEnableWrite) {
        snprintf(evoVmcNotice, sizeof(evoVmcNotice),
                 "Write operations are disabled.");
        sfxPlay(SFX_CANCEL);
        return;
    }
    if (evoVmcMode[slot] == EVO_VMC_PHYSICAL) {
        snprintf(evoVmcNotice, sizeof(evoVmcNotice),
                 "Slot %d uses the physical memory card.", slot + 1);
        sfxPlay(SFX_CANCEL);
        return;
    }

    evoVmcBuildName(slot);
    currentSize = evoEngineCheckSelectedVMC(evoVmcName[slot], 0);
    if (currentSize == evoVmcSize()) {
        snprintf(evoVmcNotice, sizeof(evoVmcNotice),
                 "Slot %d VMC already exists (%d MB).",
                 slot + 1, currentSize);
        sfxPlay(SFX_CONFIRM);
        return;
    }

    evoEngineCheckSelectedVMC(evoVmcName[slot], evoVmcSize());
    evoVmcBusy = 1;
    evoVmcBusySlot = slot;
    memset(&evoVmcStatus, 0, sizeof(evoVmcStatus));
    snprintf(evoVmcNotice, sizeof(evoVmcNotice),
             "Creating Slot %d VMC. Do not remove storage.",
             slot + 1);
}

static void evoDrawMemoryCards(void)
{
    static const char *const modeNames[] = {
        "Physical card", "Generic VMC", "Per-game VMC"
    };
    static const char *const rowNames[] = {
        "Slot 1 assignment",
        "Slot 2 assignment",
        "VMC file size",
        "Create / verify Slot 1",
        "Create / verify Slot 2",
        "Save game assignments"
    };
    char value[96];
    const char *gameName = evoEngineGetName(evoEngineGetSelected());
    int row;

    evoVmcPoll();
    evoDrawText(174, 62, "VIRTUAL MEMORY CARDS", evoColCyan);
    evoDrawSmallText(174, 84, 340,
                     gameName && gameName[0] ? gameName :
                     "No PS2 game selected", evoColWhite);

    for (row = 0; row < EVO_VMC_ROW_COUNT; row++) {
        const int y = 119 + row * 32;
        if (evoFocus == EVO_FOCUS_CONTENT && row == evoVmcSelection) {
            rmDrawRect(168, y - 4, 350, 27, evoColPanelStrong);
            evoDrawOutline(168, y - 4, 350, 27, evoColCyan);
        }
        evoDrawClippedText(178, y, 208, rowNames[row],
                           row == evoVmcSelection ? evoColWhite : evoColMuted);

        value[0] = '\0';
        if (row == EVO_VMC_ROW_SLOT1 || row == EVO_VMC_ROW_SLOT2) {
            const int slot = row - EVO_VMC_ROW_SLOT1;
            snprintf(value, sizeof(value), "%s",
                     modeNames[evoVmcMode[slot]]);
        } else if (row == EVO_VMC_ROW_SIZE) {
            snprintf(value, sizeof(value), "%d MB", evoVmcSize());
        } else if (row == EVO_VMC_ROW_CREATE1 ||
                   row == EVO_VMC_ROW_CREATE2) {
            const int slot = row - EVO_VMC_ROW_CREATE1;
            snprintf(value, sizeof(value), "%s",
                     evoVmcMode[slot] == EVO_VMC_PHYSICAL ?
                         "Not required" : evoVmcName[slot]);
        } else if (row == EVO_VMC_ROW_SAVE) {
            snprintf(value, sizeof(value), "Write CFG");
        }
        evoDrawSmallText(397, y + 1, 111, value,
                         row == evoVmcSelection ? evoColCyan : evoColWhite);
    }

    evoDrawSmallText(529, 92, 98, "CONTROLS", evoColCyan);
    evoDrawSmallText(529, 116, 98, "Left / Right", evoColWhite);
    evoDrawSmallText(529, 140, 98, "Change value", evoColMuted);
    evoDrawSmallText(529, 180, 98, "X", evoColWhite);
    evoDrawSmallText(529, 204, 98, "Create / Save", evoColMuted);
    evoDrawSmallText(529, 244, 98, "Circle", evoColWhite);
    evoDrawSmallText(529, 268, 98, "Back", evoColMuted);

    if (evoVmcBusy) {
        snprintf(value, sizeof(value), "Creating Slot %d: %d%%",
                 evoVmcBusySlot + 1, evoVmcStatus.progress);
        evoDrawSmallText(174, 322, 340, value, evoColCyan);
        evoDrawOutline(174, 341, 340, 9, evoColBlue);
        rmDrawRect(176, 343,
                   (336 * evoVmcStatus.progress) / 100, 5, evoColGreen);
    } else {
        evoDrawSmallText(174, 330, 340,
                         evoVmcNotice[0] ? evoVmcNotice :
                         "Create the VMC, then save its game assignment.",
                         evoVmcNotice[0] ? evoColCyan : evoColMuted);
    }
}

static void evoDrawNetwork(void)
{
    char text[128];

    evoDrawText(174, 62, "NETWORK", evoColCyan);
    snprintf(text, sizeof(text), "Runtime state: %s", evoNetworkStateName(gNetworkStartup));
    evoDrawText(174, 104, text, evoColWhite);
    snprintf(text, sizeof(text), "SMB startup mode: %s", evoStartModeName(gETHStartMode));
    evoDrawText(174, 140, text, evoColWhite);
    snprintf(text, sizeof(text), "IP mode: %s", ps2_ip_use_dhcp ? "DHCP" : "Static");
    evoDrawText(174, 176, text, evoColWhite);
    snprintf(text, sizeof(text), "PS2 address: %d.%d.%d.%d",
             ps2_ip[0], ps2_ip[1], ps2_ip[2], ps2_ip[3]);
    evoDrawText(174, 212, text, evoColWhite);
    if (gNetworkStartup != 0 && gNetworkStartup >= ERROR_ETH_NOT_STARTED) {
        snprintf(text, sizeof(text), "Diagnostic code: %d", gNetworkStartup);
        evoDrawText(174, 248, text, evoColMuted);
    }
    evoDrawText(174, 304, "Network status is read-only in this build.", evoColMuted);
}

static void evoDrawSettings(void)
{
    char value[32];
    int setting;
    int selected = evoSettingsGetSelected();

    if (evoFocus == EVO_FOCUS_CONTENT)
        rmDrawLine(161, 52, 520, 52, evoColCyan);
    evoDrawText(174, 62, "SETTINGS", evoColCyan);
    for (setting = 0; setting < evoSettingsGetCount(); setting++) {
        const int y = 91 + setting * 26;

        evoSettingsGetValue(setting, value, sizeof(value));
        if (evoFocus == EVO_FOCUS_CONTENT && setting == selected) {
            rmDrawRect(168, y - 4, 350, 24, evoColPanelStrong);
            evoDrawOutline(168, y - 4, 350, 24, evoColCyan);
        }
        evoDrawClippedText(178, y, 238, evoSettingsGetName(setting),
                           setting == selected && evoFocus == EVO_FOCUS_CONTENT ?
                               evoColWhite : evoColMuted);
        evoDrawClippedText(438, y, 70, value,
                           setting == selected && evoFocus == EVO_FOCUS_CONTENT ?
                               evoColCyan : evoColWhite);
    }

    evoDrawSmallText(529, 92, 98, "CONTROLS", evoColCyan);
    evoDrawSmallText(529, 116, 98, "X / Left / Right", evoColWhite);
    evoDrawSmallText(529, 140, 98, "Change value", evoColMuted);
    evoDrawSmallText(529, 180, 98, "Square", evoColWhite);
    evoDrawSmallText(529, 204, 98, "Save changes", evoColMuted);
    evoDrawSmallText(529, 244, 98, "Circle", evoColWhite);
    evoDrawSmallText(529, 268, 98, "Discard / Back", evoColMuted);
    evoDrawSmallText(174, 330, 340,
                     evoSettingsNotice[0] ? evoSettingsNotice :
                     "Display-mode changes take effect after restarting Evolution.",
                     evoSettingsNotice[0] ? evoColCyan : evoColMuted);
}

static void evoDrawPs1(void)
{
    char countText[48];
    char title[EVO_PS1_NAME_MAX];
    int first;
    int row;

    if (evoFocus == EVO_FOCUS_CONTENT)
        rmDrawLine(161, 52, 399, 52, evoColCyan);
    evoDrawText(174, 62, "UNIFIED LIBRARY", evoColCyan);
    evoDrawOutline(217, 83, 42, 23, evoColCyan);
    evoDrawText(174, 86, "PS2", evoColMuted);
    evoDrawText(221, 86, "PS1", evoColWhite);
    evoDrawText(268, 86, "HOMEBREW", evoColMuted);

    first = evoPs1Selection - EVO_PS1_VISIBLE / 2;
    if (first < 0)
        first = 0;
    if (first > evoPs1GameCount - EVO_PS1_VISIBLE)
        first = evoPs1GameCount - EVO_PS1_VISIBLE;
    if (first < 0)
        first = 0;

    for (row = 0; row < EVO_PS1_VISIBLE && first + row < evoPs1GameCount; row++) {
        const int game = first + row;
        const int y = 114 + row * 38;

        if (game == evoPs1Selection) {
            evoDrawOutline(168, y - 4, 220, 34, evoColCyan);
        }
        fntRenderString(EVO_MAIN_FONT, 174, y, ALIGN_NONE, 208, 24,
                        evoPs1Games[game], game == evoPs1Selection ? evoColWhite : evoColMuted);
    }

    rmDrawLine(400, 52, 400, 345, evoColCyan);
    evoDrawText(414, 62, "PLAYSTATION", evoColCyan);
    if (evoPs1GameCount > 0) {
        snprintf(title, sizeof(title), "%s", evoPs1Games[evoPs1Selection]);
        title[strlen(title) - 4] = '\0';
        fntRenderString(EVO_MAIN_FONT, 414, 84, ALIGN_NONE, 210, 44, title, evoColWhite);
    } else {
        evoDrawText(414, 84, "No PS1 games", evoColMuted);
    }
    evoDrawOutline(414, 133, 103, 136, evoColBlue);
    if (!(evoPs1GameCount > 0 &&
          evoPs1ArtDraw(evoPs1Device, evoPs1Games[evoPs1Selection],
                        418, 137, 95, 128)))
        fntRenderString(EVO_MAIN_FONT, 465, 188, ALIGN_CENTER, 90, 24,
                        "NO ART", evoColMuted);
    evoDrawText(529, 138, "SOURCE", evoColCyan);
    evoDrawText(529, 160, evoPs1Device >= 0 ? "USB" : "Missing", evoColWhite);
    evoDrawText(529, 196, "LOADER", evoColCyan);
    evoDrawText(529, 218, "POPStarter", evoColMuted);
    snprintf(countText, sizeof(countText), "%d of %d", evoPs1GameCount > 0 ? evoPs1Selection + 1 : 0, evoPs1GameCount);
    evoDrawText(529, 252, "ITEM", evoColCyan);
    evoDrawText(529, 274, countText, evoColMuted);
    fntRenderString(EVO_MAIN_FONT, 414, 311, ALIGN_NONE, 210, 28, evoPs1Notice, evoColMuted);
}

static void evoDrawSection(void)
{
    switch (evoSection) {
        case EVO_HOME:
            evoDrawHome();
            break;
        case EVO_LIBRARY:
            if (evoLibraryFilter == EVO_LIBRARY_PS1)
                evoDrawPs1();
            else
                evoDrawLibrary();
            break;
        case EVO_MEMORY_CARDS:
            evoDrawMemoryCards();
            break;
        case EVO_NETWORK:
            evoDrawNetwork();
            break;
        case EVO_SETTINGS:
            evoDrawSettings();
            break;
        case EVO_CONTROLLERS:
            evoDrawText(174, 62, "CONTROLLERS", evoColCyan);
            evoDrawText(174, 104, "Press Confirm to open controller status.", evoColWhite);
            break;
        case EVO_DIAGNOSTICS:
            evoDrawText(174, 62, "DIAGNOSTICS", evoColCyan);
            evoDrawText(174, 104, "Press Confirm to open read-only diagnostics.", evoColWhite);
            break;
    }
}

static void evoDrawOverlay(void)
{
    const char *const *labels;
    const char *title;
    int count;
    int panelHeight;
    int panelY;
    int item;

    if (evoOverlay == EVO_OVERLAY_NONE)
        return;

    if (evoOverlay == EVO_OVERLAY_OPTIONS) {
        labels = evoOptionLabels;
        title = "OPTIONS";
        count = EVO_OPTION_COUNT;
    } else {
        labels = evoSystemLabels;
        title = "SYSTEM MENU";
        count = EVO_SYSTEM_COUNT;
    }

    panelHeight = 70 + count * 30;
    panelY = (480 - panelHeight) / 2;
    rmDrawRect(0, 0, 640, 480, GS_SETREG_RGBA(0x00, 0x00, 0x00, 0x58));
    rmDrawRect(151, panelY, 338, panelHeight, evoColPanelStrong);
    evoDrawOutline(151, panelY, 338, panelHeight, evoColCyan);
    evoDrawClippedText(171, panelY + 14, 298, title, evoColCyan);
    rmDrawLine(171, panelY + 40, 469, panelY + 40, evoColBlue);

    for (item = 0; item < count; item++) {
        const int y = panelY + 50 + item * 30;
        if (item == evoOverlaySelection) {
            rmDrawRect(165, y - 4, 310, 26, evoColPanelStrong);
            evoDrawOutline(165, y - 4, 310, 26, evoColCyan);
        }
        evoDrawClippedText(177, y, 286, labels[item],
                           item == evoOverlaySelection ? evoColWhite : evoColMuted);
    }

    if (evoOverlayNotice[0] != '\0')
        evoDrawSmallText(171, panelY + panelHeight - 18, 298,
                         evoOverlayNotice, evoColCyan);
}

static void evoOpenEvolutionSettings(int selected)
{
    evoOverlay = EVO_OVERLAY_NONE;
    evoSection = EVO_SETTINGS;
    evoFocus = EVO_FOCUS_CONTENT;
    evoSettingsBegin();
    evoSettingsSetSelected(selected);
    evoSettingsNotice[0] = '\0';
}

static void evoRunOptionAction(void)
{
    const int action = evoOverlaySelection;

    evoOverlay = EVO_OVERLAY_NONE;
    sfxPlay(SFX_CONFIRM);
    switch (action) {
        case EVO_OPTION_EVOLUTION:
            evoOpenEvolutionSettings(0);
            break;
        case EVO_OPTION_DISPLAY:
            /*
             * Legacy OPL's modal UI dialog assumes its original menu owns the
             * foreground and can deadlock the standalone Evolution screen.
             * Open the native Evolution settings page at Widescreen instead;
             * Video mode is on the same page and applies after restart.
             */
            evoOpenEvolutionSettings(EVO_SETTING_WIDESCREEN);
            break;
        case EVO_OPTION_AUDIO:
            evoOpenEvolutionSettings(EVO_SETTING_SFX);
            break;
        case EVO_OPTION_CONTROLLER:
            guiSwitchScreen(GUI_SCREEN_EVO_CONTROLLERS);
            break;
        case EVO_OPTION_NETWORK:
            evoSection = EVO_NETWORK;
            evoFocus = EVO_FOCUS_CONTENT;
            break;
        case EVO_OPTION_DIAGNOSTICS:
            guiSwitchScreen(GUI_SCREEN_EVO_DIAGNOSTICS);
            break;
        case EVO_OPTION_BACK:
        default:
            sfxPlay(SFX_CANCEL);
            break;
    }
    readPads();
}

static void evoRunSystemAction(void)
{
    const int action = evoOverlaySelection;

    if (action == EVO_SYSTEM_RESUME) {
        evoOverlay = EVO_OVERLAY_NONE;
        sfxPlay(SFX_CANCEL);
    } else if (action == EVO_SYSTEM_OPTIONS) {
        evoOverlay = EVO_OVERLAY_OPTIONS;
        evoOverlaySelection = 0;
        evoOverlayNotice[0] = '\0';
        sfxPlay(SFX_CONFIRM);
    } else if (action == EVO_SYSTEM_SAVE) {
        const int saved = saveConfig(CONFIG_OPL | CONFIG_NETWORK | CONFIG_GAME, 0);
        snprintf(evoOverlayNotice, sizeof(evoOverlayNotice), "%s",
                 saved ? "Settings saved." : "Settings save failed.");
        sfxPlay(saved ? SFX_CONFIRM : SFX_CANCEL);
    } else if (action == EVO_SYSTEM_EXIT) {
        evoOverlay = EVO_OVERLAY_NONE;
        if (guiMsgBox("Exit OPL Evolution and return to the PS2 Browser?", 1, NULL))
            sysExecExit();
    } else if (action == EVO_SYSTEM_POWER_OFF) {
        evoOverlay = EVO_OVERLAY_NONE;
        if (guiMsgBox("Power off the PlayStation 2?", 1, NULL))
            sysPowerOff();
    }
    readPads();
}

static void evoHandleOverlayInput(void)
{
    const int count = evoOverlay == EVO_OVERLAY_OPTIONS ?
                          EVO_OPTION_COUNT :
                          EVO_SYSTEM_COUNT;

    if (getKeyOn(KEY_UP)) {
        evoOverlaySelection = evoOverlaySelection > 0 ?
                                  evoOverlaySelection - 1 :
                                  count - 1;
        sfxPlay(SFX_CURSOR);
    } else if (getKeyOn(KEY_DOWN)) {
        evoOverlaySelection = (evoOverlaySelection + 1) % count;
        sfxPlay(SFX_CURSOR);
    } else if (getKeyOn(gSelectButton)) {
        if (evoOverlay == EVO_OVERLAY_OPTIONS)
            evoRunOptionAction();
        else
            evoRunSystemAction();
    } else if (getKeyOn(evoCancelButton()) ||
               getKeyOn(KEY_SQUARE) || getKeyOn(KEY_TRIANGLE)) {
        evoOverlay = EVO_OVERLAY_NONE;
        evoOverlayNotice[0] = '\0';
        sfxPlay(SFX_CANCEL);
    }
}

void evoUiSetEnabled(int enabled)
{
    evoUiEnabled = enabled ? 1 : 0;
    if (evoUiEnabled) {
        evoEngineSync();
        if (evoMainFont == FNT_ERROR)
            evoMainFont = fntLoadFile(NULL, 15);
        if (evoSmallFont == FNT_ERROR)
            evoSmallFont = fntLoadFile(NULL, 12);
        evoSection = EVO_HOME;
        evoFocus = EVO_FOCUS_NAV;
        evoLibraryFilter = EVO_LIBRARY_PS2;
        evoLaunchInProgress = 0;
        evoLaunchRequest = EVO_LAUNCH_NONE;
        evoOverlay = EVO_OVERLAY_NONE;
        evoOverlaySelection = 0;
        evoOverlayNotice[0] = '\0';
        evoSettingsNotice[0] = '\0';
        evoRescanPs1();
    }
}

int evoUiIsEnabled(void)
{
    return evoUiEnabled;
}

void evoUiRenderMain(void)
{
    if (!evoUiEnabled)
        return;

    rmDrawRect(0, 0, 640, 480, evoColBackground);
    if (evoLaunchInProgress) {
        evoDrawOutline(128, 188, 384, 82, evoColBlue);
        fntRenderString(EVO_MAIN_FONT, 320, 210, ALIGN_CENTER, 360, 24,
                        "OPL EVOLUTION", evoColCyan);
        fntRenderString(EVO_MAIN_FONT, 320, 238, ALIGN_CENTER, 360, 24,
                        evoLaunchNotice, evoColWhite);
        if (evoLaunchRequest == EVO_LAUNCH_NONE)
            fntRenderString(EVO_MAIN_FONT, 320, 258, ALIGN_CENTER, 360, 20,
                            "Press Circle to return", evoColMuted);
        return;
    }
    evoEngineSync();
    thmLoadEvolutionDashboard();
    evoDrawTexture(EVO_DASHBOARD_BG, 0, 0, 640, 480, gDefaultCol);
    evoDrawTitle(evoSectionLabels[evoSection]);
    evoDrawNavigation();
    evoDrawSection();
    evoDrawSystemCards();
    evoDrawOverlay();
}

static void evoOpenSection(void)
{
    sfxPlay(SFX_CONFIRM);

    switch (evoSection) {
        case EVO_HOME:
            evoSection = EVO_LIBRARY;
            evoSetLibraryFilter(evoLibraryFilter);
            evoFocus = EVO_FOCUS_CONTENT;
            break;
        case EVO_LIBRARY:
            evoSetLibraryFilter(evoLibraryFilter);
            evoFocus = EVO_FOCUS_CONTENT;
            break;
        case EVO_CONTROLLERS:
            guiSwitchScreen(GUI_SCREEN_EVO_CONTROLLERS);
            break;
        case EVO_MEMORY_CARDS:
            evoVmcBegin();
            evoFocus = EVO_FOCUS_CONTENT;
            break;
        case EVO_DIAGNOSTICS:
            guiSwitchScreen(GUI_SCREEN_EVO_DIAGNOSTICS);
            break;
        case EVO_NETWORK:
            /*
             * Do not enter the legacy modal network dialog from Evolution.
             * It shares the same foreground-ownership problem as UI config.
             */
            evoFocus = EVO_FOCUS_CONTENT;
            break;
        case EVO_SETTINGS:
            evoSettingsBegin();
            evoSettingsNotice[0] = '\0';
            evoFocus = EVO_FOCUS_CONTENT;
            break;
        default:
            break;
    }
}

void evoUiHandleInputMain(void)
{
    if (!evoUiEnabled)
        return;

    /*
     * A failed clean restart must remain visible on real hardware.  Earlier
     * builds cleared this state immediately, producing only a one-frame flash
     * and hiding the actual reason that the loader returned.
     */
    if (evoLaunchInProgress && evoLaunchRequest == EVO_LAUNCH_NONE) {
        if (getKeyOn(evoCancelButton()) || getKeyOn(KEY_CIRCLE)) {
            evoLaunchInProgress = 0;
            sfxPlay(SFX_CANCEL);
        }
        return;
    }

    if (evoLaunchRequest != EVO_LAUNCH_NONE) {
        const int request = evoLaunchRequest;
        int result = 0;

        if (request == EVO_LAUNCH_PS2_PREPARE) {
            if (ioHasPendingRequests()) {
                snprintf(evoLaunchNotice, sizeof(evoLaunchNotice),
                         "Finishing background artwork I/O...");
                return;
            }

            evoLaunchRequest = EVO_LAUNCH_PS2_COMMIT;
            snprintf(evoLaunchNotice, sizeof(evoLaunchNotice),
                     "Starting Evolution PS2 engine...");
            return;
        } else if (request == EVO_LAUNCH_PS2_COMMIT) {
            evoLaunchRequest = EVO_LAUNCH_NONE;
            /*
             * Evolution owns the transition and launches through its own engine
             * selection.  The original OPL menu lifecycle is never entered.
             */
            evoBootPlay(EVO_BOOT_PS2, -1, NULL);
            thmUnloadEvolutionDashboard();
            result = evoEngineLaunchSelected();
        } else {
            evoLaunchRequest = EVO_LAUNCH_NONE;
            evoBootPlay(EVO_BOOT_PS1, evoPs1Device,
                        evoPs1GameCount > 0 ?
                            evoPs1Games[evoPs1Selection] : NULL);
            evoLaunchPs1Game();
        }

        /*
         * A successful launch transfers control and never reaches here.
         * Restore the interactive dashboard if preparation failed.
         */
        if (request != EVO_LAUNCH_PS1 && result < 0) {
            evoLaunchInProgress = 1;
            switch (result) {
                case -1:
                case -2:
                    snprintf(evoLaunchNotice, sizeof(evoLaunchNotice),
                             "No launchable PS2 item (%d)", result);
                    break;
                case -3:
                    snprintf(evoLaunchNotice, sizeof(evoLaunchNotice),
                             "PS2 configuration unavailable (-3)");
                    break;
                case -4:
                    snprintf(evoLaunchNotice, sizeof(evoLaunchNotice),
                             "Evolution PS2 engine returned (-4)");
                    break;
                case -5:
                    snprintf(evoLaunchNotice, sizeof(evoLaunchNotice),
                             "Split/UL format is not supported (-5)");
                    break;
                case -6:
                    snprintf(evoLaunchNotice, sizeof(evoLaunchNotice),
                             "Legacy launch path returned (-6)");
                    break;
                default:
                    snprintf(evoLaunchNotice, sizeof(evoLaunchNotice),
                             "Launch failed (%d)", result);
                    break;
            }
        } else {
            evoLaunchInProgress = 0;
        }
        return;
    }

    if (evoOverlay != EVO_OVERLAY_NONE) {
        evoHandleOverlayInput();
        return;
    }

    if (getKeyOn(KEY_TRIANGLE)) {
        evoOverlay = EVO_OVERLAY_SYSTEM;
        evoOverlaySelection = 0;
        evoOverlayNotice[0] = '\0';
        sfxPlay(SFX_CONFIRM);
        return;
    }
    if (getKeyOn(KEY_SQUARE) &&
        !(evoSection == EVO_SETTINGS && evoFocus == EVO_FOCUS_CONTENT)) {
        evoOverlay = EVO_OVERLAY_OPTIONS;
        evoOverlaySelection = 0;
        evoOverlayNotice[0] = '\0';
        sfxPlay(SFX_CONFIRM);
        return;
    }

    if (getKeyOn(KEY_START)) {
        evoSection = EVO_HOME;
        evoFocus = EVO_FOCUS_NAV;
        sfxPlay(SFX_CURSOR);
        return;
    }
    if (getKeyOn(KEY_SELECT)) {
        guiSwitchScreen(GUI_SCREEN_EVO_DIAGNOSTICS);
        return;
    }

    if (evoFocus == EVO_FOCUS_NAV) {
        if (getKeyOn(KEY_RIGHT) || getKeyOn(gSelectButton)) {
            evoOpenSection();
        } else if (getKeyOn(KEY_UP)) {
            evoSection = evoSection > 0 ? evoSection - 1 : EVO_SECTION_COUNT - 1;
            sfxPlay(SFX_CURSOR);
        } else if (getKeyOn(KEY_DOWN)) {
            evoSection = (evoSection + 1) % EVO_SECTION_COUNT;
            sfxPlay(SFX_CURSOR);
        } else if (evoSection == EVO_HOME && getKeyOn(KEY_SQUARE)) {
            evoEngineRefresh();
        } else if (getKeyOn(evoCancelButton()) || getKeyOn(KEY_LEFT)) {
            evoSection = EVO_HOME;
            evoFocus = EVO_FOCUS_NAV;
            sfxPlay(SFX_CANCEL);
        }
        return;
    }

    if (evoSection == EVO_LIBRARY) {
        if (getKeyOn(KEY_L1)) {
            evoSetLibraryFilter(evoLibraryFilter - 1);
            sfxPlay(SFX_CURSOR);
        } else if (getKeyOn(KEY_R1)) {
            evoSetLibraryFilter(evoLibraryFilter + 1);
            sfxPlay(SFX_CURSOR);
        } else if (evoLibraryFilter == EVO_LIBRARY_PS1) {
            if (getKeyOn(gSelectButton)) {
                sfxPlay(SFX_CONFIRM);
                evoLaunchSelectedPs1();
            } else if (getKeyOn(KEY_UP) && evoPs1Selection > 0) {
                evoPs1Selection--;
                sfxPlay(SFX_CURSOR);
            } else if (getKeyOn(KEY_DOWN) && evoPs1Selection + 1 < evoPs1GameCount) {
                evoPs1Selection++;
                sfxPlay(SFX_CURSOR);
            } else if (getKeyOn(KEY_L2) && evoPs1GameCount > 0) {
                evoPs1Selection -= EVO_PS1_VISIBLE;
                if (evoPs1Selection < 0)
                    evoPs1Selection = 0;
                sfxPlay(SFX_CURSOR);
            } else if (getKeyOn(KEY_R2) && evoPs1GameCount > 0) {
                evoPs1Selection += EVO_PS1_VISIBLE;
                if (evoPs1Selection >= evoPs1GameCount)
                    evoPs1Selection = evoPs1GameCount - 1;
                sfxPlay(SFX_CURSOR);
            } else if (getKeyOn(KEY_SQUARE)) {
                evoRescanPs1();
                sfxPlay(evoPs1GameCount > 0 ? SFX_CONFIRM : SFX_CANCEL);
            }
        } else if (evoLibraryFilter == EVO_LIBRARY_PS2) {
            if (getKeyOn(gSelectButton)) {
                sfxPlay(SFX_CONFIRM);
                evoLaunchSelectedPs2();
            } else if (getKeyOn(KEY_UP))
                evoEngineMove(-1);
            else if (getKeyOn(KEY_DOWN))
                evoEngineMove(1);
            else if (getKeyOn(KEY_L2))
                evoEngineMovePage(-1, 6);
            else if (getKeyOn(KEY_R2))
                evoEngineMovePage(1, 6);
            else if (getKeyOn(KEY_TRIANGLE)) {
                snprintf(evoLaunchNotice, sizeof(evoLaunchNotice),
                         "Standalone game options are not ready yet");
                evoLaunchInProgress = 1;
                evoLaunchRequest = EVO_LAUNCH_NONE;
            } else if (getKeyOn(KEY_SQUARE))
                evoEngineRefresh();
        } else if (getKeyOn(gSelectButton)) {
            snprintf(evoLaunchNotice, sizeof(evoLaunchNotice),
                     "Evolution homebrew scanner is not ready");
            evoLaunchInProgress = 1;
            evoLaunchRequest = EVO_LAUNCH_NONE;
        }
    } else if (evoSection == EVO_MEMORY_CARDS) {
        if (evoVmcBusy) {
            /* Creation is asynchronous; keep this screen alive until done. */
            return;
        }
        if (getKeyOn(KEY_UP)) {
            evoVmcSelection = evoVmcSelection > 0 ?
                                  evoVmcSelection - 1 :
                                  EVO_VMC_ROW_COUNT - 1;
            sfxPlay(SFX_CURSOR);
        } else if (getKeyOn(KEY_DOWN)) {
            evoVmcSelection =
                (evoVmcSelection + 1) % EVO_VMC_ROW_COUNT;
            sfxPlay(SFX_CURSOR);
        } else if (getKeyOn(KEY_LEFT) || getKeyOn(KEY_RIGHT)) {
            const int direction = getKeyOn(KEY_LEFT) ? -1 : 1;
            if (evoVmcSelection == EVO_VMC_ROW_SLOT1 ||
                evoVmcSelection == EVO_VMC_ROW_SLOT2) {
                const int slot = evoVmcSelection - EVO_VMC_ROW_SLOT1;
                evoVmcMode[slot] += direction;
                if (evoVmcMode[slot] < 0)
                    evoVmcMode[slot] = EVO_VMC_MODE_COUNT - 1;
                else if (evoVmcMode[slot] >= EVO_VMC_MODE_COUNT)
                    evoVmcMode[slot] = 0;
                evoVmcBuildName(slot);
                evoVmcNotice[0] = '\0';
                sfxPlay(SFX_CURSOR);
            } else if (evoVmcSelection == EVO_VMC_ROW_SIZE) {
                evoVmcSizeIndex += direction;
                if (evoVmcSizeIndex < 0)
                    evoVmcSizeIndex = 3;
                else if (evoVmcSizeIndex > 3)
                    evoVmcSizeIndex = 0;
                evoVmcNotice[0] = '\0';
                sfxPlay(SFX_CURSOR);
            }
        } else if (getKeyOn(gSelectButton)) {
            if (evoVmcSelection == EVO_VMC_ROW_SLOT1 ||
                evoVmcSelection == EVO_VMC_ROW_SLOT2) {
                const int slot = evoVmcSelection - EVO_VMC_ROW_SLOT1;
                evoVmcMode[slot] =
                    (evoVmcMode[slot] + 1) % EVO_VMC_MODE_COUNT;
                evoVmcBuildName(slot);
                evoVmcNotice[0] = '\0';
                sfxPlay(SFX_CONFIRM);
            } else if (evoVmcSelection == EVO_VMC_ROW_SIZE) {
                evoVmcSizeIndex = (evoVmcSizeIndex + 1) % 4;
                evoVmcNotice[0] = '\0';
                sfxPlay(SFX_CONFIRM);
            } else if (evoVmcSelection == EVO_VMC_ROW_CREATE1) {
                evoVmcCreate(0);
            } else if (evoVmcSelection == EVO_VMC_ROW_CREATE2) {
                evoVmcCreate(1);
            } else if (evoVmcSelection == EVO_VMC_ROW_SAVE) {
                evoVmcBuildName(0);
                evoVmcBuildName(1);
                if (evoEngineSaveSelectedVMC(evoVmcName[0],
                                             evoVmcName[1])) {
                    snprintf(evoVmcNotice, sizeof(evoVmcNotice),
                             "VMC assignments saved for this game.");
                    sfxPlay(SFX_CONFIRM);
                } else {
                    snprintf(evoVmcNotice, sizeof(evoVmcNotice),
                             "VMC assignment save failed.");
                    sfxPlay(SFX_CANCEL);
                }
            }
        } else if (getKeyOn(evoCancelButton())) {
            evoFocus = EVO_FOCUS_NAV;
            sfxPlay(SFX_CANCEL);
            return;
        }
    } else if (evoSection == EVO_SETTINGS) {
        if (getKeyOn(KEY_UP)) {
            evoSettingsMove(-1);
            sfxPlay(SFX_CURSOR);
        } else if (getKeyOn(KEY_DOWN)) {
            evoSettingsMove(1);
            sfxPlay(SFX_CURSOR);
        } else if (getKeyOn(KEY_LEFT)) {
            evoSettingsChange(-1);
            evoSettingsNotice[0] = '\0';
            sfxPlay(SFX_CURSOR);
        } else if (getKeyOn(KEY_RIGHT) || getKeyOn(gSelectButton)) {
            evoSettingsChange(1);
            evoSettingsNotice[0] = '\0';
            sfxPlay(SFX_CONFIRM);
        } else if (getKeyOn(KEY_SQUARE)) {
            const int saved = evoSettingsSave();
            snprintf(evoSettingsNotice, sizeof(evoSettingsNotice),
                     "%s", saved ? "Evolution settings saved." :
                                   "Settings changed, but the save failed.");
            sfxPlay(saved ? SFX_CONFIRM : SFX_CANCEL);
        } else if (getKeyOn(evoCancelButton())) {
            evoSettingsDiscard();
            evoSettingsNotice[0] = '\0';
            evoFocus = EVO_FOCUS_NAV;
            sfxPlay(SFX_CANCEL);
            return;
        }
    } else if (getKeyOn(gSelectButton)) {
        evoOpenSection();
    }

    if (evoSection != EVO_SETTINGS &&
        evoSection != EVO_MEMORY_CARDS &&
        (getKeyOn(KEY_LEFT) || getKeyOn(evoCancelButton()))) {
        evoFocus = EVO_FOCUS_NAV;
        sfxPlay(SFX_CANCEL);
    }
}

static void evoHandlePlaceholderBack(void)
{
    if (getKeyOn(KEY_START) || getKeyOn(evoCancelButton()))
        guiSwitchScreen(GUI_SCREEN_MAIN);
}

void evoUiRenderControllers(void)
{
    int i;
    char player[48];
    char status[96];

    rmDrawRect(0, 0, 640, 480, evoColBackground);
    thmLoadEvolutionDashboard();
    evoDrawTexture(EVO_DASHBOARD_BG, 0, 0, 640, 480, gDefaultCol);
    evoDrawTitle("CONTROLLERS");
    evoDrawNavigation();

#ifdef PADEMU
    evoPollControllerStatus();
#endif

    evoDrawText(174, 62, "CONTROLLER STATUS", evoColCyan);
    evoDrawText(174, 88, "Bluetooth adapter", evoColCyan);
#ifdef PADEMU
    {
        const int btStatus = evoBtStatus[0];

        if (btStatus & DS34BT_STATE_USB_CONFIGURED)
            evoDrawText(310, 88, "Detected and configured", evoColWhite);
        else if (btStatus & DS34BT_STATE_USB_AUTHORIZED)
            evoDrawText(310, 88, "Authorized; configuration pending", evoColMuted);
        else
            evoDrawText(310, 88, "Not detected / subsystem inactive", evoColMuted);

        evoDrawText(174, 114, "Adapter identity", evoColCyan);
        if (evoAdapterInfoValid) {
            snprintf(status, sizeof(status), "USB %04X:%04X  HCI %u",
                     evoAdapterInfo.vid, evoAdapterInfo.pid, evoAdapterInfo.hci_ver);
            evoDrawText(310, 114, status, evoColWhite);
        } else {
            evoDrawText(310, 114, "Unavailable until adapter initializes", evoColMuted);
        }
    }
#else
    evoDrawText(310, 88, "PADEMU not compiled", evoColMuted);
    evoDrawText(174, 114, "Controller subsystem", evoColCyan);
    evoDrawText(310, 114, "PADEMU not compiled", evoColMuted);
#endif

    for (i = 0; i < 4; i++) {
        snprintf(player, sizeof(player), "Player %d", i + 1);
        evoControllerStatus(i, status, sizeof(status));
        if (i == evoControllerSelection)
            rmDrawRect(168, 139 + i * 36, 456, 28, evoColPanelStrong);
        evoDrawText(174, 145 + i * 36, player, evoColWhite);
        evoDrawText(310, 145 + i * 36, status,
                    i == evoControllerSelection ? evoColCyan : evoColMuted);
    }

    evoDrawText(174, 299, "Configuration", evoColCyan);
    evoDrawText(310, 299,
                evoControllerNotice[0] ? evoControllerNotice : "Confirm: pairing and PADEMU settings",
                evoColMuted);
    evoDrawSystemCards();
    evoDrawStatusStrip("Up/Down: Inspect player  |  Confirm: Settings  |  Square: Re-enable 2 wireless slots  |  Back");
}

void evoUiHandleInputControllers(void)
{
    if (getKeyOn(gSelectButton)) {
        snprintf(evoControllerNotice, sizeof(evoControllerNotice),
                 "Standalone controller settings are not ready");
    } else if (getKeyOn(KEY_UP)) {
        evoControllerSelection = evoControllerSelection > 0 ? evoControllerSelection - 1 : 3;
        sfxPlay(SFX_CURSOR);
    } else if (getKeyOn(KEY_DOWN)) {
        evoControllerSelection = (evoControllerSelection + 1) % 4;
        sfxPlay(SFX_CURSOR);
    }
#ifdef PADEMU
    else if (getKeyOn(KEY_SQUARE)) {
        const int usbResult = ds34usb_reinit_ports(0x03);
        const int btResult = ds34bt_reinit_ports(0x03);

        if (usbResult || btResult)
            snprintf(evoControllerNotice, sizeof(evoControllerNotice), "Two USB/Bluetooth slots re-enabled");
        else
            snprintf(evoControllerNotice, sizeof(evoControllerNotice), "Controller modules are not active");
        sfxPlay(usbResult || btResult ? SFX_CONFIRM : SFX_MESSAGE);
    }
#endif
    else {
        evoHandlePlaceholderBack();
    }
}

void evoUiRenderDiagnostics(void)
{
    char text[160];
    int nativeWidth;
    int nativeHeight;

    rmDrawRect(0, 0, 640, 480, evoColBackground);
    thmLoadEvolutionDashboard();
    evoDrawTexture(EVO_DASHBOARD_BG, 0, 0, 640, 480, gDefaultCol);
    evoDrawTitle("DIAGNOSTICS");
    evoDrawNavigation();
#ifdef PADEMU
    evoPollControllerStatus();
#endif

    evoDrawText(174, 62, "DIAGNOSTICS", evoColCyan);
    evoDrawText(174, 88, "OPL Evolution", evoColCyan);
    evoDrawText(310, 88, OPL_EVO_VERSION, evoColWhite);
    evoDrawText(174, 114, "Upstream OPL", evoColCyan);
    evoDrawText(310, 114, OPL_VERSION, evoColWhite);
    evoDrawText(174, 140, "Console ROM", evoColCyan);
    evoDrawText(310, 140, evoRomVersion(), evoColWhite);
    snprintf(text, sizeof(text), "%d MiB", (int)(GetMemorySize() / (1024 * 1024)));
    evoDrawText(174, 166, "Main memory", evoColCyan);
    evoDrawText(310, 166, text, evoColWhite);

    rmGetScreenExtentsNative(&nativeWidth, &nativeHeight);
    snprintf(text, sizeof(text), "Mode %d, %dx%d, H-sync %u kHz", gVMode, nativeWidth, nativeHeight, rmGetHsync());
    evoDrawText(174, 192, "Active video", evoColCyan);
    evoDrawText(310, 192, text, evoColWhite);

    snprintf(text, sizeof(text), "BDM %s  HDD %s  SMB %s  Apps %s",
             evoStartModeName(gBDMStartMode), evoStartModeName(gHDDStartMode),
             evoStartModeName(gETHStartMode), evoStartModeName(gAPPStartMode));
    evoDrawText(174, 218, "Storage config", evoColCyan);
    evoDrawText(310, 218, text, evoColWhite);

    snprintf(text, sizeof(text), "MX4SIO %s  iLink %s", gEnableMX4SIO ? "on" : "off", gEnableILK ? "on" : "off");
    evoDrawText(174, 244, "BDM options", evoColCyan);
    evoDrawText(310, 244, text, evoColWhite);

    evoDrawText(174, 270, "Network", evoColCyan);
    evoDrawText(310, 270, evoNetworkStateName(gNetworkStartup),
                gNetworkStartup == 0 ? evoColWhite : evoColMuted);

    evoDrawText(174, 296, "Player 1", evoColCyan);
    evoControllerStatus(0, text, sizeof(text));
    evoDrawText(310, 296, text, evoColWhite);

    evoDrawText(174, 322, "Artwork", evoColCyan);
    evoDrawText(310, 322, gEnableArt ? "Enabled; bounded on-demand cache" : "Disabled", evoColWhite);

    evoDrawSystemCards();
    evoDrawStatusStrip("Read-only diagnostics  |  Back: regional cancel button");
}

void evoUiHandleInputDiagnostics(void)
{
    evoHandlePlaceholderBack();
}
