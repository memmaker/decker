// SDL2 frontend for the MFC shim: one 640x480 screen, scaled nearest-neighbour.
// Same code for macOS and the web (Emscripten + Asyncify).
#include "afxwin.h"
#undef min
#undef max
#include <SDL.h>
#include <map>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

namespace shim {
void DispatchPosted();
void HandleMouse(UINT msg, int x, int y);
extern std::vector<CWnd *> g_modal;
}
BOOL port_key(CWnd *top, UINT vk, UINT mods); // rvip.cpp

static SDL_Window *g_win;
static SDL_Renderer *g_ren;
static SDL_Texture *g_tex;
static Bitmap g_screen(640, 480);
static SDL_AudioDeviceID g_audio;
static SDL_AudioSpec g_have;
int g_soundOn = 1; // page toggle on the web (off by default there)
namespace shim {
BOOL g_swallowInput = FALSE;
int g_inputCount = 0;
}

// ---------------------------------------------------------------- misc Win32
DWORD GetTickCount() { return SDL_GetTicks(); }
int GetSystemMetrics(int i)
{
	switch (i) {
	case SM_CXSCREEN: return 640;
	case SM_CYSCREEN: return 480;
	case SM_CXVSCROLL: case SM_CYHSCROLL: return 16;
	case SM_CXICON: case SM_CYICON: return 32;
	case SM_CXBORDER: case SM_CYBORDER: return 1;
	}
	return 0; // no frames or captions around the game's own windows
}
// modifiers of the key event being handled (SDL's live state may already
// be past a Ctrl release when several key events arrive at once)
static int g_evMods = -1;
short GetKeyState(int vk)
{
	SDL_Keymod m = g_evMods >= 0 ? (SDL_Keymod)g_evMods : SDL_GetModState();
	if (vk == VK_CONTROL) return (m & (KMOD_CTRL | KMOD_GUI)) ? (short)0x8000 : 0;
	if (vk == VK_SHIFT) return (m & KMOD_SHIFT) ? (short)0x8000 : 0;
	if (vk == VK_MENU) return (m & KMOD_ALT) ? (short)0x8000 : 0;
	return 0;
}
short GetAsyncKeyState(int vk) { return GetKeyState(vk); }

