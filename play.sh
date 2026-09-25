#!/bin/sh
# Decker 1.12 (Shawn Overcash, 2001), SDL2 port (port/): one window scaled
# nearest-neighbour to the screen (DECKER_SCALE=2 fixes the scale).
# Saves (*.DSG) in save/. Help: F1 or doc/index.html.
cd "$(dirname "$0")" || exit 1
[ -x ./decker ] || make -j8 >/dev/null || exit 1
exec ./decker "$PWD" "$@"
