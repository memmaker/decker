// MFC shim: strings, lists, files, archives, ini files, bitmaps, DCs, text.
#include "afxwin.h"
#undef min
#undef max
#include <algorithm>
using std::max;
using std::min;
#include "res.h"
#include "font_gen.h"
#include <dirent.h>
#include <sys/stat.h>
#include <map>

// ---------------------------------------------------------------- CString
void CString::FormatV(const char *fmt, va_list ap)
{
	va_list ap2;
	va_copy(ap2, ap);
	int n = vsnprintf(nullptr, 0, fmt, ap2);
	va_end(ap2);
	std::vector<char> b(n + 1);
	vsnprintf(b.data(), n + 1, fmt, ap);
	s.assign(b.data(), n);
}
void CString::Format(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	FormatV(fmt, ap);
	va_end(ap);
}
int CString::Replace(const char *from, const char *to)
{
	int n = 0;
	size_t fl = strlen(from), tl = strlen(to), p = 0;
	if (!fl) return 0;
	while ((p = s.find(from, p)) != std::string::npos) { s.replace(p, fl, to); p += tl; n++; }
	return n;
}
int CString::Remove(char c)
{
	int n = 0;
	std::string r;
	for (char ch : s) { if (ch == c) n++; else r += ch; }
	s = r;
	return n;
}
char *CString::GetBuffer(int n)
{
	buf.assign(s.begin(), s.end());
	buf.resize(max(n, (int)s.size()) + 1, 0);
	return buf.data();
}
void CString::ReleaseBuffer(int n)
{
	if (buf.empty()) return;
	s = n < 0 ? std::string(buf.data()) : std::string(buf.data(), n);
	buf.clear();
}
BOOL CString::LoadString(UINT id)
{
	for (const StringRes *r = g_stringRes; r->text; r++)
		if ((UINT)r->id == id) { s = r->text; return TRUE; }
	return FALSE;
}
int LoadString(HINSTANCE, UINT id, LPTSTR buf, int n)
{
	CString s;
	s.LoadString(id);
	strncpy(buf, s, n);
	if (n) buf[n - 1] = 0;
	return (int)strlen(buf);
}

// ---------------------------------------------------------------- CObList
POSITION CObList::InsertBefore(POSITION p, CObject *o)
{
	Node *at = N(p), *n = new Node{nullptr, at, o};
	if (!at) { n->prev = tail; n->next = nullptr; if (tail) tail->next = n; else head = n; tail = n; }
	else { n->prev = at->prev; if (at->prev) at->prev->next = n; else head = n; at->prev = n; }
	count++;
	return (POSITION)n;
}
POSITION CObList::InsertAfter(POSITION p, CObject *o)
{
	Node *at = N(p), *n = new Node{at, nullptr, o};
	if (!at) { n->next = head; n->prev = nullptr; if (head) head->prev = n; else tail = n; head = n; }
	else { n->next = at->next; if (at->next) at->next->prev = n; else tail = n; at->next = n; }
	count++;
	return (POSITION)n;
}
void CObList::RemoveAt(POSITION p)
{
	Node *n = N(p);
	if (n->prev) n->prev->next = n->next; else head = n->next;
	if (n->next) n->next->prev = n->prev; else tail = n->prev;
	delete n;
	count--;
}
POSITION CObList::Find(CObject *o, POSITION after) const
{
	for (Node *n = after ? N(after)->next : head; n; n = n->next)
		if (n->data == o) return (POSITION)n;
	return nullptr;
}
POSITION CObList::FindIndex(int i) const
{
	if (i < 0) return nullptr;
	Node *n = head;
	while (n && i--) n = n->next;
	return (POSITION)n;
}

// ---------------------------------------------------------------- paths
namespace shim {
std::string g_saveDir = ".";

// "a\b\C.BMP" -> "a/b/c.bmp" as it exists on disk (MEMFS is case-sensitive)
std::string ResolvePath(const char *path)
{
	std::string p(path);
	for (auto &c : p) if (c == '\\') c = '/';
	while (p.find("//") != std::string::npos) p.replace(p.find("//"), 2, "/");
	struct stat st;
	if (stat(p.c_str(), &st) == 0) return p;
	std::string out = p[0] == '/' ? "/" : "";
	size_t i = p[0] == '/' ? 1 : 0;
	while (i <= p.size()) {
		size_t j = p.find('/', i);
		if (j == std::string::npos) j = p.size();
		std::string part = p.substr(i, j - i);
		std::string dir = out.empty() ? "." : out;
		std::string found = part;
		if (DIR *d = opendir(dir.c_str())) {
			while (dirent *e = readdir(d))
				if (strcasecmp(e->d_name, part.c_str()) == 0) { found = e->d_name; break; }
			closedir(d);
		}
		out += found;
		if (j < p.size()) out += "/";
		i = j + 1;
	}
	return out;
}
}

