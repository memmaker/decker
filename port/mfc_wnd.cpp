// MFC shim: windows, message maps, painting, dialogs, DDX, input routing.
#include "afxwin.h"
#undef min
#undef max
#include <algorithm>
using std::max;
using std::min;
#include "res.h"
#include <algorithm>
#include <deque>
#include <functional>

namespace shim {
std::vector<CWnd *> g_topLevel;
std::vector<CWnd *> g_modal;
CWnd *g_focus = nullptr;
CWnd *g_capture = nullptr;
BOOL g_quit = FALSE;
BOOL g_clickOutside = FALSE;
int g_mouseX = 0, g_mouseY = 0;
struct Posted { CWnd *w; UINT msg; WPARAM wp; LPARAM lp; };
std::deque<Posted> g_posted;

void AddTopLevel(CWnd *w) { g_topLevel.push_back(w); }
void RemoveTopLevel(CWnd *w) { g_topLevel.erase(std::remove(g_topLevel.begin(), g_topLevel.end(), w), g_topLevel.end()); }
CWnd *ModalTop() { return g_modal.empty() ? nullptr : g_modal.back(); }
static BOOL Alive(CWnd *w)
{
	std::function<BOOL(CWnd *)> in = [&](CWnd *p) -> BOOL {
		if (p == w) return TRUE;
		for (CWnd *c : p->m_children) if (in(c)) return TRUE;
		return FALSE;
	};
	for (CWnd *t : g_topLevel) if (in(t)) return TRUE;
	return FALSE;
}
}
using namespace shim;

// ---------------------------------------------------------------- message maps
const AFX_MSGMAP *CCmdTarget::GetThisMessageMap()
{
	static const AFX_MSGMAP_ENTRY e[] = {{0, 0, 0, 0, AfxSig_end, (AFX_PMSG)0}};
	static const AFX_MSGMAP m = {nullptr, e};
	return &m;
}
const AFX_MSGMAP *CCmdTarget::GetMessageMap() const { return GetThisMessageMap(); }
const AFX_MSGMAP_ENTRY *CCmdTarget::FindEntry(UINT msg, UINT code, UINT id) const
{
	for (const AFX_MSGMAP *m = GetMessageMap(); m; m = m->pfnGetBaseMap ? m->pfnGetBaseMap() : nullptr) {
		for (const AFX_MSGMAP_ENTRY *e = m->lpEntries; e->nSig != AfxSig_end; e++)
			if (e->nMessage == msg && e->nCode == code && (e->nID == id || e->nID == 0xFFFFFFFF)) return e;
		if (!m->pfnGetBaseMap) break;
	}
	return nullptr;
}
BOOL CCmdTarget::OnCmdMsg(UINT nID, int nCode, void *)
{
	const AFX_MSGMAP_ENTRY *e = FindEntry(WM_COMMAND, nCode, nID);
	if (!e) return FALSE;
	(this->*e->pfn)();
	return TRUE;
}
static LRESULT CallEntry(CCmdTarget *t, const AFX_MSGMAP_ENTRY *e, WPARAM wp, LPARAM lp)
{
	switch (e->nSig) {
	case AfxSig_vv: (t->*e->pfn)(); return 0;
	case AfxSig_bv: return (t->*reinterpret_cast<BOOL (CCmdTarget::*)()>(e->pfn))();
	case AfxSig_is: return (t->*reinterpret_cast<int (CCmdTarget::*)(LPCREATESTRUCT)>(e->pfn))((LPCREATESTRUCT)lp);
	case AfxSig_bHELPINFO: return (t->*reinterpret_cast<BOOL (CCmdTarget::*)(HELPINFO *)>(e->pfn))((HELPINFO *)lp);
	case AfxSig_vOWNER: (t->*reinterpret_cast<void (CCmdTarget::*)(int, LPDRAWITEMSTRUCT)>(e->pfn))((int)wp, (LPDRAWITEMSTRUCT)lp); return 1;
	case AfxSig_vwp: (t->*reinterpret_cast<void (CCmdTarget::*)(UINT, CPoint)>(e->pfn))((UINT)wp, CPoint((short)LOWORD(lp), (short)HIWORD(lp))); return 0;
	case AfxSig_vwl: (t->*reinterpret_cast<void (CCmdTarget::*)(UINT, LPARAM)>(e->pfn))((UINT)wp, lp); return 0;
	case AfxSig_hv: return (LRESULT)(t->*reinterpret_cast<HCURSOR (CCmdTarget::*)()>(e->pfn))();
	case AfxSig_lwl: return (t->*reinterpret_cast<LRESULT (CCmdTarget::*)(WPARAM, LPARAM)>(e->pfn))(wp, lp);
	}
	return 0;
}

BEGIN_MESSAGE_MAP(CWnd, CCmdTarget)
END_MESSAGE_MAP()
BEGIN_MESSAGE_MAP(CDialog, CWnd)
END_MESSAGE_MAP()
BEGIN_MESSAGE_MAP(CScrollView, CWnd)
END_MESSAGE_MAP()
BEGIN_MESSAGE_MAP(CWinApp, CCmdTarget)
END_MESSAGE_MAP()

// ---------------------------------------------------------------- CWnd
CWnd::CWnd() {}
CWnd::~CWnd()
{
	if (m_hWnd) DestroyWindow();
}
BOOL CWnd::HasHandler(UINT msg) const { return FindEntry(msg, 0, 0) != nullptr; }

LRESULT CWnd::WindowProc(UINT msg, WPARAM wp, LPARAM lp)
{
	if (msg == WM_COMMAND) return OnCommand(wp, lp);
	if (msg == WM_NOTIFY) { LRESULT r = 0; OnNotify(wp, lp, &r); return r; }
	if (const AFX_MSGMAP_ENTRY *e = FindEntry(msg, 0, 0)) return CallEntry(this, e, wp, lp);
	return DefWindowProc(msg, wp, lp);
}
LRESULT CWnd::DefWindowProc(UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
	case WM_PAINT: OnPaint(); return 0;
	case WM_CLOSE: OnClose(); return 0;
	case WM_DRAWITEM: OnDrawItem((int)wp, (LPDRAWITEMSTRUCT)lp); return 1;
	case WM_SYSCOMMAND: OnSysCommand((UINT)wp, lp); return 0;
	case WM_HELP: return m_parent ? m_parent->SendMessage(WM_HELP, wp, lp) : 0;
	}
	return 0;
}
BOOL CWnd::OnCommand(WPARAM wp, LPARAM lp)
{
	UINT id = LOWORD(wp), code = HIWORD(wp);
	if (!lp) code = CN_COMMAND; // menus and accelerators (code 1), as MFC does
	if (OnCmdMsg(id, code, nullptr)) return TRUE;
	if (IsDialog() && code == 0) {
		CDialog *d = (CDialog *)this;
		if (id == IDOK) { d->OnOK(); return TRUE; }
		if (id == IDCANCEL) { d->OnCancel(); return TRUE; }
	}
	if (this == AfxGetMainWnd() || !m_parent) return AfxGetApp()->OnCmdMsg(id, code, nullptr);
	return FALSE;
}
BOOL CWnd::OnNotify(WPARAM, LPARAM lp, LRESULT *res)
{
	NMHDR *h = (NMHDR *)lp;
	if (const AFX_MSGMAP_ENTRY *e = FindEntry(WM_NOTIFY, h->code, (UINT)h->idFrom)) {
		if (e->nSig == AfxSig_bNMHDRpl)
			return (this->*reinterpret_cast<BOOL (CCmdTarget::*)(UINT, NMHDR *, LRESULT *)>(e->pfn))((UINT)h->idFrom, h, res);
		(this->*reinterpret_cast<void (CCmdTarget::*)(NMHDR *, LRESULT *)>(e->pfn))(h, res);
		return TRUE;
	}
	return FALSE;
}
void CWnd::OnPaint()
{
	CPaintDC dc(this);
	ShimPaint(dc);
}
void CWnd::OnDrawItem(int, LPDRAWITEMSTRUCT dis)
{
	if (dis && dis->hwndItem) dis->hwndItem->DrawItem(dis);
}
void CWnd::OnSysCommand(UINT id, LPARAM)
{
	if ((id & 0xFFF0) == SC_CLOSE) SendMessage(WM_CLOSE);
}
LRESULT CWnd::SendMessage(UINT msg, WPARAM wp, LPARAM lp) { return m_hWnd ? WindowProc(msg, wp, lp) : 0; }
BOOL CWnd::PostMessage(UINT msg, WPARAM wp, LPARAM lp) { g_posted.push_back({this, msg, wp, lp}); return TRUE; }
LRESULT SendMessage(HWND h, UINT msg, WPARAM wp, LPARAM lp) { return h ? h->SendMessage(msg, wp, lp) : 0; }
BOOL PostMessage(HWND h, UINT msg, WPARAM wp, LPARAM lp) { return h ? h->PostMessage(msg, wp, lp) : FALSE; }

