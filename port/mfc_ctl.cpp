// MFC shim: common controls drawn in the Windows 98 style.
#include "afxwin.h"
#undef min
#undef max
#include <algorithm>

namespace shim {
extern std::vector<CWnd *> g_modal;
extern BOOL g_clickOutside;
void DispatchPosted();

static CString NoAmp(const CString &s)
{
	CString r;
	for (int i = 0; i < s.GetLength(); i++) {
		if (s[i] == '&') { if (i + 1 < s.GetLength() && s[i + 1] == '&') { r += '&'; i++; } continue; }
		r += s[i];
	}
	return r;
}
static void Text(CDC &dc, int x, int y, const CString &s, COLORREF c, BOOL disabled = FALSE)
{
	if (disabled) {
		DrawText(dc, dc.m_font, x + 1, y + 1, s, s.GetLength(), COLOR_HILITE);
		c = COLOR_SHADOW;
	}
	DrawText(dc, dc.m_font, x, y, s, s.GetLength(), c);
}
static void FocusRect(CDC &dc, CRect r)
{
	for (int x = r.left; x < r.right; x += 2) { dc.SetPixel(x, r.top, 0); dc.SetPixel(x, r.bottom - 1, 0); }
	for (int y = r.top; y < r.bottom; y += 2) { dc.SetPixel(r.left, y, 0); dc.SetPixel(r.right - 1, y, 0); }
}
static void Sunken(CDC &dc, CRect r) { DrawFrameCtl(dc, r, TRUE); }

// ---------------------------------------------------------------- scroll bars
struct SB { CRect r; BOOL vert; int pos, page, total; };
static int SBLen(const SB &s) { return s.vert ? s.r.Height() : s.r.Width(); }
static void SBThumb(const SB &s, int &t0, int &t1)
{
	int track = SBLen(s) - 32;
	int max = s.total - s.page;
	int len = s.total > 0 ? std::max(8, track * s.page / s.total) : track;
	if (len > track) len = track;
	t0 = 16 + (max > 0 ? (track - len) * s.pos / max : 0);
	t1 = t0 + len;
}
static void DrawSB(CDC &dc, const SB &s)
{
	CRect r = s.r;
	for (int y = r.top; y < r.bottom; y++)
		for (int x = r.left; x < r.right; x++) dc.SetPixel(x, y, ((x + y) & 1) ? COLOR_HILITE : COLOR_FACE);
	CRect a = s.vert ? CRect(r.left, r.top, r.right, r.top + 16) : CRect(r.left, r.top, r.left + 16, r.bottom);
	CRect b = s.vert ? CRect(r.left, r.bottom - 16, r.right, r.bottom) : CRect(r.right - 16, r.top, r.right, r.bottom);
	BOOL active = s.total > s.page;
	for (int k = 0; k < 2; k++) {
		CRect c = k ? b : a;
		dc.FillSolidRect(&c, COLOR_FACE);
		DrawFrameCtl(dc, c, FALSE);
		DrawArrow(dc, c, s.vert ? (k ? 2 : 0) : (k ? 1 : 3), active ? 0 : COLOR_SHADOW);
	}
	if (!active) return;
	int t0, t1;
	SBThumb(s, t0, t1);
	CRect t = s.vert ? CRect(r.left, r.top + t0, r.right, r.top + t1) : CRect(r.left + t0, r.top, r.left + t1, r.bottom);
	dc.FillSolidRect(&t, COLOR_FACE);
	DrawFrameCtl(dc, t, FALSE);
}
// click in the bar: returns the new position; *drag set when the thumb was hit
static int ClickSB(const SB &s, CPoint pt, int line, int *drag)
{
	int p = s.vert ? pt.y - s.r.top : pt.x - s.r.left, len = SBLen(s), max = std::max(0, s.total - s.page);
	int t0, t1;
	SBThumb(s, t0, t1);
	int np = s.pos;
	if (p < 16) np -= line;
	else if (p >= len - 16) np += line;
	else if (p < t0) np -= s.page;
	else if (p >= t1) np += s.page;
	else if (drag) *drag = p - t0;
	return std::max(0, std::min(max, np));
}
static int DragSB(const SB &s, CPoint pt, int off)
{
	int p = (s.vert ? pt.y - s.r.top : pt.x - s.r.left) - off;
	int t0, t1;
	SBThumb(s, t0, t1);
	int track = SBLen(s) - 32 - (t1 - t0), max = std::max(0, s.total - s.page);
	if (track <= 0) return 0;
	return std::max(0, std::min(max, (p - 16) * max / track));
}
}
using namespace shim;

