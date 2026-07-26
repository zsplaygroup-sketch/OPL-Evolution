# OPL Evolution Community Launch Kit

## Release identity

- Project: OPL Evolution
- Release: v0.27.0-alpha
- Status: Unofficial experimental OPL fork
- License: Academic Free License 3.0
- Source and download: `[GITHUB RELEASE URL]`
- Issues and test reports: `[GITHUB ISSUES URL]`

Never describe OPL Evolution as an official OPL release or as a replacement
that is ready for every console. Keep the words **unofficial alpha** near the
top of every announcement.

## Primary announcement

### OPL Evolution v0.27.0-alpha — testers wanted for a unified PS2/PS1 loader

OPL Evolution is an unofficial experimental fork of Open PS2 Loader focused
on a modern, unified interface for PS2 games, PS1 games through an existing
POPStarter installation, homebrew, storage status, controller status, and
native settings.

The current alpha has been tested on real PS2 hardware. PS2 games launch
through Evolution's integrated OPL engine, and PS1 games launch through the
user's separately installed POPStarter environment.

Current features:

- Standalone Evolution interface and boot screen
- PS2 library scanning and launching
- PS1 library scanning and POPStarter launching
- PS2 and PS1 cover artwork
- USB/BDM storage startup
- Persistent controller, memory-card, storage, and network widgets
- Native display and audio settings
- Exit-to-Browser and power-off menu
- Per-game virtual memory-card assignments
- 8, 16, 32, and 64 MB VMC creation with progress reporting

Alpha limitations:

- This is not an official Open PS2 Loader release.
- Hardware coverage is still limited.
- Network configuration is currently read-only in Evolution.
- Homebrew library scanning is not finished.
- Some display adapters may not support 480p.
- Keep a known-good OPL or wLaunchELF recovery path.

Testers should report:

- PS2 console model and region
- Storage type: USB, HDD, MX4SIO, iLink, or SMB
- Video connection and selected video mode
- Game serial and format
- Whether scanning, artwork, launching, saving, and VMCs worked
- Exact reproduction steps for freezes or black screens

Source, alpha ELF, checksums, installation instructions, and issue tracker:

`[GITHUB RELEASE URL]`

OPL Evolution is derived from Open PS2 Loader and retains its AFL-3.0
licensing and attribution. The launch transition is adapted from
Irfanlesnar/PS2-Launcher under AFL-3.0.

## Short Reddit version

**OPL Evolution v0.27.0-alpha — unofficial OPL fork, hardware testers wanted**

I have been developing an experimental OPL fork with a unified PS2/PS1
library, a new dashboard, artwork, native PS2 launching, POPStarter
integration, status widgets, settings, and per-game VMC management.

This is an early alpha, not an official OPL release. Keep your existing OPL
and wLaunchELF recovery setup. I am looking for real-hardware reports across
different PS2 models and storage devices.

Source, ELF, screenshots, checksums, known issues, and reporting template:

`[GITHUB RELEASE URL]`

## Forum-title alternatives

- `[ALPHA] OPL Evolution — unified PS2/PS1 frontend and native OPL fork`
- `[TESTERS WANTED] OPL Evolution v0.27.0-alpha for real PS2 hardware`
- `OPL Evolution: experimental modern interface for OPL and POPStarter`

## Recommended first-wave communities

1. PSX-Place — Open PS2 Loader forum
2. Reddit — r/ps2homebrew
3. GBAtemp — Other Platforms & Oldies
4. PS2-HOME — Open PS2 Loader or Other Homebrew Projects

Post to one community at a time. Read each community's rules, answer early
technical questions, and incorporate corrections before posting the next
version. Do not mass-post identical text or repeatedly bump threads.

## Release attachments

- One clean dashboard screenshot
- One unified-library screenshot
- One VMC-management screenshot
- `OPL-EVOLUTION-0.27.0-NATIVE-VMC.ELF`
- `CHECKSUMS.txt`
- Installation and recovery instructions
- Known-issues list

Do not distribute game images, PlayStation BIOS files, POPStarter binaries,
or copyrighted support files.

## Hardware test-report template

```text
Console model:
Console region:
Boot method:
Storage type and filesystem:
Video cable/adapter:
Evolution video mode:
Game title and serial:
Game format:
PS2 scan:
Artwork:
Launch:
In-game saving:
VMC mode and size:
PS1/POPStarter:
Failure description:
Exact reproduction steps:
Photo or video link:
```