namespace shim {
Bitmap *LoadBmpFile(const char *path)
{
	static std::map<std::string, Bitmap *> cache;
	std::string p = ResolvePath(path);
	auto it = cache.find(p);
	if (it != cache.end()) return new Bitmap(*it->second);
	SDL_Surface *s = SDL_LoadBMP(p.c_str());
	if (!s) return nullptr;
	SDL_Surface *c = SDL_ConvertSurfaceFormat(s, SDL_PIXELFORMAT_XBGR8888, 0);
	SDL_FreeSurface(s);
	if (!c) return nullptr;
	Bitmap *b = new Bitmap(c->w, c->h);
	for (int y = 0; y < c->h; y++) {
		const uint32_t *src = (const uint32_t *)((const uint8_t *)c->pixels + y * c->pitch);
		for (int x = 0; x < c->w; x++) b->px[(size_t)y * c->w + x] = src[x] & 0xFFFFFF;
	}
	SDL_FreeSurface(c);
	cache[p] = new Bitmap(*b);
	return b;
}

// ---------------------------------------------------------------- tooltips
static CWnd *g_hover;
static DWORD g_hoverSince;
static BOOL g_tipHidden;
static CWnd *DeepestAt(CWnd *w, CPoint scr)
{
	for (auto it = w->m_children.rbegin(); it != w->m_children.rend(); ++it) {
		CRect r;
		(*it)->GetWindowRect(&r);
		if ((*it)->m_bVisible && r.PtInRect(scr)) return DeepestAt(*it, scr);
	}
	return w;
}
static void DrawTooltip()
{
	if (!g_hover || g_tipHidden || SDL_GetTicks() - g_hoverSince < 600 || !g_modal.empty()) return;
	CWnd *top = nullptr;
	for (CWnd *t : g_topLevel) if (t->m_bVisible && t->m_rect.PtInRect(CPoint(g_mouseX, g_mouseY))) top = t;
	if (!top || !top->m_bToolTips) return;
	CWnd *w = DeepestAt(top, CPoint(g_mouseX, g_mouseY));
	if (w != g_hover) return;
	CString s;
	if (!w->m_nID || !s.LoadString(w->m_nID)) return;
	CDC dc;
	dc.m_surf = &g_screen;
	dc.m_clip = CRect(0, 0, 640, 480);
	int tw = TextWidth(0, s, s.GetLength()) + 6, th = FontHeight(0) + 4;
	int x = std::min(g_mouseX, 640 - tw), y = g_mouseY + 20;
	if (y + th > 480) y = g_mouseY - th - 4;
	dc.FillSolidRect(x, y, tw, th, RGB(255, 255, 225));
	CBrush b(0);
	CRect r(x, y, x + tw, y + th);
	dc.FrameRect(&r, &b);
	DrawText(dc, 0, x + 3, y + 2, s, s.GetLength(), 0);
}

void Present()
{
	std::fill(g_screen.px.begin(), g_screen.px.end(), 0);
	for (CWnd *t : g_topLevel) {
		if (!t->m_bVisible || !t->m_backing) continue;
		Bitmap *b = t->m_backing;
		for (int y = 0; y < b->h; y++) {
			int sy = t->m_rect.top + y;
			if (sy < 0 || sy >= 480) continue;
			for (int x = 0; x < b->w; x++) {
				int sx = t->m_rect.left + x;
				if (sx >= 0 && sx < 640) g_screen.px[sy * 640 + sx] = b->px[(size_t)y * b->w + x];
			}
		}
	}
	DrawTooltip();
	SDL_UpdateTexture(g_tex, nullptr, g_screen.px.data(), 640 * 4);
	SDL_RenderClear(g_ren);
	SDL_RenderCopy(g_ren, g_tex, nullptr, nullptr);
	SDL_RenderPresent(g_ren);
}

static UINT MapKey(SDL_Keycode k)
{
	if (k >= SDLK_a && k <= SDLK_z) return 'A' + (k - SDLK_a);
	if (k >= SDLK_0 && k <= SDLK_9) return '0' + (k - SDLK_0);
	if (k >= SDLK_F1 && k <= SDLK_F12) return VK_F1 + (k - SDLK_F1);
	if (k >= SDLK_KP_1 && k <= SDLK_KP_9) return VK_NUMPAD0 + 1 + (k - SDLK_KP_1);
	switch (k) {
	case SDLK_KP_0: return VK_NUMPAD0;
	case SDLK_KP_PERIOD: return 0x6E;
	case SDLK_KP_PLUS: return 0x6B;
	case SDLK_KP_MINUS: return 0x6D;
	case SDLK_KP_MULTIPLY: return 0x6A;
	case SDLK_RETURN: case SDLK_KP_ENTER: return VK_RETURN;
	case SDLK_ESCAPE: return VK_ESCAPE;
	case SDLK_TAB: return VK_TAB;
	case SDLK_BACKSPACE: return VK_BACK;
	case SDLK_DELETE: return VK_DELETE;
	case SDLK_SPACE: return VK_SPACE;
	case SDLK_UP: return VK_UP;
	case SDLK_DOWN: return VK_DOWN;
	case SDLK_LEFT: return VK_LEFT;
	case SDLK_RIGHT: return VK_RIGHT;
	case SDLK_HOME: return VK_HOME;
	case SDLK_END: return VK_END;
	case SDLK_PAGEUP: return VK_PRIOR;
	case SDLK_PAGEDOWN: return VK_NEXT;
	case SDLK_LSHIFT: case SDLK_RSHIFT: return VK_SHIFT;
	case SDLK_LCTRL: case SDLK_RCTRL: return VK_CONTROL;
	}
	return 0;
}
static void MouseTo(int x, int y)
{
	g_mouseX = std::max(0, std::min(639, x));
	g_mouseY = std::max(0, std::min(479, y));
}
void Idle(int ms)
{
#ifdef __EMSCRIPTEN__
	emscripten_sleep(ms);
#else
	SDL_Delay(ms);
#endif
}
// test hook (desktop): DECKER_FIFO=<fifo> takes "key <vk>", "char <text>",
// "click x y", "dbl x y", "rclick x y", "shot <file.bmp>" lines
static void TestHook()
{
#ifndef __EMSCRIPTEN__
	static FILE *f;
	static std::string pending;
	if (!f) {
		const char *p = getenv("DECKER_FIFO");
		if (!p) return;
		int fd = open(p, O_RDONLY | O_NONBLOCK);
		if (fd < 0) return;
		f = fdopen(fd, "r");
	}
	char buf[512];
	ssize_t n = read(fileno(f), buf, sizeof buf);
	if (n > 0) pending.append(buf, n);
	size_t nl;
	if ((nl = pending.find('\n')) == std::string::npos) return;
	std::string l = pending.substr(0, nl);
	pending.erase(0, nl + 1);
	char cmd[32] = "", arg[400] = "";
	int x = 0, y = 0;
	sscanf(l.c_str(), "%31s %n", cmd, &x);
	fprintf(stderr, "test: %s modal=%d\n", l.c_str(), (int)g_modal.size());
	strncpy(arg, l.c_str() + std::min((size_t)x, l.size()), sizeof arg - 1);
	if (g_swallowInput && strcmp(cmd, "shot")) { g_inputCount++; return; }
	if (!strcmp(cmd, "key")) { UINT vk = strtoul(arg, nullptr, 0); if (!(port_key(CWnd::GetActiveWindow(), vk, 0) && g_modal.empty())) { HandleKey(vk, TRUE, 0); HandleKey(vk, FALSE, 0); } }
	else if (!strcmp(cmd, "char")) for (char *c = arg; *c; c++) HandleChar((unsigned char)*c);
	else if (!strcmp(cmd, "click") || !strcmp(cmd, "dbl") || !strcmp(cmd, "rclick")) {
		sscanf(arg, "%d %d", &x, &y);
		g_mouseX = x; g_mouseY = y;
		UINT d = cmd[0] == 'r' ? WM_RBUTTONDOWN : WM_LBUTTONDOWN, u = cmd[0] == 'r' ? WM_RBUTTONUP : WM_LBUTTONUP;
		for (int k = cmd[0] == 'd' ? 2 : 1; k > 0; k--) { HandleMouse(d, x, y); HandleMouse(u, x, y); }
	} else if (!strcmp(cmd, "shot")) {
		PaintDirty();
		Present();
		SDL_Surface *s = SDL_CreateRGBSurfaceWithFormatFrom(g_screen.px.data(), 640, 480, 32, 640 * 4, SDL_PIXELFORMAT_XBGR8888);
		SDL_SaveBMP(s, arg);
		SDL_FreeSurface(s);
	}
#endif
}
void PumpEvents(BOOL wait)
{
	SDL_Event e;
	BOOL got = FALSE;
	TestHook();
#ifndef __EMSCRIPTEN__
	if (wait) got = SDL_WaitEventTimeout(&e, 50);
	else
#endif
	{
		if (wait) Idle(16);
		got = SDL_PollEvent(&e);
	}
	while (got) {
		if (g_swallowInput && e.type != SDL_QUIT && e.type != SDL_WINDOWEVENT) {
			if (e.type == SDL_KEYDOWN || e.type == SDL_MOUSEBUTTONDOWN) g_inputCount++;
			got = SDL_PollEvent(&e);
			continue;
		}
		switch (e.type) {
		case SDL_QUIT: g_quit = TRUE; break;
		case SDL_KEYDOWN:
		case SDL_KEYUP: {
			g_tipHidden = TRUE;
			UINT vk = MapKey(e.key.keysym.sym);
			UINT mods = (e.key.keysym.mod & KMOD_SHIFT ? 1 : 0) | (e.key.keysym.mod & (KMOD_CTRL | KMOD_GUI) ? 2 : 0);
			if (!vk) break;
			if (e.type == SDL_KEYDOWN) {
				CWnd *top = CWnd::GetActiveWindow();
				if (top && g_modal.empty() && port_key(top, vk, mods)) break;
				if (!g_modal.empty() && port_key(g_modal.back(), vk, mods)) break;
			}
			g_evMods = e.key.keysym.mod;
			HandleKey(vk, e.type == SDL_KEYDOWN, mods);
			g_evMods = -1;
			// Ctrl+letter and Return/Escape/Backspace produce no SDL text event
			break;
		}
		case SDL_TEXTINPUT: {
			if (SDL_GetModState() & (KMOD_CTRL | KMOD_GUI)) break;
			const unsigned char *s = (const unsigned char *)e.text.text;
			while (*s) {
				UINT ch = *s++;
				if (ch >= 0xC0 && (*s & 0xC0) == 0x80) ch = ((ch & 0x1F) << 6) | (*s++ & 0x3F); // 2-byte UTF-8 -> Latin-1
				else if (ch >= 0x80) { while ((*s & 0xC0) == 0x80) s++; continue; }
				HandleChar(ch);
			}
			break;
		}
		case SDL_MOUSEMOTION: {
			MouseTo(e.motion.x, e.motion.y);
			CWnd *top = nullptr;
			for (CWnd *t : g_topLevel) if (t->m_bVisible && t->m_rect.PtInRect(CPoint(g_mouseX, g_mouseY))) top = t;
			CWnd *w = top ? DeepestAt(top, CPoint(g_mouseX, g_mouseY)) : nullptr;
			if (w != g_hover) { g_hover = w; g_hoverSince = SDL_GetTicks(); g_tipHidden = FALSE; }
			HandleMouse(WM_MOUSEMOVE, g_mouseX, g_mouseY);
			break;
		}
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP: {
			g_tipHidden = TRUE;
			MouseTo(e.button.x, e.button.y);
			BOOL down = e.type == SDL_MOUSEBUTTONDOWN;
			if (e.button.button == SDL_BUTTON_LEFT) HandleMouse(down ? WM_LBUTTONDOWN : WM_LBUTTONUP, g_mouseX, g_mouseY);
			else if (e.button.button == SDL_BUTTON_RIGHT) HandleMouse(down ? WM_RBUTTONDOWN : WM_RBUTTONUP, g_mouseX, g_mouseY);
			break;
		}
		case SDL_MOUSEWHEEL:
			if (e.wheel.y) HandleWheel(g_mouseX, g_mouseY, e.wheel.y > 0 ? 1 : -1);
			break;
		case SDL_WINDOWEVENT:
			if (e.window.event == SDL_WINDOWEVENT_EXPOSED) Present();
			break;
		}
		got = SDL_PollEvent(&e);
	}
}

void SyncFS()
{
#ifdef __EMSCRIPTEN__
	EM_ASM(if (typeof deckerSync === 'function') deckerSync(););
#endif
}
}