// ---------------------------------------------------------------- buttons
BOOL CButton::Create(LPCTSTR text, DWORD style, const RECT &r, CWnd *parent, UINT id)
{
	return CWnd::Create(nullptr, text, style, r, parent, id);
}
BOOL CButton::ShimWantsFocus() const
{
	int t = Type();
	return t != BS_GROUPBOX && t != BS_OWNERDRAW && IsWindowEnabled();
}
void CButton::Click()
{
	int t = Type();
	if (t == BS_AUTOCHECKBOX) SetCheck(!m_check);
	else if (t == BS_AUTO3STATE) SetCheck((m_check + 1) % 3);
	else if (t == BS_AUTORADIOBUTTON && m_parent) {
		// uncheck the rest of the group
		auto &k = m_parent->m_children;
		auto me = std::find(k.begin(), k.end(), this);
		auto first = me;
		while (first != k.begin() && !((*first)->m_style & WS_GROUP)) --first;
		for (auto it = first; it != k.end(); ++it) {
			if (it != first && ((*it)->m_style & WS_GROUP)) break;
			CButton *b = dynamic_cast<CButton *>(*it);
			if (b && b->Type() == BS_AUTORADIOBUTTON) b->SetCheck(b == this);
		}
	}
	if (m_parent) m_parent->SendMessage(WM_COMMAND, MAKELONG(m_nID, BN_CLICKED), (LPARAM)this);
}
BOOL CButton::ShimMouse(UINT msg, CPoint pt)
{
	if (Type() == BS_GROUPBOX) return FALSE;
	CRect r;
	GetClientRect(&r);
	if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK) { m_pressed = TRUE; RedrawWindow(); }
	else if (msg == WM_MOUSEMOVE && g_capture == this) {
		BOOL in = r.PtInRect(pt);
		if (in != m_pressed) { m_pressed = in; Invalidate(); }
	} else if (msg == WM_LBUTTONUP) {
		BOOL was = m_pressed && r.PtInRect(pt);
		m_pressed = FALSE;
		RedrawWindow();
		if (was) Click();
	}
	return TRUE;
}
BOOL CButton::ShimKey(UINT vk, BOOL down)
{
	if (vk == VK_SPACE && down) { Click(); return TRUE; }
	int t = Type();
	if (down && (t == BS_AUTORADIOBUTTON) && (vk == VK_DOWN || vk == VK_RIGHT || vk == VK_UP || vk == VK_LEFT) && m_parent) {
		auto &k = m_parent->m_children;
		auto me = std::find(k.begin(), k.end(), this);
		auto first = me, last = me;
		while (first != k.begin() && !((*first)->m_style & WS_GROUP)) --first;
		while (last + 1 != k.end() && !((*(last + 1))->m_style & WS_GROUP) && dynamic_cast<CButton *>(*(last + 1))) ++last;
		auto nx = (vk == VK_DOWN || vk == VK_RIGHT) ? (me == last ? first : me + 1) : (me == first ? last : me - 1);
		if (CButton *b = dynamic_cast<CButton *>(*nx)) { b->SetFocus(); b->Click(); }
		return TRUE;
	}
	return FALSE;
}
void CButton::ShimPaint(CDC &dc)
{
	CRect r;
	GetClientRect(&r);
	int t = Type();
	BOOL en = IsWindowEnabled();
	BOOL focus = g_focus == this;
	CString txt = NoAmp(m_text);
	int fh = FontHeight(dc.m_font);
	if (t == BS_OWNERDRAW) {
		DRAWITEMSTRUCT dis = {ODT_BUTTON, m_nID, 0, ODA_DRAWENTIRE, 0, this, &dc, r};
		if (m_pressed) dis.itemState |= ODS_SELECTED;
		if (focus) dis.itemState |= ODS_FOCUS;
		if (!en) dis.itemState |= ODS_DISABLED;
		if (m_parent) m_parent->SendMessage(WM_DRAWITEM, m_nID, (LPARAM)&dis);
		return;
	}
	if (t == BS_GROUPBOX) {
		int y = fh / 2;
		CRect f(r.left, y, r.right, r.bottom);
		dc.Draw3dRect(f.left, f.top, f.Width() - 1, f.Height() - 1, COLOR_SHADOW, COLOR_SHADOW);
		dc.Draw3dRect(f.left + 1, f.top + 1, f.Width() - 1, f.Height() - 1, COLOR_HILITE, COLOR_HILITE);
		if (!txt.IsEmpty()) {
			int tw = TextWidth(dc.m_font, txt, txt.GetLength());
			dc.FillSolidRect(8, 0, tw + 4, fh, COLOR_FACE);
			Text(dc, 10, 0, txt, 0, !en);
		}
		return;
	}
	dc.FillSolidRect(&r, COLOR_FACE);
	if (t == BS_CHECKBOX || t == BS_AUTOCHECKBOX || t == BS_3STATE || t == BS_AUTO3STATE || t == BS_RADIOBUTTON || t == BS_AUTORADIOBUTTON) {
		BOOL radio = t == BS_RADIOBUTTON || t == BS_AUTORADIOBUTTON;
		int by = (r.Height() - 13) / 2;
		CRect b(0, by, 13, by + 13);
		if (radio) {
			static const char *ring[12] = {"....aaaa....", "..aabbbbaa..", ".abcccccced.", ".abcccccced.", "abcccccccced", "abcccccccced",
			                               "abcccccccced", "abcccccccced", ".bcccccccce.", ".acccccccce.", "..ddcccced..", "....eeee...."};
			for (int y = 0; y < 12; y++)
				for (int x = 0; x < 12; x++) {
					char c = ring[y][x];
					if (c == '.') continue;
					COLORREF col = c == 'a' ? COLOR_SHADOW : c == 'b' ? COLOR_DKSHADOW : c == 'c' ? (en ? COLOR_HILITE : COLOR_FACE) : c == 'd' ? COLOR_LIGHT : COLOR_HILITE;
					dc.SetPixel(b.left + x, b.top + y, col);
				}
			if (m_check) dc.FillSolidRect(b.left + 4, b.top + 4, 4, 4, en ? 0 : COLOR_SHADOW), dc.FillSolidRect(b.left + 5, b.top + 3, 2, 6, en ? 0 : COLOR_SHADOW), dc.FillSolidRect(b.left + 3, b.top + 5, 6, 2, en ? 0 : COLOR_SHADOW);
		} else {
			dc.FillSolidRect(&b, en && !m_pressed ? COLOR_HILITE : COLOR_FACE);
			Sunken(dc, b);
			if (m_check)
				for (int i = 0; i < 7; i++) {
					int x = b.left + 3 + i, y = b.top + (i < 3 ? 5 + i : 9 - i) ;
					dc.FillSolidRect(x, y, 1, 3, en ? 0 : COLOR_SHADOW);
				}
		}
		int tx = 17, ty = (r.Height() - fh) / 2;
		Text(dc, tx, ty, txt, 0, !en);
		if (focus) FocusRect(dc, CRect(tx - 1, ty - 1, tx + TextWidth(dc.m_font, txt, txt.GetLength()) + 1, ty + fh + 1));
		return;
	}
	// push buttons
	CRect f = r;
	BOOL def = t == BS_DEFPUSHBUTTON || (focus && t == BS_PUSHBUTTON);
	if (def) { dc.Draw3dRect(f, 0, 0); f.DeflateRect(1, 1); }
	if (m_pressed) { dc.Draw3dRect(f, COLOR_SHADOW, COLOR_SHADOW); }
	else DrawFrameCtl(dc, f, FALSE);
	int off = m_pressed ? 1 : 0;
	if (m_bmp) {
		dc.blit(m_bmp, 0, 0, (r.Width() - m_bmp->w) / 2 + off, (r.Height() - m_bmp->h) / 2 + off, m_bmp->w, m_bmp->h, SRCCOPY);
	} else {
		int tw = TextWidth(dc.m_font, txt, txt.GetLength());
		Text(dc, (r.Width() - tw) / 2 + off, (r.Height() - fh) / 2 + off, txt, 0, !en);
	}
	if (focus) FocusRect(dc, CRect(4, 4, r.right - 4, r.bottom - 4));
}
BOOL CBitmapButton::LoadBitmaps(UINT a, UINT b, UINT c, UINT d)
{
	m_bitmap.LoadBitmap(a);
	if (b) m_bitmapSel.LoadBitmap(b);
	if (c) m_bitmapFocus.LoadBitmap(c);
	if (d) m_bitmapDisabled.LoadBitmap(d);
	return m_bitmap.m_hObject != nullptr;
}
void CBitmapButton::SizeToContent()
{
	if (HBITMAP h = (HBITMAP)m_bitmap) MoveWindow(m_rect.left, m_rect.top, h->w, h->h);
}
void CBitmapButton::DrawItem(LPDRAWITEMSTRUCT dis)
{
	CBitmap *b = &m_bitmap;
	if ((dis->itemState & ODS_SELECTED) && m_bitmapSel.m_hObject) b = &m_bitmapSel;
	else if ((dis->itemState & ODS_FOCUS) && m_bitmapFocus.m_hObject) b = &m_bitmapFocus;
	else if ((dis->itemState & ODS_DISABLED) && m_bitmapDisabled.m_hObject) b = &m_bitmapDisabled;
	HBITMAP h = (HBITMAP)*b;
	if (h) dis->hDC->blit(h, 0, 0, 0, 0, dis->rcItem.right, dis->rcItem.bottom, SRCCOPY);
}