// ---------------------------------------------------------------- files
BOOL CFile::Open(LPCTSTR name, UINT flags, CFileException *)
{
	Close();
	std::string p = shim::ResolvePath(name);
	m_bWrite = (flags & (modeWrite | modeReadWrite)) != 0;
	const char *mode = (flags & modeCreate) ? "wb" : (flags & modeReadWrite) ? "r+b" : m_bWrite ? "wb" : "rb";
	m_fp = fopen(p.c_str(), mode);
	m_strFileName = p.c_str();
	return m_fp != nullptr;
}
UINT CFile::Read(void *buf, UINT n) { return m_fp ? (UINT)fread(buf, 1, n, m_fp) : 0; }
void CFile::Write(const void *buf, UINT n) { if (m_fp) fwrite(buf, 1, n, m_fp); }
long CFile::Seek(long off, UINT from)
{
	if (!m_fp) return -1;
	fseek(m_fp, off, from == begin ? SEEK_SET : from == current ? SEEK_CUR : SEEK_END);
	return ftell(m_fp);
}
void CFile::Close()
{
	if (!m_fp) return;
	fclose(m_fp);
	m_fp = nullptr;
	if (m_bWrite) shim::SyncFS();
}
UINT CArchive::Read(void *p, UINT n) { return m_pFile->Read(p, n); }
void CArchive::Write(const void *p, UINT n) { m_pFile->Write(p, n); }
// MFC string format: BYTE len, or 0xFF + WORD len, or 0xFF 0xFFFF + DWORD len
CArchive &CArchive::operator<<(const CString &s)
{
	DWORD n = s.GetLength();
	if (n < 255) *this << (BYTE)n;
	else if (n < 0xFFFE) *this << (BYTE)0xFF << (WORD)n;
	else *this << (BYTE)0xFF << (WORD)0xFFFF << (UINT)n;
	Write((const char *)s, n);
	return *this;
}
CArchive &CArchive::operator>>(CString &s)
{
	BYTE b;
	*this >> b;
	DWORD n = b;
	if (b == 0xFF) {
		WORD w;
		*this >> w;
		n = w;
		if (w == 0xFFFE) throw new CArchiveException(CArchiveException::badSchema); // unicode marker
		if (w == 0xFFFF) { UINT d; *this >> d; n = d; }
	}
	std::string t(n, '\0');
	if (n && Read(&t[0], n) != n) throw new CArchiveException(CArchiveException::endOfFile);
	s = t;
	return *this;
}

