# Decker 1.12: handover

Decker (Shawn Overcash, 2001–2004, GPL) is a Windows MFC hacking game in the
spirit of Shadowrun. RVIP case O (own GUI). Upstream: `DeckerSource_1_12.zip`
from sourceforge.net/projects/decker, commit c61cf6e (sha256 in its message).

## How it runs
- `port/`: MFC/Win32 shim + SDL2 frontend (`fe_sdl.cpp`). Game sources stay
  MFC; `port/res_gen.cpp` is generated from `Decker.rc` by `port/rc2res.py`,
  fonts (`port/font_gen.h`) by `port/mkfont.py` (X11 helvR10/helvB12).
- Mac: `make`, `./play.sh` (window scaled in quarter steps to the screen,
  `DECKER_SCALE=n` fixes it). Saves `*.DSG` in `save/`. Desktop shortcut:
  `~/Desktop/Games/Roguelikes/Decker.app` (icon = the default persona sprite).
- Web: `web/build.sh` → `web/dist`, `web/deploy.sh` →
  https://ruzzoli.de/roguelikes/decker/. Saves in IndexedDB at
  `/decker/save`; `CFile::Close` after writing calls `deckerSync`.
- Tiles/sound: the game's own bitmaps (`DefaultGraphics/`, `res/`) and WAVs
  (`Sound/`). Web sound off by default (top-bar toggle, remembered).
- Help: `doc/index.html` from `Help/Decker.rtf` (`port/rtf2html.py`), numeric
  anchors `#h<HID>` so F1 opens the topic of the current screen.

## RVIP features (`port/rvip.cpp`)
- Enter: command menu (home and Matrix), built from the screen's buttons;
  hidden/disabled ones are left out. Letters after the tab select.
- X: auto-explore in the Matrix (BFS over known nodes of the area to the
  nearest one not stood on; stops on ICE, any message but "Entering node",
  any key). Keypad 8/6/2/4 move, 5 waits.
- Not applicable: `<`/`>` stair-walking (no stairs) and inventory item menus
  (no item inventory; programs are handled in the game's own lists).

## Testing
- `DECKER_FIFO=<fifo> DECKER_HIDDEN=1 ./decker <dir>`: send `key <vk>`,
  `char <text>`, `click x y`, `dbl x y`, `rclick x y`, `shot f.bmp`.
- ASan run done (new game, all home dialogs, contract, Matrix, explore,
  menu, save/load, quit). Found and fixed: use-after-free when Quit closes
  the Matrix view from inside its own Options dialog (shim defers the
  destroy of a window that owns a running modal).

## RVIP progress
All stages done (1–8). Open: nothing known. Web quirks worth knowing are in
RVIP.md O-Decker.