// ---------------------------------------------------------------- static
BOOL CStatic::Create(LPCTSTR text, DWORD style, const RECT &r, CWnd *parent, UINT id)
{
	return CWnd::Create(nullptr, text, style, r, parent, id);
}
void CStatic::ShimPaint(CDC &dc)
{
	CRect r;
	GetClientRect(&r);
	int type = m_style & SS_TYPEMASK;
	if (type == SS_BLACKFRAME) { CBrush b(0); dc.FrameRect(&r, &b); return; }
	if (type == SS_ETCHEDFRAME) { dc.Draw3dRect(r, COLOR_SHADOW, COLOR_HILITE); return; }
	dc.FillSolidRect(&r, COLOR_FACE);
	if (m_style & SS_SUNKEN) { dc.Draw3dRect(r, COLOR_SHADOW, COLOR_HILITE); r.DeflateRect(1, 1); }
	if (type == SS_BITMAP) {
		if (m_bmp) dc.blit(m_bmp, 0, 0, 0, 0, m_bmp->w, m_bmp->h, SRCCOPY);
		return;
	}
	if (type == SS_BLACKRECT) { dc.FillSolidRect(&r, 0); return; }
	UINT fmt = type == SS_CENTER ? DT_CENTER : type == SS_RIGHT ? DT_RIGHT : DT_LEFT;
	if (type != SS_LEFTNOWORDWRAP) fmt |= DT_WORDBREAK;
	if (m_style & SS_CENTERIMAGE) fmt |= DT_VCENTER | DT_SINGLELINE;
	dc.SetBkMode(TRANSPARENT);
	CString t = NoAmp(m_text);
	if (!IsWindowEnabled()) {
		CRect r2 = r;
		r2.OffsetRect(1, 1);
		dc.SetTextColor(COLOR_HILITE);
		dc.DrawText(t, &r2, fmt);
		dc.SetTextColor(COLOR_SHADOW);
	} else
		dc.SetTextColor(0);
	dc.DrawText(t, &r, fmt);
}

// ---------------------------------------------------------------- edit
void CEdit::ReplaceSel(LPCTSTR s, BOOL)
{
	if (m_sel) { m_text.Empty(); m_caret = 0; m_sel = FALSE; }
	m_text.Insert(m_caret, s);
	m_caret += (int)strlen(s);
	Invalidate();
	if (m_parent) m_parent->SendMessage(WM_COMMAND, MAKELONG(m_nID, EN_CHANGE), (LPARAM)this);
}
void CEdit::ShimPaint(CDC &dc)
{
	CRect r;
	GetClientRect(&r);
	BOOL ro = (m_style & ES_READONLY) || !IsWindowEnabled();
	dc.FillSolidRect(&r, ro ? COLOR_FACE : COLOR_HILITE);
	if (m_style & WS_BORDER) Sunken(dc, r);
	int fh = FontHeight(dc.m_font);
	int y = (r.Height() - fh) / 2, x = 3;
	if (m_caret > m_text.GetLength()) m_caret = m_text.GetLength();
	int cw = TextWidth(dc.m_font, m_text, m_caret);
	int scroll = std::max(0, cw - (r.Width() - 8));
	CRect save = dc.m_clip;
	dc.m_clip.left = std::max(dc.m_clip.left, dc.m_ox + 2);
	dc.m_clip.right = std::min(dc.m_clip.right, dc.m_ox + (int)r.right - 2);
	BOOL focus = g_focus == this;
	if (focus && m_sel && !m_text.IsEmpty()) {
		dc.FillSolidRect(x - scroll, y, TextWidth(dc.m_font, m_text, m_text.GetLength()), fh, COLOR_SELBG);
		Text(dc, x - scroll, y, m_text, 0xFFFFFF);
	} else
		Text(dc, x - scroll, y, m_text, IsWindowEnabled() ? 0 : COLOR_SHADOW);
	if (focus && !m_sel) dc.FillSolidRect(x - scroll + cw, y, 1, fh, 0);
	dc.m_clip = save;
}
BOOL CEdit::ShimMouse(UINT msg, CPoint pt)
{
	if (msg == WM_LBUTTONDOWN) {
		m_sel = FALSE;
		int x = 3;
		m_caret = 0;
		while (m_caret < m_text.GetLength() && x + TextWidth(m_font, m_text.s.c_str() + m_caret, 1) / 2 < pt.x) x += TextWidth(m_font, m_text.s.c_str() + m_caret++, 1);
		Invalidate();
	}
	return TRUE;
}
BOOL CEdit::ShimKey(UINT vk, BOOL down)
{
	if (!down || (m_style & ES_READONLY)) return FALSE;
	int n = m_text.GetLength();
	switch (vk) {
	case VK_LEFT: m_sel = FALSE; if (m_caret > 0) m_caret--; break;
	case VK_RIGHT: m_sel = FALSE; if (m_caret < n) m_caret++; break;
	case VK_HOME: m_sel = FALSE; m_caret = 0; break;
	case VK_END: m_sel = FALSE; m_caret = n; break;
	case VK_BACK:
		if (m_sel) { m_text.Empty(); m_caret = 0; m_sel = FALSE; }
		else if (m_caret > 0) m_text.Delete(--m_caret);
		if (m_parent) m_parent->SendMessage(WM_COMMAND, MAKELONG(m_nID, EN_CHANGE), (LPARAM)this);
		break;
	case VK_DELETE:
		if (m_sel) { m_text.Empty(); m_caret = 0; m_sel = FALSE; }
		else if (m_caret < n) m_text.Delete(m_caret);
		if (m_parent) m_parent->SendMessage(WM_COMMAND, MAKELONG(m_nID, EN_CHANGE), (LPARAM)this);
		break;
	default: return FALSE;
	}
	Invalidate();
	return TRUE;
}
BOOL CEdit::ShimChar(UINT ch)
{
	if ((m_style & ES_READONLY) || ch < 32 || ch > 255 || ch == 127) return FALSE;
	if ((m_style & ES_NUMBER) && !isdigit(ch)) return TRUE;
	if (m_style & ES_UPPERCASE) ch = toupper(ch);
	if (m_limit && !m_sel && m_text.GetLength() >= m_limit) return TRUE;
	char s[2] = {(char)ch, 0};
	ReplaceSel(s);
	return TRUE;
}