// ---------------------------------------------------------------- ini + registry
static std::map<std::string, std::map<std::string, std::map<std::string, std::string>>> g_ini;
static std::map<std::string, std::map<std::string, std::string>> &IniFile(LPCTSTR file)
{
	std::string p = shim::ResolvePath(file);
	auto it = g_ini.find(p);
	if (it != g_ini.end()) return it->second;
	auto &m = g_ini[p];
	if (FILE *f = fopen(p.c_str(), "r")) {
		char line[1024];
		std::string sec;
		while (fgets(line, sizeof line, f)) {
			std::string l(line);
			while (!l.empty() && strchr("\r\n \t", l.back())) l.pop_back();
			size_t a = l.find_first_not_of(" \t");
			if (a == std::string::npos || l[a] == ';') continue;
			l = l.substr(a);
			if (l[0] == '[') { sec = l.substr(1, l.find(']') - 1); for (auto &c : sec) c = tolower(c); continue; }
			size_t e = l.find('=');
			if (e == std::string::npos) continue;
			std::string k = l.substr(0, e), v = l.substr(e + 1);
			while (!k.empty() && isspace((unsigned char)k.back())) k.pop_back();
			size_t vs = v.find_first_not_of(" \t");
			v = vs == std::string::npos ? "" : v.substr(vs);
			for (auto &c : k) c = tolower(c);
			m[sec][k] = v;
		}
		fclose(f);
	}
	return m;
}
DWORD GetPrivateProfileString(LPCTSTR sec, LPCTSTR key, LPCTSTR def, LPTSTR out, DWORD n, LPCTSTR file)
{
	std::string s(sec), k(key);
	for (auto &c : s) c = tolower(c);
	for (auto &c : k) c = tolower(c);
	auto &m = IniFile(file);
	std::string v = def ? def : "";
	if (m.count(s) && m[s].count(k)) v = m[s][k];
	strncpy(out, v.c_str(), n);
	if (n) out[n - 1] = 0;
	return (DWORD)strlen(out);
}
UINT GetPrivateProfileInt(LPCTSTR sec, LPCTSTR key, int def, LPCTSTR file)
{
	char b[64];
	GetPrivateProfileString(sec, key, "", b, sizeof b, file);
	return b[0] ? atoi(b) : def;
}
BOOL WritePrivateProfileString(LPCTSTR sec, LPCTSTR key, LPCTSTR val, LPCTSTR file)
{
	std::string s(sec), k(key);
	for (auto &c : s) c = tolower(c);
	for (auto &c : k) c = tolower(c);
	IniFile(file)[s][k] = val ? val : "";
	return TRUE; // ponytail: kept in memory only; Decker never writes its ini
}
LONG RegOpenKeyEx(HKEY, LPCTSTR, DWORD, DWORD, HKEY *) { return 2; }
LONG RegQueryValueEx(HKEY, LPCTSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD) { return 2; }
LONG RegCloseKey(HKEY) { return 0; }

// ---------------------------------------------------------------- bitmaps
namespace shim {
Bitmap *LoadBmpFile(const char *path);
Bitmap *LoadBmpRes(UINT id)
{
	for (const BitmapRes *r = g_bitmapRes; r->file; r++)
		if ((UINT)r->id == id) return LoadBmpFile(r->file);
	return nullptr;
}
}
HANDLE LoadImage(HINSTANCE, LPCTSTR name, UINT type, int, int, UINT flags)
{
	if (type != IMAGE_BITMAP) return nullptr;
	if (IS_INTRESOURCE(name)) return shim::LoadBmpRes((UINT)(uintptr_t)name);
	return shim::LoadBmpFile(name);
}
HBITMAP LoadBitmap(HINSTANCE, LPCTSTR name) { return (HBITMAP)LoadImage(nullptr, name, IMAGE_BITMAP, 0, 0, 0); }
HANDLE CopyImage(HANDLE h, UINT, int, int, UINT)
{
	return h ? new Bitmap(*(Bitmap *)h) : nullptr;
}
// ponytail: GDI objects are never freed (a few hundred KB of bitmaps loaded once)
BOOL DeleteObject(HGDIOBJ) { return TRUE; }
int GetObject(HGDIOBJ h, int, LPVOID out)
{
	Bitmap *b = (Bitmap *)h;
	BITMAP *bm = (BITMAP *)out;
	memset(bm, 0, sizeof *bm);
	if (!b) return 0;
	bm->bmWidth = b->w;
	bm->bmHeight = b->h;
	bm->bmBitsPixel = 24;
	bm->bmPlanes = 1;
	return sizeof *bm;
}
int GetDIBits(HDC, HBITMAP h, UINT, UINT, LPVOID, BITMAPINFO *bi, UINT)
{
	BITMAPV4HEADER *hd = (BITMAPV4HEADER *)bi;
	hd->bV4Width = h ? h->w : 0;
	hd->bV4Height = h ? h->h : 0;
	return 1;
}
BOOL GetBitmapDimensionEx(HBITMAP h, LPSIZE s) { s->cx = h->w; s->cy = h->h; return TRUE; }
HPALETTE CreatePalette(const LOGPALETTE *) { return nullptr; }
HCURSOR LoadCursor(HINSTANCE, LPCTSTR) { return nullptr; }
HGDIOBJ SelectObject(HDC dc, HGDIOBJ h) { return dc->SelectObject((HBITMAP)h); }