// Win32 Sleep: show what was drawn, wait, don't dispatch input (as on Windows)
void Sleep(DWORD ms)
{
	shim::Present();
	shim::Idle(ms);
}

// ---------------------------------------------------------------- sound
struct Sample { Uint8 *buf; Uint32 len; };
BOOL PlaySound(LPCSTR file, HINSTANCE, DWORD flags)
{
	if (!g_audio) return FALSE;
	if (!file || (flags & SND_PURGE)) { SDL_ClearQueuedAudio(g_audio); return TRUE; }
	if (!g_soundOn) return FALSE;
	static std::map<std::string, Sample> cache;
	std::string p = shim::ResolvePath(file);
	auto it = cache.find(p);
	if (it == cache.end()) {
		SDL_AudioSpec spec;
		Uint8 *buf;
		Uint32 len;
		Sample s = {nullptr, 0};
		if (SDL_LoadWAV(p.c_str(), &spec, &buf, &len)) {
			SDL_AudioCVT cvt;
			SDL_BuildAudioCVT(&cvt, spec.format, spec.channels, spec.freq, g_have.format, g_have.channels, g_have.freq);
			cvt.len = len;
			cvt.buf = (Uint8 *)SDL_malloc(len * (cvt.len_mult > 0 ? cvt.len_mult : 1));
			memcpy(cvt.buf, buf, len);
			SDL_FreeWAV(buf);
			if (cvt.needed) SDL_ConvertAudio(&cvt);
			s = {cvt.buf, (Uint32)cvt.len_cvt};
			if (!cvt.needed) s.len = len;
		}
		it = cache.emplace(p, s).first;
	}
	if (!it->second.buf) return FALSE;
	SDL_ClearQueuedAudio(g_audio); // SND_ASYNC replaces the playing sound
	SDL_QueueAudio(g_audio, it->second.buf, it->second.len);
	SDL_PauseAudioDevice(g_audio, 0);
	return TRUE;
}

