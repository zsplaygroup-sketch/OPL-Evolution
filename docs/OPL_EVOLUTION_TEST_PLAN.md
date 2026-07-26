# OPL Evolution Milestone 1 Test Plan

## Status vocabulary

- **PASS**: executed and met the stated acceptance criteria.
- **FAIL**: executed and did not meet the stated acceptance criteria.
- **NOT RUN — emulator required**: requires a configured PS2 emulator.
- **NOT RUN — real hardware required**: requires a PlayStation 2 and the named peripherals.

No emulator or physical-console result is claimed by this milestone.

## Automated host validation

| Test | Acceptance criteria | Result |
| --- | --- | --- |
| Official baseline clean release | `make --trace clean release` exits 0 and creates a non-empty packed ELF | **PASS** |
| Evolution clean build | `make --trace clean evolution` exits 0 and creates `OPL-EVOLUTION.ELF` | **PASS** |
| Normal target regression | After the source changes, `make --trace clean all` exits 0 and creates `OPNPS2LD.ELF` | **PASS** |
| Patch hygiene | `git diff --check` reports no whitespace errors | **PASS** |
| Backend isolation | No diff in BDM, HDD, SMB, support-base, system, EE core, or IOP module paths | **PASS** |
| Artifact integrity | Baseline, normal, and Evolution ELFs are non-empty and have recorded SHA-256 hashes | **PASS** |
| Warning regression | Evolution and normal builds contain the same three compiler-warning categories as baseline and no new compiler-warning category | **PASS** |
| Package verification | Packaged Evolution ELF hash equals the build artifact hash | **PASS** |

The CRLF/LF messages in the two modified-tree logs are Git working-copy notices,
not compiler warnings. The three compiler warnings also occur in the untouched
baseline: a discarded `const` qualifier in `system.c`, the existing `UIItem`
declaration warning, and an unused `config` variable in `coreconfig.h`.

## Emulator smoke tests

| ID | Test | Expected result | Status |
| --- | --- | --- | --- |
| E-01 | Boot the packaged ELF from a host path | Evolution branding and main shell appear without a crash | **NOT RUN — emulator required** |
| E-02 | Navigate every Evolution entry with a virtual DualShock 2 | Focus remains visible; Confirm/Cancel/Start/Select and L1/R1 behave consistently | **NOT RUN — emulator required** |
| E-03 | Start the ELF, then use Start → Enter OPL Evolution and Start → Original OPL Interface | Boot begins in original OPL; both explicit transitions work | **NOT RUN — emulator required** |
| E-04 | Exercise empty USB/HDD/SMB/MX4SIO lists | Empty/unavailable sources are presented honestly and navigation remains usable | **NOT RUN — emulator required** |
| E-05 | Launch one known-good title per emulator-supported backend | The existing OPL launch path is reached and the title boots | **NOT RUN — emulator required** |
| E-06 | Use missing/corrupt artwork | List and selected-item panel remain readable with no crash | **NOT RUN — emulator required** |
| E-07 | Enter Controllers and Diagnostics | Placeholder actions do not claim execution; diagnostic values reflect configuration/runtime data only | **NOT RUN — emulator required** |
| E-08 | Repeat navigation under constrained emulator memory/debug instrumentation | No obvious leak, corruption, or progressive slowdown | **NOT RUN — emulator required** |

## Real-hardware boot and display tests

Run each test first by launching directly from `mass:/`, then from a separate
memory-card entry. Keep a known-good original OPL ELF available throughout.

| ID | Test | Expected result | Status |
| --- | --- | --- | --- |
| H-01 | Boot via wLaunchELF from FAT32 USB (`mass:/OPL-EVOLUTION.ELF`) | Clean startup into the Evolution shell | **NOT RUN — real hardware required** |
| H-02 | Boot from `mc0:/OPL-EVO/OPL-EVOLUTION.ELF` through a separate FMCB entry | Clean startup; stable OPL entry remains untouched | **NOT RUN — real hardware required** |
| H-03 | Cold boot, then use both interface-switching Start-menu actions | Original OPL appears first and remains the restart recovery route | **NOT RUN — real hardware required** |
| H-04 | Test NTSC and PAL-capable consoles/displays where available | UI remains within safe bounds and readable | **NOT RUN — real hardware required** |
| H-05 | Test 4:3 CRT at 480i/576i | Navigation, text, and bottom status strip are legible and not clipped | **NOT RUN — real hardware required** |
| H-06 | Test widescreen output | Layout remains usable; no required control is off-screen | **NOT RUN — real hardware required** |

