#!/bin/sh
# Build Decker for the browser (Emscripten SDL2 + Asyncify) into web/dist.
# The same port/ shim and SDL frontend as the Mac build; web/decker.js runs
# the page (IndexedDB saves, sound toggle, help). Deploy with web/deploy.sh.
# -sGLOBAL_BASE=65536: static data above 64K, else IS_INTRESOURCE (port/afxwin.h)
# takes string literals for resource IDs and they come out empty.
set -e
cd "$(dirname "$0")/.."
OUT=web/dist
rm -rf "$OUT" web/stage && mkdir -p "$OUT" web/stage
cp -R DefaultGraphics Sound Decker.ini web/stage/
cp -R RES web/stage/res   # RES in git; lower-case for case-sensitive FS
rm -f web/stage/res/*.ico web/stage/res/*.rc2
SRCS=$(ls *.cpp port/*.cpp)
em++ -O2 -std=c++17 -Iport -I. -w -fno-delete-null-pointer-checks -sUSE_SDL=2 \
	$SRCS -o "$OUT/decker-core.js" \
	--preload-file web/stage@/decker \
	-sASYNCIFY -sASYNCIFY_STACK_SIZE=262144 -sSTACK_SIZE=2097152 \
	-sALLOW_MEMORY_GROWTH -sINITIAL_MEMORY=64MB -sGLOBAL_BASE=65536 \
	-sEXPORTED_FUNCTIONS=_main,_web_set_sound \
	-sEXPORTED_RUNTIME_METHODS=FS,IDBFS,addRunDependency,removeRunDependency \
	-sFORCE_FILESYSTEM -lidbfs.js -sENVIRONMENT=web
rm -rf web/stage
cp web/index.html web/decker.js "$OUT/"
cp -R doc "$OUT/doc"
python3 web/make-help.py > "$OUT/help.html"
ls -la "$OUT"