BOOL CWnd::Create(LPCTSTR cls, LPCTSTR name, DWORD style, const RECT &r, CWnd *parent, UINT id, void *)
{
	return CreateEx(0, cls, name, style, r.left, r.top, r.right - r.left, r.bottom - r.top, parent ? parent->m_hWnd : nullptr, (HMENU)(uintptr_t)id);
}
BOOL CWnd::CreateEx(DWORD exStyle, LPCTSTR, LPCTSTR name, DWORD style, int x, int y, int cx, int cy, HWND parent, HMENU id, LPVOID)
{
	CREATESTRUCT cs = {};
	cs.x = x; cs.y = y; cs.cx = cx; cs.cy = cy;
	cs.style = style; cs.dwExStyle = exStyle;
	cs.hwndParent = parent; cs.hMenu = id; cs.lpszName = name;
	if (!PreCreateWindow(cs)) return FALSE;
	m_hWnd = this;
	m_style = cs.style & ~WS_VISIBLE;
	m_exStyle = cs.dwExStyle;
	m_text = cs.lpszName;
	BOOL child = (cs.style & WS_CHILD) && parent;
	m_parent = child ? parent : nullptr;
	m_nID = child ? (UINT)(uintptr_t)cs.hMenu : 0;
	m_rect = CRect(cs.x, cs.y, cs.x + cs.cx, cs.y + cs.cy);
	if (child) {
		parent->m_children.push_back(this);
		m_font = parent->IsDialog() ? 0 : 1;
	} else {
		if (IsDialog()) { m_clientX = m_clientRight = m_clientBottom = 3; m_clientY = 3 + 18; }
		m_backing = new Bitmap(max(1, (int)m_rect.Width()), max(1, (int)m_rect.Height()));
		AddTopLevel(this);
	}
	m_bDirty = TRUE;
	if (SendMessage(WM_CREATE, 0, (LPARAM)&cs) == -1) { DestroyWindow(); return FALSE; }
	if (cs.style & WS_VISIBLE) ShowWindow(SW_SHOW);
	return TRUE;
}
BOOL CWnd::DestroyWindow()
{
	if (!m_hWnd) return FALSE;
	// a window whose modal dialog is still on the stack (OptionsDlg Quit closes
	// the Matrix view from inside OnOptions) goes once the modal has returned
	if (!m_parent) for (CWnd *d : g_modal) if (d->m_owner == this) { PostMessage(WM_CLOSE); return TRUE; }
	// Unlink everything first: a WM_DESTROY handler may `delete this`
	// (CMatrixView does), so `this` is not touched after it runs.
	// Children go first (Windows sends WM_DESTROY to the parent first;
	// no Decker child handles it).
	while (!m_children.empty()) {
		CWnd *c = m_children.back();
		c->DestroyWindow();
		if (c->m_bOwned) delete c;
	}
	if (m_parent) {
		auto &v = m_parent->m_children;
		v.erase(std::remove(v.begin(), v.end(), this), v.end());
		m_parent->Invalidate();
	} else
		RemoveTopLevel(this);
	if (g_focus == this) g_focus = nullptr;
	if (g_capture == this) g_capture = nullptr;
	for (auto &p : g_posted) if (p.w == this) p.w = nullptr;
	delete m_backing;
	m_backing = nullptr;
	m_hWnd = nullptr;
	m_parent = nullptr;
	if (AfxGetApp() && AfxGetApp()->m_pMainWnd == this) { AfxGetApp()->m_pMainWnd = nullptr; g_quit = TRUE; }
	WindowProc(WM_DESTROY, 0, 0);
	return TRUE;
}

CWnd *CWnd::GetTopLevel()
{
	CWnd *w = this;
	while (w && w->m_parent) w = w->m_parent;
	return w;
}
CWnd *CWnd::GetDlgItem(int id) const
{
	for (CWnd *c : m_children) if ((int)c->m_nID == id) return c;
	return nullptr;
}
CPoint CWnd::BackingOrigin() const
{
	if (!m_parent) return CPoint(m_clientX, m_clientY);
	CPoint p = m_parent->BackingOrigin();
	return CPoint(p.x + m_rect.left + m_clientX, p.y + m_rect.top + m_clientY);
}
CPoint CWnd::ScreenOrigin() const
{
	CPoint p = BackingOrigin();
	const CWnd *t = this;
	while (t->m_parent) t = t->m_parent;
	return CPoint(p.x + t->m_rect.left, p.y + t->m_rect.top);
}
CRect CWnd::VisibleRect() const
{
	if (!m_parent) return CRect(0, 0, m_rect.Width(), m_rect.Height());
	CPoint o = m_parent->BackingOrigin();
	CRect r(o.x + m_rect.left, o.y + m_rect.top, o.x + m_rect.right, o.y + m_rect.bottom);
	CRect pr = m_parent->VisibleRect();
	CRect pc;
	m_parent->GetClientRect(&pc);
	pc.OffsetRect(o.x, o.y);
	r.left = max(max(r.left, pr.left), pc.left); r.top = max(max(r.top, pr.top), pc.top);
	r.right = min(min(r.right, pr.right), pc.right); r.bottom = min(min(r.bottom, pr.bottom), pc.bottom);
	if (r.right < r.left) r.right = r.left;
	if (r.bottom < r.top) r.bottom = r.top;
	return r;
}
void CWnd::GetWindowRect(LPRECT r) const
{
	CPoint o = ScreenOrigin();
	r->left = o.x - m_clientX; r->top = o.y - m_clientY;
	r->right = r->left + m_rect.Width(); r->bottom = r->top + m_rect.Height();
}
void CWnd::ClientToScreen(LPPOINT p) const { CPoint o = ScreenOrigin(); p->x += o.x; p->y += o.y; }
void CWnd::ClientToScreen(LPRECT r) const { CPoint o = ScreenOrigin(); ((CRect *)r)->OffsetRect(o.x, o.y); }
void CWnd::ScreenToClient(LPPOINT p) const { CPoint o = ScreenOrigin(); p->x -= o.x; p->y -= o.y; }
void CWnd::ScreenToClient(LPRECT r) const { CPoint o = ScreenOrigin(); ((CRect *)r)->OffsetRect(-o.x, -o.y); }
void CWnd::MapWindowPoints(CWnd *to, LPPOINT p, UINT n) const
{
	for (UINT i = 0; i < n; i++) { ClientToScreen(&p[i]); if (to) to->ScreenToClient(&p[i]); }
}
void CWnd::MapWindowPoints(CWnd *to, LPRECT r) const { MapWindowPoints(to, (LPPOINT)r, 2); }