## Real-hardware navigation and storage tests

| ID | Test | Expected result | Status |
| --- | --- | --- | --- |
| H-10 | Navigate with an official DualShock 2 | No stuck focus or loss of directional navigation | **PASS — user reported functional movement on 0.2.0-alpha, 2026-07-24; Cancel behavior still needs explicit confirmation** |
| H-11 | USB list with zero, one, and many games | Correct source/title summary; smooth scrolling; existing list behavior retained | **NOT RUN — real hardware required** |
| H-12 | Internal HDD list with zero, one, and many games | Same acceptance criteria as H-11 | **NOT RUN — real hardware required** |
| H-13 | SMB list after successful and failed network setup | Honest status, recovery from failure, and successful listing when configured | **NOT RUN — real hardware required** |
| H-14 | MX4SIO list with supported media | Existing backend initializes and lists content without Evolution-specific regression | **NOT RUN — real hardware required** |
| H-15 | Remove USB/MX4SIO media while browsing | No false success state; UI recovers after refresh/reconnect | **NOT RUN — real hardware required** |
| H-16 | Refresh lists with Select | Existing refresh behavior occurs without navigation corruption | **NOT RUN — real hardware required** |

## Real-hardware launch and recovery tests

| ID | Test | Expected result | Status |
| --- | --- | --- | --- |
| H-20 | Launch known-good USB title | Same launch compatibility as the baseline commit | **PASS — user reported successful selection and gameplay through Evolution 0.2.0-alpha on 2026-07-24** |
| H-21 | Launch known-good HDD title | Same launch compatibility as the baseline commit | **NOT RUN — real hardware required** |
| H-22 | Launch known-good SMB title | Same launch compatibility as the baseline commit | **NOT RUN — real hardware required** |
| H-23 | Launch known-good MX4SIO title | Same launch compatibility as the baseline commit | **NOT RUN — real hardware required** |
| H-24 | Return/restart after a title exits or IGR is used | Expected upstream OPL restart behavior; Evolution shell can be re-entered | **PASS — user confirmed original-interface return and original OPL restart recovery on 0.3.1-alpha, 2026-07-24** |
| H-25 | Boot with missing art, malformed config, or unavailable network | No crash loop; original-UI boot fallback remains available | **NOT RUN — real hardware required** |
| H-26 | Remove Evolution ELF or its separate FMCB entry | Original stable OPL remains bootable | **NOT RUN — real hardware required** |

## Version 0.3 artwork and section actions

| ID | Test | Expected result | Status |
| --- | --- | --- | --- |
| U-01 | Pause on a game with valid COV artwork | Cover loads on demand without blocking navigation | **PASS — user confirmed corrected cover presentation on 0.3.1-alpha, 2026-07-24** |
| U-02 | Select a game with missing artwork | Flat Evolution `NO ART` panel appears; no legacy theme fallback, crash, or stale cover | **PASS — user confirmed games without artwork remain usable on 0.3.1-alpha, 2026-07-24** |
| U-03 | Inspect selected-game metadata | Title, source, startup ID, and list position match selection | **NOT RUN — real hardware required** |
| U-04 | Open Controller settings from Controllers | Existing OPL dialog opens and returns safely | **NOT RUN — real hardware required** |
| U-05 | Open per-game options from Memory Cards | Existing game options/VMC route opens for current selection | **NOT RUN — real hardware required** |
| U-06 | Open Storage, Network, general, display, and audio settings | Existing dialogs open and cancel safely | **NOT RUN — real hardware required** |
| U-07 | Launch the previously validated USB game | Game remains playable with artwork integration enabled | **PASS — user reported all tested games playable on 0.3.1-alpha, 2026-07-24** |
| U-08 | Inspect a valid cover with an old/custom OPL theme active | Only the raw cover appears in Evolution's frame; no legacy case overlay or theme default is drawn | **PASS — user confirmed the cover-art issue was fixed on 0.3.1-alpha, 2026-07-24** |

## Controller/Bluetooth scenarios

The milestone Controllers page is deliberately informational. Pair, Reconnect,
Test, and Assign are labeled future actions and must not report success.

