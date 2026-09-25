# Decker 1.12 for macOS and the web

Upstream: **Decker 1.12** by Shawn Overcash (GPL), source archive
`DeckerSource_1_12.zip` from https://sourceforge.net/projects/decker/,
committed untouched as [c61cf6e](https://github.com/memmaker/decker/tree/c61cf6e).
All our changes: https://github.com/memmaker/decker/compare/c61cf6e...main

Play in the browser: https://ruzzoli.de/roguelikes/decker/

Decker is a Windows MFC game. This repo keeps the game sources as they are
(a few casts aside) and adds:

- `port/`: a small MFC/Win32 shim (windows, message maps, dialogs built from
  `Decker.rc`, common controls, GDI, CArchive saves) on SDL2.
- RVIP additions (`port/rvip.cpp`): Enter opens a menu of every command,
  X explores the Matrix, keypad movement.
- `doc/`: the WinHelp file converted to HTML (`port/rtf2html.py`); F1 opens it.
- `web/`: Emscripten build (`web/build.sh`), page and deploy script.

Build and run on the Mac: `brew install sdl2 && make && ./play.sh`.