BOOL CBitmap::LoadBitmap(UINT id) { m_hObject = shim::LoadBmpRes(id); return m_hObject != nullptr; }
BOOL CBitmap::LoadBitmap(LPCTSTR name) { m_hObject = LoadImage(nullptr, name, IMAGE_BITMAP, 0, 0, 0); return m_hObject != nullptr; }
int CBitmap::GetBitmap(BITMAP *bm) { return GetObject(m_hObject, sizeof *bm, bm); }
CBitmap *CBitmap::FromHandle(HBITMAP h)
{
	static std::map<HBITMAP, CBitmap *> tmp;
	CBitmap *&b = tmp[h];
	if (!b) { b = new CBitmap; b->m_hObject = h; }
	return b;
}
// scroll-bar style arrows (16x16) for CBitmapButton::LoadOEMBitmap
BOOL CBitmap::LoadOEMBitmap(UINT id)
{
	Bitmap *b = new Bitmap(16, 16, COLOR_FACE);
	CDC dc;
	dc.SelectObject(b);
	BOOL down = id == OBM_UPARROWD || id == OBM_DNARROWD;
	BOOL grey = id == OBM_UPARROWI || id == OBM_DNARROWI;
	shim::DrawFrameCtl(dc, CRect(0, 0, 16, 16), down);
	int dir = (id == OBM_UPARROW || id == OBM_UPARROWD || id == OBM_UPARROWI) ? 0 : (id == OBM_RGARROW ? 1 : id == OBM_LFARROW ? 3 : 2);
	CRect r(0, 0, 16, 16);
	if (down) r.OffsetRect(1, 1);
	shim::DrawArrow(dc, r, dir, grey ? COLOR_SHADOW : 0);
	m_hObject = b;
	return TRUE;
}