BOOL CWnd::IsWindowVisible() const
{
	for (const CWnd *w = this; w; w = w->m_parent) if (!w->m_bVisible) return FALSE;
	return m_hWnd != nullptr;
}
BOOL CWnd::ShowWindow(int cmd)
{
	BOOL was = m_bVisible;
	m_bVisible = cmd != SW_HIDE;
	if (m_bVisible) m_style |= WS_VISIBLE; else m_style &= ~WS_VISIBLE;
	if (was != m_bVisible) {
		if (m_parent) m_parent->Invalidate();
		Invalidate();
		if (!m_bVisible && g_focus && g_focus->GetTopLevel() == this) g_focus = nullptr;
	}
	return was;
}
BOOL CWnd::EnableWindow(BOOL e)
{
	BOOL was = !IsWindowEnabled();
	if (e) m_style &= ~WS_DISABLED; else m_style |= WS_DISABLED;
	if (!e && g_focus == this) g_focus = nullptr;
	Invalidate();
	return was;
}
void CWnd::SetWindowText(LPCTSTR s)
{
	m_text = s;
	Invalidate();
	if (m_parent && !m_parent->IsDialog()) m_parent->Invalidate();
}
void CWnd::CheckDlgButton(int id, UINT c) { if (CButton *b = (CButton *)GetDlgItem(id)) b->SetCheck(c); }
UINT CWnd::IsDlgButtonChecked(int id) const { CButton *b = (CButton *)GetDlgItem(id); return b ? b->GetCheck() : 0; }
void CWnd::CheckRadioButton(int first, int last, int check)
{
	for (int i = first; i <= last; i++) CheckDlgButton(i, i == check);
}
void CWnd::Invalidate(BOOL) { m_bDirty = TRUE; }
BOOL CWnd::SetWindowPos(const CWnd *, int x, int y, int cx, int cy, UINT flags)
{
	CRect r = m_rect;
	if (!(flags & SWP_NOMOVE)) r.OffsetRect(x - r.left, y - r.top);
	if (!(flags & SWP_NOSIZE)) { r.right = r.left + cx; r.bottom = r.top + cy; }
	MoveWindow(r.left, r.top, r.Width(), r.Height());
	if (flags & SWP_SHOWWINDOW) ShowWindow(SW_SHOW);
	return TRUE;
}
void CWnd::MoveWindow(int x, int y, int cx, int cy, BOOL)
{
	BOOL resized = cx != m_rect.Width() || cy != m_rect.Height();
	m_rect = CRect(x, y, x + cx, y + cy);
	if (!m_parent && resized) { delete m_backing; m_backing = new Bitmap(max(1, cx), max(1, cy)); }
	if (m_parent) m_parent->Invalidate();
	Invalidate();
}
void CWnd::CenterWindow(CWnd *)
{
	if (m_parent) return;
	MoveWindow((640 - m_rect.Width()) / 2, (480 - m_rect.Height()) / 2, m_rect.Width(), m_rect.Height());
}
CWnd *CWnd::SetFocus()
{
	CWnd *old = g_focus;
	if (old == this) return old;
	g_focus = this;
	if (old && Alive(old)) {
		old->Invalidate();
		if (old->m_parent && dynamic_cast<CEdit *>(old))
			old->m_parent->SendMessage(WM_COMMAND, MAKELONG(old->m_nID, EN_KILLFOCUS), 0);
	}
	Invalidate();
	return old;
}
CWnd *CWnd::GetFocus() { return g_focus; }
CWnd *CWnd::GetActiveWindow()
{
	if (CWnd *m = ModalTop()) return m;
	for (auto it = g_topLevel.rbegin(); it != g_topLevel.rend(); ++it) if ((*it)->m_bVisible) return *it;
	return nullptr;
}
HWND GetActiveWindow() { return CWnd::GetActiveWindow(); }
HWND GetParent(HWND h) { return h ? h->m_parent : nullptr; }
int GetDlgCtrlID(HWND h) { return h ? h->m_nID : 0; }
CDC *CWnd::GetDC() { return new CClientDC(this); }
int CWnd::ReleaseDC(CDC *dc) { delete dc; return 1; }
int CWnd::MessageBox(LPCTSTR text, LPCTSTR caption, UINT type) { return ::MessageBox(this, text, caption, type); }
BOOL CWnd::SubclassDlgItem(UINT, CWnd *) { return FALSE; }
void CWnd::WinHelp(DWORD data, UINT cmd) { AfxGetApp()->WinHelp(data, cmd); }

// ---------------------------------------------------------------- DCs + painting
static void SetupDC(CDC &dc, CWnd *w, BOOL client)
{
	CWnd *top = w->GetTopLevel();
	dc.m_pWnd = w;
	dc.m_surf = top ? top->m_backing : nullptr;
	CPoint o = w->BackingOrigin();
	dc.m_ox = o.x; dc.m_oy = o.y;
	CRect vis = w->VisibleRect();
	if (client) {
		CRect cr;
		w->GetClientRect(&cr);
		cr.OffsetRect(o.x, o.y);
		vis.left = max(vis.left, cr.left); vis.top = max(vis.top, cr.top);
		vis.right = min(vis.right, cr.right); vis.bottom = min(vis.bottom, cr.bottom);
	}
	dc.m_clip = vis;
	dc.m_font = w->m_font;
	if (!w->IsWindowVisible()) dc.m_clip = CRect();
}
CPaintDC::CPaintDC(CWnd *w) { SetupDC(*this, w, TRUE); w->m_bDirty = FALSE; }
CClientDC::CClientDC(CWnd *w) { SetupDC(*this, w, TRUE); }
CWindowDC::CWindowDC(CWnd *w) { SetupDC(*this, w, TRUE); }

