# OPL Evolution Architecture

This document maps upstream OPL at commit `3e3f34e9f94b058f7fd5b13727cb86e94fd3b35d`. It was prepared after the untouched baseline build and before application-source changes.

## Runtime shape

OPL is a native PlayStation 2 program split across:

1. An EE-side UI/application (`src/`) linked as `opl.elf`.
2. An embedded EE game-loader core (`ee_core/ee_core.elf`) converted into a C array and linked into the UI ELF.
3. Multiple IOP IRX modules (`modules/` and PS2SDK IRX files), also embedded into the UI ELF.
4. A support abstraction (`item_list_t` in `include/iosupport.h`) that connects device discovery, list generation, metadata/config, artwork, and launching to the common menu.

The packed release flow is `opl.elf` → `opl_stripped.elf` → `OPNPS2LD.ELF` via `ps2-packer`.

## Module map and Evolution policy

| Area and source path | Responsibility and dependencies | Evolution policy / integration point | Risk and memory notes |
| --- | --- | --- | --- |
| Application entry: `src/opl.c:main`, `init`, `deferredInit` | Resets the PS2, initializes pads/config/rendering/language/theme/GUI/I/O/menu, starts deferred device work, then runs intro and main GUI loops. | Extend only for Evolution startup identity and a bounded recovery-combination decision. Keep initialization order. | Very high boot-risk. A wait or allocation here can brick startup. No unbounded hardware waits may be added. |
| EE UI/application: `src/*.c`, `include/*.h` | Main CPU application, rendering orchestration, configuration, device support, settings, launch preparation. | Add a compact, isolated Evolution UI module and minimal hooks. | EE has 32 MiB RAM shared by application data, embedded modules, textures, and launch preparation. Avoid permanent large buffers. |
| IOP modules: `modules/` | Storage, network, CD/DVD emulation, VMC, PADEMU, DS3/DS4, debug, and service IRX modules. | Remain unchanged in milestone 1. Future controller work should extend existing RPC/IRX boundaries rather than duplicate them. | IOP memory is highly constrained; IRX growth directly affects module load viability and game compatibility. |
| Embedded game loader: `ee_core/src/main.c`, `ee_core/src/*` | Final EE-side loader core; receives prepared module/config data and transfers control to the game. | Remain unchanged in milestone 1. | Critical compatibility path. Changes risk all launch backends and in-game reset behavior. |
| Menu model/input: `src/menusys.c`, `include/menusys.h` | Linked-list menu and submenu model, current selection, paging, main/settings/info screens, region-aware Cross/Circle callbacks. | Extend with read-only selection accessors and two Evolution placeholder menu entries. Existing launch callbacks remain authoritative. | Compact linked nodes, but cache arrays are allocated per item. Do not duplicate game lists. Input changes can make recovery impossible. |
| GUI loop/transitions: `src/gui.c`, `include/gui.h` | Frame loop, pad read timing, screen handler table, transitions, overlays, notifications, message boxes, dialogs. | Register Evolution wrappers/screens behind `OPL_EVO`. Normal handlers remain compiled for the upstream target. | Runs every frame. Avoid allocation, I/O, hardware RPC, or complex effects in render functions. |
| Rendering: `src/renderman.c`, `include/renderman.h` | gsKit/dmaKit setup, virtual 640×480 coordinate system, aspect correction, rectangles/lines/pixmaps, frame queueing. | Reuse flat rectangles, lines, and existing fonts only. | GS VRAM is 4 MiB. Do not add textures or off-screen buffers for the milestone shell. |
| Fonts: `src/fntsys.c` | FreeType-backed text measurement and rendering with theme fonts. | Reuse for CRT-readable labels and status values. | Avoid long or frequently reformatted strings and extra fonts. |
| Theme system: `src/themes.c`, `include/themes.h`, `misc/conf_theme_OPL.cfg` | Parses theme elements, renders backgrounds, item lists, attributes, images, hints, and default fallback layout. | Do not refactor. Evolution shell overlays the existing rendered list so custom themes and list behavior survive. | Theme art and caches consume EE RAM/VRAM. Overlay must not preload additional art. |
| Texture/artwork: `src/textures.c`, `src/texcache.c`, `src/atlas.c` | Embedded texture registration, asynchronous/cache-managed artwork, atlas/font assets. | Remain unchanged in milestone 1. Future unified library should keep on-demand bounded caches. | High VRAM/RAM and I/O risk, especially over USB 1.1. Never preload a full library. |
| Common game/support model: `include/iosupport.h`, `src/supportbase.c` | `item_list_t` virtual interface; ISO/UL parsing, config population, cheats, image lookup, preparation shared by backends. | Preserve. Future unified records should adapt to this interface rather than bypass it. | Central compatibility code. Incorrect metadata or buffer sizing can corrupt launch data. |
| Game scanning/list model: `src/supportbase.c`, `src/bdmsupport.c`, `src/hddsupport.c`, `src/ethsupport.c`, `src/appsupport.c` | Scans ISO/ZSO/UL/HDLoader/apps by backend, creates `base_game_info_t` or backend records, feeds submenu nodes. | Reuse current submenu as milestone central list. Future indexing belongs behind support implementations with bounded cache files. | Slow devices and fragmentation make scanning latency-sensitive. Avoid blocking the GUI or retaining duplicate records. |
| USB/MX4SIO/iLink/BDM: `src/bdmsupport.c`, `modules/bdmevent`, PS2SDK BDM/USB/iLink/MX4SIO IRXs | Common block-device mode with per-device data; selects USB, iLink, MX4SIO, or ATA launch mode. | Remain unchanged. Diagnostics may display configured/enabled state only until a safe status API exists. | Core driver modifications can regress all removable-media launches. Device removal must remain handled. |
| Internal HDD: `src/hddsupport.c`, `src/hdd.c`, `modules/hdd`, `modules/iopcore/cdvdman` | APA/PFS and exFAT-related HDD list/launch preparation, HDLoader records, HDD CDVDMAN and VMC paths. | Remain unchanged. | High compatibility and data-integrity risk. Never change partition/write behavior for UI work. |
| SMB/network: `src/ethsupport.c`, `modules/network`, `src/httpclient.c`, `src/nbns.c` | DEV9/network startup, DHCP/static configuration, SMBv1 game lists, SMB CDVDMAN, network diagnostics/error codes, compatibility downloads, NBD. | Remain unchanged. Diagnostics may read `gNetworkStartup`; no new protocols in milestone 1. | Blocking network operations and IOP memory are major risks. Credentials must never be logged. |
| PS2 launch dispatch: `src/opl.c:itemExecSelect`, `item_list_t.itemLaunch`, backend `*LaunchGame`, `src/system.c:sysLaunchLoaderElf` | Region-aware select callback loads config, invokes backend launch, embeds/prepares IRXs, deinitializes UI, and enters the existing EE core. | Must remain the only PS2 launch path. Evolution main input delegates to `menuHandleInputMain`. | Highest functional risk. A duplicate loader or changed compatibility flags is prohibited. |
| ELF application launch: `src/appsupport.c`, `src/system.c` | Finds application ELF, resolves device paths, deinitializes OPL, and executes the selected ELF. | Remain unchanged. Future PS1 integration should be an explicit configured external-ELF adapter here or beside it, not a native PS2 launch. | Must validate paths and loader presence. Never bundle BIOS, games, or proprietary components. |
| Configuration: `src/config.c`, `include/config.h`, load/save flow in `src/opl.c` | Key/value sets for OPL, last played, apps, network, and game defaults; searches boot/storage/MC locations; MC default is `mc?:OPL`. | Do not change namespace in milestone 1. Document migration to `mc?:OPL-EVO` plus storage-specific discovery rules after compatibility tests. | A partial namespace change could load from one location and save to another or overwrite stable USB/HDD settings. Memory-card writes must stay bounded and recoverable. |
| Language: `src/lang.c`, generated `src/lang_internal.c`, `include/lang_autogen.h`, `lang_compiler.py`, `lng_tmpl/`, downloaded `lng_src/` | Internal English plus external LNG files and generated string IDs. | Preserve. Milestone Evolution-only labels are fixed short English strings to avoid altering upstream language catalogs before product terminology stabilizes. | Generated sources and downloaded translations can drift; new permanent UI requires a coordinated language update later. |
| VMC: `src/vmc_groups.c`, `modules/vmc/genvmc`, `modules/mcemu`, backend VMC preparation | Virtual memory-card creation/grouping and BDM/HDD/SMB emulation integration. | Remain unchanged. | Data-loss and in-game compatibility risk; memory and IRX constraints are significant. |
| GSM: `src/gsm.c`, `include/pggsm.h`, GSM dialogs in `src/guigame.c` | Per-game/global video-mode configuration and GSM command preparation. | Remain unchanged; diagnostics may report current OPL renderer dimensions, not claim GSM state. | Incorrect modes can produce no display; existing Triangle+Cross recovery remains important. |
| Cheats: `src/cheatman.c`, cheat UI in `src/gui.c`, preparation in `src/supportbase.c` | Loads PS2RD-compatible cheat files, presents selection, and injects configured codes. | Remain unchanged. | Code buffers and game compatibility are sensitive. No piracy tooling or bundled codes. |
| PADEMU / Bluetooth / USB controllers: `modules/pademu`, `modules/ds34bt`, `modules/ds34usb`, EE RPC libraries, `src/guigame.c` | Existing DS3/DS4 USB/Bluetooth virtual-pad support, HCI/L2CAP/HID handling, per-game/global settings. | No rewrite. Milestone screen explicitly reports status as unavailable/not initialized unless the existing subsystem is actually active. Future manager layers transport, parsing, assignment, and diagnostics over these APIs. | IOP timing/memory and adapter variability are high-risk. Adapter detection alone is not proof of functional compatibility. |
| Native pads/input: `src/pad.c`, `include/pad.h` | Reads PS2 controller ports, edge/repeat state, analog-to-navigation translation, button delay. | Reuse completely. Recovery combination uses existing startup pad state. | Input regression can lock users out; retain regional select/back behavior. |
| In-game reset: `ee_core/src/igr.c` and related EE/IOP reset modules | Restarts/exits from game context where supported. | Remain unchanged. | Extremely compatibility-sensitive and memory-position-sensitive. |
| Logging/debug: `include/debug.h`, `src/debug.c`, debug modules, `LOG`/`PREINIT_LOG`, Makefile debug targets | UDPTTY, PPC UART, EE SIO, DECI2 and in-game debug variants. | Reuse for bounded Evolution startup state messages. Do not enable globally in release. | Debug IRXs and symbols increase size/memory and may change timing. Never log credentials. |
| Build/release: root `Makefile`, `.github/workflows/compilation.yml` | Builds all EE/IOP components, embeds assets/modules, strips/packs and packages release variants. | Add an opt-in `evolution` target producing `OPL-EVOLUTION.ELF`; retain `all`/`release` and `OPNPS2LD.ELF`. | Recursive variable propagation and stale artifacts are risks; clean builds are mandatory for comparison. |