// ---------------------------------------------------------------- CDC
HBITMAP CDC::SelectObject(HBITMAP h)
{
	HBITMAP o = m_selBitmap;
	m_selBitmap = h;
	m_surf = h;
	m_ox = m_oy = 0;
	m_clip = h ? CRect(0, 0, h->w, h->h) : CRect();
	return o;
}
CBitmap *CDC::SelectObject(CBitmap *b)
{
	HBITMAP o = SelectObject(b ? (HBITMAP)*b : nullptr);
	return o ? CBitmap::FromHandle(o) : CBitmap::FromHandle(nullptr);
}
HGDIOBJ CDC::SelectStockObject(int i)
{
	if (i == WHITE_PEN) m_crPen = 0xFFFFFF;
	else if (i == BLACK_PEN) m_crPen = 0;
	else if (i == SYSTEM_FONT) m_font = 1;
	else if (i == DEFAULT_GUI_FONT || i == ANSI_VAR_FONT) m_font = 0;
	return nullptr;
}
void CDC::FillSolidRect(int x, int y, int cx, int cy, COLORREF c)
{
	if (!m_surf) return;
	CRect r(x + m_ox, y + m_oy, x + m_ox + cx, y + m_oy + cy);
	r.left = max(r.left, m_clip.left); r.top = max(r.top, m_clip.top);
	r.right = min(r.right, m_clip.right); r.bottom = min(r.bottom, m_clip.bottom);
	c &= 0xFFFFFF;
	for (int yy = r.top; yy < r.bottom; yy++) {
		DWORD *p = m_surf->row(yy);
		for (int xx = r.left; xx < r.right; xx++) p[xx] = c;
	}
	m_crBk = c; // FillSolidRect sets the background colour, as in MFC
}
void CDC::hline(int x0, int x1, int y, COLORREF c) { FillSolidRect(x0, y, x1 - x0 + 1, 1, c); }
void CDC::vline(int x, int y0, int y1, COLORREF c) { FillSolidRect(x, y0, 1, y1 - y0 + 1, c); }
void CDC::FrameRect(LPCRECT r, CBrush *b)
{
	COLORREF c = b ? b->m_cr : 0, bk = m_crBk;
	hline(r->left, r->right - 1, r->top, c);
	hline(r->left, r->right - 1, r->bottom - 1, c);
	vline(r->left, r->top, r->bottom - 1, c);
	vline(r->right - 1, r->top, r->bottom - 1, c);
	m_crBk = bk;
}
void CDC::Draw3dRect(int x, int y, int cx, int cy, COLORREF tl, COLORREF br)
{
	COLORREF bk = m_crBk;
	hline(x, x + cx - 2, y, tl);
	vline(x, y, y + cy - 2, tl);
	hline(x, x + cx - 1, y + cy - 1, br);
	vline(x + cx - 1, y, y + cy - 1, br);
	m_crBk = bk;
}
BOOL CDC::Rectangle(int l, int t, int r, int b)
{
	COLORREF bk = m_crBk;
	FillSolidRect(l, t, r - l, b - t, 0xFFFFFF);
	CBrush br(m_crPen);
	CRect rc(l, t, r, b);
	FrameRect(&rc, &br);
	m_crBk = bk;
	return TRUE;
}
COLORREF CDC::SetPixel(int x, int y, COLORREF c)
{
	x += m_ox; y += m_oy;
	if (m_surf && m_clip.PtInRect(CPoint(x, y))) m_surf->row(y)[x] = c & 0xFFFFFF;
	return c;
}
COLORREF CDC::GetPixel(int x, int y) const
{
	x += m_ox; y += m_oy;
	if (!m_surf || x < 0 || y < 0 || x >= m_surf->w || y >= m_surf->h) return CLR_NONE;
	return m_surf->px[(size_t)y * m_surf->w + x];
}
BOOL CDC::LineTo(int x, int y)
{
	// only axis-aligned and simple lines are needed (map cross-hair)
	int x0 = m_pos.x, y0 = m_pos.y;
	int dx = abs(x - x0), dy = -abs(y - y0), sx = x0 < x ? 1 : -1, sy = y0 < y ? 1 : -1, err = dx + dy;
	for (;;) {
		if (x0 == x && y0 == y) break; // end point is not drawn, as in GDI
		SetPixel(x0, y0, m_crPen);
		int e2 = 2 * err;
		if (e2 >= dy) { err += dy; x0 += sx; }
		if (e2 <= dx) { err += dx; y0 += sy; }
	}
	m_pos = CPoint(x, y);
	return TRUE;
}
void CDC::blit(Bitmap *src, int sx, int sy, int x, int y, int cx, int cy, DWORD rop, bool keyed, DWORD key)
{
	if (!m_surf || !src) return;
	x += m_ox; y += m_oy;
	for (int j = 0; j < cy; j++) {
		int ty = y + j, fy = sy + j;
		if (ty < m_clip.top || ty >= m_clip.bottom || fy < 0 || fy >= src->h) continue;
		DWORD *d = m_surf->row(ty), *s = src->row(fy);
		for (int i = 0; i < cx; i++) {
			int tx = x + i, fx = sx + i;
			if (tx < m_clip.left || tx >= m_clip.right || fx < 0 || fx >= src->w) continue;
			DWORD c = s[fx];
			if (keyed && c == key) continue;
			switch (rop) {
			case SRCAND: d[tx] &= c; break;
			case SRCPAINT: d[tx] |= c; break;
			case SRCINVERT: d[tx] ^= c; break;
			case NOTSRCCOPY: d[tx] = ~c & 0xFFFFFF; break;
			case BLACKNESS: d[tx] = 0; break;
			case WHITENESS: d[tx] = 0xFFFFFF; break;
			default: d[tx] = c;
			}
		}
	}
}
BOOL CDC::BitBlt(int x, int y, int cx, int cy, CDC *src, int sx, int sy, DWORD rop)
{
	if (rop == BLACKNESS || rop == WHITENESS) { FillSolidRect(x, y, cx, cy, rop == BLACKNESS ? 0 : 0xFFFFFF); return TRUE; }
	if (!src || !src->m_surf) return FALSE;
	if (src->m_surf == m_surf) {
		Bitmap tmp(*m_surf); // overlapping copy
		blit(&tmp, sx + src->m_ox, sy + src->m_oy, x, y, cx, cy, rop);
	} else
		blit(src->m_surf, sx + src->m_ox, sy + src->m_oy, x, y, cx, cy, rop);
	return TRUE;
}
BOOL CDC::StretchBlt(int x, int y, int cx, int cy, CDC *src, int sx, int sy, int scx, int scy, DWORD rop)
{
	if (!src || !src->m_surf || !cx || !cy) return FALSE;
	Bitmap tmp(cx, cy);
	for (int j = 0; j < cy; j++)
		for (int i = 0; i < cx; i++) {
			int fx = sx + src->m_ox + i * scx / cx, fy = sy + src->m_oy + j * scy / cy;
			if (fx >= 0 && fy >= 0 && fx < src->m_surf->w && fy < src->m_surf->h) tmp.px[j * cx + i] = src->m_surf->px[(size_t)fy * src->m_surf->w + fx];
		}
	blit(&tmp, 0, 0, x, y, cx, cy, rop);
	return TRUE;
}
BOOL CDC::DrawState(CPoint pt, CSize sz, HBITMAP h, UINT flags, CBrush *)
{
	if (!h) return FALSE;
	int cx = sz.cx ? min((int)sz.cx, h->w) : h->w, cy = sz.cy ? min((int)sz.cy, h->h) : h->h;
	blit(h, 0, 0, pt.x, pt.y, cx, cy, SRCCOPY);
	return TRUE;
}
int CDC::GetClipBox(LPRECT r) const
{
	r->left = m_clip.left - m_ox; r->top = m_clip.top - m_oy;
	r->right = m_clip.right - m_ox; r->bottom = m_clip.bottom - m_oy;
	return 1;
}