// ---------------------------------------------------------------- help
void port_help(DWORD ctx)
{
#ifdef __EMSCRIPTEN__
	EM_ASM({ if (typeof deckerHelp === 'function') deckerHelp($0); }, ctx);
#else
	char cmd[1200];
	snprintf(cmd, sizeof cmd, "open 'file://%s/doc/index.html#h%u' &", getcwd(nullptr, 0), (unsigned)ctx);
	system(cmd);
#endif
}
#ifdef __EMSCRIPTEN__
extern "C" EMSCRIPTEN_KEEPALIVE void web_set_sound(int on)
{
	g_soundOn = on;
	if (!on && g_audio) SDL_ClearQueuedAudio(g_audio);
}
#endif

// ---------------------------------------------------------------- main
int main(int argc, char **argv)
{
#ifdef __EMSCRIPTEN__
	chdir("/decker");
	g_soundOn = 0;
#else
	if (argc > 1) chdir(argv[1]);
#endif
	mkdir("save", 0755);
	shim::g_saveDir = "save";
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
	SDL_SetHint(SDL_HINT_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK, "1");
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS);
	int scale = 1;
#ifndef __EMSCRIPTEN__
	SDL_Rect usable;
	if (SDL_GetDisplayUsableBounds(0, &usable) == 0) scale = std::max(1, std::min((usable.w - 20) * 4 / 640, (usable.h - 40) * 4 / 480)); // quarter steps
	int ww = 640 * scale / 4, wh = 480 * scale / 4;
	if (const char *s = getenv("DECKER_SCALE")) ww = 640 * atoi(s), wh = 480 * atoi(s);