## Current Evolution integration design

- `OPL_EVO=1` is a compile-time opt-in. A normal `make` remains the
  upstream-style interface and artifact.
- The ELF always starts in original OPL. Evolution is entered explicitly and
  Cancel from its navigation restores original OPL without restarting.
- Evolution renders an independent flat shell instead of drawing over the
  active OPL layout.
- Small `menusys` accessors operate on the existing linked list and invoke its
  existing launch, options, refresh, and regional Confirm callbacks. No game
  list or launch path is duplicated.
- Cover art is drawn through the active theme's existing bounded `COV` cache;
  the Evolution renderer adds no asset cache or preload pass.
- Existing OPL dialogs remain authoritative for general, display/theme, audio,
  controller, network, storage/startup, and per-game VMC configuration.
- Controllers and Diagnostics are additional screen handlers. Back behavior
  follows configured regional Cross/Circle selection.
- Configuration remains in the existing namespace, but no new Evolution
  setting is saved. A separate namespace remains deferred until all boot-device
  and HDD/SMB/BDM discovery paths can migrate atomically.

## Future boundaries

- Unified PS2/PS1/homebrew library: adapter layer above `item_list_t`; do not duplicate loaders.
- PS1: configured external ELF loader interface with presence/file validation and clear compatibility/error reporting.
- Controllers: transport → Bluetooth HCI/L2CAP or USB HID → controller parser → normalized input → virtual PS2 pad → player assignment/diagnostics.
- Diagnostics: read-only snapshots of real subsystems, with “unavailable” rather than invented success.
- Recovery: versioned configuration migrations, backup/rollback records, and a no-write safe mode designed around memory-card limits.