// ---------------------------------------------------------------- text
namespace shim {
int FontHeight(int font) { return font ? font_bold_height : font_normal_height; }
static int GlyphW(int font, unsigned char c) { return c < 32 ? 0 : (font ? font_bold_width : font_normal_width)[c - 32]; }
int TextWidth(int font, const char *s, int n)
{
	int w = 0;
	for (int i = 0; i < n; i++) w += GlyphW(font, (unsigned char)s[i]);
	return w;
}
void DrawText(CDC &dc, int font, int x, int y, const char *s, int n, COLORREF c)
{
	int h = FontHeight(font);
	c &= 0xFFFFFF;
	for (int i = 0; i < n; i++) {
		unsigned char ch = s[i];
		if (ch < 32) continue;
		const unsigned short *rows = font ? font_bold_rows[ch - 32] : font_normal_rows[ch - 32];
		for (int yy = 0; yy < h; yy++)
			for (int xx = 0; rows[yy] >> xx; xx++)
				if (rows[yy] >> xx & 1) dc.SetPixel(x + xx, y + yy, c);
		x += GlyphW(font, ch);
	}
}
}
BOOL CDC::TextOut(int x, int y, const char *s, int n)
{
	if (m_nBkMode == OPAQUE) {
		COLORREF bk = m_crBk;
		FillSolidRect(x, y, shim::TextWidth(m_font, s, n), shim::FontHeight(m_font), bk);
	}
	shim::DrawText(*this, m_font, x, y, s, n, m_crText);
	return TRUE;
}
BOOL CDC::ExtTextOut(int x, int y, UINT opt, LPCRECT r, const CString &s, int *)
{
	if ((opt & ETO_OPAQUE) && r) { COLORREF bk = m_crBk; FillSolidRect(r, bk); }
	CRect save = m_clip;
	if ((opt & ETO_CLIPPED) && r) {
		m_clip.left = max(m_clip.left, r->left + m_ox); m_clip.top = max(m_clip.top, r->top + m_oy);
		m_clip.right = min(m_clip.right, r->right + m_ox); m_clip.bottom = min(m_clip.bottom, r->bottom + m_oy);
	}
	shim::DrawText(*this, m_font, x, y, s, s.GetLength(), m_crText);
	m_clip = save;
	return TRUE;
}
CSize CDC::GetTextExtent(const char *s, int n) const { return CSize(shim::TextWidth(m_font, s, n), shim::FontHeight(m_font)); }
BOOL CDC::GetTextMetrics(TEXTMETRIC *tm) const
{
	memset(tm, 0, sizeof *tm);
	tm->tmHeight = shim::FontHeight(m_font);
	tm->tmAscent = tm->tmHeight - 3;
	tm->tmDescent = 3;
	tm->tmAveCharWidth = m_font ? 7 : 6;
	tm->tmMaxCharWidth = 13;
	tm->tmExternalLeading = 1;
	return TRUE;
}
// word-wrapping text box (statics, message boxes)
int CDC::DrawText(const CString &str, LPRECT r, UINT fmt)
{
	const char *s = str;
	int n = str.GetLength(), lh = shim::FontHeight(m_font), y = r->top, w = r->right - r->left, maxw = 0;
	std::vector<std::pair<int, int>> lines;
	int i = 0;
	while (i <= n) {
		int start = i, lastBreak = -1, lw = 0;
		while (i < n && s[i] != '\n') {
			if (s[i] == '\r') { i++; continue; }
			int cw = shim::TextWidth(m_font, s + i, 1);
			if ((fmt & DT_WORDBREAK) && !(fmt & DT_SINGLELINE) && lw + cw > w && i > start) {
				if (lastBreak > start) i = lastBreak;
				break;
			}
			if (s[i] == ' ') lastBreak = i + 1;
			lw += cw;
			i++;
		}
		int end = i;
		while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\r')) end--;
		lines.push_back({start, end});
		if (i < n && s[i] == '\n') i++;
		else if (i >= n) break;
		while (i < n && s[i] == ' ' && (fmt & DT_WORDBREAK)) i++;
		if (fmt & DT_SINGLELINE) break;
	}
	if (fmt & DT_CALCRECT) {
		for (auto &l : lines) maxw = max(maxw, shim::TextWidth(m_font, s + l.first, l.second - l.first));
		r->right = r->left + maxw;
		r->bottom = r->top + lh * (int)lines.size();
		return lh * (int)lines.size();
	}
	if ((fmt & DT_VCENTER) && (fmt & DT_SINGLELINE)) y = r->top + (r->bottom - r->top - lh) / 2;
	CRect save = m_clip;
	m_clip.left = max(m_clip.left, r->left + m_ox); m_clip.top = max(m_clip.top, r->top + m_oy);
	m_clip.right = min(m_clip.right, r->right + m_ox); m_clip.bottom = min(m_clip.bottom, r->bottom + m_oy);
	for (auto &l : lines) {
		int lw = shim::TextWidth(m_font, s + l.first, l.second - l.first);
		int x = (fmt & DT_CENTER) ? r->left + (w - lw) / 2 : (fmt & DT_RIGHT) ? r->right - lw : r->left;
		if (m_nBkMode == OPAQUE) { COLORREF bk = m_crBk; FillSolidRect(x, y, lw, lh, bk); }
		shim::DrawText(*this, m_font, x, y, s + l.first, l.second - l.first, m_crText);
		y += lh;
	}
	m_clip = save;
	return y - r->top;
}