| ID | Test | Expected result | Status |
| --- | --- | --- | --- |
| C-01 | No Bluetooth adapter | Page remains usable and reports no fabricated adapter state | **NOT RUN — real hardware required** |
| C-02 | Unsupported Bluetooth adapter | No success claim or hang; original pad continues to work | **NOT RUN — real hardware required** |
| C-03 | Supported adapter with one DS3/DS4 | Existing PADEMU behavior is not disrupted | **NOT RUN — real hardware required** |
| C-04 | Supported adapter with two controllers | Existing PADEMU behavior is not disrupted; no invented player assignment | **NOT RUN — real hardware required** |
| C-05 | Disconnect/reconnect during navigation | UI retains a safe state and accepts input after upstream recovery | **NOT RUN — real hardware required** |

## Version 0.5 diagnostics and two-slot controller readiness

| ID | Test | Expected result | Status |
| --- | --- | --- | --- |
| D-01 | Open Diagnostics on physical PS2 | ROM version, memory size, video, storage, network, artwork, and Player 1 fields display without a stall | **NOT RUN — real hardware required** |
| D-02 | Change network configuration/runtime state | Network page reports the matching ready, starting, disabled/not-started, or specific error state | **NOT RUN — real hardware required** |
| D-03 | Open Controllers with no Bluetooth adapter | Adapter is reported as unavailable; wired controller remains responsive | **NOT RUN — real hardware required** |
| D-04 | Open Controllers with supported Bluetooth adapter | Configured adapter state and real VID/PID/HCI data appear when exposed by the existing module | **NOT RUN — real hardware required** |
| D-05 | Connect two supported DS3/DS4 controllers | Players 1 and 2 report independent active transports | **NOT RUN — real hardware required** |
| D-06 | Press Square on Controllers | Existing DS34 RPC modules re-enable two slots without blocking UI input | **NOT RUN — real hardware required** |
| D-07 | Launch tested games with PADEMU off and on | Existing launch compatibility and controller behavior are preserved | **NOT RUN — real hardware required** |

## Version 0.6 PS1 external-loader handoff

The baseline POPSLoader installation and Spider-Man PS1 launch were confirmed
on physical hardware before integrating the handoff. Free McBoot resolves the
working target from
`mass:/POPSLoader/APPS/PS1_POPSLOADER/POPSLOADER.ELF`; the menu entry itself is
stored on the memory card, but its ELF remains on USB.

| ID | Test | Expected result | Status |
| --- | --- | --- | --- |
| P-01 | Open PS1 with the working USB connected | Evolution detects POPStarter and lists `.VCD` titles from the POPS folder | **PASS — physical PS2, 0.6.2-alpha, 2026-07-24** |
| P-02 | Select Spider-Man and press Confirm | Evolution hands the selected game directly to POPSTARTER.ELF without stopping at the POPSLoader menu | **PASS — physical PS2, 0.6.2-alpha, 2026-07-24** |
| P-03 | Complete the direct Spider-Man handoff | The same already validated PS1 title reaches its title screen | **PASS — physical PS2, 0.6.2-alpha, 2026-07-24; startup was noticeably slow** |
| P-04 | Open PS1 without the USB drive | Missing-loader status appears; no launch or file write occurs | **NOT RUN — 0.6.1 hardware build required** |
| P-05 | Reconnect USB and press Square | Rescan rebuilds the game list without restarting Evolution | **NOT RUN — 0.6.1 hardware build required** |
| P-06 | Press Triangle on the PS1 list | The installed POPSLoader frontend opens as a fallback | **NOT RUN — 0.6.2 hardware build required** |
| P-07 | Cancel from PS1 and return to original OPL | Evolution navigation and the original recovery interface remain intact | **NOT RUN — 0.6.2 hardware build required** |

## Exit criteria for a hardware release candidate

All applicable H and C tests must be executed on at least one retail PS2, with
USB plus every advertised storage/backend path. Any launch compatibility
difference from commit `3e3f34e9f94b058f7fd5b13727cb86e94fd3b35d` is release-blocking.

## Version 0.7 unified-library and dashboard tests