// ---------------------------------------------------------------- list box
static int ItemH() { return FontHeight(0) + 1; }
int CListBox::AddString(LPCTSTR s)
{
	if (m_style & LBS_SORT) {
		int i = 0;
		while (i < GetCount() && strcasecmp(m_items[i].text, s) <= 0) i++;
		return InsertString(i, s);
	}
	m_items.push_back({s, 0});
	Invalidate();
	return GetCount() - 1;
}
int CListBox::InsertString(int i, LPCTSTR s)
{
	if (i < 0 || i > GetCount()) i = GetCount();
	m_items.insert(m_items.begin() + i, Item{s, 0});
	if (m_sel >= i) m_sel++;
	Invalidate();
	return i;
}
int CListBox::DeleteString(UINT i)
{
	if ((int)i >= GetCount()) return LB_ERR;
	m_items.erase(m_items.begin() + i);
	if (m_sel == (int)i) m_sel = -1; else if (m_sel > (int)i) m_sel--;
	Invalidate();
	return GetCount();
}
int CListBox::SetCurSel(int i)
{
	m_sel = (i >= 0 && i < GetCount()) ? i : -1;
	if (m_sel >= 0) {
		int v = VisibleRows();
		if (m_sel < m_top) m_top = m_sel;
		else if (v > 0 && m_sel >= m_top + v) m_top = m_sel - v + 1;
	}
	Invalidate();
	return m_sel;
}
int CListBox::FindStringExact(int start, LPCTSTR s) const
{
	for (int i = 0; i < GetCount(); i++) if (!strcasecmp(m_items[(start + 1 + i) % GetCount()].text, s)) return (start + 1 + i) % GetCount();
	return LB_ERR;
}
int CListBox::VisibleRows() const { return std::max(1, ((int)m_rect.Height() - 4) / ItemH()); }
void CListBox::Notify(UINT code)
{
	if (m_parent && (m_style & LBS_NOTIFY || dynamic_cast<CComboBox *>(this))) m_parent->SendMessage(WM_COMMAND, MAKELONG(m_nID, code), (LPARAM)this);
}
void CListBox::ShimPaint(CDC &dc)
{
	CRect r;
	GetClientRect(&r);
	dc.FillSolidRect(&r, IsWindowEnabled() ? COLOR_HILITE : COLOR_FACE);
	Sunken(dc, r);
	int v = VisibleRows();
	BOOL bar = GetCount() > v;
	int w = r.Width() - 4 - (bar ? 16 : 0);
	CRect save = dc.m_clip;
	dc.m_clip.right = std::min(dc.m_clip.right, dc.m_ox + 2 + w);
	for (int i = m_top; i < GetCount() && i < m_top + v; i++) {
		int y = 2 + (i - m_top) * ItemH();
		BOOL sel = i == m_sel;
		if (sel) dc.FillSolidRect(2, y, w, ItemH(), g_focus == this ? COLOR_SELBG : COLOR_FACE);
		Text(dc, 4, y, m_items[i].text, sel && g_focus == this ? 0xFFFFFF : 0);
	}
	dc.m_clip = save;
	if (bar) DrawSB(dc, {CRect(r.right - 18, 2, r.right - 2, r.bottom - 2), TRUE, m_top, v, GetCount()});
}
BOOL CListBox::ShimMouse(UINT msg, CPoint pt)
{
	CRect r;
	GetClientRect(&r);
	int v = VisibleRows();
	if (GetCount() > v && pt.x >= r.right - 18) {
		if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK) { m_top = ClickSB({CRect(r.right - 18, 2, r.right - 2, r.bottom - 2), TRUE, m_top, v, GetCount()}, pt, 1, nullptr); Invalidate(); }
		return TRUE;
	}
	if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK) {
		int i = m_top + (pt.y - 2) / ItemH();
		if (i >= 0 && i < GetCount()) {
			BOOL ch = i != m_sel;
			SetCurSel(i);
			if (ch) Notify(LBN_SELCHANGE);
			if (msg == WM_LBUTTONDBLCLK) Notify(LBN_DBLCLK);
		}
	}
	return TRUE;
}
BOOL CListBox::ShimKey(UINT vk, BOOL down)
{
	if (!down || !GetCount()) return FALSE;
	int i = m_sel, v = VisibleRows();
	switch (vk) {
	case VK_UP: i = std::max(0, i - 1); break;
	case VK_DOWN: i = std::min(GetCount() - 1, i + 1); break;
	case VK_PRIOR: i = std::max(0, i - v); break;
	case VK_NEXT: i = std::min(GetCount() - 1, i + v); break;
	case VK_HOME: i = 0; break;
	case VK_END: i = GetCount() - 1; break;
	default: return FALSE;
	}
	if (i != m_sel) { SetCurSel(i); Notify(LBN_SELCHANGE); }
	return TRUE;
}
BOOL CListBox::ShimWheel(int dy)
{
	m_top = std::max(0, std::min(GetCount() - VisibleRows(), m_top - dy * 3));
	if (m_top < 0) m_top = 0;
	Invalidate();
	return TRUE;
}