namespace shim {
void DrawFrameCtl(CDC &dc, CRect r, BOOL sunken, BOOL thick)
{
	COLORREF a = sunken ? COLOR_SHADOW : COLOR_HILITE, b = sunken ? COLOR_HILITE : COLOR_DKSHADOW;
	COLORREF a2 = sunken ? COLOR_DKSHADOW : COLOR_LIGHT, b2 = sunken ? COLOR_LIGHT : COLOR_SHADOW;
	dc.Draw3dRect(r, a, b);
	if (thick) { r.DeflateRect(1, 1); dc.Draw3dRect(r, a2, b2); }
}
void DrawArrow(CDC &dc, CRect r, int dir, COLORREF c)
{
	int cx = (r.left + r.right) / 2, cy = (r.top + r.bottom) / 2;
	for (int i = 0; i < 4; i++) {
		switch (dir) {
		case 0: dc.FillSolidRect(cx - i, cy - 2 + i, 2 * i + 1, 1, c); break;
		case 2: dc.FillSolidRect(cx - 3 + i, cy - 1 + i, 7 - 2 * i, 1, c); break;
		case 1: dc.FillSolidRect(cx - 1 + i, cy - 3 + i, 1, 7 - 2 * i, c); break;
		case 3: dc.FillSolidRect(cx - 2 + i, cy - i, 1, 2 * i + 1, c); break;
		}
	}
}
static void DrawNonClient(CWnd *w)
{
	if (w->m_parent || !w->IsDialog()) return;
	CDC dc;
	dc.m_surf = w->m_backing;
	dc.m_clip = CRect(0, 0, w->m_rect.Width(), w->m_rect.Height());
	CRect r(0, 0, w->m_rect.Width(), w->m_rect.Height());
	dc.FillSolidRect(&r, COLOR_FACE);
	DrawFrameCtl(dc, r, FALSE);
	BOOL active = GetActiveWindow() == w;
	CRect cap(3, 3, r.right - 3, 3 + 18);
	for (int x = cap.left; x < cap.right; x++) { // blue gradient caption (Win98)
		int t = (x - cap.left) * 255 / max(1, (int)cap.Width());
		COLORREF c = active ? RGB(t * 16 / 255, t * 132 / 255, 128 + t * 80 / 255) : RGB(128 + t * 64 / 255, 128 + t * 64 / 255, 128 + t * 64 / 255);
		dc.FillSolidRect(x, cap.top, 1, cap.Height() - 1, c);
	}
	dc.m_font = 1;
	shim::DrawText(dc, 1, cap.left + 4, cap.top + 2, w->m_text, w->m_text.GetLength(), active ? 0xFFFFFF : COLOR_FACE);
	if (w->m_style & WS_SYSMENU) { // close box
		CRect b(cap.right - 18, cap.top + 2, cap.right - 2, cap.top + 16);
		dc.FillSolidRect(&b, COLOR_FACE);
		DrawFrameCtl(dc, b, FALSE);
		for (int i = 0; i < 6; i++) {
			dc.FillSolidRect(b.left + 4 + i, b.top + 3 + i, 2, 1, 0);
			dc.FillSolidRect(b.left + 9 - i, b.top + 3 + i, 2, 1, 0);
		}
	}
}
static void PaintSubtree(CWnd *w);
static void PaintOne(CWnd *w)
{
	if (!w->IsWindowVisible()) { w->m_bDirty = FALSE; return; }
	DrawNonClient(w);
	w->SendMessage(WM_PAINT);
	w->m_bDirty = FALSE;
}
static void PaintSubtree(CWnd *w)
{
	PaintOne(w);
	std::vector<CWnd *> kids = w->m_children;
	for (CWnd *c : kids) if (c->m_bVisible) PaintSubtree(c);
}
// repaint w and every sibling above it that overlaps it
static void PaintWithOverlaps(CWnd *w)
{
	PaintSubtree(w);
	if (!w->m_parent) return;
	auto &sib = w->m_parent->m_children;
	auto it = std::find(sib.begin(), sib.end(), w);
	if (it == sib.end()) return;
	std::vector<CWnd *> above(it + 1, sib.end());
	for (CWnd *s : above) {
		CRect a = w->m_rect, b = s->m_rect;
		if (s->m_bVisible && a.left < b.right && b.left < a.right && a.top < b.bottom && b.top < a.bottom) PaintSubtree(s);
	}
}
static void PaintDirtyIn(CWnd *w)
{
	if (!w->m_bVisible) return;
	if (w->m_bDirty) { PaintWithOverlaps(w); return; }
	std::vector<CWnd *> kids = w->m_children;
	for (CWnd *c : kids) if (Alive(c)) PaintDirtyIn(c);
}
void PaintDirty()
{
	std::vector<CWnd *> tops = g_topLevel;
	for (CWnd *t : tops) if (Alive(t)) PaintDirtyIn(t);
}
}
BOOL CWnd::RedrawWindow(LPCRECT, CRgn *, UINT flags)
{
	if (!m_hWnd) return FALSE;
	if (flags & RDW_UPDATENOW) shim::PaintWithOverlaps(this);
	else Invalidate();
	return TRUE;
}
void CWnd::UpdateWindow()
{
	if (m_hWnd && m_bDirty) shim::PaintWithOverlaps(this);
}

// ---------------------------------------------------------------- input routing
namespace shim {
static CWnd *ChildAt(CWnd *w, CPoint scr)
{
	for (auto it = w->m_children.rbegin(); it != w->m_children.rend(); ++it) {
		CWnd *c = *it;
		if (!c->m_bVisible) continue;
		CRect r;
		c->GetWindowRect(&r);
		if (r.PtInRect(scr)) {
			if ((c->m_style & BS_TYPEMASK) == BS_GROUPBOX && dynamic_cast<CButton *>(c)) continue;
			if (dynamic_cast<CStatic *>(c) && !(c->m_style & SS_NOTIFY)) continue;
			return ChildAt(c, scr);
		}
	}
	return w;
}
CWnd *TopAt(CPoint p)
{
	for (auto it = g_topLevel.rbegin(); it != g_topLevel.rend(); ++it) {
		CWnd *t = *it;
		if (t->m_bVisible && t->m_rect.PtInRect(p)) return t;
	}
	return nullptr;
}
// top-level windows the user may use: the modal dialog and anything above it
BOOL InputAllowed(CWnd *top)
{
	CWnd *m = ModalTop();
	if (!m) return TRUE;
	auto im = std::find(g_topLevel.begin(), g_topLevel.end(), m);
	auto it = std::find(g_topLevel.begin(), g_topLevel.end(), top);
	return it >= im;
}
static void ToFront(CWnd *t)
{
	if (!t || ModalTop()) return;
	RemoveTopLevel(t);
	AddTopLevel(t);
}
static DWORD g_lastClick = 0;
static CWnd *g_lastClickWnd = nullptr;
void HandleMouse(UINT msg, int x, int y)
{
	g_mouseX = x; g_mouseY = y;
	CPoint scr(x, y);
	CWnd *top = TopAt(scr);
	CWnd *target = g_capture ? g_capture : top ? ChildAt(top, scr) : nullptr;
	if (msg == WM_LBUTTONDOWN && !g_capture) {
		if (!top || !InputAllowed(top)) { g_clickOutside = TRUE; return; }
		ToFront(top);
		// caption close box
		if (top->IsDialog() && (top->m_style & WS_SYSMENU)) {
			CRect cb(top->m_rect.right - 21, top->m_rect.top + 5, top->m_rect.right - 5, top->m_rect.top + 19);
			if (cb.PtInRect(scr)) { top->SendMessage(WM_COMMAND, IDCANCEL); return; }
		}
		for (CWnd *w = target; w; w = w->m_parent) if (!w->IsWindowEnabled()) return;
		if (target->ShimWantsFocus()) target->SetFocus();
		DWORD now = GetTickCount();
		if (target == g_lastClickWnd && now - g_lastClick < 400) { msg = WM_LBUTTONDBLCLK; g_lastClickWnd = nullptr; }
		else { g_lastClick = now; g_lastClickWnd = target; }
		g_capture = target;
	}
	if (!target || !target->m_hWnd) return;
	if (msg != WM_MOUSEMOVE && !g_capture && top && !InputAllowed(top)) return;
	for (CWnd *w = target; w; w = w->m_parent) if (!w->IsWindowEnabled()) { if (msg == WM_LBUTTONUP) g_capture = nullptr; return; }
	CPoint o = target->ScreenOrigin();
	CPoint pt(x - o.x, y - o.y);
	CWnd *keep = target;
	// release before dispatching: the click may open a modal loop
	if (msg == WM_LBUTTONUP || msg == WM_RBUTTONUP) g_capture = nullptr;
	if (!target->ShimMouse(msg, pt)) {
		if (msg == WM_LBUTTONDBLCLK) msg = WM_LBUTTONDOWN;
		if (msg != WM_MOUSEMOVE && Alive(keep)) keep->SendMessage(msg, 0, MAKELONG(pt.x, pt.y));
	}
	if (msg == WM_LBUTTONUP || msg == WM_RBUTTONUP) g_capture = nullptr;
}
void HandleWheel(int x, int y, int dy)
{
	CWnd *top = TopAt(CPoint(x, y));
	if (!top || !InputAllowed(top)) return;
	for (CWnd *w = ChildAt(top, CPoint(x, y)); w; w = w->m_parent)
		if (w->ShimWheel(dy)) return;
}
static std::vector<CWnd *> Focusables(CWnd *dlg)
{
	std::vector<CWnd *> v;
	for (CWnd *c : dlg->m_children)
		if (c->m_bVisible && c->IsWindowEnabled() && (c->m_style & WS_TABSTOP) && c->ShimWantsFocus()) v.push_back(c);
	return v;
}
static CWnd *KeyTop()
{
	if (CWnd *m = ModalTop()) {
		// popups above the modal (menus, combo drop-downs) come first
		for (auto it = g_topLevel.rbegin(); it != g_topLevel.rend() && *it != m; ++it) if ((*it)->m_bVisible) return *it;
		return m;
	}
	return CWnd::GetActiveWindow();
}
void HandleKey(UINT vk, BOOL down, UINT mods)
{
	CWnd *top = KeyTop();
	if (!top) return;
	CWnd *f = g_focus && g_focus->m_hWnd && g_focus->GetTopLevel() == top ? g_focus : top;
	if (!down) { f->ShimKey(vk, FALSE); return; }
	if (vk == VK_F1) {
		HELPINFO hi = {sizeof hi};
		hi.iCtrlId = f->m_nID;
		top->SendMessage(WM_HELP, 0, (LPARAM)&hi);
		return;
	}
	MSG m = {f, WM_KEYDOWN, vk, 0};
	for (CWnd *w = f; w; w = w->m_parent) if (w->PreTranslateMessage(&m)) return;
	if (!Alive(f)) return;
	if (f->ShimKey(vk, TRUE)) return;
	if (!top->IsDialog()) return;
	CDialog *dlg = (CDialog *)top;
	if (vk == VK_TAB) {
		if (mods & 1) dlg->PrevDlgCtrl(); else dlg->NextDlgCtrl();
	} else if (vk == VK_RETURN) {
		CButton *b = dynamic_cast<CButton *>(f);
		if (b && b->Type() <= BS_DEFPUSHBUTTON) b->Click();
		else if (CWnd *d = dlg->DefaultButton()) { if (d->IsWindowEnabled()) ((CButton *)d)->Click(); }
		else dlg->SendMessage(WM_COMMAND, IDOK);
	} else if (vk == VK_ESCAPE) {
		dlg->SendMessage(WM_COMMAND, IDCANCEL);
	}
}
void HandleChar(UINT ch)
{
	CWnd *top = KeyTop();
	if (!top) return;
	CWnd *f = g_focus && g_focus->m_hWnd && g_focus->GetTopLevel() == top ? g_focus : nullptr;
	if (f && f->ShimChar(ch)) return;
	// dialogs: a letter clicks the button whose caption starts with it (as Alt+mnemonic)
	if (top->IsDialog() && ch > ' ' && ch < 127 && !(f && dynamic_cast<CEdit *>(f)))
		for (CWnd *c : top->m_children) {
			CButton *b = dynamic_cast<CButton *>(c);
			if (!b || !b->m_bVisible || !b->IsWindowEnabled() || b->Type() > BS_DEFPUSHBUTTON) continue;
			int amp = b->m_text.Find('&');
			if (amp >= 0 && amp + 1 < b->m_text.GetLength() && toupper(b->m_text[amp + 1]) == toupper(ch)) { b->Click(); return; }
		}
}
void DispatchPosted()
{
	// only what was queued before: a handler may post again (a deferred close)
	for (size_t n = g_posted.size(); n-- && !g_posted.empty();) {
		Posted p = g_posted.front();
		g_posted.pop_front();
		if (p.msg == WM_QUIT) { g_quit = TRUE; continue; }
		if (p.w && p.w->m_hWnd) p.w->SendMessage(p.msg, p.wp, p.lp);
	}
}
}
void PostQuitMessage(int) { shim::g_posted.push_back({nullptr, WM_QUIT, 0, 0}); }