| ID | Test | Expected result | Status |
| --- | --- | --- | --- |
| L-01 | Enter Evolution with the working PS1 USB connected | PS1 titles are pre-scanned without opening a separate PS1 page or pressing Square | **NOT RUN - 0.7.0 hardware build required** |
| L-02 | Open Library and press L1/R1 | Platform changes among PS2, PS1, and HOMEBREW while focus remains in the Library | **NOT RUN - 0.7.0 hardware build required** |
| L-03 | Switch repeatedly among all three platforms | Each platform restores a usable list and selection without a hang or stale launch target | **NOT RUN - 0.7.0 hardware build required** |
| L-04 | Launch one known-good PS2 title from the unified Library | Existing OPL launch compatibility is unchanged | **NOT RUN - 0.7.0 hardware build required** |
| L-05 | Launch Spider-Man from the unified PS1 filter | Direct POPStarter handoff reaches the game without opening POPSLoader or wLaunchELF | **NOT RUN - 0.7.0 hardware build required** |
| L-06 | Open the HOMEBREW filter and launch one existing app | Existing Apps backend and launch action remain functional | **NOT RUN - 0.7.0 hardware build required** |
| L-07 | Inspect Home after entering Evolution | Controller, storage, PS1 count, and network summaries reflect existing runtime/config state | **NOT RUN - 0.7.0 hardware build required** |
| L-08 | Inspect Library and Home on the target display | Dark navy/cyan shell, focus, tabs, details, and bottom cards remain readable and unclipped | **NOT RUN - 0.7.0 hardware build required** |
| L-09 | Cancel from Evolution and restart the console | Original OPL recovery path remains intact | **NOT RUN - 0.7.0 hardware build required** |

## Version 0.9 embedded-dashboard tests

| ID | Test | Expected result | Status |
| --- | --- | --- | --- |
| V-01 | Enter Evolution using the 0.9 build | Embedded dashboard chrome fills the 640x480 canvas without a black texture or crash | **NOT RUN - 0.9.0 hardware build required** |
| V-02 | Inspect Home and Library | Live content aligns with the clean center/right panels; static labels are not duplicated | **NOT RUN - 0.9.0 hardware build required** |
| V-03 | Navigate all seven left-rail items | Thin focus outline tracks the matching static label and remains TV-safe | **NOT RUN - 0.9.0 hardware build required** |
| V-04 | Browse six PS2 rows with artwork | Thumbnails and selected cover load without corrupting the dashboard or exhausting VRAM | **NOT RUN - 0.9.0 hardware build required** |
| V-05 | Switch PS2, PS1, and Homebrew | Each platform remains functional inside the new chrome | **NOT RUN - 0.9.0 hardware build required** |
| V-06 | Inspect persistent bottom cards | Controller, VMC guidance, storage/PS1 count, and network values remain readable on every main section | **NOT RUN - 0.9.0 hardware build required** |

## Version 0.9.1 menu-transition regression tests

| ID | Test | Expected result | Status |
| --- | --- | --- | --- |
| B-01 | From original OPL, open and close the Start menu five times | Menu appears every time; no black screen or hang | **NOT RUN - 0.9.1 hardware build required** |
| B-02 | Enter Evolution, open its Start menu, then return | Dashboard reloads and remains controllable | **NOT RUN - 0.9.1 hardware build required** |
| B-03 | Open and close Settings, Controllers, Diagnostics, and game options | Every transition returns safely without a black screen | **NOT RUN - 0.9.1 hardware build required** |
| B-04 | Launch one known-good PS2 title | Dashboard texture is released and normal OPL launch behavior is preserved | **NOT RUN - 0.9.1 hardware build required** |
| B-05 | Launch one known-good PS1 title | Dashboard texture is released and direct POPStarter handoff still reaches the game | **NOT RUN - 0.9.1 hardware build required** |
| B-06 | Return to original OPL and restart the PS2 | Stable original-OPL recovery path remains intact | **NOT RUN - 0.9.1 hardware build required** |

## Version 0.9.2 display and status tests

| ID | Test | Expected result | Status |
| --- | --- | --- | --- |
| S-01 | In Evolution Settings, press Triangle | Display/Theme dialog opens and remains controllable | **NOT RUN - 0.9.2 hardware build required** |
| S-02 | Exit Display/Theme settings | Evolution redraws without freezing or black-screening | **NOT RUN - 0.9.2 hardware build required** |
| S-03 | Compare the dashboard at the same video mode | Static dashboard avoids additional bilinear softening | **NOT RUN - 0.9.2 hardware build required** |
| S-04 | Connect/disconnect Player 1 and Player 2 pads | Bottom card reports each real wired, USB, or Bluetooth state | **NOT RUN - 0.9.2 hardware build required** |
| S-05 | Insert/remove cards in Slot 1 and Slot 2 | Bottom card updates presence and free-space status within five seconds | **NOT RUN - 0.9.2 hardware build required** |

