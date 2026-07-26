# OPL Evolution Full Application Rebuild

## Product boundary

OPL Evolution is a new PS2 application fork, not a theme, overlay, or alternate
menu inside the original OPL application. Evolution owns:

- ELF `main()` and application lifetime
- boot and recovery policy
- foreground render and input loop
- navigation and screen state
- unified PS2, PS1, and homebrew library records
- selection, search, filtering, favorites, and history
- configuration namespace and persistence
- controller management
- storage and network diagnostics
- per-game options
- launch coordination and error reporting

The original OPL interface is not a parent screen, hidden background, or
required launch intermediary.

## Reused low-level modules

The following proven, open-source OPL/PS2DEV components remain reusable. This
preserves compatibility and avoids rewriting hardware code without evidence:

| Area | Reused implementation |
| --- | --- |
| USB/BDM | USB mass, BDM event/filesystem, fragment and LBA services |
| Alternate storage | internal HDD, SMB, MX4SIO, iLink support modules |
| Disc formats | ISO, ZSO, UL path and media helpers |
| Game loader | EE core, IOPRP, cdvdman variants, loader ELF handoff |
| Compatibility | mode flags, GSM hooks, game patches, cheats |
| Virtual memory cards | BDM/HDD/SMB VMC modules and validation |
| Controllers | libpad and existing PADEMU transport modules |
| PS1 | external POPStarter execution contract |
| Toolchain | PS2SDK, gsKit, FreeType, existing IRX build pipeline |

These are libraries and hardware services inside the Evolution ELF. Reusing
them does not make original OPL the foreground application.

## Application-layer code to replace

| Original subsystem | Evolution replacement |
| --- | --- |
| `main()` in `opl.c` | `evo_main.c` |
| `menusys` selection and callbacks | `evo_library` and `evo_launch` |
| OPL main/menu/game-menu screens | Evolution screen controller |
| OPL configuration dialogs | Evolution settings models and screens |
| mixed menu/device list ownership | Evolution device registry |
| original launch confirmation path | Evolution launch coordinator |
| OPL config namespace for alpha UI | `OPL-EVO` configuration namespace |

## Migration phases

### Phase 1: application ownership

- Add an Evolution-only `main()`.
- Expose a reusable runtime initializer for hardware and device services.
- Disable original command-line auto-launch in Evolution.
- Enter Evolution before the intro completes.

### Phase 2: service-owned library

- Replace original menu nodes with stable Evolution library records.
- Keep bounded metadata and artwork caches.
- Add source generation counters so USB removal cannot leave stale pointers.
- Make PS2 and PS1 scans deterministic and cancellable.

### Phase 3: launch coordinator

- Snapshot the selected record.
- Quiesce artwork and metadata work with bounded waits.
- Validate the source generation and game path.
- Build a per-game launch configuration in Evolution.
- Transfer ownership to the selected low-level loader.
- Keep stage/error telemetry independent of the renderer.

### Phase 4: standalone settings and diagnostics

- Implement Evolution display, audio, controller, storage, network, VMC, and
  per-game option screens.
- Persist only under the Evolution namespace.
- Import existing OPL settings explicitly rather than silently sharing them.

### Phase 5: remove original application layer

- Remove `menusys`, original screen handlers, and original configuration
  dialogs from the Evolution link when no remaining Evolution service depends
  on them.
- Keep the normal upstream-style OPL target available as a separate recovery
  artifact during development.

## Current checkpoint

The Evolution build now has a separate `evo_main.c` entrypoint and calls
`oplRuntimeInitialize(..., allowAutoLaunch = 0)` only to initialize reusable
runtime/device services. The normal OPL target retains its own `main()` and
behavior. This is the first structural separation; original application-layer
objects remain linked until their dependencies are replaced in later phases.

## Hardware policy

Every phase produces a separately named USB-test ELF. No build replaces the
stable memory-card OPL installation until PS2 and PS1 launches, restart,
storage removal, missing artwork, and recovery have passed on real hardware.