HACCEL LoadAccelerators(HINSTANCE, LPCTSTR id) { return (HACCEL)(uintptr_t)id; }
int TranslateAccelerator(HWND h, HACCEL table, LPMSG m)
{
	if (m->message != WM_KEYDOWN) return 0;
	BOOL ctrl = GetKeyState(VK_CONTROL) < 0, shift = GetKeyState(VK_SHIFT) < 0;
	for (const AccelRes *a = g_accelRes; a->table; a++) {
		if (a->table != table || (UINT)a->key != m->wParam) continue;
		if (!!(a->flags & FCONTROL) != ctrl || !!(a->flags & FSHIFT) != shift) continue;
		h->SendMessage(WM_COMMAND, MAKELONG(a->cmd, 1), 0);
		return 1;
	}
	return 0;
}

// ---------------------------------------------------------------- app
static CWinApp *g_app;
CWinApp::CWinApp() { g_app = this; }
CWinApp *AfxGetApp() { return g_app; }
CWnd *AfxGetMainWnd() { return g_app ? g_app->m_pMainWnd : nullptr; }
HINSTANCE AfxGetInstanceHandle() { return (HINSTANCE)1; }
HINSTANCE AfxGetResourceHandle() { return (HINSTANCE)1; }
LPCTSTR AfxRegisterWndClass(UINT, HCURSOR, HBRUSH, HICON) { return "AfxWnd"; }
int CWinApp::Run()
{
	while (!shim::g_quit) {
		shim::PumpEvents(TRUE);
		shim::DispatchPosted();
		shim::PaintDirty();
		shim::Present();
	}
	return ExitInstance();
}
void port_help(DWORD ctx);
void CWinApp::OnHelp() { port_help(0); }
void CWinApp::WinHelp(DWORD data, UINT cmd) { port_help(cmd == HELP_CONTEXT ? data : 0); }