## Version 0.9.3 confirmation-route regression tests

| ID | Test | Expected result | Status |
| --- | --- | --- | --- |
| F-01 | Confirm Controllers and Diagnostics | Each page opens, accepts input, and returns without freezing | **NOT RUN - 0.9.3 hardware build required** |
| F-02 | Confirm Memory Cards, Network, and Settings | Existing OPL screen/dialog opens and returns without freezing | **NOT RUN - 0.9.3 hardware build required** |
| F-03 | Press Triangle and Square in Evolution Settings | Display/Theme and Audio dialogs remain controllable | **NOT RUN - 0.9.3 hardware build required** |
| F-04 | Open per-game options with Triangle | Game-options screen remains controllable and returns to Evolution | **NOT RUN - 0.9.3 hardware build required** |

## Version 0.9.4 persistent-widget tests

| ID | Test | Expected result | Status |
| --- | --- | --- | --- |
| W-01 | Move through all seven navigation sections | Controller, memory-card, storage, and network widgets remain visible | **NOT RUN - 0.9.4 hardware build required** |
| W-02 | Inspect the four widget headers | Each has a matching crisp native line icon | **NOT RUN - 0.9.4 hardware build required** |
| W-03 | Confirm Controllers and Diagnostics | Detailed content uses the same shell and retains all four widgets | **NOT RUN - 0.9.4 hardware build required** |
| W-04 | Connect/disconnect controllers and memory cards | Widget text continues to reflect real hardware state | **NOT RUN - 0.9.4 hardware build required** |

## Version 0.9.5 modal-VRAM tests

| ID | Test | Expected result | Status |
| --- | --- | --- | --- |
| M-01 | Confirm General OPL settings | Modal dialog renders and accepts input without a hard lock | **NOT RUN - 0.9.5 hardware build required** |
| M-02 | Open Display/Theme and Audio settings | Each modal dialog renders and returns to the dashboard | **NOT RUN - 0.9.5 hardware build required** |
| M-03 | Open Network and Controller settings | Dashboard VRAM is yielded and both dialogs remain controllable | **NOT RUN - 0.9.5 hardware build required** |
| M-04 | Open per-game options | Options screen opens without VRAM exhaustion or texture destruction | **NOT RUN - 0.9.5 hardware build required** |
| M-05 | Inspect all four bottom widgets | Smaller icons and compact labels fit without clipping | **NOT RUN - 0.9.5 hardware build required** |

## Version 0.9.6 confirm-input tests

| ID | Test | Expected result | Status |
| --- | --- | --- | --- |
| I-01 | Hold the left stick slightly upward and press X | X opens the selected section instead of scrolling upward | **NOT RUN - 0.9.6 hardware build required** |
| I-02 | Select a PS2 game and press X | Confirm reaches the existing OPL launch action | **NOT RUN - 0.9.6 hardware build required** |
| I-03 | Select General OPL settings and press X | Confirm opens the modal settings dialog | **NOT RUN - 0.9.6 hardware build required** |
| I-04 | Inspect controller and memory rows | Each row has its own compact icon | **NOT RUN - 0.9.6 hardware build required** |
| I-05 | Inspect Storage | Separate USB and HDD icons and statuses fit inside the card | **NOT RUN - 0.9.6 hardware build required** |

## Version 0.9.7 safe-handoff tests

| ID | Test | Expected result | Status |
| --- | --- | --- | --- |
| E-01 | Select a known-good PS2 game and press X | Evolution yields to original OPL context and the game launches | **NOT RUN - 0.9.7 hardware build required** |
| E-02 | Select a known-good PS1 game and press X | Evolution yields before direct POPStarter execution | **NOT RUN - 0.9.7 hardware build required** |
| E-03 | Open General, Display, or Audio settings | Modal dialog starts from original OPL rendering context and remains controllable | **NOT RUN - 0.9.7 hardware build required** |
| E-04 | Cancel a modal dialog | Evolution restores the same section without rescanning or losing selection | **NOT RUN - 0.9.7 hardware build required** |