// ---------------------------------------------------------------- combo box (drop-down list)
class CComboDrop : public CListBox {
public:
	CComboBox *m_combo;
	BOOL m_done = FALSE;
	BOOL ShimMouse(UINT msg, CPoint pt) override
	{
		CRect r;
		GetClientRect(&r);
		int v = VisibleRows();
		if (GetCount() > v && pt.x >= r.right - 18) return CListBox::ShimMouse(msg, pt);
		int i = m_top + (pt.y - 2) / ItemH();
		if (i >= 0 && i < GetCount() && i != m_sel) { m_sel = i; Invalidate(); }
		if (msg == WM_LBUTTONUP && r.PtInRect(pt)) m_done = TRUE;
		return TRUE;
	}
	BOOL ShimKey(UINT vk, BOOL down) override
	{
		if (down && (vk == VK_RETURN || vk == VK_ESCAPE)) { if (vk == VK_ESCAPE) m_sel = -2; m_done = TRUE; return TRUE; }
		return CListBox::ShimKey(vk, down);
	}
	void ShimPaint(CDC &dc) override
	{
		CWnd *f = g_focus;
		g_focus = this; // draw the selection highlighted
		CListBox::ShimPaint(dc);
		g_focus = f;
		CRect r;
		GetClientRect(&r);
		CBrush b(0);
		dc.FrameRect(&r, &b);
	}
};
static const int COMBO_H = 21;
void CComboBox::ShimPaint(CDC &dc)
{
	CRect r(0, 0, m_rect.Width(), COMBO_H);
	BOOL en = IsWindowEnabled();
	dc.FillSolidRect(&r, en ? COLOR_HILITE : COLOR_FACE);
	Sunken(dc, r);
	CRect b(r.right - 18, 2, r.right - 2, r.bottom - 2);
	dc.FillSolidRect(&b, COLOR_FACE);
	DrawFrameCtl(dc, b, FALSE);
	DrawArrow(dc, b, 2, en ? 0 : COLOR_SHADOW);
	if (m_sel >= 0) {
		int y = (COMBO_H - FontHeight(dc.m_font)) / 2;
		BOOL focus = g_focus == this;
		if (focus) dc.FillSolidRect(3, 3, b.left - 4, COMBO_H - 6, COLOR_SELBG);
		Text(dc, 5, y, m_items[m_sel].text, focus ? 0xFFFFFF : en ? 0 : COLOR_SHADOW);
	}
}
BOOL CComboBox::ShimMouse(UINT msg, CPoint)
{
	if (msg != WM_LBUTTONDOWN && msg != WM_LBUTTONDBLCLK) return TRUE;
	if (!GetCount()) return TRUE;
	CComboDrop d;
	d.m_combo = this;
	d.m_items = m_items;
	d.m_sel = m_sel;
	d.m_style = 0;
	int rows = std::min(GetCount(), 10);
	CPoint o = ScreenOrigin();
	int h = rows * ItemH() + 4, y = o.y + COMBO_H;
	if (y + h > 480) y = o.y - h;
	d.CreateEx(0, nullptr, nullptr, WS_POPUP, o.x, y, m_rect.Width(), h, nullptr, nullptr);
	d.m_font = 0;
	d.m_style = 0;
	d.m_rect = CRect(o.x, y, o.x + m_rect.Width(), y + h);
	d.SetCurSel(m_sel);
	d.ShowWindow(SW_SHOW);
	CWnd *oldFocus = g_focus;
	g_focus = &d;
	g_modal.push_back(&d);
	g_capture = nullptr;
	g_clickOutside = FALSE;
	while (!d.m_done && !g_quit) {
		PaintDirty();
		Present();
		PumpEvents(TRUE);
		if (g_clickOutside) { g_clickOutside = FALSE; d.m_sel = -2; d.m_done = TRUE; }
	}
	g_modal.pop_back();
	int sel = d.m_sel;
	d.DestroyWindow();
	g_focus = oldFocus;
	Invalidate();
	if (m_parent) m_parent->Invalidate();
	if (sel >= 0 && sel != m_sel) { SetCurSel(sel); Notify(CBN_SELCHANGE); }
	return TRUE;
}
BOOL CComboBox::ShimKey(UINT vk, BOOL down)
{
	if (!down || !GetCount()) return FALSE;
	int i = m_sel;
	if (vk == VK_UP || vk == VK_LEFT) i = std::max(0, i - 1);
	else if (vk == VK_DOWN || vk == VK_RIGHT) i = std::min(GetCount() - 1, i + 1);
	else if (vk == VK_SPACE) { ShimMouse(WM_LBUTTONDOWN, CPoint()); return TRUE; }
	else return FALSE;
	if (i != m_sel) { SetCurSel(i); Notify(CBN_SELCHANGE); }
	return TRUE;
}
BOOL CComboBox::ShimWheel(int dy) { return FALSE; }
int CComboBox::FindString(int start, LPCTSTR s) const
{
	size_t n = strlen(s);
	for (int i = 0; i < GetCount(); i++) if (!strncasecmp(m_items[i].text, s, n)) return i;
	return CB_ERR;
}
int CComboBox::SelectString(int start, LPCTSTR s)
{
	int i = FindString(start, s);
	if (i >= 0) SetCurSel(i);
	return i;
}

// ---------------------------------------------------------------- spin, progress
void CSpinButtonCtrl::ShimPaint(CDC &dc)
{
	CRect r;
	GetClientRect(&r);
	BOOL horz = m_style & UDS_HORZ;
	for (int k = 0; k < 2; k++) {
		CRect b = horz ? CRect(k ? r.Width() / 2 : 0, 0, k ? r.right : r.Width() / 2, r.bottom) : CRect(0, k ? r.Height() / 2 : 0, r.right, k ? r.bottom : r.Height() / 2);
		dc.FillSolidRect(&b, COLOR_FACE);
		if (m_pressed == k + 1) dc.Draw3dRect(b, COLOR_SHADOW, COLOR_SHADOW); else DrawFrameCtl(dc, b, FALSE);
		DrawArrow(dc, b, horz ? (k ? 1 : 3) : (k ? 2 : 0), IsWindowEnabled() ? 0 : COLOR_SHADOW);
	}
}
BOOL CSpinButtonCtrl::ShimMouse(UINT msg, CPoint pt)
{
	CRect r;
	GetClientRect(&r);
	BOOL horz = m_style & UDS_HORZ;
	int k = horz ? (pt.x >= r.Width() / 2) : (pt.y >= r.Height() / 2);
	if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK) {
		m_pressed = k + 1;
		RedrawWindow();
		// horizontal: right = +1; vertical: up = +1
		int delta = horz ? (k ? 1 : -1) : (k ? -1 : 1);
		NM_UPDOWN nm = {{this, m_nID, UDN_DELTAPOS}, m_pos, delta};
		LRESULT res = 0;
		if (m_parent) m_parent->SendMessage(WM_NOTIFY, m_nID, (LPARAM)&nm), res = 0;
		int np = m_pos + nm.iDelta;
		if (np > m_hi) np = (m_style & UDS_WRAP) ? m_lo : m_hi;
		if (np < m_lo) np = (m_style & UDS_WRAP) ? m_hi : m_lo;
		m_pos = np;
		(void)res;
	} else if (msg == WM_LBUTTONUP) {
		m_pressed = 0;
		Invalidate();
	}
	return TRUE;
}
void CProgressCtrl::ShimPaint(CDC &dc)
{
	CRect r;
	GetClientRect(&r);
	dc.FillSolidRect(&r, COLOR_FACE);
	dc.Draw3dRect(r, COLOR_SHADOW, COLOR_HILITE);
	int w = m_hi > m_lo ? (r.Width() - 4) * (m_pos - m_lo) / (m_hi - m_lo) : 0;
	dc.FillSolidRect(2, 2, w, r.Height() - 4, COLOR_SELBG);
}

