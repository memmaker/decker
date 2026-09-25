# Decker on macOS (SDL2) - the MFC calls go to the shim in port/
CXX ?= clang++
SDL_CFLAGS := $(shell sdl2-config --cflags)
SDL_LIBS := $(shell sdl2-config --libs)
CXXFLAGS += -std=c++17 -O2 -g -Iport -I. $(SDL_CFLAGS) -fno-delete-null-pointer-checks \
	-Wno-deprecated-declarations -Wno-writable-strings -Wno-c++11-narrowing -Wno-format-security \
	-Wno-dangling-else -Wno-parentheses -Wno-switch -Wno-unused-value -Wno-tautological-pointer-compare \
	-Wno-undefined-bool-conversion -Wno-null-conversion -Wno-pointer-bool-conversion
GAME := $(filter-out StdAfx.cpp,$(wildcard *.cpp))
PORT := port/mfc_core.cpp port/mfc_wnd.cpp port/mfc_ctl.cpp port/fe_sdl.cpp port/res_gen.cpp port/rvip.cpp
OBJ := $(patsubst %.cpp,obj/%.o,$(GAME) $(PORT))

decker: $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ) $(SDL_LIBS) $(LDFLAGS)

obj/%.o: %.cpp port/afxwin.h port/font_gen.h port/res.h
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

port/res_gen.cpp: Decker.rc port/rc2res.py
	python3 port/rc2res.py

clean:
	rm -rf obj decker
.PHONY: clean