#else
	int ww = 640, wh = 480; // fixed canvas; the page scales it with CSS (pixelated)
#endif
	g_win = SDL_CreateWindow("Decker", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, ww, wh,
#ifdef __EMSCRIPTEN__
		0);
#else
		SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | (getenv("DECKER_HIDDEN") ? SDL_WINDOW_HIDDEN : 0));
#endif
	g_ren = SDL_CreateRenderer(g_win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!g_ren) g_ren = SDL_CreateRenderer(g_win, -1, 0);
	SDL_RenderSetLogicalSize(g_ren, 640, 480);
	g_tex = SDL_CreateTexture(g_ren, SDL_PIXELFORMAT_XBGR8888, SDL_TEXTUREACCESS_STREAMING, 640, 480);
	SDL_AudioSpec want = {};
	want.freq = 22050;
	want.format = AUDIO_S16SYS;
	want.channels = 2;
	want.samples = 1024;
	g_audio = SDL_OpenAudioDevice(nullptr, 0, &want, &g_have, 0);
	SDL_StartTextInput();
	CWinApp *app = AfxGetApp();
	int rc = 0;
	if (app->InitInstance()) rc = app->Run();
	else app->ExitInstance();
#ifdef __EMSCRIPTEN__
	EM_ASM(if (typeof deckerEnd === 'function') deckerEnd(););
#endif
	SDL_Quit();
	return rc;
}
