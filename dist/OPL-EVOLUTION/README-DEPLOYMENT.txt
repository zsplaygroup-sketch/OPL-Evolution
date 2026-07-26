OPL Evolution 0.27.0-alpha - Native VMC Management
==================================================

Evolution owns the executable, game library, launch coordinator, and settings
screen. The original OPL menu system is not linked into this ELF.

USB test installation
---------------------

Do not replace the working memory-card OPL recovery entry yet.

1. Copy OPL-EVOLUTION-0.27.0-NATIVE-VMC.ELF to a FAT32 USB drive.
2. Launch it from mass:/ with wLaunchELF.
3. Open Library and confirm the bottom Storage widget says USB Auto.
4. Wait for the PS2 list to populate, then select the same known-working PS2
   title used to verify version 0.21.
5. Press X once and confirm the game starts.

PS2 USB recovery
----------------

Standalone Evolution now starts USB/BDM in Auto mode before its functional
boot screen can report completion. A legacy OPL configuration containing
usb_mode=Off can no longer leave the Evolution PS2 library without a backend.
This changes the standalone runtime state only; it does not overwrite the
memory-card configuration during startup.

UI recovery
-----------

- The embedded 640x480 dashboard now shares the exact coordinate space used
  by the live panels. It is no longer compressed a second time in widescreen
  mode.
- Evolution uses dedicated 13-pixel and 9-pixel dashboard fonts instead of
  OPL's larger 17-pixel menu font.
- Long game names and right-side metadata have more room and should no longer
  be clipped by oversized glyphs.
- PS2 USB/BDM Auto startup from 0.25.1 remains enabled.

Readable-widget refinement
--------------------------

- Settings rows use the readable 13-pixel Evolution font and remain clipped
  inside their label/value columns.
- Widget and metadata text is increased from 9 to 11 pixels.
- Muted text uses higher contrast against the dark dashboard.
- PS2 covers use a cover-shaped frame and are centered after widescreen aspect
  correction.
- PS1 covers are also centered using their final rendered width.
- Video mode now includes an optional 480p setting for displays and adapters
  that support progressive scan. Auto remains the default.

Application boot screen
-----------------------

The supplied OPL Evolution artwork is embedded as the complete 4:3 startup
screen. Its logo, text, glow, separator lines, and tagline are not redrawn.

The loading bar is functional:

- 42%: core runtime and renderer ready
- 58%: standalone PS2 engine connected
- 82%: Evolution UI and PS1 scan ready
- 82-96%: deferred device/audio initialization
- 100%: GUI initialization completion received

Evolution opens automatically when the bar reaches 100%. The boot texture is
released before the dashboard is loaded.

PS1 artwork
-----------

OPL Manager's standard PS1 cover path is now supported:

massN:/ART/<VCD filename without .VCD>_COV.png

Example:

mass0:/POPS/Crash Bandicoot.VCD
mass0:/ART/Crash Bandicoot_COV.png

Evolution also checks POPS/ART, serial-based OPL artwork names, and older
POPS/<game>.png sidecar covers. The selected PS1 artwork is displayed without
stretching and is reused by the launch transition.

Launch transition
-----------------

The transition is adapted from Irfanlesnar/PS2-Launcher under AFL-3.0:
https://github.com/Irfanlesnar/PS2-Launcher

- PS2: the selected cover expands from the dashboard and fades to black.
- PS1: the same sequence uses a blue placeholder until PS1 cover caching is
  implemented.
- Both paths finish on a black loading screen with an animated lower-right
  loader.
- The existing Evolution-owned PS2 and PS1 launch backends are unchanged.

Native Settings controls
------------------------

- Up/Down: select a setting.
- X or Left/Right: change the selected value.
- Square: save all staged changes.
- Circle: discard unsaved changes and return.

Available settings
------------------

- Cover artwork
- Widescreen layout
- Notifications
- Interface sounds
- Background music
- Automatic sorting
- Automatic refresh
- Confirm button
- Video mode

Video-mode and audio-startup changes should be checked after restarting
Evolution. Saving may briefly display an Evolution loading overlay while the
configuration file is written.

Options and system menus
------------------------

- Square opens Options from the dashboard.
- Triangle opens the System Menu from the dashboard.
- Display and theme, Audio, and Network open the native OPL configuration
  dialogs inside Evolution.
- Controller status opens the Evolution controller page.
- The System Menu provides Resume, Options, Save settings, Exit to PS2
  Browser, and Power off console.
- Exit and Power off require confirmation. Circle safely cancels either one.
- Square remains the Save button while editing Evolution's Settings page.

Readable UI refinement
----------------------

- Main dashboard text is rendered at 15 pixels instead of 13.
- Compact widget and metadata text is rendered at 12 pixels instead of 11.
- PS2 cover artwork preserves its original aspect ratio and is centered inside
  the cover frame in both 4:3 and widescreen layouts.
- The game launcher and storage backends are unchanged.
- For the sharpest output on a compatible component cable or HDMI adapter,
  select 480p under Display and theme, save, and restart Evolution. Keep Auto
  if the display or adapter does not support 480p.

Standalone options safety
-------------------------

- Display settings now opens Evolution's own settings screen with Widescreen
  selected. Video mode is available on the same screen.
- Audio settings opens the same native screen at Interface sounds; Background
  music is directly below it.
- The legacy OPL display, audio, and network modal dialogs are no longer
  entered from Evolution because they can lock when the original menu does not
  own the foreground.
- Network settings opens Evolution's read-only network status page in this
  checkpoint.
- PS2 cover rendering again uses one widescreen correction only, filling the
  inner artbox without the double-narrowing introduced in 0.26.1.

Native virtual memory cards
---------------------------

Open Memory Cards from the sidebar after selecting a PS2 game.

- Slot 1 and Slot 2 can independently use a physical card, shared Generic
  VMC, or a Per-game VMC.
- VMC files can be created at 8, 16, 32, or 64 MB.
- Creation reports live progress and keeps the page locked until the storage
  operation completes.
- Create or verify each enabled VMC, then choose Save game assignments.
- Assignments are written to the selected game's normal OPL CFG file and are
  consumed directly by Evolution's existing PS2 launch backend.
- Evolution does not create a VMC when a slot is set to Physical card.

Important verification
----------------------

1. Launch one known-working PS2 title and verify the complete transition.
2. Confirm that title still reaches gameplay.
3. Launch one known-working PS1 title and verify POPStarter still begins.
4. Press Square and test Display/theme, Audio, and Network.
5. Press Triangle and confirm the System Menu appears.
6. Enter Settings and move through every row.
7. Change Cover artwork, Automatic sorting, or another safe toggle.
8. Press Square once and confirm "Evolution settings saved" appears.
9. Restart Evolution and verify the value remained saved.

Recovery
--------

This remains a USB alpha checkpoint. Keep Free McBoot, wLaunchELF, and the
known-good OPL memory-card entry unchanged.