// ---------------------------------------------------------------- dialogs
static CWnd *MakeControl(const char *cls)
{
	if (!strcasecmp(cls, "Button")) return new CButton;
	if (!strcasecmp(cls, "Static")) return new CStatic;
	if (!strcasecmp(cls, "Edit")) return new CEdit;
	if (!strcasecmp(cls, "ComboBox")) return new CComboBox;
	if (!strcasecmp(cls, "ListBox")) return new CListBox;
	if (!strcasecmp(cls, "SysListView32")) return new CListCtrl;
	if (!strcasecmp(cls, "msctls_updown32")) return new CSpinButtonCtrl;
	if (!strcasecmp(cls, "msctls_progress32")) return new CProgressCtrl;
	return new CStatic;
}
void CDialog::MapDialogRect(LPRECT r) const
{
	r->left = r->left * 6 / 4; r->right = r->right * 6 / 4;
	r->top = r->top * 13 / 8; r->bottom = r->bottom * 13 / 8;
}
BOOL CDialog::CreateFromTemplate(const DlgTemplate *t, CWnd *parent)
{
	m_pTemplate = t;
	// which controls are C++ members (DDX_Control)
	CDataExchange dx(this, FALSE);
	dx.m_bDiscover = TRUE;
	DoDataExchange(&dx);
	CRect r(0, 0, t->cx, t->cy);
	MapDialogRect(&r);
	int w = r.Width() + 6, h = r.Height() + 3 + 18 + 3;
	m_hWnd = nullptr;
	if (!CreateEx(0, nullptr, t->caption, (t->style | WS_POPUP) & ~WS_VISIBLE & ~WS_CHILD, (640 - w) / 2, (480 - h) / 2, w, h, nullptr, nullptr)) return FALSE;
	for (int i = 0; i < t->count; i++) {
		const DlgItem &it = t->items[i];
		CWnd *c = nullptr;
		for (auto &d : dx.m_found) if (d.id == it.id && it.id != IDC_STATIC) c = d.w;
		BOOL owned = !c;
		if (!c) c = MakeControl(it.cls);
		CRect cr(it.x, it.y, it.x + it.cx, it.y + it.cy);
		MapDialogRect(&cr);
		c->m_bOwned = owned;
		c->CreateEx(it.exStyle, it.cls, it.text, it.style & ~WS_VISIBLE, cr.left, cr.top, cr.Width(), cr.Height(), this, (HMENU)(uintptr_t)(UINT)it.id);
		c->m_font = 0;
		if (CStatic *s = dynamic_cast<CStatic *>(c))
			if ((it.style & SS_TYPEMASK) == SS_BITMAP && isdigit((unsigned char)it.text[0])) s->m_bmp = shim::LoadBmpRes(atoi(it.text));
		if (CComboBox *cb = dynamic_cast<CComboBox *>(c))
			for (const DlgInitRes *d = g_dlgInitRes; d->text; d++)
				if (d->dlg == t->id && d->ctl == it.id) cb->AddString(d->text);
		if (it.style & WS_VISIBLE) c->ShowWindow(SW_SHOW);
	}
	return TRUE;
}
INT_PTR CDialog::DoModal()
{
	const DlgTemplate *t = m_pTemplate;
	for (const DlgTemplate *d = g_dlgTemplates; !t && d->items; d++) if ((UINT)d->id == m_nIDTemplate) t = d;
	if (!t) return -1;
	CWnd *oldFocus = g_focus;
	g_capture = nullptr; // a modal window takes the mouse (the click that opened it may still be down)
	m_owner = m_pParentWnd ? m_pParentWnd->GetTopLevel() : CWnd::GetActiveWindow();
	m_bModalDone = FALSE;
	m_nModalResult = -1;
	if (!CreateFromTemplate(t, m_pParentWnd)) return -1;
	g_modal.push_back(this);
	if (OnInitDialog()) {
		std::vector<CWnd *> f = Focusables(this);
		CWnd *def = DefaultButton();
		if (!f.empty()) f[0]->SetFocus();
		else if (def) def->SetFocus();
	}
	ShowWindow(SW_SHOW);
	for (CWnd *t2 : g_topLevel) t2->Invalidate(); // captions change active state
	while (!m_bModalDone && !g_quit) {
		shim::PaintDirty();
		shim::Present();
		shim::PumpEvents(TRUE);
		shim::DispatchPosted();
	}
	g_modal.pop_back();
	DestroyWindow();
	for (CWnd *t2 : g_topLevel) t2->Invalidate();
	if (oldFocus && Alive(oldFocus)) g_focus = oldFocus;
	return m_nModalResult;
}
BOOL CDialog::OnInitDialog()
{
	UpdateData(FALSE);
	return TRUE;
}
void CDialog::OnOK()
{
	if (!UpdateData(TRUE)) return;
	EndDialog(IDOK);
}
void CDialog::OnCancel() { EndDialog(IDCANCEL); }
void CDialog::EndDialog(int r)
{
	m_nModalResult = r;
	m_bModalDone = TRUE;
}
static BOOL g_ddxFailed;
BOOL CDialog::UpdateData(BOOL save)
{
	CDataExchange dx(this, save);
	g_ddxFailed = FALSE;
	DoDataExchange(&dx);
	return !g_ddxFailed;
}
void CDataExchange::Fail() { g_ddxFailed = TRUE; }
void CDialog::OnPaint()
{
	CPaintDC dc(this);
	CRect r;
	GetClientRect(&r);
	dc.FillSolidRect(&r, COLOR_FACE);
}
CWnd *CDialog::DefaultButton()
{
	for (CWnd *c : m_children)
		if (dynamic_cast<CButton *>(c) && (c->m_style & BS_TYPEMASK) == BS_DEFPUSHBUTTON && c->m_bVisible) return c;
	if (CWnd *ok = GetDlgItem(IDOK)) if (ok->m_bVisible) return ok;
	return nullptr;
}
void CDialog::NextDlgCtrl()
{
	std::vector<CWnd *> f = Focusables(this);
	if (f.empty()) return;
	auto it = std::find(f.begin(), f.end(), g_focus);
	(it == f.end() || it + 1 == f.end() ? f[0] : *(it + 1))->SetFocus();
}
void CDialog::PrevDlgCtrl()
{
	std::vector<CWnd *> f = Focusables(this);
	if (f.empty()) return;
	auto it = std::find(f.begin(), f.end(), g_focus);
	(it == f.end() || it == f.begin() ? f.back() : *(it - 1))->SetFocus();
}

// ---------------------------------------------------------------- DDX
void DDX_Control(CDataExchange *pDX, int id, CWnd &w)
{
	if (pDX->m_bDiscover) pDX->m_found.push_back({id, &w});
}
static CWnd *Item(CDataExchange *pDX, int id) { return pDX->m_bDiscover ? nullptr : pDX->m_pDlgWnd->GetDlgItem(id); }
void DDX_Text(CDataExchange *pDX, int id, CString &v)
{
	CWnd *w = Item(pDX, id);
	if (!w) return;
	if (pDX->m_bSaveAndValidate) v = w->m_text; else w->SetWindowText(v);
}
template <class T> static void DDX_Num(CDataExchange *pDX, int id, T &v, const char *fmt)
{
	CWnd *w = Item(pDX, id);
	if (!w) return;
	if (pDX->m_bSaveAndValidate) {
		const char *s = w->m_text;
		char *end;
		double d = strtod(s, &end);
		while (*end == ' ') end++;
		if (!*s || *end) {
			AfxMessageBox("Please enter a number.");
			w->SetFocus();
			pDX->Fail();
			return;
		}
		v = (T)d;
	} else {
		CString s;
		s.Format(fmt, v);
		w->SetWindowText(s);
	}
}
void DDX_Text(CDataExchange *pDX, int id, int &v) { DDX_Num(pDX, id, v, "%d"); }
void DDX_Text(CDataExchange *pDX, int id, UINT &v) { DDX_Num(pDX, id, v, "%u"); }
void DDX_Text(CDataExchange *pDX, int id, long &v) { DDX_Num(pDX, id, v, "%ld"); }
void DDX_Text(CDataExchange *pDX, int id, short &v) { int i = v; DDX_Num(pDX, id, i, "%d"); v = (short)i; }
void DDX_Text(CDataExchange *pDX, int id, BYTE &v) { int i = v; DDX_Num(pDX, id, i, "%d"); v = (BYTE)i; }
void DDX_Text(CDataExchange *pDX, int id, double &v) { DDX_Num(pDX, id, v, "%g"); }
void DDX_Check(CDataExchange *pDX, int id, int &v)
{
	CButton *b = (CButton *)Item(pDX, id);
	if (!b) return;
	if (pDX->m_bSaveAndValidate) v = b->GetCheck(); else b->SetCheck(v);
}
void DDX_Radio(CDataExchange *pDX, int id, int &v)
{
	CWnd *first = Item(pDX, id);
	if (!first) return;
	auto &kids = pDX->m_pDlgWnd->m_children;
	auto it = std::find(kids.begin(), kids.end(), first);
	if (pDX->m_bSaveAndValidate) v = -1;
	for (int n = 0; it != kids.end(); ++it, ++n) {
		CButton *b = dynamic_cast<CButton *>(*it);
		if (!b || (n > 0 && (b->m_style & WS_GROUP))) break;
		if (pDX->m_bSaveAndValidate) { if (b->GetCheck()) v = n; }
		else b->SetCheck(n == v);
	}
}
void DDX_CBIndex(CDataExchange *pDX, int id, int &v)
{
	CComboBox *c = (CComboBox *)Item(pDX, id);
	if (!c) return;
	if (pDX->m_bSaveAndValidate) v = c->GetCurSel(); else c->SetCurSel(v);
}
void DDX_LBIndex(CDataExchange *pDX, int id, int &v)
{
	CListBox *c = (CListBox *)Item(pDX, id);
	if (!c) return;
	if (pDX->m_bSaveAndValidate) v = c->GetCurSel(); else c->SetCurSel(v);
}
void DDX_CBString(CDataExchange *pDX, int id, CString &v)
{
	CComboBox *c = (CComboBox *)Item(pDX, id);
	if (!c) return;
	if (pDX->m_bSaveAndValidate) c->GetLBText(c->GetCurSel(), v); else c->SelectString(-1, v);
}
void DDX_LBString(CDataExchange *pDX, int id, CString &v)
{
	CListBox *c = (CListBox *)Item(pDX, id);
	if (!c) return;
	if (pDX->m_bSaveAndValidate) c->GetText(c->GetCurSel(), v); else c->SetCurSel(c->FindStringExact(-1, v));
}
void DDV_MaxChars(CDataExchange *pDX, CString const &v, int n)
{
	if (pDX->m_bDiscover) return;
	if (pDX->m_bSaveAndValidate && v.GetLength() > n) {
		CString s;
		s.Format("Enter no more than %d characters.", n);
		AfxMessageBox(s);
		pDX->Fail();
	}
}
void DDV_MinMaxInt(CDataExchange *pDX, int v, int lo, int hi)
{
	if (pDX->m_bDiscover) return;
	if (pDX->m_bSaveAndValidate && (v < lo || v > hi)) {
		CString s;
		s.Format("Please enter an integer between %d and %d.", lo, hi);
		AfxMessageBox(s);
		pDX->Fail();
	}
}
void DDV_MinMaxUInt(CDataExchange *pDX, UINT v, UINT lo, UINT hi) { DDV_MinMaxInt(pDX, (int)v, (int)lo, (int)hi); }