// ---------------------------------------------------------------- image lists
static void SplitInto(CImageList *il, Bitmap *b, COLORREF mask)
{
	if (!b || il->m_cx <= 0) return;
	if (!il->m_cy) il->m_cy = b->h;
	for (int x = 0; x + il->m_cx <= b->w; x += il->m_cx) {
		Bitmap *img = new Bitmap(il->m_cx, il->m_cy);
		for (int y = 0; y < il->m_cy && y < b->h; y++)
			memcpy(img->row(y), b->row(y) + x, il->m_cx * sizeof(DWORD));
		il->m_imgs.push_back(img);
		il->m_mask.push_back(mask);
	}
}
BOOL CImageList::Create(UINT id, int cx, int, COLORREF mask)
{
	m_cx = cx;
	Bitmap *b = shim::LoadBmpRes(id);
	SplitInto(this, b, mask);
	return b != nullptr;
}
int CImageList::Add(CBitmap *b, COLORREF mask)
{
	int first = (int)m_imgs.size();
	SplitInto(this, b ? (HBITMAP)*b : nullptr, mask);
	return first;
}
BOOL CImageList::Draw(CDC *dc, int i, POINT pt, UINT style)
{
	return DrawIndirect(dc, i, pt, CSize(m_cx, m_cy), CPoint(0, 0), style);
}
BOOL CImageList::DrawIndirect(CDC *dc, int i, POINT pt, SIZE sz, POINT org, UINT style, DWORD rop, COLORREF, COLORREF)
{
	if (i < 0 || i >= (int)m_imgs.size()) return FALSE;
	bool keyed = !(style & ILD_ROP) && m_mask[i] != CLR_NONE;
	dc->blit(m_imgs[i], org.x, org.y, pt.x, pt.y, sz.cx ? sz.cx : m_cx, sz.cy ? sz.cy : m_cy, (style & ILD_ROP) ? rop : SRCCOPY, keyed, m_mask[i] & 0xFFFFFF);
	return TRUE;
}