// ---------------------------------------------------------------- list view (report mode)
BOOL CListCtrl::Create(DWORD style, const RECT &r, CWnd *parent, UINT id)
{
	return CWnd::Create(nullptr, nullptr, style | WS_CHILD, r, parent, id);
}
int CListCtrl::HeaderH() const { return (m_style & LVS_NOCOLUMNHEADER) ? 0 : FontHeight(m_font) + 5; }
int CListCtrl::RowH() const { return std::max(FontHeight(m_font) + 2, m_pSmall ? m_pSmall->m_cy + 1 : 0); }
static int Border(const CWnd *w) { return (w->m_style & WS_BORDER) || (w->m_exStyle & WS_EX_CLIENTEDGE) ? 2 : 0; }
int CListCtrl::GetCountPerPage() const { return std::max(1, ((int)m_rect.Height() - 2 * Border(this) - HeaderH()) / RowH()); }
BOOL CListCtrl::NeedVScroll() const { return GetItemCount() > GetCountPerPage(); }
int CListCtrl::InsertColumn(int i, LPCTSTR text, int fmt, int width, int)
{
	if (i < 0 || i > (int)m_cols.size()) i = (int)m_cols.size();
	m_cols.insert(m_cols.begin() + i, Col{text, width < 0 ? 60 : width, fmt});
	for (auto &r : m_rows) if ((int)r.text.size() > i) r.text.insert(r.text.begin() + i, CString());
	Invalidate();
	return i;
}
int CListCtrl::InsertItem(UINT mask, int i, LPCTSTR text, UINT state, UINT stateMask, int image, LPARAM lp)
{
	if (i < 0 || i > GetItemCount()) i = GetItemCount();
	Row r;
	r.text.resize(std::max<size_t>(1, m_cols.size()));
	r.text[0] = (mask & LVIF_TEXT) && text ? text : "";
	r.image = (mask & LVIF_IMAGE) ? image : -1;
	r.data = (mask & LVIF_PARAM) ? lp : 0;
	r.state = (mask & LVIF_STATE) ? (state & stateMask) : 0;
	m_rows.insert(m_rows.begin() + i, r);
	Invalidate();
	return i;
}
int CListCtrl::InsertItem(int i, LPCTSTR text, int image) { return InsertItem(LVIF_TEXT | (image >= 0 ? LVIF_IMAGE : 0), i, text, 0, 0, image, 0); }
int CListCtrl::InsertItem(const LVITEM *it) { return InsertItem(it->mask, it->iItem, it->pszText, it->state, it->stateMask, it->iImage, it->lParam); }
BOOL CListCtrl::SetItemText(int i, int sub, LPCTSTR text)
{
	if (i < 0 || i >= GetItemCount() || sub < 0) return FALSE;
	auto &t = m_rows[i].text;
	if ((int)t.size() <= sub) t.resize(sub + 1);
	t[sub] = text ? text : "";
	Invalidate();
	return TRUE;
}
CString CListCtrl::GetItemText(int i, int sub) const
{
	if (i < 0 || i >= GetItemCount() || sub < 0 || sub >= (int)m_rows[i].text.size()) return CString();
	return m_rows[i].text[sub];
}
int CListCtrl::GetItemText(int i, int sub, LPTSTR buf, int n) const
{
	CString s = GetItemText(i, sub);
	strncpy(buf, s, n);
	if (n) buf[n - 1] = 0;
	return (int)strlen(buf);
}
BOOL CListCtrl::SetItem(int i, int sub, UINT mask, LPCTSTR text, int image, UINT state, UINT stateMask, LPARAM lp)
{
	if (i < 0 || i >= GetItemCount()) return FALSE;
	if (mask & LVIF_TEXT) SetItemText(i, sub, text);
	if ((mask & LVIF_IMAGE) && sub == 0) m_rows[i].image = image;
	if (mask & LVIF_PARAM) m_rows[i].data = lp;
	if (mask & LVIF_STATE) SetItemState(i, state, stateMask);
	Invalidate();
	return TRUE;
}
BOOL CListCtrl::SetItem(const LVITEM *it) { return SetItem(it->iItem, it->iSubItem, it->mask, it->pszText, it->iImage, it->state, it->stateMask, it->lParam); }
BOOL CListCtrl::GetItem(LVITEM *it) const
{
	int i = it->iItem;
	if (i < 0 || i >= GetItemCount()) return FALSE;
	if (it->mask & LVIF_TEXT) GetItemText(i, it->iSubItem, it->pszText, it->cchTextMax);
	if (it->mask & LVIF_IMAGE) it->iImage = m_rows[i].image;
	if (it->mask & LVIF_PARAM) it->lParam = m_rows[i].data;
	if (it->mask & LVIF_STATE) it->state = m_rows[i].state & it->stateMask;
	return TRUE;
}
void CListCtrl::Notify(UINT code, int item, UINT ns, UINT os, int sub)
{
	if (!m_parent) return;
	NM_LISTVIEW nm = {};
	nm.hdr = {this, m_nID, code};
	nm.iItem = item;
	nm.iSubItem = sub;
	nm.uNewState = ns;
	nm.uOldState = os;
	nm.uChanged = code == LVN_ITEMCHANGED ? LVIF_STATE : 0;
	nm.lParam = (item >= 0 && item < GetItemCount()) ? m_rows[item].data : 0;
	m_parent->SendMessage(WM_NOTIFY, m_nID, (LPARAM)&nm);
}
BOOL CListCtrl::SetItemState(int i, UINT state, UINT mask)
{
	if (i == -1) { for (int k = 0; k < GetItemCount(); k++) SetItemState(k, state, mask); return TRUE; }
	if (i < 0 || i >= GetItemCount()) return FALSE;
	if ((m_style & LVS_SINGLESEL) && (state & mask & LVIS_SELECTED))
		for (int k = 0; k < GetItemCount(); k++)
			if (k != i && (m_rows[k].state & LVIS_SELECTED)) SetItemState(k, 0, LVIS_SELECTED);
	if (state & mask & LVIS_FOCUSED)
		for (int k = 0; k < GetItemCount(); k++) if (k != i) m_rows[k].state &= ~LVIS_FOCUSED;
	UINT os = m_rows[i].state, ns = (os & ~mask) | (state & mask);
	if (ns == os) return TRUE;
	m_rows[i].state = ns;
	Invalidate();
	Notify(LVN_ITEMCHANGED, i, ns, os);
	return TRUE;
}
int CListCtrl::GetNextItem(int start, int flags) const
{
	for (int i = start + 1; i < GetItemCount(); i++) {
		if (flags == LVNI_ALL) return i;
		if ((flags & LVNI_SELECTED) && !(m_rows[i].state & LVIS_SELECTED)) continue;
		if ((flags & LVNI_FOCUSED) && !(m_rows[i].state & LVIS_FOCUSED)) continue;
		return i;
	}
	return -1;
}
int CListCtrl::GetSelectedCount() const
{
	int n = 0;
	for (auto &r : m_rows) n += (r.state & LVIS_SELECTED) != 0;
	return n;
}
BOOL CListCtrl::DeleteItem(int i)
{
	if (i < 0 || i >= GetItemCount()) return FALSE;
	m_rows.erase(m_rows.begin() + i);
	m_top = std::max(0, std::min(m_top, GetItemCount() - GetCountPerPage()));
	Invalidate();
	return TRUE;
}
BOOL CListCtrl::DeleteAllItems()
{
	m_rows.clear();
	m_top = 0;
	Invalidate();
	return TRUE;
}
BOOL CListCtrl::EnsureVisible(int i, BOOL)
{
	int v = GetCountPerPage();
	if (i < m_top) m_top = i;
	else if (i >= m_top + v) m_top = i - v + 1;
	m_top = std::max(0, m_top);
	Invalidate();
	return TRUE;
}
int CListCtrl::FindItem(LVFINDINFO *fi, int start) const
{
	for (int i = start + 1; i < GetItemCount(); i++) {
		if ((fi->flags & LVFI_PARAM) && m_rows[i].data == fi->lParam) return i;
		if ((fi->flags & LVFI_STRING) && !strcasecmp(m_rows[i].text[0], fi->psz)) return i;
	}
	return -1;
}
BOOL CListCtrl::SortItems(PFNLVCOMPARE fn, DWORD_PTR data)
{
	std::stable_sort(m_rows.begin(), m_rows.end(), [&](const Row &a, const Row &b) { return fn(a.data, b.data, data) < 0; });
	Invalidate();
	return TRUE;
}
void CListCtrl::Select(int i, BOOL)
{
	if (i < 0 || i >= GetItemCount()) return;
	SetItemState(i, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	EnsureVisible(i, FALSE);
}
void CListCtrl::ShimPaint(CDC &dc)
{
	CRect r;
	GetClientRect(&r);
	int b = Border(this);
	dc.FillSolidRect(&r, COLOR_HILITE);
	if (b) Sunken(dc, r);
	r.DeflateRect(b, b);
	BOOL vbar = NeedVScroll();
	int w = r.Width() - (vbar ? 16 : 0);
	int hh = HeaderH(), rh = RowH();
	CRect save = dc.m_clip;
	dc.m_clip.left = std::max(dc.m_clip.left, dc.m_ox + (int)r.left);
	dc.m_clip.right = std::min(dc.m_clip.right, dc.m_ox + (int)r.left + w);
	dc.m_clip.bottom = std::min(dc.m_clip.bottom, dc.m_oy + (int)r.bottom);
	CRect inner = dc.m_clip;
	int x = r.left;
	if (hh) {
		for (auto &c : m_cols) {
			CRect h(x, r.top, x + c.cx, r.top + hh);
			dc.FillSolidRect(&h, COLOR_FACE);
			DrawFrameCtl(dc, h, FALSE, FALSE);
			dc.m_clip.right = std::min(inner.right, dc.m_ox + (int)h.right - 2);
			Text(dc, x + 6, r.top + 2, c.text, 0);
			dc.m_clip = inner;
			x += c.cx;
		}
		if (x < r.left + w) { CRect h(x, r.top, r.left + w, r.top + hh); dc.FillSolidRect(&h, COLOR_FACE); DrawFrameCtl(dc, h, FALSE, FALSE); }
	}
	BOOL focus = g_focus == this;
	for (int i = m_top; i < GetItemCount(); i++) {
		int y = r.top + hh + (i - m_top) * rh;
		if (y >= r.bottom) break;
		const Row &row = m_rows[i];
		x = r.left;
		for (size_t c = 0; c < m_cols.size(); c++) {
			int cx = m_cols[c].cx;
			dc.m_clip.left = std::max(inner.left, dc.m_ox + x);
			dc.m_clip.right = std::min(inner.right, dc.m_ox + x + cx);
			int tx = x + 6;
			CString t = c < row.text.size() ? row.text[c] : CString();
			if (c == 0) {
				tx = x + 2;
				if (m_pSmall) {
					if (row.image >= 0) m_pSmall->Draw(&dc, row.image, CPoint(tx, y), ILD_NORMAL);
					tx += m_pSmall->m_cx + 2;
				}
				if (row.state & LVIS_SELECTED) {
					int tw = TextWidth(dc.m_font, t, t.GetLength());
					dc.FillSolidRect(tx - 1, y, std::min(tw + 4, x + cx - tx + 1), rh, focus ? COLOR_SELBG : COLOR_FACE);
				}
				Text(dc, tx + 1, y + (rh - FontHeight(dc.m_font)) / 2, t, (row.state & LVIS_SELECTED) && focus ? 0xFFFFFF : 0);
			} else {
				int tw = TextWidth(dc.m_font, t, t.GetLength());
				if (m_cols[c].fmt == LVCFMT_RIGHT) tx = x + cx - 6 - tw;
				else if (m_cols[c].fmt == LVCFMT_CENTER) tx = x + (cx - tw) / 2;
				Text(dc, tx, y + (rh - FontHeight(dc.m_font)) / 2, t, 0);
			}
			x += cx;
		}
		dc.m_clip = inner;
	}
	dc.m_clip = save;
	if (vbar) DrawSB(dc, {CRect(r.left + w, r.top, r.right, r.bottom), TRUE, m_top, GetCountPerPage(), GetItemCount()});
}
BOOL CListCtrl::ShimMouse(UINT msg, CPoint pt)
{
	CRect r;
	GetClientRect(&r);
	int b = Border(this);
	r.DeflateRect(b, b);
	int w = r.Width() - (NeedVScroll() ? 16 : 0);
	SB sb = {CRect(r.left + w, r.top, r.right, r.bottom), TRUE, m_top, GetCountPerPage(), GetItemCount()};
	if (m_dragThumb) {
		if (msg == WM_MOUSEMOVE) { m_top = DragSB(sb, pt, m_left); Invalidate(); }
		if (msg == WM_LBUTTONUP) m_dragThumb = FALSE;
		return TRUE;
	}
	if (NeedVScroll() && pt.x >= r.left + w) {
		if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK) {
			int off = -1;
			m_top = ClickSB(sb, pt, 1, &off);
			if (off >= 0) { m_dragThumb = TRUE; m_left = off; }
			Invalidate();
		}
		return TRUE;
	}
	if (msg != WM_LBUTTONDOWN && msg != WM_LBUTTONDBLCLK && msg != WM_RBUTTONUP) return TRUE;
	if (HeaderH() && pt.y < r.top + HeaderH()) {
		if (msg == WM_LBUTTONDOWN && !(m_style & LVS_NOSORTHEADER)) {
			int x = r.left;
			for (size_t c = 0; c < m_cols.size(); c++) {
				if (pt.x >= x && pt.x < x + m_cols[c].cx) { Notify(LVN_COLUMNCLICK, -1, 0, 0, (int)c); break; }
				x += m_cols[c].cx;
			}
		}
		return TRUE;
	}
	int i = m_top + (pt.y - r.top - HeaderH()) / RowH();
	if (pt.y < r.top + HeaderH() || i >= GetItemCount()) {
		if (msg == WM_LBUTTONDOWN)
			for (int k = 0; k < GetItemCount(); k++) SetItemState(k, 0, LVIS_SELECTED);
		return TRUE;
	}
	if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK) Select(i);
	if (msg == WM_LBUTTONDBLCLK) Notify(NM_DBLCLK, i);
	return TRUE;
}
BOOL CListCtrl::ShimKey(UINT vk, BOOL down)
{
	if (!down || !GetItemCount()) return FALSE;
	int cur = GetNextItem(-1, LVNI_FOCUSED);
	if (cur < 0) cur = GetNextItem(-1, LVNI_SELECTED);
	int i = cur, v = GetCountPerPage();
	switch (vk) {
	case VK_UP: i = std::max(0, i - 1); break;
	case VK_DOWN: i = std::min(GetItemCount() - 1, i + 1); break;
	case VK_PRIOR: i = std::max(0, i - v); break;
	case VK_NEXT: i = std::min(GetItemCount() - 1, i + v); break;
	case VK_HOME: i = 0; break;
	case VK_END: i = GetItemCount() - 1; break;
	default: return FALSE;
	}
	if (i < 0) i = 0;
	Select(i);
	return TRUE;
}
BOOL CListCtrl::ShimWheel(int dy)
{
	m_top = std::max(0, std::min(GetItemCount() - GetCountPerPage(), m_top - dy * 3));
	if (m_top < 0) m_top = 0;
	Invalidate();
	return TRUE;
}