// ---------------------------------------------------------------- runtime dialogs
// template built in code; sizes in dialog units like the .rc ones
struct RuntimeDlg {
	DlgTemplate t;
	std::vector<DlgItem> items;
	std::vector<std::string> strs;
	RuntimeDlg(int cx, int cy, const char *cap) { strs.reserve(64); t = {0, cx, cy, S(cap), WS_POPUP | WS_CAPTION | WS_SYSMENU, nullptr, 0}; }
	const char *S(const char *s) { strs.push_back(s ? s : ""); return strs.back().c_str(); }
	void Add(const char *cls, const char *text, int id, int x, int y, int cx, int cy, DWORD style)
	{
		items.push_back({S(cls), S(text), id, x, y, cx, cy, WS_CHILD | WS_VISIBLE | style, 0});
		t.items = items.data();
		t.count = (int)items.size();
	}
};
class CMsgBoxDlg : public CDialog {
public:
	BOOL OnCommand(WPARAM wp, LPARAM lp) override
	{
		UINT id = LOWORD(wp);
		if (id >= IDOK && id <= IDNO) { EndDialog(id); return TRUE; }
		return FALSE;
	}
	void OnCancel() override
	{
		// no Cancel button (Yes/No box): Escape means No
		EndDialog(GetDlgItem(IDCANCEL) ? IDCANCEL : GetDlgItem(IDNO) ? IDNO : IDOK);
	}
};
int MessageBox(HWND, LPCTSTR text, LPCTSTR caption, UINT type)
{
	CDC dc;
	dc.m_font = 0;
	CRect r(0, 0, 330, 0);
	dc.DrawText(CString(text), &r, DT_CALCRECT | DT_WORDBREAK);
	int tw = (r.Width() + 5) * 4 / 6 + 1, th = (r.Height() + 5) * 8 / 13 + 1;
	int btn = type & 0xF;
	std::vector<std::pair<const char *, int>> b;
	if (btn == MB_YESNO) b = {{"&Yes", IDYES}, {"&No", IDNO}};
	else if (btn == MB_YESNOCANCEL) b = {{"&Yes", IDYES}, {"&No", IDNO}, {"Cancel", IDCANCEL}};
	else if (btn == MB_OKCANCEL) b = {{"OK", IDOK}, {"Cancel", IDCANCEL}};
	else b = {{"OK", IDOK}};
	int bw = (int)b.size() * 50 + ((int)b.size() - 1) * 6;
	int cx = max(tw + 14, bw + 14), cy = th + 7 + 7 + 14 + 7;
	RuntimeDlg rd(cx, cy, caption ? caption : "Decker");
	rd.Add("Static", text, IDC_STATIC, 7, 7, cx - 14, th, SS_LEFT);
	int x = (cx - bw) / 2;
	for (size_t i = 0; i < b.size(); i++, x += 56)
		rd.Add("Button", b[i].first, b[i].second, x, cy - 21, 50, 14, (i == 0 ? BS_DEFPUSHBUTTON : BS_PUSHBUTTON) | WS_TABSTOP);
	CMsgBoxDlg dlg;
	dlg.m_pTemplate = &rd.t;
	return (int)dlg.DoModal();
}
int AfxMessageBox(LPCTSTR text, UINT type, UINT) { return MessageBox(nullptr, text, "Decker", type); }
int AfxMessageBox(UINT id, UINT type, UINT h) { CString s; s.LoadString(id); return AfxMessageBox(s, type, h); }

// ---------------------------------------------------------------- file dialog
#include <dirent.h>
CFileDialog::CFileDialog(BOOL open, LPCTSTR ext, LPCTSTR name, DWORD, LPCTSTR, CWnd *parent)
	: CDialog(0, parent), m_bOpen(open), m_ext(ext ? ext : ""), m_path(name ? name : "") {}
CString CFileDialog::GetFileName() const
{
	int i = m_path.ReverseFind('/');
	return i < 0 ? m_path : m_path.Mid(i + 1);
}
class CFileDlgImpl : public CDialog {
public:
	CFileDialog *m_owner;
	CListBox *m_list = nullptr;
	CEdit *m_edit = nullptr;
	BOOL OnInitDialog() override
	{
		CDialog::OnInitDialog();
		m_list = (CListBox *)GetDlgItem(100);
		m_edit = (CEdit *)GetDlgItem(101);
		std::vector<std::string> names;
		if (DIR *d = opendir(shim::g_saveDir.c_str())) {
			while (dirent *e = readdir(d)) {
				std::string n = e->d_name;
				std::string ext = std::string(".") + (const char *)m_owner->m_ext;
				if (n.size() > ext.size() && strcasecmp(n.c_str() + n.size() - ext.size(), ext.c_str()) == 0) names.push_back(n.substr(0, n.size() - ext.size()));
			}
			closedir(d);
		}
		std::sort(names.begin(), names.end());
		for (auto &n : names) m_list->AddString(n.c_str());
		if (!names.empty()) { m_list->SetCurSel(0); m_edit->SetWindowText(names[0].c_str()); }
		else if (!m_owner->m_bOpen) m_edit->SetWindowText("decker");
		return TRUE;
	}
	BOOL OnCommand(WPARAM wp, LPARAM lp) override
	{
		UINT id = LOWORD(wp), code = HIWORD(wp);
		if (id == 100 && (code == LBN_SELCHANGE || code == LBN_DBLCLK)) {
			CString s;
			m_list->GetText(m_list->GetCurSel(), s);
			m_edit->SetWindowText(s);
			if (code == LBN_DBLCLK) OnOK();
			return TRUE;
		}
		return CDialog::OnCommand(wp, lp);
	}
	void OnOK() override
	{
		CString n = m_edit->m_text;
		n.TrimLeft(); n.TrimRight();
		if (n.IsEmpty()) return;
		for (const char *bad = "/\\:"; *bad; bad++) n.Remove(*bad);
		CString path = CString(shim::g_saveDir.c_str()) + "/" + n + "." + m_owner->m_ext;
		FILE *f = fopen(path, "rb");
		if (f) fclose(f);
		if (m_owner->m_bOpen && !f) { AfxMessageBox("There is no saved game with that name."); return; }
		if (!m_owner->m_bOpen && f && AfxMessageBox(n + " already exists.\nDo you want to replace it?", MB_YESNO) != IDYES) return;
		m_owner->m_path = path;
		EndDialog(IDOK);
	}
};
INT_PTR CFileDialog::DoModal()
{
	RuntimeDlg rd(160, 150, m_bOpen ? "Load Game" : "Save Game");
	rd.Add("Static", "Saved games:", IDC_STATIC, 7, 7, 146, 8, SS_LEFT);
	rd.Add("ListBox", "", 100, 7, 17, 146, 86, WS_BORDER | WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY);
	rd.Add("Static", "Name:", IDC_STATIC, 7, 108, 30, 8, SS_LEFT);
	rd.Add("Edit", "", 101, 36, 106, 117, 12, WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL);
	rd.Add("Button", m_bOpen ? "Load" : "Save", IDOK, 43, 129, 50, 14, BS_DEFPUSHBUTTON | WS_TABSTOP);
	rd.Add("Button", "Cancel", IDCANCEL, 103, 129, 50, 14, BS_PUSHBUTTON | WS_TABSTOP);
	CFileDlgImpl dlg;
	dlg.m_owner = this;
	dlg.m_pTemplate = &rd.t;
	return dlg.DoModal();
}