// ---------------------------------------------------------------- scroll view
int CScrollView::GetSystemMetrics_(int i) { return ::GetSystemMetrics(i); }
int CScrollView::ViewW() const { return m_rect.Width() - (HasV() ? 16 : 0); }
int CScrollView::ViewH() const { return m_rect.Height() - (HasH() ? 16 : 0); }
BOOL CScrollView::HasV() const
{
	if (!(m_style & WS_VSCROLL)) return FALSE;
	if (m_totalLog.cy > m_rect.Height()) return TRUE;
	return m_totalLog.cx > m_rect.Width() && m_totalLog.cy > m_rect.Height() - 16;
}
BOOL CScrollView::HasH() const
{
	if (!(m_style & WS_HSCROLL)) return FALSE;
	if (m_totalLog.cx > m_rect.Width()) return TRUE;
	return m_totalLog.cy > m_rect.Height() && m_totalLog.cx > m_rect.Width() - 16;
}
void CScrollView::Clamp()
{
	m_scroll.x = std::max(0, std::min((int)m_scroll.x, (int)m_totalLog.cx - ViewW()));
	m_scroll.y = std::max(0, std::min((int)m_scroll.y, (int)m_totalLog.cy - ViewH()));
}
void CScrollView::SetScrollSizes(int, SIZE total, const SIZE &, const SIZE &)
{
	m_totalLog = total;
	m_clientRight = HasV() ? 16 : 0;
	m_clientBottom = HasH() ? 16 : 0;
	Clamp();
	Invalidate();
}
void CScrollView::ScrollToPosition(POINT pt)
{
	m_scroll = pt;
	Clamp();
	Invalidate();
}
void CScrollView::OnPrepareDC(CDC *pDC)
{
	pDC->m_ox -= m_scroll.x;
	pDC->m_oy -= m_scroll.y;
}
void CScrollView::OnPaint()
{
	CPaintDC dc(this);
	OnPrepareDC(&dc);
	OnDraw(&dc);
	ShimPaint(dc);
}
// scroll bars live outside the client area
void CScrollView::ShimPaint(CDC &)
{
	CClientDC dc(this);
	dc.m_clip = VisibleRect();
	int w = m_rect.Width(), h = m_rect.Height();
	if (HasV()) DrawSB(dc, {CRect(w - 16, 0, w, h - (HasH() ? 16 : 0)), TRUE, (int)m_scroll.y, ViewH(), (int)m_totalLog.cy});
	if (HasH()) DrawSB(dc, {CRect(0, h - 16, w - (HasV() ? 16 : 0), h), FALSE, (int)m_scroll.x, ViewW(), (int)m_totalLog.cx});
	if (HasV() && HasH()) dc.FillSolidRect(w - 16, h - 16, 16, 16, COLOR_FACE);
}
BOOL CScrollView::ShimMouse(UINT msg, CPoint pt)
{
	int w = m_rect.Width(), h = m_rect.Height();
	SB v = {CRect(w - 16, 0, w, h - (HasH() ? 16 : 0)), TRUE, (int)m_scroll.y, ViewH(), (int)m_totalLog.cy};
	SB hz = {CRect(0, h - 16, w - (HasV() ? 16 : 0), h), FALSE, (int)m_scroll.x, ViewW(), (int)m_totalLog.cx};
	if (m_drag) {
		if (msg == WM_MOUSEMOVE) {
			if (m_drag == 1) m_scroll.y = DragSB(v, pt, m_dragOff); else m_scroll.x = DragSB(hz, pt, m_dragOff);
			Invalidate();
		}
		if (msg == WM_LBUTTONUP) m_drag = 0;
		return TRUE;
	}
	BOOL down = msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK;
	if (HasV() && pt.x >= w - 16 && pt.y < v.r.bottom) {
		if (down) { int off = -1; m_scroll.y = ClickSB(v, pt, 16, &off); if (off >= 0) { m_drag = 1; m_dragOff = off; } Invalidate(); }
		return TRUE;
	}
	if (HasH() && pt.y >= h - 16 && pt.x < hz.r.right) {
		if (down) { int off = -1; m_scroll.x = ClickSB(hz, pt, 16, &off); if (off >= 0) { m_drag = 2; m_dragOff = off; } Invalidate(); }
		return TRUE;
	}
	if (HasV() && HasH() && pt.x >= w - 16 && pt.y >= h - 16) return TRUE;
	return FALSE;
}
BOOL CScrollView::ShimWheel(int dy)
{
	if (!HasV() && !HasH()) return FALSE;
	if (HasV()) m_scroll.y -= dy * 16 * 3; else m_scroll.x -= dy * 16 * 3;
	Clamp();
	Invalidate();
	return TRUE;
}