// ---------------------------------------------------------------- popup menus
class CPopupWnd : public CWnd {
public:
	CMenu *m_menu;
	CWnd *m_owner;
	int m_hot = -1;
	BOOL m_done = FALSE;
	UINT m_result = 0;
	int ItemH() const { return shim::FontHeight(0) + 5; }
	void ShimPaint(CDC &dc) override
	{
		CRect r;
		GetClientRect(&r);
		dc.FillSolidRect(&r, COLOR_FACE);
		shim::DrawFrameCtl(dc, r, FALSE);
		for (int i = 0; i < (int)m_menu->m_items.size(); i++) {
			auto &it = m_menu->m_items[i];
			int y = 3 + i * ItemH();
			if (it.id == 0) { // group header
				dc.m_font = 1;
				shim::DrawText(dc, 1, 6, y + 2, it.text, it.text.GetLength(), COLOR_SHADOW);
				dc.m_font = 0;
				continue;
			}
			BOOL hot = i == m_hot;
			if (hot) dc.FillSolidRect(3, y, r.Width() - 6, ItemH(), COLOR_SELBG);
			CString t = it.text, key;
			int tab = t.Find('\t');
			if (tab >= 0) { key = t.Mid(tab + 1); t = t.Left(tab); }
			shim::DrawText(dc, 0, 16, y + 2, t, t.GetLength(), hot ? 0xFFFFFF : 0);
			if (!key.IsEmpty()) shim::DrawText(dc, 0, r.Width() - 10 - shim::TextWidth(0, key, key.GetLength()), y + 2, key, key.GetLength(), hot ? 0xFFFFFF : 0);
		}
	}
	int Hit(CPoint pt) const
	{
		int i = (pt.y - 3) / ItemH();
		if (pt.x < 0 || pt.x >= m_rect.Width() || pt.y < 3 || i >= (int)m_menu->m_items.size()) return -1;
		return m_menu->m_items[i].id ? i : -1;
	}
	BOOL ShimMouse(UINT msg, CPoint pt) override
	{
		int h = Hit(pt);
		if (h != m_hot) { m_hot = h; Invalidate(); }
		if (msg == WM_LBUTTONUP && h >= 0) { m_result = m_menu->m_items[h].id; m_done = TRUE; }
		return TRUE;
	}
	void Move(int d)
	{
		int n = (int)m_menu->m_items.size();
		for (int k = 0; k < n; k++) {
			m_hot = (m_hot + d + n) % n;
			if (m_hot < 0) m_hot = 0;
			if (m_menu->m_items[m_hot].id) break;
		}
		Invalidate();
	}
	BOOL ShimKey(UINT vk, BOOL down) override
	{
		if (!down) return TRUE;
		if (vk == VK_DOWN || vk == VK_NUMPAD0 + 2) Move(1);
		else if (vk == VK_UP || vk == VK_NUMPAD0 + 8) Move(-1);
		else if (vk == VK_ESCAPE || vk == VK_NUMPAD0 || vk == 0x6E /* keypad . */) m_done = TRUE;
		else if ((vk == VK_RETURN || vk == VK_NUMPAD0 + 5 || vk == VK_SPACE) && m_hot >= 0) { m_result = m_menu->m_items[m_hot].id; m_done = TRUE; }
		return TRUE;
	}
	BOOL ShimChar(UINT ch) override
	{
		// the key shown after a tab selects the entry, as in the game itself
		for (auto &it : m_menu->m_items) {
			int tab = it.text.Find('\t');
			if (it.id && tab >= 0 && it.text.GetLength() == tab + 2 && toupper((unsigned char)it.text[tab + 1]) == toupper((int)ch)) { m_result = it.id; m_done = TRUE; return TRUE; }
		}
		return TRUE;
	}
};
BOOL CMenu::LoadMenu(UINT id)
{
	m_sub = new CMenu;
	for (const MenuRes *m = g_menuRes; m->text; m++)
		if ((UINT)m->menu == id) m_sub->m_items.push_back({m->text, (UINT)m->id});
	return TRUE;
}
BOOL CMenu::TrackPopupMenu(UINT flags, int x, int y, CWnd *owner, LPCRECT)
{
	CPopupWnd p;
	p.m_menu = this;
	p.m_owner = owner;
	int w = 0;
	for (auto &it : m_items) {
		CString t = it.text;
		t.Replace("\t", "    ");
		w = max(w, shim::TextWidth(it.id ? 0 : 1, t, t.GetLength()));
	}
	w += 34;
	int h = (int)m_items.size() * p.ItemH() + 6;
	if (h > 480) h = 480; // ponytail: no scrolling; the longest menu fits
	x = max(0, min(x, 640 - w));
	y = max(0, min(y, 480 - h));
	p.CreateEx(0, nullptr, nullptr, WS_POPUP, x, y, w, h, nullptr, nullptr);
	p.m_font = 0;
	p.ShowWindow(SW_SHOW);
	CWnd *oldFocus = g_focus;
	p.SetFocus();
	g_modal.push_back(&p);
	CWnd *oldCap = g_capture;
	g_capture = nullptr;
	g_clickOutside = FALSE;
	while (!p.m_done && !g_quit) {
		shim::PaintDirty();
		shim::Present();
		shim::PumpEvents(TRUE);
		if (g_clickOutside) { g_clickOutside = FALSE; p.m_done = TRUE; }
	}
	g_modal.pop_back();
	g_capture = oldCap;
	p.DestroyWindow();
	for (CWnd *t : g_topLevel) t->Invalidate();
	if (oldFocus && Alive(oldFocus)) g_focus = oldFocus;
	if (flags & TPM_RETURNCMD) return p.m_result;
	if (p.m_result && owner && owner->m_hWnd) owner->SendMessage(WM_COMMAND, p.m_result, 0);
	return p.m_result != 0;
}
BOOL GetCursorPos(LPPOINT p) { p->x = shim::g_mouseX; p->y = shim::g_mouseY; return TRUE; }
BOOL IsWindowEnabled(HWND h) { return h && h->IsWindowEnabled(); }
BOOL EnableWindow(HWND h, BOOL e) { return h ? h->EnableWindow(e) : FALSE; }
BOOL IsWindowVisible(HWND h) { return h && h->IsWindowVisible(); }
